// The exported interface in include/stack_api.h. Private Storage Master exports
// the same names from its own copy of the feature, so Master Looter's Stacks tab
// talks to whichever of the two is installed. This mod is the one that applies
// when both are, which is why nothing here ever reports standing down for
// another mod.
#define STACK_API __declspec(dllexport)
#include "stack_api.h"

#include <Windows.h>
#include <cstdio>
#include <cstring>

#include "core/log.h"
#include "core/settings.h"
#include "stacks/stacks.h"
#include "version.h"

#define STACK_EXPORT extern "C" STACK_API

// snprintf truncates without complaining, so a name that outgrows its field
// would quietly lose its tail. Fail the build instead.
static_assert(sizeof SM_NAME <= sizeof(StackStatus::provider), "SM_NAME does not fit StackStatus::provider");
static_assert(sizeof SM_MODULE <= sizeof(StackStatus::providerModule), "SM_MODULE does not fit StackStatus::providerModule");
static_assert(sizeof SM_VERSION <= sizeof(StackStatus::version), "SM_VERSION does not fit StackStatus::version");
static_assert(sizeof SM_GAME <= sizeof(StackStatus::gameVersion), "SM_GAME does not fit StackStatus::gameVersion");
static_assert(STACK_STANDDOWN_NONE == sm::stacks::kApplying && STACK_STANDDOWN_OFF == sm::stacks::kOff &&
              STACK_STANDDOWN_OTHER_MOD == sm::stacks::kOtherMod && STACK_STANDDOWN_NO_ANCHOR == sm::stacks::kNoAnchor &&
              STACK_STANDDOWN_HOOK_FAILED == sm::stacks::kHookFailed && STACK_STANDDOWN_TOO_LATE == sm::stacks::kTooLate,
              "the STACK_STANDDOWN_ codes and stacks::Reason must match");
static_assert(STACK_MAX_MULTIPLIER == sm::Settings::kMaxMultiplier,
              "STACK_MAX_MULTIPLIER and Settings::kMaxMultiplier must match");
static_assert(STACK_CEILING == sm::Settings::kMaxStackSize, "STACK_CEILING and Settings::kMaxStackSize must match");

STACK_EXPORT int StackApiVersion(void) { return STACK_API_VERSION; }

STACK_EXPORT int StackGetStatus(StackStatus* out)
{
    // Any size from the first published layout upward, so a caller built against
    // an older header keeps working: it is a prefix of this struct, it gets the
    // fields it knows, and `size` comes back saying how many bytes were written.
    if (!out || out->size < static_cast<uint32_t>(STACK_STATUS_V1)) return 0;
    const uint32_t want = out->size < sizeof(StackStatus) ? out->size : static_cast<uint32_t>(sizeof(StackStatus));
    const sm::stacks::Report r = sm::stacks::Status();
    StackStatus full;
    StackStatus* const fill = &full;
    memset(fill, 0, sizeof full);
    fill->size = want;
    snprintf(fill->provider, sizeof fill->provider, "%s", SM_NAME);
    snprintf(fill->providerModule, sizeof fill->providerModule, "%s", SM_MODULE);
    snprintf(fill->version, sizeof fill->version, "%s", SM_VERSION);
    snprintf(fill->gameVersion, sizeof fill->gameVersion, "%s", SM_GAME);
    fill->applying = r.reason == sm::stacks::kApplying;
    fill->standDownReason = r.reason;
    fill->hooked = r.hooked;
    fill->multiplier = r.multiplier;
    fill->multiplierSetting = sm::Settings::Get().multiplier;
    // While applying, against what is in force, not against what the launch
    // started with: a live raise makes the two agree, and then no restart is
    // needed. While not applying, `multiplier` is 1 whatever the ini holds, so
    // that comparison would ask for a restart the player never caused and that
    // would change nothing. Then only a setting changed since the launch began
    // waits for one. With Enabled=0 in the ini a restart changes nothing, so a
    // multiplier picked then asks for none; the stand-down text says the mod is
    // off. Enabled=1 saved over a launch that started with 0 does need one.
    const sm::Settings::Values& launch = sm::Settings::Startup();
    const bool enabledNow = sm::Settings::Get().enabled;
    // StackSize too: a pick on the tab refreshes Get() from the ini, so a size
    // set or cleared in INI Master shows here and waits for the next launch.
    // An Enabled=0 read the same way turns the mod off at the next launch.
    fill->restartNeeded = fill->applying ? !enabledNow || fill->multiplierSetting != fill->multiplier ||
                                               sm::Settings::Get().stackSize != r.stackSize
                                         : enabledNow && (!launch.enabled || fill->multiplierSetting != launch.multiplier);
    fill->itemsRaised = r.patched;
    fill->itemsUnstackable = r.unstackable;
    fill->ceiling = STACK_CEILING;
    fill->maxMultiplier = STACK_MAX_MULTIPLIER;
    // StackApplyMultiplier raises nothing once a pick has read Enabled=0.
    fill->liveRaise = sm::stacks::CanRaiseNow() && enabledNow;
    fill->biggest = r.biggest;
    fill->stackSize = r.stackSize;
    memcpy(out, fill, want);
    return 1;
}

STACK_EXPORT int StackStandDownText(int reason, char* out, int outLen)
{
    if (!out || outLen <= 0) return 0;
    switch (reason)
    {
    case STACK_STANDDOWN_NONE:
        // Master Looter's Stacks tab shows this line, and a tab built before
        // stackSize was added has no other way to learn that the multiplier it
        // offers is not the one in charge.
        if (const int size = sm::stacks::StackSizeInForce())
            snprintf(out, static_cast<size_t>(outLen),
                     "%s is setting every stack to at least %d, from StackSize in StackMaster.ini. The multiplier is not used while "
                     "StackSize is set.",
                     SM_NAME, size);
        else
            snprintf(out, static_cast<size_t>(outLen), "%s is setting the stack sizes.", SM_NAME);
        return 1;
    case STACK_STANDDOWN_OFF:
        // Multiplier=1 is the old default, and a tab that only said "switched off"
        // left a player hunting for the switch. Told from the launch, since that
        // is what left stacks alone this session.
        // The next start is promised only while the ini as last read still has
        // the mod on.
        if (!sm::Settings::Startup().enabled)
            snprintf(out, static_cast<size_t>(outLen), "Turned off in StackMaster.ini with Enabled=0.");
        else if (!sm::Settings::Get().enabled)
            snprintf(out, static_cast<size_t>(outLen), "Started with Multiplier=1 and StackSize=0, so stacks hold what the game gives them. "
                                                      "Enabled=0 is now saved in StackMaster.ini, so the mod is off from the next start.");
        else
            snprintf(out, static_cast<size_t>(outLen), "Installed, but Multiplier=1 and StackSize=0 in StackMaster.ini, so stacks hold what the "
                                                      "game gives them. A multiplier picked here takes hold at the next start.");
        return 1;
    case STACK_STANDDOWN_OTHER_MOD:
        snprintf(out, static_cast<size_t>(outLen), "Another mod is setting the stack sizes.");
        return 1;
    case STACK_STANDDOWN_NO_ANCHOR:
        snprintf(out, static_cast<size_t>(outLen), "This game version keeps its item table somewhere the mod does not recognise, so stacks are "
                                                  "left alone. An update to %s is needed.", SM_NAME);
        return 1;
    case STACK_STANDDOWN_HOOK_FAILED:
        snprintf(out, static_cast<size_t>(outLen), "The item table could not be hooked, so stacks are left alone. The log says why.");
        return 1;
    case STACK_STANDDOWN_TOO_LATE:
        snprintf(out, static_cast<size_t>(outLen), "The game read its item table before the mod started, so stacks are the game's own this "
                                                  "session. A restart fixes it.");
        return 1;
    default:
        snprintf(out, static_cast<size_t>(outLen), "Stacks are not being changed, and this version does not know why (reason %d).", reason);
        return 0;
    }
}

STACK_EXPORT int StackGetMultiplier(void) { return sm::Settings::Get().multiplier; }

STACK_EXPORT int StackApplyMultiplier(int multiplier, char* why, int whyLen)
{
    if (why && whyLen > 0) why[0] = 0;
    if (multiplier < 1 || multiplier > STACK_MAX_MULTIPLIER)
    {
        if (why && whyLen > 0) snprintf(why, static_cast<size_t>(whyLen), "the multiplier has to be between 1 and %d", STACK_MAX_MULTIPLIER);
        return 0;
    }
    // Saving it while StackSize is set would change nothing, now or after a
    // restart, and the tab would look as if it had worked. Refused with the
    // reason, which the tab shows. Tested on what the ini holds now, inside
    // Update, so a StackSize set or cleared in INI Master since the launch
    // counts.
    const auto change = [multiplier, why, whyLen](sm::Settings::Values& v)
    {
        if (v.stackSize)
        {
            if (why && whyLen > 0)
                snprintf(why, static_cast<size_t>(whyLen),
                         "StackSize=%d in StackMaster.ini sets every stack, so the multiplier does nothing. Set StackSize=0 there to use it.",
                         v.stackSize);
            return false;
        }
        v.multiplier = multiplier;
        return true;
    };
    if (!sm::Settings::Update(change, why, why && whyLen > 0 ? static_cast<size_t>(whyLen) : 0)) return 0;
    // Raising can take hold now. Anything else waits for the next launch, which
    // the caller sees as restartNeeded on its next status read. A refusal there is
    // not a failure of this call: the setting is saved either way.
    // Enabled=0 saved in INI Master since the launch means the player turned the
    // mod off, so nothing is raised for a launch that will not raise it.
    if (!sm::Settings::Get().enabled)
    {
        LOG("[stacks] x%d is saved, but not raised now, because Enabled=0 in the ini turns the mod off at the next start.", multiplier);
        return 1;
    }
    char note[192];
    if (!sm::stacks::RaiseNow(multiplier, note, sizeof note)) LOG("[stacks] x%d is saved for the next launch. Not now, because %s.", multiplier, note);
    return 1;
}

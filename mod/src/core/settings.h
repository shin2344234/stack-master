#pragma once
#include <cstddef>
#include <functional>

// StackMaster.ini. Four settings and nothing else: whether the mod runs at all,
// whether the log is verbose, and how much deeper stacks go, as a multiplier or
// as one size for everything.
//
// Settings can change while the game runs (Master Looter's Stacks tab through
// StackApplyMultiplier). Get() hands out the current copy; a change publishes a
// new copy and the old one is kept alive, so a reader on another thread never
// sees a half-written value. The multiplier in force is the one this launch
// started with, so a change needs a restart and Startup() is what the hook uses.
namespace sm::Settings
{
    // The largest multiple a stack limit is raised by. The per-item ceiling in
    // stacks/stacks stops the result running away; this stops a typo like
    // 100000 from turning every small stack into a huge one.
    inline constexpr int kMaxMultiplier = 1000;
    // The largest StackSize accepted. It is the per-item ceiling in
    // stacks/stacks and STACK_CEILING in stack_api.h, and a static_assert in
    // api.cpp holds the three together.
    inline constexpr int kMaxStackSize = 999999;

    struct Values
    {
        bool enabled = true;
        bool debugLog = false;
        // How much of one item a slot holds, as a multiple of the game's own
        // limit for that item. 1 changes nothing and installs no hook.
        //
        // Five from the first run. It was 1, so a fresh install wrote an ini
        // that changed nothing and the mod looked broken until the player
        // found the setting. okolkye94 reported it on the Nexus posts tab,
        // and zerorushhour2 answered that setting 5 made it work. Five is the
        // number every example in the ini and the READMEs already uses.
        int  multiplier = 5;
        // One size for every item that already stacks, in place of the
        // multiplier. 0 is off and leaves the multiplier in charge. Set, it
        // wins: an item whose own limit is under it is raised to it, one at or
        // over it keeps its own, and Multiplier is not used at all. willief23
        // asked on the Nexus posts tab which multiplier gives a stack of 9999,
        // and no single one does, because the game's own limits differ by item.
        int  stackSize = 0;
    };

    void Load();               // once, at startup
    const Values& Get();       // current settings
    const Values& Startup();   // what this launch started with; the hook uses these
    Values Defaults();

    // Checks, publishes and writes the ini. False with a reason when refused.
    bool Apply(const Values& v, char* why, size_t whyLen);
    // The same for a change to some fields, under the write lock, so two threads
    // changing different fields cannot undo each other.
    bool Update(const std::function<void(Values&)>& change, char* why, size_t whyLen);
    // True when a setting differs from what this launch started with.
    bool RestartNeeded();
}

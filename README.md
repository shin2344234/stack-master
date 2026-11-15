# Stack Master

A Crimson Desert plugin that raises how much of an item one inventory slot
holds. Pick a multiplier and every item that already stacks holds that many
times as much: at 5, a stack of 100 becomes 500 and a stack of 20 becomes 100.
Or set one stack size, such as 9999, for everything that stacks.
Built for Crimson Desert 2.05.00, exe 1.0.0.3663. Every DLC build from exe
1.0.0.3392 on shares the item record it writes. Versions before the DLC need
Stack Master 1.0.3.

It changes no game file. The game reads its item table once while it starts, and
the plugin raises each item's own limit as that happens, in memory. That is the
difference from a data mod: nothing has to be reinstalled after a game patch,
and there is no archive to restore when you remove it.

Items the game does not stack, such as gear and quest items, never start
stacking. No stack goes past 999999, and an item the game already lets you hold
more of than that, like money, is left as it is. Replenishing Arrows, Bullets
and Cannonballs keep the game's own stack size.

## Install

Copy `StackMaster.asi` into `Crimson Desert\bin64`, beside the ASI loader you
already use. On first run it writes `StackMaster.ini` next to itself with

    Multiplier=5

so every stack holds five times the game's own from the first launch. Change
the number and restart the game to pick another.

An ini written by 1.0.4 or earlier started at `Multiplier=1`, and updating the
plugin keeps your ini, so that 1 stays and nothing is raised. The log says so
with an error line naming the setting to change, and Master Looter's Stacks tab
shows the same reason.

`StackSize=9999` gives every item that stacks a limit of 9999 in place of the
multiplier. It is 0, off, by default. `Enabled=0` turns the mod off.
`DebugLog=1` writes a line for the first ten items raised and keeps the last
five sessions' logs, which is what to send with a report.

With [Master Looter](https://www.nexusmods.com/crimsondesert/mods/3402) 1.6.33 or
later you can change the multiplier from its Stacks tab in game. Raising it takes
hold straight away if [Private Storage
Master](https://www.nexusmods.com/crimsondesert/mods/3521) is installed as well,
because this mod borrows its free-play check to know when that is safe. On the
DLC update that needs Private Storage Master 1.1.8 or later. On its own, and for
any smaller number, the change waits for the next start.

[INI Master](https://www.nexusmods.com/crimsondesert/mods/3578) can edit the four
settings with a label, a range and the default for each, all read out of the
plugin itself. Multiplier runs from 1 to 1000 and StackSize from 0 to 999999.
The plugin reads the ini once at startup, so a change made there waits for the
next launch.

A multiplier picked on the Stacks tab is saved on top of the ini as it is at that
moment, so settings you changed in INI Master during the session stay. If the ini
cannot be read or written, the tab refuses the pick and says so, and nothing is
saved. A pick is also refused when StackSize is set in the ini, even if you set
it after the game started. With `Enabled=0` saved in the ini, the tab raises
nothing and asks for no restart, since the mod is off from the next start.

## One stack size for everything

A multiplier scales each item's own limit, and the game's limits differ by item,
so there is no multiplier that makes every stack 9999. An item that stacks to
100 would need 100 and one that stacks to 20 would need 500. `StackSize` sets
the number directly.

It only ever raises. An item the game already stacks higher than `StackSize`
keeps its own limit, items that do not stack stay that way, the replenishing
ammo keeps the game's size, and 999999 is the most it accepts. When it is set
it wins over `Multiplier`, which is then not used at all, and Master Looter's
Stacks tab refuses a multiplier change with the reason. `StackSize=0` hands
control back to the multiplier. An ini from 1.0.5 or older has no `StackSize`
line, so add it under `[StackMaster]`.

For callers of the interface, `StackStatus::stackSize` is the size in force, 0
when the multiplier decides.

## Turning it back down

Lowering the multiplier always waits for the next start, and so does lowering
`StackSize`. A slot can hold more
than the game allows, and shrinking a limit under a stack already over it would
leave it stranded there.

Your save records the stacks you build, so a slot can still hold more than the
game allows after you set the multiplier back to 1. The plugin never writes to
your save and never removes an item. What the game does with a slot over its
normal limit has not been tested yet. It might keep the whole count or cut it
down. Until that is tested, empty the big stacks before turning it down or
uninstalling.

## Other mods

[Private Storage Master](https://www.nexusmods.com/crimsondesert/mods/3521) 1.1.2
and later carry the same feature. With both installed, Stack Master is the one
that applies and Private Storage Master leaves stacks alone and says so in its
log, so the two never multiply the same limit twice. Set the multiplier here.

Do not run a stack-size data mod such as Fat Stacks as well. Both would change
the same limits and the result is whichever the game reads last.

## Building

MSVC Build Tools 2022 with the CMake and Ninja it bundles, then
`mod\build.bat`, which stages `mod\dist\StackMaster.asi`. MinHook is fetched at
configure time for its instruction length decoder; pass
`-DFETCHCONTENT_FULLY_DISCONNECTED=ON` to reconfigure without fetching again.

For other plugins, [mod/include/stack_api.h](mod/include/stack_api.h) is the
versioned C interface Master Looter's Stacks tab uses. Private Storage Master
exports the same names, so a caller finds whichever is installed.

## Antivirus

VirusTotal flags 1.0.8 once on each file. DeepInstinct, which reports the word
MALICIOUS and names nothing it found, flagged the plugin (1 of 71), the DMM
archive (1 of 67) and the full archive (1 of 67), and no other scanner flagged
any of the three. Releases 1.0.0 to 1.0.7 were clean on every file, so this is
the first flag.

A model can take the shape of what the plugin does for a trainer. It is a DLL loaded into the game that searches the
game's code for a byte pattern and writes a jump over one of its functions. It
imports only kernel32 and user32, so there is no network code in it, and it reads
and writes no registry key and no game file. It is code signed by Seth Walker
under Microsoft's identity-verified chain. Every line is here to read or build
yourself.

SHA-256 for 1.0.8:

    f66f4b5c0f8ca44d68a1a574831abba2f4fdd4ca886b7d8eebcebeec0f0bf773  StackMaster-1.0.8-DMM.zip
    4cefafa5f96cd4d286344cc9729387e54fd4b3e1255670dc55b353de07559d3c  StackMaster-1.0.8.zip
    cdc4011b001cf797fabcda374c48875dc0c1dc41839664aa09b05973d09a279c  StackMaster.asi

## Discord and Patreon

[Shin234's Mods 'n Stuff](https://discord.gg/AZ2ztQYy74) is the Discord. Bugs are
best as GitHub issues or on the Nexus bugs tab so they get tracked. Patreon is
[patreon.com/cw/Shin234](https://www.patreon.com/cw/Shin234). Everything
published stays free and nothing is held back for it.

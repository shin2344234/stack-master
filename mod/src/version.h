#pragma once

// Bump it with the release that goes with it, never on its own.
#define SM_VERSION  "1.0.6"
#define SM_NAME     "Stack Master"
// The game builds the plugin was checked against: exe 1.0.0.3392, the
// Charting the Unknown DLC build (Steam 26308950), and its 30 October update
// 1.0.0.3424 (Steam 26317560), where the reader moved and nothing in it
// changed, and up to 1.0.0.3576 (2.04.08), where Seth's run on 6 November 2026 found the
// reader at +1719E80 and raised stacks. The DLC build reordered the ItemInfo
// record, so builds up to 1.0.0.2976 (2.03.02) are no longer supported. Exe
// versions stand in until the patch names are confirmed. It has to fit
// StackStatus::gameVersion, which is char[16]: "1.0.0.3392-3576" is 15
// characters.
#define SM_GAME     "1.0.0.3392-3576"
// Base name of the plugin's files next to it: StackMaster.asi, .ini, .log.
#define SM_FILEBASE L"StackMaster"
#define SM_INI      L"StackMaster.ini"
// The same as narrow text, for the module name another plugin looks us up by.
#define SM_MODULE   "StackMaster.asi"

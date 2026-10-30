#pragma once

// Bump it with the release that goes with it, never on its own.
#define SM_VERSION  "1.0.3"
#define SM_NAME     "Stack Master"
// The game build the plugin was last checked against: exe 1.0.0.3392, the
// Charting the Unknown DLC build (Steam 26308950). It reordered the ItemInfo
// record, so builds up to 1.0.0.2976 (2.03.02) are no longer supported. The
// exe version stands in until the patch name is confirmed. It has to fit
// StackStatus::gameVersion, which is char[16].
#define SM_GAME     "1.0.0.3392"
// Base name of the plugin's files next to it: StackMaster.asi, .ini, .log.
#define SM_FILEBASE L"StackMaster"
#define SM_INI      L"StackMaster.ini"
// The same as narrow text, for the module name another plugin looks us up by.
#define SM_MODULE   "StackMaster.asi"

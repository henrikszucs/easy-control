#pragma once
#ifndef GAMEPAD_WIN_H
#define GAMEPAD_WIN_H

// The Windows virtual gamepad, through easy-control's own driver
// (src/native/windows-gamepad): the broker service plugs a pad in, and this
// side sends it every state change.

#include <string>

struct WinPad;

// why something failed: a message for people and a stable code for programs
struct WinPadError {
    std::string message;
    std::string code;   // EASYCONTROL_DRIVER_MISSING, EASYCONTROL_DRIVER_OUTDATED, EASYCONTROL_NO_SLOT, ...
};

// plugs in a pad; nullptr and the reason when it cannot
WinPad* WinPadCreate(WinPadError& error);
// W3C Standard Gamepad indices: buttons 0-16, axes 0-5 with -1..1
bool WinPadSetButton(WinPad* pad, int button, bool isDown);
bool WinPadSetAxis(WinPad* pad, int axis, double value);
// unplugs it and frees it
void WinPadDestroy(WinPad* pad);

struct WinDriverStatus {
    bool isInstalled;
    int version;        // installed version, 0 when none
    int required;       // the version this addon needs
};
WinDriverStatus WinDriverGetStatus();

// runs the setup script elevated (one UAC prompt) and waits for it;
// action is "install" or "uninstall"
bool WinDriverRunSetup(const wchar_t* action, WinPadError& error);

#endif

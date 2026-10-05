#pragma once
#ifndef UINPUT_H
#define UINPUT_H

#if defined(IS_LINUX)
    #include <string>

    // Opens /dev/uinput for writing; -1 and a message saying why on failure.
    int OpenUinput(std::string& error);

    // A virtual pointer and keyboard made with uinput. The kernel hands their
    // events to whatever reads input devices - the Wayland compositor (or the
    // X server) through libinput - so they reach every application, unlike
    // XTest, which reaches only X clients. Each device is made on first use;
    // the calls return false, with the reason in `error`, when it cannot be.
    namespace VirtualInput {
        // moves the pointer to a point of the desktop's bounding box, given as
        // fractions of it (0 left/top - 1 right/bottom)
        bool PointerMoveTo(double x, double y, std::string& error);
        // BTN_LEFT, BTN_RIGHT, BTN_MIDDLE, BTN_SIDE or BTN_EXTRA
        bool PointerButton(unsigned short button, bool isDown, std::string& error);
        // REL_WHEEL (positive up) or REL_HWHEEL (positive right), in notches
        bool PointerScroll(unsigned short axis, int amount, std::string& error);
        // an evdev key code (KEY_*)
        bool KeyboardKey(unsigned short key, bool isDown, std::string& error);
    }
#endif

#endif

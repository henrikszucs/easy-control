#pragma once
#ifndef WAYLAND_H
#define WAYLAND_H

#if defined(IS_LINUX)
    #include <vector>

    // Whether input goes through the Wayland path (uinput) rather than X11
    // (XTest). EASY_CONTROL_BACKEND=wayland or =x11 decides; without it, a
    // Wayland session (WAYLAND_DISPLAY set, or XDG_SESSION_TYPE=wayland) does.
    bool IsWaylandSession();

    // An output in the compositor's logical coordinates.
    struct WaylandOutput {
        int x;
        int y;
        int width;
        int height;
        double scaleFactor;
        bool isPrimary;
    };

    // The compositor's outputs, read over the Wayland protocol (wl_output and
    // xdg-output). False when there is no compositor to ask or
    // libwayland-client is missing - it is loaded at run time, so the addon
    // neither builds nor loads against it.
    bool ListWaylandOutputs(std::vector<WaylandOutput>& outputs);
#endif

#endif

#pragma once
#ifndef PLATFORM_H
#define PLATFORM_H

// Helpers the modules share, one set per platform.

#if defined(IS_WINDOWS)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN 1
    #endif
    #include <windows.h>
    #include <napi.h>
    #include <string>
    #include <vector>

    // Throws the Error of input Windows did not take (SendInput sending
    // nothing, the pointer not to be read or set): the secure desktop - a UAC
    // prompt, the lock or sign-in screen - is showing, or input is otherwise
    // not this process's to send. UIPI drops are not reported this way;
    // Platform.getInputBlock() tells about those.
    inline void ThrowInputBlocked(Napi::Env env, const std::string& what) {
        Napi::Error error = Napi::Error::New(env, "Windows did not take the input (" + what +
            "); the secure desktop (a UAC prompt, the lock screen) may be showing, see Platform.getInputBlock()");
        error.Set("code", Napi::String::New(env, "EASYCONTROL_INPUT_BLOCKED"));
        error.ThrowAsJavaScriptException();
    }

    // Runs the enclosed Win32 calls per-monitor DPI aware, so every coordinate
    // they take or return is in physical pixels, whatever DPI awareness the
    // host process (node.exe, Electron, ...) was started with.
    // SetThreadDpiAwarenessContext (Windows 10 1607+) is looked up at run time,
    // so neither the SDK's WINVER nor an older Windows stops the addon loading.
    class DpiScope {
        public:
            DpiScope() {
                SetContextFn setContext = SetContext();
                if (setContext != nullptr) {
                    // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
                    m_previous = setContext((HANDLE)(LONG_PTR)-4);
                }
            }
            ~DpiScope() {
                if (m_previous != NULL) {
                    SetContext()(m_previous);
                }
            }
            DpiScope(const DpiScope&) = delete;
            DpiScope& operator=(const DpiScope&) = delete;
        private:
            typedef HANDLE (WINAPI *SetContextFn)(HANDLE);
            static SetContextFn SetContext() {
                static SetContextFn fn = (SetContextFn)GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetThreadDpiAwarenessContext");
                return fn;
            }
            HANDLE m_previous = NULL;
    };

    // A monitor in physical pixels, with its scale (effective DPI / 96).
    // The logical coordinate space easy-control reports is each monitor's
    // physical rectangle divided by that monitor's own scale.
    struct MonitorLayout {
        RECT rect;
        std::wstring device;    // the GDI device name, "\\.\DISPLAY1"
        double scaleFactor;
        bool isPrimary;
    };

    // all monitors; call it inside a DpiScope (defined in screen.cpp)
    std::vector<MonitorLayout> ListMonitors();

    // physical pixel -> logical coordinate, and back (defined in screen.cpp);
    // call them inside a DpiScope
    void PhysicalToLogical(POINT point, double& x, double& y);
    POINT LogicalToPhysical(double x, double y);

#elif defined(IS_MACOS)
    #include <ApplicationServices/ApplicationServices.h>

    // One event source for every synthesized event, sharing the HID system's
    // state, so the events look like they come from the same device.
    // The pointer warps easy-control makes do not hold back the real mouse.
    inline CGEventSourceRef EventSource() {
        static CGEventSourceRef source = [] {
            CGEventSourceRef created = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
            if (created != NULL) {
                CGEventSourceSetLocalEventsSuppressionInterval(created, 0.0);
            }
            return created;
        }();
        return source;
    }

    // Modifier keys held down through Keyboard.keyDown. Events made with
    // CGEventCreate* do not pick them up by themselves, so every keyboard and
    // mouse event gets them set.
    inline CGEventFlags& ModifierFlags() {
        static CGEventFlags flags = 0;
        return flags;
    }

    // Mouse buttons held down through Mouse.buttonDown (bit = CGMouseButton),
    // so a move while one is held is posted as a drag.
    inline uint32_t& PressedButtons() {
        static uint32_t buttons = 0;
        return buttons;
    }

#elif defined(IS_LINUX)
    #include <X11/Xlib.h>
    #include <string>
    #include <vector>

    // A screen in the coordinates Mouse and Screen use: the compositor's
    // logical ones on Wayland, X11 pixels otherwise.
    struct ScreenRect {
        int x;
        int y;
        int width;
        int height;
        double scaleFactor;
        bool isPrimary;
        std::string id;     // the output name ("HDMI-1", ...)
        std::string name;   // the monitor name for people, "" when unknown
    };

    // every screen; from the Wayland compositor in a Wayland session (when it
    // can be asked), from XRandR otherwise (defined in screen.cpp)
    std::vector<ScreenRect> ListScreens();

    // The X11 connection every module shares, opened on first use (and tried
    // again on the next call while it cannot be opened).
    inline Display* XGetMainDisplay() {
        static Display* display = nullptr;
        if (display == nullptr) {
            display = XOpenDisplay(nullptr);
        }
        return display;
    }
#endif

#endif

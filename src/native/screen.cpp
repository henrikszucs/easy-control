#include "screen.h"
#include "platform.h"

#include <vector>

#if defined(IS_WINDOWS)
    #include <cmath>
    #include <shellscalingapi.h>
    #pragma comment(lib, "Shcore.lib")
#elif defined(IS_MACOS)
    #include <ApplicationServices/ApplicationServices.h>
    #include <CoreGraphics/CoreGraphics.h>
#elif defined(IS_LINUX)
    #include "wayland.h"
    #include <X11/Xlib.h>
    #include <X11/extensions/Xrandr.h>
#endif


#if defined(IS_WINDOWS)
// A monitor's place in the logical coordinate space: its physical rectangle
// and scale, and its logical origin and size.
struct LogicalMonitor {
    MonitorLayout layout;
    LONG x;
    LONG y;
    LONG width;
    LONG height;
    bool isPlaced;
};

static double MonitorScale(HMONITOR hMonitor) {
    UINT dpiX = 96;
    UINT dpiY = 96;
    if (!SUCCEEDED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
        // Fallback to system DPI
        HDC hdc = GetDC(NULL);
        dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(NULL, hdc);
    }
    return (double)dpiX / 96.0;
}

static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    std::vector<MonitorLayout>* monitors = reinterpret_cast<std::vector<MonitorLayout>*>(dwData);

    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfo(hMonitor, &monitorInfo)) {
        MonitorLayout monitor;
        monitor.rect = monitorInfo.rcMonitor;
        monitor.scaleFactor = MonitorScale(hMonitor);
        monitor.isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
        monitors->push_back(monitor);
    }
    return TRUE; // Continue enumeration
}

std::vector<MonitorLayout> ListMonitors() {
    std::vector<MonitorLayout> monitors;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}

// Lay the monitors out in logical pixels. Dividing every physical rectangle by
// its own monitor's scale would make monitors of different scales overlap (and
// leave gaps), so, as Chromium does for Electron's screen API, the primary
// monitor is placed first and every monitor touching a placed one is put
// against it: its logical edge on the placed one's, its offset along that edge
// scaled by the placed one's factor. A monitor touching none falls back to its
// physical origin divided by its own scale.
static std::vector<LogicalMonitor> LayoutMonitors() {
    std::vector<LogicalMonitor> monitors;
    for (const MonitorLayout& layout : ListMonitors()) {
        LogicalMonitor monitor;
        monitor.layout = layout;
        monitor.width = std::lround((layout.rect.right - layout.rect.left) / layout.scaleFactor);
        monitor.height = std::lround((layout.rect.bottom - layout.rect.top) / layout.scaleFactor);
        monitor.x = 0;
        monitor.y = 0;
        monitor.isPlaced = false;
        monitors.push_back(monitor);
    }
    if (monitors.empty()) {
        return monitors;
    }

    size_t primary = 0;
    for (size_t i = 0; i < monitors.size(); i++) {
        if (monitors[i].layout.isPrimary) {
            primary = i;
            break;
        }
    }

    const auto placeAtOrigin = [](LogicalMonitor& monitor) {
        monitor.x = std::lround(monitor.layout.rect.left / monitor.layout.scaleFactor);
        monitor.y = std::lround(monitor.layout.rect.top / monitor.layout.scaleFactor);
        monitor.isPlaced = true;
    };
    placeAtOrigin(monitors[primary]);

    std::vector<size_t> queue = {primary};
    for (size_t q = 0; q < queue.size(); q++) {
        const LogicalMonitor& parent = monitors[queue[q]];
        const RECT& p = parent.layout.rect;
        const double scale = parent.layout.scaleFactor;
        for (size_t i = 0; i < monitors.size(); i++) {
            LogicalMonitor& monitor = monitors[i];
            if (monitor.isPlaced) {
                continue;
            }
            const RECT& m = monitor.layout.rect;
            const bool isRowOverlap = m.top < p.bottom && m.bottom > p.top;
            const bool isColumnOverlap = m.left < p.right && m.right > p.left;
            if (isRowOverlap && m.left == p.right) {
                monitor.x = parent.x + parent.width;
                monitor.y = parent.y + std::lround((m.top - p.top) / scale);
            } else if (isRowOverlap && m.right == p.left) {
                monitor.x = parent.x - monitor.width;
                monitor.y = parent.y + std::lround((m.top - p.top) / scale);
            } else if (isColumnOverlap && m.top == p.bottom) {
                monitor.x = parent.x + std::lround((m.left - p.left) / scale);
                monitor.y = parent.y + parent.height;
            } else if (isColumnOverlap && m.bottom == p.top) {
                monitor.x = parent.x + std::lround((m.left - p.left) / scale);
                monitor.y = parent.y - monitor.height;
            } else {
                continue;
            }
            monitor.isPlaced = true;
            queue.push_back(i);
        }
    }

    for (LogicalMonitor& monitor : monitors) {
        if (!monitor.isPlaced) {
            placeAtOrigin(monitor);
        }
    }
    return monitors;
}

void PhysicalToLogical(POINT point, double& x, double& y) {
    std::vector<LogicalMonitor> monitors = LayoutMonitors();

    // the monitor holding the point, or the nearest one
    const LogicalMonitor* found = nullptr;
    double bestDistance = 0;
    for (const LogicalMonitor& monitor : monitors) {
        const RECT& r = monitor.layout.rect;
        const double dx = (point.x < r.left) ? (r.left - point.x) : (point.x >= r.right ? point.x - r.right + 1 : 0);
        const double dy = (point.y < r.top) ? (r.top - point.y) : (point.y >= r.bottom ? point.y - r.bottom + 1 : 0);
        const double distance = dx * dx + dy * dy;
        if (found == nullptr || distance < bestDistance) {
            found = &monitor;
            bestDistance = distance;
        }
    }
    if (found == nullptr) {
        x = point.x;
        y = point.y;
        return;
    }
    const RECT& r = found->layout.rect;
    x = found->x + (point.x - r.left) / found->layout.scaleFactor;
    y = found->y + (point.y - r.top) / found->layout.scaleFactor;
}

POINT LogicalToPhysical(double x, double y) {
    std::vector<LogicalMonitor> monitors = LayoutMonitors();

    // the monitor holding the point, or the nearest one
    const LogicalMonitor* found = nullptr;
    double bestDistance = 0;
    for (const LogicalMonitor& monitor : monitors) {
        const double right = (double)monitor.x + monitor.width;
        const double bottom = (double)monitor.y + monitor.height;
        const double dx = (x < monitor.x) ? (monitor.x - x) : (x >= right ? x - right + 1 : 0);
        const double dy = (y < monitor.y) ? (monitor.y - y) : (y >= bottom ? y - bottom + 1 : 0);
        const double distance = dx * dx + dy * dy;
        if (found == nullptr || distance < bestDistance) {
            found = &monitor;
            bestDistance = distance;
        }
    }

    POINT point;
    if (found == nullptr) {
        point.x = std::lround(x);
        point.y = std::lround(y);
        return point;
    }
    const RECT& r = found->layout.rect;
    const double scale = found->layout.scaleFactor;
    point.x = r.left + std::lround((x - found->x) * scale);
    point.y = r.top + std::lround((y - found->y) * scale);

    // keep it on that monitor
    point.x = (point.x < r.left) ? r.left : (point.x > r.right - 1 ? r.right - 1 : point.x);
    point.y = (point.y < r.top) ? r.top : (point.y > r.bottom - 1 ? r.bottom - 1 : point.y);
    return point;
}
#endif

#if defined(IS_LINUX)
// the screens as XRandR reports them. X11 has no logical coordinate space: the
// mouse and the screens are both in pixels, so the scale factor is 1.0 - a
// desktop's own UI scaling (GDK_SCALE, Xft.dpi, ...) does not change them.
static std::vector<ScreenRect> ListXScreens() {
    std::vector<ScreenRect> screens;
    Display *display = XGetMainDisplay();
    if (display == NULL) {
        return screens;
    }

    Window root = DefaultRootWindow(display);

    // Check if XRandR extension is available
    int eventBase, errorBase;
    if (XRRQueryExtension(display, &eventBase, &errorBase)) {
        // the current configuration, without probing the outputs again
        XRRScreenResources *screenRes = XRRGetScreenResourcesCurrent(display, root);

        if (screenRes) {
            // Get primary output
            RROutput primary = XRRGetOutputPrimary(display, root);

            for (int i = 0; i < screenRes->noutput; i++) {
                XRROutputInfo *outputInfo = XRRGetOutputInfo(display, screenRes, screenRes->outputs[i]);
                if (outputInfo == NULL) {
                    continue;
                }

                if (outputInfo->connection == RR_Connected && outputInfo->crtc) {
                    XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo(display, screenRes, outputInfo->crtc);
                    if (crtcInfo) {
                        ScreenRect screen;
                        screen.x = crtcInfo->x;
                        screen.y = crtcInfo->y;
                        screen.width = (int)crtcInfo->width;
                        screen.height = (int)crtcInfo->height;
                        screen.scaleFactor = 1.0;
                        screen.isPrimary = (screenRes->outputs[i] == primary);
                        screens.push_back(screen);
                        XRRFreeCrtcInfo(crtcInfo);
                    }
                }

                XRRFreeOutputInfo(outputInfo);
            }

            XRRFreeScreenResources(screenRes);
        }
    }

    if (screens.empty()) {
        // Fallback: Single screen without XRandR
        int screenNumber = DefaultScreen(display);
        ScreenRect screen;
        screen.x = 0;
        screen.y = 0;
        screen.width = DisplayWidth(display, screenNumber);
        screen.height = DisplayHeight(display, screenNumber);
        screen.scaleFactor = 1.0;
        screen.isPrimary = true;
        screens.push_back(screen);
    }
    return screens;
}

std::vector<ScreenRect> ListScreens() {
    // in a Wayland session the compositor knows the layout in its logical
    // coordinates; XWayland only has its own pixels of it
    if (IsWaylandSession()) {
        std::vector<WaylandOutput> outputs;
        if (ListWaylandOutputs(outputs)) {
            std::vector<ScreenRect> screens;
            for (const WaylandOutput& output : outputs) {
                screens.push_back({output.x, output.y, output.width, output.height, output.scaleFactor, output.isPrimary});
            }
            return screens;
        }
    }
    return ListXScreens();
}
#endif

Napi::Array IScreen::list(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    Napi::Array result = Napi::Array::New(env);

    #if defined(IS_WINDOWS)
        DpiScope dpiScope;
        std::vector<LogicalMonitor> monitors = LayoutMonitors();

        for (uint32_t i = 0; i < (uint32_t)monitors.size(); i++) {
            Napi::Object screenObj = Napi::Object::New(env);
            screenObj.Set("isPrimary", Napi::Boolean::New(env, monitors[i].layout.isPrimary));
            screenObj.Set("width", Napi::Number::New(env, monitors[i].width));
            screenObj.Set("height", Napi::Number::New(env, monitors[i].height));
            screenObj.Set("x", Napi::Number::New(env, monitors[i].x));
            screenObj.Set("y", Napi::Number::New(env, monitors[i].y));
            screenObj.Set("scaleFactor", Napi::Number::New(env, monitors[i].layout.scaleFactor));

            result.Set(i, screenObj);
        }

    #elif defined(IS_MACOS)
        uint32_t displayCount = 0;
        CGDirectDisplayID displays[32]; // Support up to 32 displays

        // Get all active displays
        if (CGGetActiveDisplayList(32, displays, &displayCount) == kCGErrorSuccess) {
            CGDirectDisplayID mainDisplay = CGMainDisplayID();

            for (uint32_t i = 0; i < displayCount; i++) {
                Napi::Object screenObj = Napi::Object::New(env);

                CGDirectDisplayID display = displays[i];

                // Check if this is the primary (main) display
                bool isPrimary = (display == mainDisplay);

                // Display bounds in points, in the global display space: origin at
                // the top left of the main display, y growing downward - the same
                // space CGEventGetLocation reports the mouse in
                CGRect bounds = CGDisplayBounds(display);

                // Backing scale factor (2 on Retina displays)
                double scaleFactor = 1.0;
                CGDisplayModeRef mode = CGDisplayCopyDisplayMode(display);
                if (mode) {
                    size_t pixelWidth = CGDisplayModeGetPixelWidth(mode);
                    if (pixelWidth > 0 && bounds.size.width > 0) {
                        scaleFactor = pixelWidth / bounds.size.width;
                    }
                    CGDisplayModeRelease(mode);
                }

                screenObj.Set("isPrimary", Napi::Boolean::New(env, isPrimary));
                screenObj.Set("width", Napi::Number::New(env, (int)bounds.size.width));
                screenObj.Set("height", Napi::Number::New(env, (int)bounds.size.height));
                screenObj.Set("x", Napi::Number::New(env, (int)bounds.origin.x));
                screenObj.Set("y", Napi::Number::New(env, (int)bounds.origin.y));
                screenObj.Set("scaleFactor", Napi::Number::New(env, scaleFactor));

                result.Set(i, screenObj);
            }
        }

    #elif defined(IS_LINUX)
        std::vector<ScreenRect> screens = ListScreens();
        for (uint32_t i = 0; i < (uint32_t)screens.size(); i++) {
            Napi::Object screenObj = Napi::Object::New(env);
            screenObj.Set("isPrimary", Napi::Boolean::New(env, screens[i].isPrimary));
            screenObj.Set("width", Napi::Number::New(env, screens[i].width));
            screenObj.Set("height", Napi::Number::New(env, screens[i].height));
            screenObj.Set("x", Napi::Number::New(env, screens[i].x));
            screenObj.Set("y", Napi::Number::New(env, screens[i].y));
            screenObj.Set("scaleFactor", Napi::Number::New(env, screens[i].scaleFactor));
            result.Set(i, screenObj);
        }

    #endif

    return result;
}


Napi::Object IScreen::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set(Napi::String::New(env, "list"), Napi::Function::New(env, IScreen::list));
    return obj;
}

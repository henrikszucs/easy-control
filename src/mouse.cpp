#include "mouse.h"

#if defined(IS_WINDOWS)
    #include <windows.h>
#elif defined(IS_MACOSX)
    #include <ApplicationServices/ApplicationServices.h>
#elif defined(USE_X11)
    #include <X11/Xlib.h>
    #include <X11/extensions/XTest.h>
    #include <stdlib.h>
    #include "xdisplay.h"
#endif


Napi::Number Mouse::getX(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    #if defined(IS_WINDOWS)
        POINT point;
        GetCursorPos(&point);
        return Napi::Number::New(env, point.x);

    #elif defined(IS_MACOSX)
        CGEventRef event = CGEventCreate(NULL);
        CGPoint cursor = CGEventGetLocation(event);
        CFRelease(event);
        return Napi::Number::New(env, cursor.x);

    #elif defined(USE_X11)
        Display *display = XGetMainDisplay();
        if (display == NULL) {
            return Napi::Number::New(env, 0);
        }
        
        Window root = DefaultRootWindow(display);
        Window window_returned;
        int root_x, root_y;
        int win_x, win_y;
        unsigned int mask_return;
        
        XQueryPointer(display, root, &window_returned,
            &window_returned, &root_x, &root_y,
            &win_x, &win_y, &mask_return);
        
        return Napi::Number::New(env, root_x);

    #endif
}

Napi::Number Mouse::getY(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    #if defined(IS_WINDOWS)
        POINT point;
        GetCursorPos(&point);
        return Napi::Number::New(env, point.y);

    #elif defined(IS_MACOSX)
        CGEventRef event = CGEventCreate(NULL);
        CGPoint cursor = CGEventGetLocation(event);
        CFRelease(event);
        return Napi::Number::New(env, cursor.y);

    #elif defined(USE_X11)
        Display *display = XGetMainDisplay();
        if (display == NULL) {
            return Napi::Number::New(env, 0);
        }
        
        Window root = DefaultRootWindow(display);
        Window window_returned;
        int root_x, root_y;
        int win_x, win_y;
        unsigned int mask_return;
        
        XQueryPointer(display, root, &window_returned, 
            &window_returned, &root_x, &root_y,
            &win_x, &win_y, &mask_return);
        
        return Napi::Number::New(env, root_y);

    #endif
}


Napi::Object Mouse::getIcon(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);

    #if defined(IS_WINDOWS)
        // Get information about the global cursor.
        CURSORINFO ci;
        ci.cbSize = sizeof(ci);
        if (!GetCursorInfo(&ci)) {
            result.Set("width", 0);
            result.Set("height", 0);
            result.Set("data", Napi::Array::New(env, 0));
			result.Set("xOffset", 0);
			result.Set("yOffset", 0);
            return result;
        }

        // Get icon information to determine actual size
        ICONINFO iconInfo;
        if (!GetIconInfo(ci.hCursor, &iconInfo)) {
            result.Set("width", 0);
            result.Set("height", 0);
            result.Set("data", Napi::Array::New(env, 0));
			result.Set("xOffset", 0);
			result.Set("yOffset", 0);
            return result;
        }

        // Get bitmap dimensions
        BITMAP bmp;
        GetObject(iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask, sizeof(BITMAP), &bmp);
        int width = bmp.bmWidth;
        int height = iconInfo.hbmColor ? bmp.bmHeight : bmp.bmHeight / 2;

        // Get your device contexts.
        HDC hdcScreen = GetDC(NULL);
        HDC hdcMem = CreateCompatibleDC(hdcScreen);

        // Create the bitmap to use as a canvas.
        HBITMAP hbmCanvas = CreateCompatibleBitmap(hdcScreen, width, height);

        // Select the bitmap into the device context.
        HGDIOBJ hbmOld = SelectObject(hdcMem, hbmCanvas);

        // Draw the cursor into the canvas.
        DrawIconEx(hdcMem, 0, 0, ci.hCursor, width, height, 0, NULL, DI_NORMAL);

        // Get the pixel data
        Napi::Array pixelData = Napi::Array::New(env, width * height);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                COLORREF clr = GetPixel(hdcMem, x, y);
                pixelData.Set(y * width + x, clr);
            }
        }

        // Clean up after yourself.
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmCanvas);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        
        // Clean up icon info
        if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
        if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);

        result.Set("width", width);
        result.Set("height", height);
        result.Set("data", pixelData);
        result.Set("xOffset", iconInfo.xHotspot);
        result.Set("yOffset", iconInfo.yHotspot);
        
        return result;

    #elif defined(IS_MACOSX)
        // Get the current cursor
        NSCursor *cursor = [NSCursor currentSystemCursor];
        if (cursor == nil) {
            result.Set("width", 0);
            result.Set("height", 0);
            result.Set("data", Napi::Array::New(env, 0));
            result.Set("xOffset", 0);
            result.Set("yOffset", 0);
            return result;
        }

        NSImage *image = [cursor image];
        NSPoint hotspot = [cursor hotSpot];
        
        if (image == nil) {
            result.Set("width", 0);
            result.Set("height", 0);
            result.Set("data", Napi::Array::New(env, 0));
            result.Set("xOffset", 0);
            result.Set("yOffset", 0);
            return result;
        }

        NSSize size = [image size];
        int width = (int)size.width;
        int height = (int)size.height;

        // Create a bitmap representation
        NSBitmapImageRep *bitmap = [[NSBitmapImageRep alloc]
            initWithBitmapDataPlanes:NULL
            pixelsWide:width
            pixelsHigh:height
            bitsPerSample:8
            samplesPerPixel:4
            hasAlpha:YES
            isPlanar:NO
            colorSpaceName:NSDeviceRGBColorSpace
            bytesPerRow:width * 4
            bitsPerPixel:32];

        // Draw the image into the bitmap
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithBitmapImageRep:bitmap]];
        [image drawInRect:NSMakeRect(0, 0, width, height)];
        [NSGraphicsContext restoreGraphicsState];

        // Extract pixel data
        unsigned char *bitmapData = [bitmap bitmapData];
        Napi::Array pixelData = Napi::Array::New(env, width * height);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int offset = (y * width + x) * 4;
                unsigned char r = bitmapData[offset];
                unsigned char g = bitmapData[offset + 1];
                unsigned char b = bitmapData[offset + 2];
                unsigned char a = bitmapData[offset + 3];
                
                // Convert to COLORREF format (0x00BBGGRR) with alpha in high byte
                uint32_t color = (a << 24) | (r << 16) | (g << 8) | b;
                pixelData.Set(y * width + x, color);
            }
        }

        [bitmap release];

        result.Set("width", width);
        result.Set("height", height);
        result.Set("data", pixelData);
        result.Set("xOffset", (int)hotspot.x);
        result.Set("yOffset", (int)hotspot.y);
        
        return result;

    #elif defined(USE_X11)
        Display *display = XGetMainDisplay();
        if (display == NULL) {
            result.Set("width", 0);
            result.Set("height", 0);
            result.Set("data", Napi::Array::New(env, 0));
            result.Set("xOffset", 0);
            result.Set("yOffset", 0);
            return result;
        }

        Window root = DefaultRootWindow(display);
        
        // Query the cursor image using XFixes extension
        XFixesCursorImage *cursorImage = XFixesGetCursorImage(display);
        if (cursorImage == NULL) {
            result.Set("width", 0);
            result.Set("height", 0);
            result.Set("data", Napi::Array::New(env, 0));
            result.Set("xOffset", 0);
            result.Set("yOffset", 0);
            return result;
        }

        int width = cursorImage->width;
        int height = cursorImage->height;
        int xOffset = cursorImage->xhot;
        int yOffset = cursorImage->yhot;

        // Extract pixel data (XFixes returns ARGB format as unsigned long)
        Napi::Array pixelData = Napi::Array::New(env, width * height);
        for (int i = 0; i < width * height; i++) {
            // XFixes cursor pixels are in ARGB format
            unsigned long pixel = cursorImage->pixels[i];
            pixelData.Set(i, (uint32_t)pixel);
        }

        XFree(cursorImage);

        result.Set("width", width);
        result.Set("height", height);
        result.Set("data", pixelData);
        result.Set("xOffset", xOffset);
        result.Set("yOffset", yOffset);
        
        return result;

    #endif
    
}


void Mouse::setX(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected number argument").ThrowAsJavaScriptException();
        return;
    }

    int x = info[0].As<Napi::Number>().Int32Value();

    #if defined(IS_WINDOWS)
        POINT point;
        GetCursorPos(&point);
        SetCursorPos(x, point.y);

    #elif defined(IS_MACOSX)
        CGEventRef event = CGEventCreate(NULL);
        CGPoint cursor = CGEventGetLocation(event);
        CFRelease(event);
        
        CGPoint newPosition;
        newPosition.x = (CGFloat)x;
        newPosition.y = cursor.y;
        
        CGWarpMouseCursorPosition(newPosition);

    #elif defined(USE_X11)
        Display *display = XGetMainDisplay();
        if (display == NULL) {
            return;
        }
        
        Window root = DefaultRootWindow(display);
        Window window_returned;
        int root_x, root_y;
        int win_x, win_y;
        unsigned int mask_return;
        d
        // Get current Y position
        XQueryPointer(display, root, &window_returned,
            &window_returned, &root_x, &root_y,
            &win_x, &win_y, &mask_return);
        
        // Move cursor to new X position, keeping Y the same
        XWarpPointer(display, None, root, 0, 0, 0, 0, x, root_y);
        XFlush(display);

    #endif

    return;
}

void Mouse::setY(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected number argument").ThrowAsJavaScriptException();
        return;
    }

    int y = info[0].As<Napi::Number>().Int32Value();

    #if defined(IS_WINDOWS)
        POINT point;
        GetCursorPos(&point);
        SetCursorPos(point.x, y);

    #elif defined(IS_MACOSX)
        CGEventRef event = CGEventCreate(NULL);
        CGPoint cursor = CGEventGetLocation(event);
        CFRelease(event);
        
        CGPoint newPosition;
        newPosition.x = cursor.x;
        newPosition.y = (CGFloat)y;
        
        CGWarpMouseCursorPosition(newPosition);

    #elif defined(USE_X11)
        Display *display = XGetMainDisplay();
        if (display == NULL) {
            return;
        }
        
        Window root = DefaultRootWindow(display);
        Window window_returned;
        int root_x, root_y;
        int win_x, win_y;
        unsigned int mask_return;
        
        // Get current X position
        XQueryPointer(display, root, &window_returned,
            &window_returned, &root_x, &root_y,
            &win_x, &win_y, &mask_return);
        
        // Move cursor to new Y position, keeping X the same
        XWarpPointer(display, None, root, 0, 0, 0, 0, root_x, y);
        XFlush(display);

    #endif

    return;
}


void Mouse::buttonDown(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Expected string argument").ThrowAsJavaScriptException();
        return;
    }

    std::string button = info[0].As<Napi::String>().Utf8Value();

    if (button != "left" &&
        button != "middle" &&
        button != "right" &&
        button != "back" &&
        button != "forward") {
        Napi::TypeError::New(env, "Expected 'left', 'middle', 'right', 'back', or 'forward'").ThrowAsJavaScriptException();
        return;
    }

    #if defined(IS_WINDOWS)
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        
        if (button == "left") {
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        } else if (button == "right") {
            input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        } else if (button == "middle") {
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        } else if (button == "back") {
            input.mi.dwFlags = MOUSEEVENTF_XDOWN;
            input.mi.mouseData = XBUTTON1;
        } else if (button == "forward") {
            input.mi.dwFlags = MOUSEEVENTF_XDOWN;
            input.mi.mouseData = XBUTTON2;
        }
        
        SendInput(1, &input, sizeof(INPUT));

    #elif defined(IS_MACOSX)
        CGEventRef event = CGEventCreate(NULL);
        CGPoint cursor = CGEventGetLocation(event);
        CFRelease(event);
        
        CGEventType eventType;
        CGMouseButton mouseButton;
        
        if (button == "left") {
            eventType = kCGEventLeftMouseDown;
            mouseButton = kCGMouseButtonLeft;
        } else if (button == "right") {
            eventType = kCGEventRightMouseDown;
            mouseButton = kCGMouseButtonRight;
        } else if (button == "middle") {
            eventType = kCGEventOtherMouseDown;
            mouseButton = kCGMouseButtonCenter;
        } else if (button == "back") {
            eventType = kCGEventOtherMouseDown;
            mouseButton = (CGMouseButton)3; // Back button
        } else if (button == "forward") {
            eventType = kCGEventOtherMouseDown;
            mouseButton = (CGMouseButton)4; // Forward button
        }
        
        CGEventRef mouseEvent = CGEventCreateMouseEvent(NULL, eventType, cursor, mouseButton);
        CGEventPost(kCGHIDEventTap, mouseEvent);
        CFRelease(mouseEvent);

    #elif defined(USE_X11)
        Display *display = XGetMainDisplay();
        if (display == NULL) {
            return;
        }
        
        unsigned int xButton;
        
        if (button == "left") {
            xButton = Button1;
        } else if (button == "middle") {
            xButton = Button2;
        } else if (button == "right") {
            xButton = Button3;
        } else if (button == "back") {
            xButton = 8; // X11 back button
        } else if (button == "forward") {
            xButton = 9; // X11 forward button
        }
        
        XTestFakeButtonEvent(display, xButton, True, CurrentTime);
        XFlush(display);

    #endif

    return;

    return;
}

void Mouse::buttonUp(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Expected string argument").ThrowAsJavaScriptException();
        return;
    }

    std::string button = info[0].As<Napi::String>().Utf8Value();

    if (button != "left" &&
        button != "middle" &&
        button != "right" &&
        button != "back" &&
        button != "forward") {
        Napi::TypeError::New(env, "Expected 'left', 'middle', 'right', 'back', or 'forward'").ThrowAsJavaScriptException();
        return;
    }

    #if defined(IS_WINDOWS)
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        
        if (button == "left") {
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        } else if (button == "right") {
            input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        } else if (button == "middle") {
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        } else if (button == "back") {
            input.mi.dwFlags = MOUSEEVENTF_XUP;
            input.mi.mouseData = XBUTTON1;
        } else if (button == "forward") {
            input.mi.dwFlags = MOUSEEVENTF_XUP;
            input.mi.mouseData = XBUTTON2;
        }
        
        SendInput(1, &input, sizeof(INPUT));

    #elif defined(IS_MACOSX)
        CGEventRef event = CGEventCreate(NULL);
        CGPoint cursor = CGEventGetLocation(event);
        CFRelease(event);
        
        CGEventType eventType;
        CGMouseButton mouseButton;
        
        if (button == "left") {
            eventType = kCGEventLeftMouseUp;
            mouseButton = kCGMouseButtonLeft;
        } else if (button == "right") {
            eventType = kCGEventRightMouseUp;
            mouseButton = kCGMouseButtonRight;
        } else if (button == "middle") {
            eventType = kCGEventOtherMouseUp;
            mouseButton = kCGMouseButtonCenter;
        } else if (button == "back") {
            eventType = kCGEventOtherMouseUp;
            mouseButton = (CGMouseButton)3; // Back button
        } else if (button == "forward") {
            eventType = kCGEventOtherMouseUp;
            mouseButton = (CGMouseButton)4; // Forward button
        }
        
        CGEventRef mouseEvent = CGEventCreateMouseEvent(NULL, eventType, cursor, mouseButton);
        CGEventPost(kCGHIDEventTap, mouseEvent);
        CFRelease(mouseEvent);

    #elif defined(USE_X11)
        Display *display = XGetMainDisplay();
        if (display == NULL) {
            return;
        }
        
        unsigned int xButton;
        
        if (button == "left") {
            xButton = Button1;
        } else if (button == "middle") {
            xButton = Button2;
        } else if (button == "right") {
            xButton = Button3;
        } else if (button == "back") {
            xButton = 8; // X11 back button
        } else if (button == "forward") {
            xButton = 9; // X11 forward button
        }
        
        XTestFakeButtonEvent(display, xButton, False, CurrentTime);
        XFlush(display);

    #endif

    return;
}


void Mouse::scrollDown(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Expected 2 argument").ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected number in 1st argument").ThrowAsJavaScriptException();
        return;
    }

    if (!info[1].IsBoolean()) {
        Napi::TypeError::New(env, "Expected boolean 2nd argument").ThrowAsJavaScriptException();
        return;
    }

    int amount = info[0].As<Napi::Number>().Int32Value();
    bool isHorizontal = info[1].As<Napi::Boolean>().Value();

    #if defined(IS_WINDOWS)
        INPUT input = {0};
        input.type = INPUT_MOUSE;

        if (isHorizontal) {
            // Horizontal scroll (scroll right)
            input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
            input.mi.mouseData = -amount * WHEEL_DELTA; // Negative for scrolling right
        } else {
            // Vertical scroll (scroll down)
            input.mi.dwFlags = MOUSEEVENTF_WHEEL;
            input.mi.mouseData = -amount * WHEEL_DELTA; // Negative for scrolling down
        }
        
        SendInput(1, &input, sizeof(INPUT));

    #elif defined(IS_MACOSX)
        CGEventRef event = CGEventCreate(NULL);
        CGPoint cursor = CGEventGetLocation(event);
        CFRelease(event);
        
        CGEventRef scrollEvent;
        
        if (isHorizontal) {
            // Horizontal scroll (scroll right)
            scrollEvent = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitLine, 2, 0, -amount);
        } else {
            // Vertical scroll (scroll down)
            scrollEvent = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitLine, 1, -amount);
        }
        
        CGEventPost(kCGHIDEventTap, scrollEvent);
        CFRelease(scrollEvent);

    #elif defined(USE_X11)
        Display *display = XGetMainDisplay();
        if (display == NULL) {
            return;
        }
        
        unsigned int button;
        
        if (isHorizontal) {
            // Horizontal scroll right (button 7)
            button = 7;
        } else {
            // Vertical scroll down (button 5)
            button = 5;
        }
        
        // Simulate multiple scroll events based on amount
        for (int i = 0; i < amount; i++) {
            XTestFakeButtonEvent(display, button, True, CurrentTime);
            XTestFakeButtonEvent(display, button, False, CurrentTime);
        }
        
        XFlush(display);

    #endif

    return;
}

void Mouse::scrollUp(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Expected 2 argument").ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected number in 1st argument").ThrowAsJavaScriptException();
        return;
    }

    if (!info[1].IsBoolean()) {
        Napi::TypeError::New(env, "Expected boolean 2nd argument").ThrowAsJavaScriptException();
        return;
    }

    int amount = info[0].As<Napi::Number>().Int32Value();
    bool isHorizontal = info[1].As<Napi::Boolean>().Value();

    #if defined(IS_WINDOWS)
        INPUT input = {0};
        input.type = INPUT_MOUSE;

        if (isHorizontal) {
            // Horizontal scroll (scroll left)
            input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
            input.mi.mouseData = amount * WHEEL_DELTA; // Positive for scrolling left
        } else {
            // Vertical scroll (scroll up)
            input.mi.dwFlags = MOUSEEVENTF_WHEEL;
            input.mi.mouseData = amount * WHEEL_DELTA; // Positive for scrolling up
        }
        
        SendInput(1, &input, sizeof(INPUT));

    #elif defined(IS_MACOSX)
        CGEventRef event = CGEventCreate(NULL);
        CGPoint cursor = CGEventGetLocation(event);
        CFRelease(event);
        
        CGEventRef scrollEvent;
        
        if (isHorizontal) {
            // Horizontal scroll (scroll left)
            scrollEvent = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitLine, 2, 0, amount);
        } else {
            // Vertical scroll (scroll up)
            scrollEvent = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitLine, 1, amount);
        }
        
        CGEventPost(kCGHIDEventTap, scrollEvent);
        CFRelease(scrollEvent);

    #elif defined(USE_X11)
        Display *display = XGetMainDisplay();
        if (display == NULL) {
            return;
        }
        
        unsigned int button;
        
        if (isHorizontal) {
            // Horizontal scroll left (button 6)
            button = 6;
        } else {
            // Vertical scroll up (button 4)
            button = 4;
        }
        
        // Simulate multiple scroll events based on amount
        for (int i = 0; i < amount; i++) {
            XTestFakeButtonEvent(display, button, True, CurrentTime);
            XTestFakeButtonEvent(display, button, False, CurrentTime);
        }
        
        XFlush(display);

    #endif

    return;
}


Napi::Object Mouse::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set(Napi::String::New(env, "getX"), Napi::Function::New(env, Mouse::getX));
    obj.Set(Napi::String::New(env, "getY"), Napi::Function::New(env, Mouse::getY));

    obj.Set(Napi::String::New(env, "getIcon"), Napi::Function::New(env, Mouse::getIcon));

    obj.Set(Napi::String::New(env, "setX"), Napi::Function::New(env, Mouse::setX));
    obj.Set(Napi::String::New(env, "setY"), Napi::Function::New(env, Mouse::setY));

    obj.Set(Napi::String::New(env, "buttonDown"), Napi::Function::New(env, Mouse::buttonDown));
    obj.Set(Napi::String::New(env, "buttonUp"), Napi::Function::New(env, Mouse::buttonUp));

    obj.Set(Napi::String::New(env, "scrollDown"), Napi::Function::New(env, Mouse::scrollDown));
    obj.Set(Napi::String::New(env, "scrollUp"), Napi::Function::New(env, Mouse::scrollUp));
    return obj;
}
#include "keyboard.h"

#if defined(IS_WINDOWS)
    #define STRICT
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <cctype>  // for toupper
#elif defined(IS_MACOS)
    #include <ApplicationServices/ApplicationServices.h>
    #include <Carbon/Carbon.h>
    #include <CoreFoundation/CoreFoundation.h>
    #include <cctype>  // for tolower
#elif defined(IS_LINUX)
    #include <X11/Xlib.h>
    #include <X11/keysym.h>
    #include <X11/extensions/XTest.h>
    #include "xdisplay.h"
#endif

#include <string>


// Helper function to convert KeyboardEvent.key string to Windows Virtual Key Code
#if defined(IS_WINDOWS)
WORD keyToVirtualKeyCode(const std::string& key) {
    WORD vkCode = 0;
    
    if (key.length() == 1) {
        // Single character - convert to uppercase for VK code
        char c = toupper(key[0]);
        if (c >= 'A' && c <= 'Z') {
            vkCode = c;
        } else if (c >= '0' && c <= '9') {
            vkCode = c;
        } else {
            // Special characters
            vkCode = VkKeyScanA(key[0]) & 0xFF;
        }
    } else {
        // Web API KeyboardEvent.key standard strings
        if (key == "Enter") vkCode = VK_RETURN;
        else if (key == "Escape") vkCode = VK_ESCAPE;
        else if (key == "Backspace") vkCode = VK_BACK;
        else if (key == "Tab") vkCode = VK_TAB;
        else if (key == " " || key == "Space") vkCode = VK_SPACE;
        else if (key == "Shift") vkCode = VK_SHIFT;
        else if (key == "Control") vkCode = VK_CONTROL;
        else if (key == "Alt") vkCode = VK_MENU;
        else if (key == "CapsLock") vkCode = VK_CAPITAL;
        else if (key == "ArrowUp") vkCode = VK_UP;
        else if (key == "ArrowDown") vkCode = VK_DOWN;
        else if (key == "ArrowLeft") vkCode = VK_LEFT;
        else if (key == "ArrowRight") vkCode = VK_RIGHT;
        else if (key == "Home") vkCode = VK_HOME;
        else if (key == "End") vkCode = VK_END;
        else if (key == "PageUp") vkCode = VK_PRIOR;
        else if (key == "PageDown") vkCode = VK_NEXT;
        else if (key == "Delete") vkCode = VK_DELETE;
        else if (key == "Insert") vkCode = VK_INSERT;
        else if (key == "F1") vkCode = VK_F1;
        else if (key == "F2") vkCode = VK_F2;
        else if (key == "F3") vkCode = VK_F3;
        else if (key == "F4") vkCode = VK_F4;
        else if (key == "F5") vkCode = VK_F5;
        else if (key == "F6") vkCode = VK_F6;
        else if (key == "F7") vkCode = VK_F7;
        else if (key == "F8") vkCode = VK_F8;
        else if (key == "F9") vkCode = VK_F9;
        else if (key == "F10") vkCode = VK_F10;
        else if (key == "F11") vkCode = VK_F11;
        else if (key == "F12") vkCode = VK_F12;
        else if (key == "NumLock") vkCode = VK_NUMLOCK;
        else if (key == "ScrollLock") vkCode = VK_SCROLL;
        else if (key == "Pause") vkCode = VK_PAUSE;
        else if (key == "PrintScreen") vkCode = VK_SNAPSHOT;
        else if (key == "Meta" || key == "OS") vkCode = VK_LWIN;
        else if (key == "ContextMenu") vkCode = VK_APPS;
        // Additional modifier keys
        else if (key == "ShiftLeft") vkCode = VK_LSHIFT;
        else if (key == "ShiftRight") vkCode = VK_RSHIFT;
        else if (key == "ControlLeft") vkCode = VK_LCONTROL;
        else if (key == "ControlRight") vkCode = VK_RCONTROL;
        else if (key == "AltLeft") vkCode = VK_LMENU;
        else if (key == "AltRight") vkCode = VK_RMENU;
        else if (key == "MetaLeft" || key == "OSLeft") vkCode = VK_LWIN;
        else if (key == "MetaRight" || key == "OSRight") vkCode = VK_RWIN;
    }
    
    return vkCode;
}

#elif defined(IS_MACOS)
CGKeyCode keyToVirtualKeyCode(const std::string& key) {
    CGKeyCode keyCode = 0;
    
    if (key.length() == 1) {
        // Single character
        char c = tolower(key[0]);
        if (c >= 'a' && c <= 'z') {
            // Map a-z to keycodes 0-25
            keyCode = c - 'a';
        } else if (c >= '0' && c <= '9') {
            // Map 0-9 to keycodes
            if (c == '0') keyCode = 29;
            else keyCode = 18 + (c - '1');
        }
    } else {
        // Web API KeyboardEvent.key standard strings
        if (key == "Enter") keyCode = 36;
        else if (key == "Escape") keyCode = 53;
        else if (key == "Backspace") keyCode = 51;
        else if (key == "Tab") keyCode = 48;
        else if (key == " " || key == "Space") keyCode = 49;
        else if (key == "Shift" || key == "ShiftLeft") keyCode = 56;
        else if (key == "ShiftRight") keyCode = 60;
        else if (key == "Control" || key == "ControlLeft") keyCode = 59;
        else if (key == "ControlRight") keyCode = 62;
        else if (key == "Alt" || key == "AltLeft") keyCode = 58;
        else if (key == "AltRight") keyCode = 61;
        else if (key == "Meta" || key == "OS" || key == "MetaLeft" || key == "OSLeft") keyCode = 55;
        else if (key == "MetaRight" || key == "OSRight") keyCode = 54;
        else if (key == "CapsLock") keyCode = 57;
        else if (key == "ArrowUp") keyCode = 126;
        else if (key == "ArrowDown") keyCode = 125;
        else if (key == "ArrowLeft") keyCode = 123;
        else if (key == "ArrowRight") keyCode = 124;
        else if (key == "Home") keyCode = 115;
        else if (key == "End") keyCode = 119;
        else if (key == "PageUp") keyCode = 116;
        else if (key == "PageDown") keyCode = 121;
        else if (key == "Delete") keyCode = 117;
        else if (key == "F1") keyCode = 122;
        else if (key == "F2") keyCode = 120;
        else if (key == "F3") keyCode = 99;
        else if (key == "F4") keyCode = 118;
        else if (key == "F5") keyCode = 96;
        else if (key == "F6") keyCode = 97;
        else if (key == "F7") keyCode = 98;
        else if (key == "F8") keyCode = 100;
        else if (key == "F9") keyCode = 101;
        else if (key == "F10") keyCode = 109;
        else if (key == "F11") keyCode = 103;
        else if (key == "F12") keyCode = 111;
    }
    
    return keyCode;
}

#elif defined(IS_LINUX)
KeySym keyToVirtualKeyCode(const std::string& key) {
    KeySym keySym = 0;
    
    if (key.length() == 1) {
        // Single character - use directly
        char c = key[0];
        if (c >= 'A' && c <= 'Z') {
            keySym = XK_A + (c - 'A');
        } else if (c >= 'a' && c <= 'z') {
            keySym = XK_a + (c - 'a');
        } else if (c >= '0' && c <= '9') {
            keySym = XK_0 + (c - '0');
        } else {
            // Special characters - map common ones
            switch (c) {
                case ' ': keySym = XK_space; break;
                case '!': keySym = XK_exclam; break;
                case '@': keySym = XK_at; break;
                case '#': keySym = XK_numbersign; break;
                case '$': keySym = XK_dollar; break;
                case '%': keySym = XK_percent; break;
                case '^': keySym = XK_asciicircum; break;
                case '&': keySym = XK_ampersand; break;
                case '*': keySym = XK_asterisk; break;
                case '(': keySym = XK_parenleft; break;
                case ')': keySym = XK_parenright; break;
                case '-': keySym = XK_minus; break;
                case '_': keySym = XK_underscore; break;
                case '=': keySym = XK_equal; break;
                case '+': keySym = XK_plus; break;
                case '[': keySym = XK_bracketleft; break;
                case ']': keySym = XK_bracketright; break;
                case '{': keySym = XK_braceleft; break;
                case '}': keySym = XK_braceright; break;
                case '\\': keySym = XK_backslash; break;
                case '|': keySym = XK_bar; break;
                case ';': keySym = XK_semicolon; break;
                case ':': keySym = XK_colon; break;
                case '\'': keySym = XK_apostrophe; break;
                case '"': keySym = XK_quotedbl; break;
                case ',': keySym = XK_comma; break;
                case '.': keySym = XK_period; break;
                case '<': keySym = XK_less; break;
                case '>': keySym = XK_greater; break;
                case '/': keySym = XK_slash; break;
                case '?': keySym = XK_question; break;
                case '`': keySym = XK_grave; break;
                case '~': keySym = XK_asciitilde; break;
            }
        }
    } else {
        // Web API KeyboardEvent.key standard strings
        if (key == "Enter") keySym = XK_Return;
        else if (key == "Escape") keySym = XK_Escape;
        else if (key == "Backspace") keySym = XK_BackSpace;
        else if (key == "Tab") keySym = XK_Tab;
        else if (key == " " || key == "Space") keySym = XK_space;
        else if (key == "Shift" || key == "ShiftLeft") keySym = XK_Shift_L;
        else if (key == "ShiftRight") keySym = XK_Shift_R;
        else if (key == "Control" || key == "ControlLeft") keySym = XK_Control_L;
        else if (key == "ControlRight") keySym = XK_Control_R;
        else if (key == "Alt" || key == "AltLeft") keySym = XK_Alt_L;
        else if (key == "AltRight") keySym = XK_Alt_R;
        else if (key == "Meta" || key == "OS" || key == "MetaLeft" || key == "OSLeft") keySym = XK_Super_L;
        else if (key == "MetaRight" || key == "OSRight") keySym = XK_Super_R;
        else if (key == "CapsLock") keySym = XK_Caps_Lock;
        else if (key == "ArrowUp") keySym = XK_Up;
        else if (key == "ArrowDown") keySym = XK_Down;
        else if (key == "ArrowLeft") keySym = XK_Left;
        else if (key == "ArrowRight") keySym = XK_Right;
        else if (key == "Home") keySym = XK_Home;
        else if (key == "End") keySym = XK_End;
        else if (key == "PageUp") keySym = XK_Page_Up;
        else if (key == "PageDown") keySym = XK_Page_Down;
        else if (key == "Delete") keySym = XK_Delete;
        else if (key == "Insert") keySym = XK_Insert;
        else if (key == "F1") keySym = XK_F1;
        else if (key == "F2") keySym = XK_F2;
        else if (key == "F3") keySym = XK_F3;
        else if (key == "F4") keySym = XK_F4;
        else if (key == "F5") keySym = XK_F5;
        else if (key == "F6") keySym = XK_F6;
        else if (key == "F7") keySym = XK_F7;
        else if (key == "F8") keySym = XK_F8;
        else if (key == "F9") keySym = XK_F9;
        else if (key == "F10") keySym = XK_F10;
        else if (key == "F11") keySym = XK_F11;
        else if (key == "F12") keySym = XK_F12;
        else if (key == "NumLock") keySym = XK_Num_Lock;
        else if (key == "ScrollLock") keySym = XK_Scroll_Lock;
        else if (key == "Pause") keySym = XK_Pause;
        else if (key == "PrintScreen") keySym = XK_Print;
        else if (key == "ContextMenu") keySym = XK_Menu;
    }
    
    return keySym;
}

#endif


void Keyboard::keyDown(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Expected string argument").ThrowAsJavaScriptException();
        return;
    }

    std::string key = info[0].As<Napi::String>().Utf8Value();

    #if defined(IS_WINDOWS)
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        
        WORD vkCode = keyToVirtualKeyCode(key);
        
        if (vkCode != 0) {
            input.ki.wVk = vkCode;
            input.ki.dwFlags = 0; // 0 means key press (down)
            SendInput(1, &input, sizeof(INPUT));
        }

    #elif defined(IS_MACOS)
        CGKeyCode keyCode = keyToVirtualKeyCode(key);
        
        if (keyCode != 0) {
            CGEventRef keyDownEvent = CGEventCreateKeyboardEvent(NULL, keyCode, true);
            CGEventPost(kCGHIDEventTap, keyDownEvent);
            CFRelease(keyDownEvent);
        }

    #elif defined(IS_LINUX)
        Display *display = XGetMainDisplay();
        if (display == NULL) {
            return;
        }
        
        KeySym keySym = keyToVirtualKeyCode(key);
        
        if (keySym != 0) {
            KeyCode keyCode = XKeysymToKeycode(display, keySym);
            if (keyCode != 0) {
                XTestFakeKeyEvent(display, keyCode, True, CurrentTime);
                XFlush(display);
            }
        }

    #endif

    return;
}

void Keyboard::keyUp(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Expected string argument").ThrowAsJavaScriptException();
        return;
    }

    std::string key = info[0].As<Napi::String>().Utf8Value();

    #if defined(IS_WINDOWS)
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        
        WORD vkCode = keyToVirtualKeyCode(key);
        
        if (vkCode != 0) {
            input.ki.wVk = vkCode;
            input.ki.dwFlags = KEYEVENTF_KEYUP; // Key release (up)
            SendInput(1, &input, sizeof(INPUT));
        }

    #elif defined(IS_MACOS)
        CGKeyCode keyCode = keyToVirtualKeyCode(key);
        
        if (keyCode != 0) {
            CGEventRef keyUpEvent = CGEventCreateKeyboardEvent(NULL, keyCode, false);
            CGEventPost(kCGHIDEventTap, keyUpEvent);
            CFRelease(keyUpEvent);
        }

    #elif defined(IS_LINUX)
        Display *display = XGetMainDisplay();
        if (display == NULL) {
            return;
        }
        
        KeySym keySym = keyToVirtualKeyCode(key);
        
        if (keySym != 0) {
            KeyCode keyCode = XKeysymToKeycode(display, keySym);
            if (keyCode != 0) {
                XTestFakeKeyEvent(display, keyCode, False, CurrentTime);
                XFlush(display);
            }
        }

    #endif

    return;
}

void Keyboard::type(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Expected string argument").ThrowAsJavaScriptException();
        return;
    }

    std::string text = info[0].As<Napi::String>().Utf8Value();

    #if defined(IS_WINDOWS)
        for (size_t i = 0; i < text.length(); i++) {
            INPUT inputs[2] = {0};
            
            // Key down
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = 0;
            inputs[0].ki.wScan = text[i];
            inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
            
            // Key up
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = 0;
            inputs[1].ki.wScan = text[i];
            inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
            
            SendInput(2, inputs, sizeof(INPUT));
        }

    #elif defined(IS_MACOS)
        // Convert std::string to UTF-16 for macOS
        CFStringRef stringRef = CFStringCreateWithCString(NULL, text.c_str(), kCFStringEncodingUTF8);
        CFIndex length = CFStringGetLength(stringRef);
        
        for (CFIndex i = 0; i < length; i++) {
            UniChar character = CFStringGetCharacterAtIndex(stringRef, i);
            
            // Create keyboard event with Unicode character
            CGEventRef keyDownEvent = CGEventCreateKeyboardEvent(NULL, 0, true);
            CGEventRef keyUpEvent = CGEventCreateKeyboardEvent(NULL, 0, false);
            
            // Set the Unicode character
            CGEventKeyboardSetUnicodeString(keyDownEvent, 1, &character);
            CGEventKeyboardSetUnicodeString(keyUpEvent, 1, &character);
            
            // Post the events
            CGEventPost(kCGHIDEventTap, keyDownEvent);
            CGEventPost(kCGHIDEventTap, keyUpEvent);
            
            // Release events
            CFRelease(keyDownEvent);
            CFRelease(keyUpEvent);
        }
        
        CFRelease(stringRef);

    #elif defined(IS_LINUX)
        Display *display = XGetMainDisplay();
        if (display == NULL) {
            return;
        }
        
        // Type each character in the string
        for (size_t i = 0; i < text.length(); i++) {
            char c = text[i];
            KeySym keySym = 0;
            
            // Convert character to KeySym
            if (c >= 'A' && c <= 'Z') {
                keySym = XK_A + (c - 'A');
            } else if (c >= 'a' && c <= 'z') {
                keySym = XK_a + (c - 'a');
            } else if (c >= '0' && c <= '9') {
                keySym = XK_0 + (c - '0');
            } else {
                // Special characters
                switch (c) {
                    case ' ': keySym = XK_space; break;
                    case '!': keySym = XK_exclam; break;
                    case '@': keySym = XK_at; break;
                    case '#': keySym = XK_numbersign; break;
                    case '$': keySym = XK_dollar; break;
                    case '%': keySym = XK_percent; break;
                    case '^': keySym = XK_asciicircum; break;
                    case '&': keySym = XK_ampersand; break;
                    case '*': keySym = XK_asterisk; break;
                    case '(': keySym = XK_parenleft; break;
                    case ')': keySym = XK_parenright; break;
                    case '-': keySym = XK_minus; break;
                    case '_': keySym = XK_underscore; break;
                    case '=': keySym = XK_equal; break;
                    case '+': keySym = XK_plus; break;
                    case '[': keySym = XK_bracketleft; break;
                    case ']': keySym = XK_bracketright; break;
                    case '{': keySym = XK_braceleft; break;
                    case '}': keySym = XK_braceright; break;
                    case '\\': keySym = XK_backslash; break;
                    case '|': keySym = XK_bar; break;
                    case ';': keySym = XK_semicolon; break;
                    case ':': keySym = XK_colon; break;
                    case '\'': keySym = XK_apostrophe; break;
                    case '"': keySym = XK_quotedbl; break;
                    case ',': keySym = XK_comma; break;
                    case '.': keySym = XK_period; break;
                    case '<': keySym = XK_less; break;
                    case '>': keySym = XK_greater; break;
                    case '/': keySym = XK_slash; break;
                    case '?': keySym = XK_question; break;
                    case '`': keySym = XK_grave; break;
                    case '~': keySym = XK_asciitilde; break;
                    case '\n': keySym = XK_Return; break;
                    case '\t': keySym = XK_Tab; break;
                }
            }
            
            if (keySym != 0) {
                KeyCode keyCode = XKeysymToKeycode(display, keySym);
                if (keyCode != 0) {
                    // Check if shift is needed for uppercase or special chars
                    bool needShift = (c >= 'A' && c <= 'Z') || 
                                   (c == '!' || c == '@' || c == '#' || c == '$' || c == '%' || 
                                    c == '^' || c == '&' || c == '*' || c == '(' || c == ')' ||
                                    c == '_' || c == '+' || c == '{' || c == '}' || c == '|' ||
                                    c == ':' || c == '"' || c == '<' || c == '>' || c == '?' ||
                                    c == '~');
                    
                    if (needShift) {
                        KeyCode shiftCode = XKeysymToKeycode(display, XK_Shift_L);
                        XTestFakeKeyEvent(display, shiftCode, True, CurrentTime);
                    }
                    
                    // Type the character
                    XTestFakeKeyEvent(display, keyCode, True, CurrentTime);
                    XTestFakeKeyEvent(display, keyCode, False, CurrentTime);
                    
                    if (needShift) {
                        KeyCode shiftCode = XKeysymToKeycode(display, XK_Shift_L);
                        XTestFakeKeyEvent(display, shiftCode, False, CurrentTime);
                    }
                }
            }
        }
        
        XFlush(display);

    #endif

    return;
}

Napi::Object Keyboard::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set(Napi::String::New(env, "keyDown"), Napi::Function::New(env, Keyboard::keyDown));
    obj.Set(Napi::String::New(env, "keyUp"), Napi::Function::New(env, Keyboard::keyUp));

    obj.Set(Napi::String::New(env, "type"), Napi::Function::New(env, Keyboard::type));
    
    return obj;
}





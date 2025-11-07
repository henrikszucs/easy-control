#include "keyboard.h"

#if defined(IS_WINDOWS)
    #define STRICT
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <cctype>  // for toupper
    #include <fcntl.h>
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
#include <map>



// Helper function to convert KeyboardEvent.key string to Windows Virtual Key Code
#if defined(IS_WINDOWS)
std::map<std::string, WORD> SpecialKeys = {
    {"Escape", 0x0001},
    {"Digit1", 0x0002},
    {"Digit2", 0x0003},
    {"Digit3", 0x0004},
    {"Digit4", 0x0005},
    {"Digit5", 0x0006},
    {"Digit6", 0x0007},
    {"Digit7", 0x0008},
    {"Digit8", 0x0009},
    {"Digit9", 0x000A},
    {"Digit0", 0x000B},
    {"Minus", 0x000C},
    {"Equal", 0x000D},
    {"Backspace", 0x000E},
    {"Tab", 0x000F},
    {"KeyQ", 0x0010},
    {"KeyW", 0x0011},
    {"KeyE", 0x0012},
    {"KeyR", 0x0013},
    {"KeyT", 0x0014},
    {"KeyY", 0x0015},
    {"KeyU", 0x0016},
    {"KeyI", 0x0017},
    {"KeyO", 0x0018},
    {"KeyP", 0x0019},
    {"BracketLeft", 0x001A},
    {"BracketRight", 0x001B},
    {"Enter", 0x001C},
    {"ControlLeft", 0x001D},
    {"KeyA", 0x001E},
    {"KeyS", 0x001F},
    {"KeyD", 0x0020},
    {"KeyF", 0x0021},
    {"KeyG", 0x0022},
    {"KeyH", 0x0023},
    {"KeyJ", 0x0024},
    {"KeyK", 0x0025},
    {"KeyL", 0x0026},
    {"Semicolon", 0x0027},
    {"Quote", 0x0028},
    {"Backquote", 0x0029},
    {"ShiftLeft", 0x002A},
    {"Backslash", 0x002B},
    {"KeyZ", 0x002C},
    {"KeyX", 0x002D},
    {"KeyC", 0x002E},
    {"KeyV", 0x002F},
    {"KeyB", 0x0030},
    {"KeyN", 0x0031},
    {"KeyM", 0x0032},
    {"Comma", 0x0033},
    {"Period", 0x0034},
    {"Slash", 0x0035},
    {"ShiftRight", 0x0036},
    {"NumpadMultiply", 0x0037},
    {"AltLeft", 0x0038},
    {"Space", 0x0039},
    {"CapsLock", 0x003A},
    {"F1", 0x003B},
    {"F2", 0x003C},
    {"F3", 0x003D},
    {"F4", 0x003E},
    {"F5", 0x003F},
    {"F6", 0x0040},
    {"F7", 0x0041},
    {"F8", 0x0042},
    {"F9", 0x0043},
    {"F10", 0x0044},
    {"Pause", 0x0045},
    {"ScrollLock", VK_FINAL},
    {"Numpad7", 0},
    {"Numpad8", 0},
    {"Numpad9", 0},
    {"NumpadSubtract", 0},
    {"Numpad4", VK_MODECHANGE},
    {"Numpad5", 0},
    {"Numpad6", VK_NONCONVERT},
    {"NumpadAdd", 0},
    {"Numpad1", VK_PROCESSKEY},
    {"Numpad2", 0},
    {"Numpad3", VK_HANGUL},
    {"Numpad0", VK_HANJA},
    {"NumpadDecimal", VK_JUNJA},
    {"PrintScreen", 0},
    {"IntlBackslash", VK_OEM_AUTO},
    {"F11", VK_OEM_COPY},
    {"F12", 0},
    {"NumpadEqual", VK_KANA},
    {"F13", VK_F13},
    {"F14", VK_F14},
    {"F15", VK_F15},
    {"F16", VK_F16},
    {"F17", VK_F17},
    {"F18", VK_F18},
    {"F19", VK_F19},
    {"F20", VK_F20},
    {"F21", VK_F21},
    {"F22", VK_F22},
    {"F23", VK_F23},
    {"KanaMode", 0},
    {"Lang2", 0},
    {"Lang1", 0},
    {"IntlRo", 0},
    {"F24", 0},

    {"Convert", 0},
    {"NonConvert", 0},
    {"IntlYen", 0},
    {"NumpadComma", 0},
    {"MediaTrackPrevious", 0},
    {"MediaTrackNext", 0},

    {"NumpadEnter", 0},
    {"ControlRight", 0},
    {"AudioVolumeMute", 0},
    {"LaunchApp2", APPCOMMAND_MEDIA_CHANNEL_DOWN},
    {"MediaPlayPause", APPCOMMAND_MEDIA_CHANNEL_UP},
    {"MediaStop", APPCOMMAND_MEDIA_FAST_FORWARD},
    {"Eject", APPCOMMAND_MEDIA_PAUSE},
    {"VolumeDown", APPCOMMAND_MEDIA_PLAY},
    {"VolumeUp", VK_MEDIA_PLAY_PAUSE},
    {"BrowserHome", APPCOMMAND_MEDIA_RECORD},
    {"NumpadDivide", APPCOMMAND_MEDIA_REWIND},
    {"PrintScreen", APPCOMMAND_MEDIA_STOP},
    {"AltRight", APPCOMMAND_MEDIA_NEXTTRACK},
    {"Help", APPCOMMAND_MEDIA_PREVIOUSTRACK},
    {"NumLock", 0},
    {"Pause", 0},
    {"Home", APPCOMMAND_BASS_DOWN},
    {"ArrowUp", 0},
    {"ArrowLeft", APPCOMMAND_BASS_BOOST},
    {"ArrowRight", 0},
    {"End", APPCOMMAND_BASS_UP},
    {"ArrowDown", 0},
    {"PageDown", 0},
    {"Insert", 0},
    {"Delete", APPCOMMAND_TREBLE_DOWN},
    {"MetaLeft", APPCOMMAND_TREBLE_UP},
    {"OSLeft", 0},
    {"MetaRight", VK_VOLUME_DOWN},
    {"OSRight", VK_VOLUME_MUTE},
    {"ContextMenu", VK_VOLUME_UP},
    {"Power", APPCOMMAND_MIC_ON_OFF_TOGGLE},
    {"WakeUp", 0},
    {"BrowserSearch", 0},
    {"BrowserFavorites", 0},
    {"BrowserRefresh", 0},
    {"BrowserStop", 0},
    {"BrowserForward", 0},
    {"BrowserBack", 0},
    {"LaunchApp1", 0},
    {"LaunchMail", 0},
    {"MediaSelect", 0},
    {"Lang2", 0},
    {"Lang1", 0},
};

#elif defined(IS_MACOS)

#elif defined(IS_LINUX)


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

    if (key.length() == 0) {
        Napi::TypeError::New(env, "Expected non empty string").ThrowAsJavaScriptException();
        return;
    }

    #if defined(IS_WINDOWS)
        bool isNotSupported = false;
        auto it = SpecialKeys.find(key);
        if (it == SpecialKeys.end()) {
            Napi::Error::New(env, "Failed to send key up event").ThrowAsJavaScriptException();
            return;
        }
        WORD wScan = it->second;

        INPUT Input = {0};
        Input.type = INPUT_KEYBOARD;
        Input.ki.wVk = 0;
        Input.ki.wScan = wScan;
        Input.ki.dwFlags = KEYEVENTF_SCANCODE;
        SendInput(1, &Input, sizeof(INPUT));
        
    #elif defined(IS_MACOS)
        //Later implementation
    #elif defined(IS_LINUX)
        //Later implementation
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

    if (key.length() == 0) {
        Napi::TypeError::New(env, "Expected non empty string").ThrowAsJavaScriptException();
        return;
    }

    #if defined(IS_WINDOWS)
        bool isNotSupported = false;
        auto it = SpecialKeys.find(key);
        if (it == SpecialKeys.end()) {
            Napi::Error::New(env, "Failed to send key up event").ThrowAsJavaScriptException();
            return;
        }
        WORD wScan = it->second;

        INPUT Input = {0};
        Input.type = INPUT_KEYBOARD;
        Input.ki.wVk = 0;
        Input.ki.wScan = wScan;
        Input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
        SendInput(1, &Input, sizeof(INPUT));

    #elif defined(IS_MACOS)
        //Later implementation
    #elif defined(IS_LINUX)
        //Later implementation
    #endif

    return;
}

Napi::Boolean Keyboard::isKeySupported(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Expected string argument").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string key = info[0].As<Napi::String>().Utf8Value();

    if (key.length() == 0) {
        Napi::TypeError::New(env, "Expected non empty string").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    bool isNotSupported = false;

    #if defined(IS_WINDOWS)
        auto it = SpecialKeys.find(key);
        if (it != SpecialKeys.end()) {
            if (it->second == 0) {
                isNotSupported = true;
            }
        } else {
            isNotSupported = true;
        }
    #elif defined(IS_MACOS)
        //Later implementation
    #elif defined(IS_LINUX)
        //Later implementation
    #endif
    return Napi::Boolean::New(env, !isNotSupported);
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

    std::string key = info[0].As<Napi::String>().Utf8Value();
    
    if (key.length() == 0) {
        Napi::TypeError::New(env, "Expected non empty string").ThrowAsJavaScriptException();
        return;
    }

    #if defined(IS_WINDOWS)
        int wideSize = MultiByteToWideChar(CP_UTF8, 0, key.c_str(), -1, NULL, 0);
        wchar_t* wideStr = new wchar_t[wideSize];
        memset(wideStr, 0, wideSize * sizeof(wchar_t));
        MultiByteToWideChar(CP_UTF8, 0, key.c_str(), -1, wideStr, wideSize);
        
        INPUT Input[1] = {0};
        Input[0].type = INPUT_KEYBOARD;
        Input[0].ki.wVk = 0;
        Input[0].ki.wScan = wideStr[0];
        Input[0].ki.dwFlags = KEYEVENTF_UNICODE;
        SendInput(1, Input, sizeof(INPUT));
        delete[] wideStr;

    #elif defined(IS_MACOS)
        //Later implementation
    #elif defined(IS_LINUX)
        //Later implementation
    #endif
}

Napi::String Keyboard::GetLayout(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    #if defined(IS_WINDOWS)
        HKL layout = GetKeyboardLayout(0);
        wchar_t layoutName[9];
        if (GetKeyboardLayoutNameW(layoutName)) {
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, layoutName, -1, NULL, 0, NULL, NULL);
            std::string layoutStr(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, layoutName, -1, &layoutStr[0], size_needed, NULL, NULL);
            return Napi::String::New(env, layoutStr.c_str());
        } else {
            Napi::Error::New(env, "Failed to get keyboard layout").ThrowAsJavaScriptException();
            return Napi::String::New(env, "");
        }
    #elif defined(IS_MACOS)
        //Later implementation
    #elif defined(IS_LINUX)
        //Later implementation
    #endif
    return Napi::String::New(env, "");
}

void Keyboard::SetLayout(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Expected string argument").ThrowAsJavaScriptException();
        return;
    }

    std::string layout = info[0].As<Napi::String>().Utf8Value();

    if (layout.length() == 0) {
        Napi::TypeError::New(env, "Expected non empty string").ThrowAsJavaScriptException();
        return;
    }

    #if defined(IS_WINDOWS)
        HKL hkl = LoadKeyboardLayoutW(std::wstring(layout.begin(), layout.end()).c_str(), KLF_ACTIVATE);
        if (hkl == NULL) {
            Napi::Error::New(env, "Failed to set keyboard layout").ThrowAsJavaScriptException();
            return;
        }
        PostMessageW(GetForegroundWindow(), WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)hkl);
        PostMessageW(GetForegroundWindow(), WM_INPUTLANGCHANGE, 0, (LPARAM)hkl);    

    #elif defined(IS_MACOS)
        //Later implementation
    #elif defined(IS_LINUX)
        //Later implementation
    #endif
    return;
}

Napi::Object Keyboard::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set(Napi::String::New(env, "keyDown"), Napi::Function::New(env, Keyboard::keyDown));
    obj.Set(Napi::String::New(env, "keyUp"), Napi::Function::New(env, Keyboard::keyUp));
    obj.Set(Napi::String::New(env, "isKeySupported"), Napi::Function::New(env, Keyboard::isKeySupported));
    obj.Set(Napi::String::New(env, "type"), Napi::Function::New(env, Keyboard::type));
    obj.Set(Napi::String::New(env, "GetLayout"), Napi::Function::New(env, Keyboard::GetLayout));
    obj.Set(Napi::String::New(env, "SetLayout"), Napi::Function::New(env, Keyboard::SetLayout));
    return obj;
}





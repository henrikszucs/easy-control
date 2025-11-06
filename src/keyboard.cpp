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
#include <map>



// Helper function to convert KeyboardEvent.key string to Windows Virtual Key Code
#if defined(IS_WINDOWS)
std::map<std::string, WORD> SpecialKeys = {
    {"Alt", VK_MENU},
    {"AltGraph", 0},
    {"CapsLock", VK_CAPITAL},
    {"Control", VK_CONTROL},
    {"Fn", 0},
    {"FnLock", 0},
    {"Hyper", 0},
    {"Meta", VK_LWIN},
    {"NumLock", VK_NUMLOCK},
    {"ScrollLock", VK_SCROLL},
    {"Shift", VK_SHIFT},
    {"Super", 0},
    {"Symbol", 0},
    {"SymbolLock", 0},
    {"Enter", VK_RETURN},
    {"Tab", VK_TAB},
    {" ", VK_SPACE},
    {"ArrowDown", VK_DOWN},
    {"ArrowLeft", VK_LEFT},
    {"ArrowRight", VK_RIGHT},
    {"ArrowUp", VK_UP},
    {"End", VK_END},
    {"Home", VK_HOME},
    {"PageDown", VK_NEXT},
    {"PageUp", VK_PRIOR},
    {"Backspace", VK_BACK},
    {"Clear", VK_CLEAR},
    {"Copy", APPCOMMAND_COPY},
    {"CrSel", VK_CRSEL},
    {"Cut", APPCOMMAND_CUT},
    {"Delete", VK_DELETE},
    {"EraseEof", VK_EREOF},
    {"ExSel", VK_EXSEL},
    {"Insert", VK_INSERT},
    {"Paste", APPCOMMAND_PASTE},
    {"Redo", APPCOMMAND_REDO},
    {"Undo", APPCOMMAND_UNDO},
    {"Accept", VK_ACCEPT},
    {"Again", 0},
    {"Attn", VK_OEM_ATTN},
    {"Cancel", VK_CANCEL},
    {"ContextMenu", VK_APPS},
    {"Escape", VK_ESCAPE},
    {"Execute", VK_EXECUTE},
    {"Find", APPCOMMAND_FIND},
    {"Finish", VK_OEM_FINISH},
    {"Help", VK_HELP},
    {"Pause", VK_PAUSE},
    {"Play", VK_PLAY},
    {"Props", 0},
    {"Select", VK_SELECT},
    {"ZoomIn", 0},
    {"ZoomOut", 0},
    {"BrightnessDown", 0},
    {"BrightnessUp", 0},
    {"Eject", 0},
    {"LogOff", 0},
    {"Power", 0},
    {"PowerOff", 0},
    {"PrintScreen", VK_SNAPSHOT},
    {"Hibernate", 0},
    {"Standby", VK_SLEEP},
    {"WakeUp", 0},
    {"AllCandidates", 0},
    {"Alphanumeric", VK_OEM_ATTN},
    {"CodeInput", 0},
    {"Compose", 0},
    {"Convert", VK_CONVERT},
    {"Dead", 0},
    {"FinalMode", VK_FINAL},
    {"GroupFirst", 0},
    {"GroupLast", 0},
    {"GroupNext", 0},
    {"GroupPrevious", 0},
    {"ModeChange", VK_MODECHANGE},
    {"NextCandidate", 0},
    {"NonConvert", VK_NONCONVERT},
    {"PreviousCandidate", 0},
    {"Process", VK_PROCESSKEY},
    {"SingleCandidate", 0},
    {"HangulMode", VK_HANGUL},
    {"HanjaMode", VK_HANJA},
    {"JunjaMode", VK_JUNJA},
    {"Eisu", 0},
    {"Hankaku", VK_OEM_AUTO},
    {"Hiragana", VK_OEM_COPY},
    {"HiraganaKatakana", 0},
    {"KanaMode", VK_KANA},
    {"KanjiMode", VK_KANJI},
    {"Katakana", VK_OEM_FINISH},
    {"Romaji", VK_OEM_BACKTAB},
    {"Zenkaku", VK_OEM_ENLW},
    {"ZenkakuHanaku", 0},
    {"F1", VK_F1},
    {"F2", VK_F2},
    {"F3", VK_F3},
    {"F4", VK_F4},
    {"F5", VK_F5},
    {"F6", VK_F6},
    {"F7", VK_F7},
    {"F8", VK_F8},
    {"F9", VK_F9},
    {"F10", VK_F10},
    {"F11", VK_F11},
    {"F12", VK_F12},
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
    {"F24", VK_F24},
    {"Soft1", 0},
    {"Soft2", 0},
    {"Soft3", 0},
    {"Soft4", 0},
    {"AppSwitch", 0},
    {"Call", 0},
    {"Camera", 0},
    {"GoBack", 0},
    {"GoHome", 0},
    {"HeadsetHook", 0},
    {"LastNumberRedial", 0},
    {"Notification", 0},
    {"MannerMode", 0},
    {"VoiceDial", 0},
    {"ChannelDown", APPCOMMAND_MEDIA_CHANNEL_DOWN},
    {"ChannelUp", APPCOMMAND_MEDIA_CHANNEL_UP},
    {"MediaFastForward", APPCOMMAND_MEDIA_FAST_FORWARD},
    {"MediaPause", APPCOMMAND_MEDIA_PAUSE},
    {"MediaPlay", APPCOMMAND_MEDIA_PLAY},
    {"MediaPlayPause", VK_MEDIA_PLAY_PAUSE},
    {"MediaRecord", APPCOMMAND_MEDIA_RECORD},
    {"MediaRewind", APPCOMMAND_MEDIA_REWIND},
    {"MediaStop", APPCOMMAND_MEDIA_STOP},
    {"MediaTrackNext", APPCOMMAND_MEDIA_NEXTTRACK},
    {"MediaTrackPrevious", APPCOMMAND_MEDIA_PREVIOUSTRACK},
    {"AudioBalanceLeft", 0},
    {"AudioBalanceRight", 0},
    {"AudioBassDown", APPCOMMAND_BASS_DOWN},
    {"AudioBassBoostDown", 0},
    {"AudioBassBoostToggle", APPCOMMAND_BASS_BOOST},
    {"AudioBassBoostUp", 0},
    {"AudioBassUp", APPCOMMAND_BASS_UP},
    {"AudioFaderFront", 0},
    {"AudioFaderRear", 0},
    {"AudioSurroundModeNext", 0},
    {"AudioTrebleDown", APPCOMMAND_TREBLE_DOWN},
    {"AudioTrebleUp", APPCOMMAND_TREBLE_UP},
    {"AudioVolumeDown", VK_VOLUME_DOWN},
    {"AudioVolumeMute", VK_VOLUME_MUTE},
    {"AudioVolumeUp", VK_VOLUME_UP},
    {"MicrophoneToggle", APPCOMMAND_MIC_ON_OFF_TOGGLE},
    {"MicrophoneVolumeDown", 0},
    {"MicrophoneVolumeMute", 0},
    {"MicrophoneVolumeUp", 0},
    {"TV", 0},
    {"TV3DMode", 0},
    {"TVAntennaCable", 0},
    {"TVAudioDescription", 0},
    {"TVAudioDescriptionMixDown", 0},
    {"TVAudioDescriptionMixUp", 0},
    {"TVContentsMenu", 0},
    {"TVDataService", 0},
    {"TVInput", 0},
    {"TVInputComponent1", 0},
    {"TVInputComponent2", 0},
    {"TVInputComposite1", 0},
    {"TVInputComposite2", 0},
    {"TVInputHDMI1", 0},
    {"TVInputHDMI2", 0},
    {"TVInputHDMI3", 0},
    {"TVInputHDMI4", 0},
    {"TVInputVGA1", 0},
    {"TVMediaContext", 0},
    {"TVNetwork", 0},
    {"TVNumberEntry", 0},
    {"TVPower", 0},
    {"TVRadioService", 0},
    {"TVSatellite", 0},
    {"TVSatelliteBS", 0},
    {"TVSatelliteCS", 0},
    {"TVSatelliteToggle", 0},
    {"TVTerrestrialAnalog", 0},
    {"TVTerrestrialDigital", 0},
    {"TVTimer", 0},
    {"AVRInput", 0},
    {"AVRPower", 0},
    {"ColorF0Red", 0},
    {"ColorF1Green", 0},
    {"ColorF2Yellow", 0},
    {"ColorF3Blue", 0},
    {"ColorF4Grey", 0},
    {"ColorF5Brown", 0},
    {"ClosedCaptionToggle", 0},
    {"Dimmer", 0},
    {"DisplaySwap", 0},
    {"DVR", 0},
    {"Exit", 0},
    {"FavoriteClear0", 0},
    {"FavoriteClear1", 0},
    {"FavoriteClear2", 0},
    {"FavoriteClear3", 0},
    {"FavoriteRecall0", 0},
    {"FavoriteRecall1", 0},
    {"FavoriteRecall2", 0},
    {"FavoriteRecall3", 0},
    {"FavoriteStore0", 0},
    {"FavoriteStore1", 0},
    {"FavoriteStore2", 0},
    {"FavoriteStore3", 0},
    {"Guide", 0},
    {"GuideNextDay", 0},
    {"GuidePreviousDay", 0},
    {"Info", 0},
    {"InstantReplay", 0},
    {"Link", 0},
    {"ListProgram", 0},
    {"LiveContent", 0},
    {"Lock", 0},
    {"MediaApps", VK_APPS},
    {"MediaAudioTrack", 0},
    {"MediaLast", 0},
    {"MediaSkipBackward", 0},
    {"MediaSkipForward", 0},
    {"MediaStepBackward", 0},
    {"MediaStepForward", 0},
    {"MediaTopMenu", 0},
    {"NavigateIn", 0},
    {"NavigateNext", 0},
    {"NavigateOut", 0},
    {"NavigatePrevious", 0},
    {"NextFavoriteChannel", 0},
    {"NextUserProfile", 0},
    {"OnDemand", 0},
    {"Pairing", 0},
    {"PinPDown", 0},
    {"PinPMove", 0},
    {"PinPToggle", 0},
    {"PinPUp", 0},
    {"PlaySpeedDown", 0},
    {"PlaySpeedReset", 0},
    {"PlaySpeedUp", 0},
    {"RandomToggle", 0},
    {"RcLowBattery", 0},
    {"RecordSpeedNext", 0},
    {"ScanChannelsToggle", 0},
    {"ScreenModeNext", 0},
    {"Settings", 0},
    {"SplitScreenToggle", 0},
    {"STBInput", 0},
    {"STBPower", 0},
    {"Subtitle", 0},
    {"Teletext", 0},
    {"VideoModeNext", 0},
    {"Wink", 0},
    {"ZoomToggle", VK_ZOOM},
    {"SpeechCorrectionList", APPCOMMAND_CORRECTION_LIST},
    {"SpeechInputToggle", APPCOMMAND_DICTATE_OR_COMMAND_CONTROL_TOGGLE},
    {"Close", APPCOMMAND_CLOSE},
    {"New", APPCOMMAND_NEW},
    {"Open", APPCOMMAND_OPEN},
    {"Print", APPCOMMAND_PRINT},
    {"Save", APPCOMMAND_SAVE},
    {"SpellCheck", APPCOMMAND_SPELL_CHECK},
    {"MailForward", APPCOMMAND_FORWARD_MAIL},
    {"MailReply", APPCOMMAND_REPLY_TO_MAIL},
    {"MailSend", APPCOMMAND_SEND_MAIL},
    {"LaunchCalculator", APPCOMMAND_LAUNCH_APP2},
    {"LaunchCalendar", 0},
    {"LaunchContacts", 0},
    {"LaunchMail", APPCOMMAND_LAUNCH_MAIL},
    {"LaunchMediaPlayer", APPCOMMAND_LAUNCH_MEDIA_SELECT},
    {"LaunchMusicPlayer", 0},
    {"LaunchMyComputer", APPCOMMAND_LAUNCH_APP1},
    {"LaunchPhone", 0},
    {"LaunchScreenSaver", 0},
    {"LaunchSpreadsheet", 0},
    {"LaunchWebBrowser", 0},
    {"LaunchWebCam", 0},
    {"LaunchWordProcessor", 0},
    {"LaunchApplication1", APPCOMMAND_LAUNCH_APP1},
    {"LaunchApplication2", APPCOMMAND_LAUNCH_APP2},
    {"LaunchApplication3", 0},
    {"LaunchApplication4", 0},
    {"LaunchApplication5", 0},
    {"LaunchApplication6", 0},
    {"LaunchApplication7", 0},
    {"LaunchApplication8", 0},
    {"LaunchApplication9", 0},
    {"LaunchApplication10", 0},
    {"LaunchApplication11", 0},
    {"LaunchApplication12", 0},
    {"LaunchApplication13", 0},
    {"LaunchApplication14", 0},
    {"LaunchApplication15", 0},
    {"LaunchApplication16", 0},
    {"BrowserBack", APPCOMMAND_BROWSER_BACKWARD},
    {"BrowserFavorites", APPCOMMAND_BROWSER_FAVORITES},
    {"BrowserForward", APPCOMMAND_BROWSER_FORWARD},
    {"BrowserHome", APPCOMMAND_BROWSER_HOME},
    {"BrowserRefresh", APPCOMMAND_BROWSER_REFRESH},
    {"BrowserSearch", APPCOMMAND_BROWSER_SEARCH},
    {"BrowserStop", APPCOMMAND_BROWSER_STOP},
    {"Decimal", VK_DECIMAL},
    {"Key11", 0},
    {"Key12", 0},
    {"Multiply", VK_MULTIPLY},
    {"Add", VK_ADD},
    {"Clear", 0},
    {"Divide", VK_DIVIDE},
    {"Subtract", VK_SUBTRACT},
    {"Separator", VK_SEPARATOR},
    {"0", VK_NUMPAD0},
    {"1", VK_NUMPAD1},
    {"2", VK_NUMPAD2},
    {"3", VK_NUMPAD3},
    {"4", VK_NUMPAD4},
    {"5", VK_NUMPAD5},
    {"6", VK_NUMPAD6},
    {"7", VK_NUMPAD7},
    {"8", VK_NUMPAD8},
    {"9", VK_NUMPAD9}
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
        //search for non character keys
        bool isSpecial = false;
        bool isNotSupported = false;
        WORD vkSpecial = 0;
        auto it = SpecialKeys.find(key);
        if (it != SpecialKeys.end()) {
            isSpecial = true;
            vkSpecial = it->second;
            if (vkSpecial == 0) {
                isNotSupported = true;
            }
        }

        if (isSpecial) {
            // Handle special keys (non-printable keys) if exists
            if (isNotSupported) {
                return;
            }
            INPUT * input = new INPUT[1];
            memset(input, 0, sizeof(INPUT));
            input[0].type = INPUT_KEYBOARD;
            input[0].ki.wVk = vkSpecial;
            input[0].ki.dwFlags = 0;
            UINT uSent = SendInput(1, input, sizeof(INPUT));
            delete[] input;
            if (uSent != 1) {
                Napi::Error::New(env, "Failed to send key up event").ThrowAsJavaScriptException();
            }
        } else {
            int wideSize = MultiByteToWideChar(CP_UTF8, 0, key.c_str(), -1, NULL, 0);
            if (wideSize > 5) {
                Napi::TypeError::New(env, "Character too long (maybe not a character)").ThrowAsJavaScriptException();
                return;
            }

            wchar_t* wideStr = new wchar_t[wideSize];
            memset(wideStr, 0, wideSize * sizeof(wchar_t));
            MultiByteToWideChar(CP_UTF8, 0, key.c_str(), -1, wideStr, wideSize);
                
            // Send the first character as Unicode
            INPUT * input = new INPUT[wideSize];
            memset(input, 0, wideSize * sizeof(INPUT));
            for (int i = 0; i < wideSize -1; i++) { // -1 to skip null terminator
                input[i].type = INPUT_KEYBOARD;
                input[i].ki.wVk = 0;
                input[i].ki.wScan = wideStr[i];
                input[i].ki.dwFlags = KEYEVENTF_UNICODE;

            }
            SendInput(wideSize, input, sizeof(INPUT));
                
            delete[] input;
            delete[] wideStr;
        }
        
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
        bool isSpecial = false;
        bool isNotSupported = false;
        WORD vkSpecial = 0;
        
        auto it = SpecialKeys.find(key);
        if (it != SpecialKeys.end()) {
            isSpecial = true;
            vkSpecial = it->second;
            if (vkSpecial == 0) {
                isNotSupported = true;
            }
        }
        if (isSpecial) {
            // Handle special keys (non-printable keys) if exists
            if (isNotSupported) {
                return;
            }
            INPUT * input = new INPUT[1];
            memset(input, 0, sizeof(INPUT));
            input[0].type = INPUT_KEYBOARD;
            input[0].ki.wVk = vkSpecial;
            input[0].ki.dwFlags = KEYEVENTF_KEYUP;
            UINT uSent = SendInput(1, input, sizeof(INPUT));
            delete[] input;
            if (uSent != 1) {
                Napi::Error::New(env, "Failed to send key up event").ThrowAsJavaScriptException();
            }
            
        } else {
            int wideSize = MultiByteToWideChar(CP_UTF8, 0, key.c_str(), -1, NULL, 0);
            if (wideSize > 5) {
                Napi::TypeError::New(env, "Character too long (maybe not a character)").ThrowAsJavaScriptException();
                return;
            }

            wchar_t* wideStr = new wchar_t[wideSize];
            memset(wideStr, 0, wideSize * sizeof(wchar_t));
            MultiByteToWideChar(CP_UTF8, 0, key.c_str(), -1, wideStr, wideSize);
                
            // Send the first character as Unicode
            INPUT * input = new INPUT[wideSize];
            memset(input, 0, wideSize * sizeof(INPUT));
            for (int i = 0; i < wideSize -1; i++) { // -1 to skip null terminator
                input[i].type = INPUT_KEYBOARD;
                input[i].ki.wVk = 0;
                input[i].ki.wScan = wideStr[i];
                input[i].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

            }
            SendInput(wideSize, input, sizeof(INPUT));
                
            delete[] input;
            delete[] wideStr;
        }

    #elif defined(IS_MACOS)
        //Later implementation
    #elif defined(IS_LINUX)
        //Later implementation
    #endif

    return;
}

Napi::Boolean Keyboard::isHotkeySupported(const Napi::CallbackInfo& info) {
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

Napi::Object Keyboard::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set(Napi::String::New(env, "keyDown"), Napi::Function::New(env, Keyboard::keyDown));
    obj.Set(Napi::String::New(env, "keyUp"), Napi::Function::New(env, Keyboard::keyUp));
    obj.Set(Napi::String::New(env, "isHotkeySupported"), Napi::Function::New(env, Keyboard::isHotkeySupported));

    return obj;
}





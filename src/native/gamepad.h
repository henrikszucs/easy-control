#pragma once
#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <napi.h>
#include <cstdint>
#include <vector>

#if defined(IS_WINDOWS)
    // Opaque pointer - the actual type is defined in the .cpp file
    typedef struct _VIGEM_CLIENT_T* PVIGEM_CLIENT;
    typedef struct _VIGEM_TARGET_T* PVIGEM_TARGET;

    // Forward declare XUSB_REPORT structure as a pointer
    struct _XUSB_REPORT;
    typedef struct _XUSB_REPORT XUSB_REPORT;
    typedef struct _XUSB_REPORT* PXUSB_REPORT;
#endif

// Button and axis indices follow the W3C Standard Gamepad layout:
//     buttons 0-16: A, B, X, Y, LB, RB, LT, RT, Back, Start, LS, RS,
//                   D-pad up, down, left, right, Home
//     axes 0-3:     left stick X, Y; right stick X, Y (-1 to 1, Y down positive)
//     axes 4-5:     left and right trigger, analog (-1 released to 1 pulled)
class Gamepad : public Napi::ObjectWrap<Gamepad> {
    public:
        static Napi::Object Init(Napi::Env env, Napi::Object exports);
        static Napi::Value list(const Napi::CallbackInfo& info);
        static Napi::Value CreateObject(const Napi::CallbackInfo& info);
        Gamepad(const Napi::CallbackInfo& info);
        ~Gamepad();
        Napi::Value IsActive(const Napi::CallbackInfo& info);
        void Destroy(const Napi::CallbackInfo& info);
        void ButtonDown(const Napi::CallbackInfo& info);
        void ButtonUp(const Napi::CallbackInfo& info);
        void SetAxis(const Napi::CallbackInfo& info);
    private:
        // unplugs the virtual device and frees it; safe to call more than once
        void Release();
        void SetButton(const Napi::CallbackInfo& info, bool isDown);

        bool m_active = false;
        #if defined(IS_WINDOWS)
            PVIGEM_CLIENT m_client = nullptr;
            PVIGEM_TARGET m_pad = nullptr;
            PXUSB_REPORT m_report = nullptr;
        #elif defined(IS_MACOS)
            int m_gamepad_id = -1;
        #elif defined(IS_LINUX)
            int m_uinput_fd = -1;
            // D-pad directions held: bit 0 up, 1 down, 2 left, 3 right
            uint8_t m_dpad = 0;
        #endif
};

#endif

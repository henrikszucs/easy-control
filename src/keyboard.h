#pragma once
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <napi.h>

class Keyboard {
    public:
        static Napi::Object Init(Napi::Env env, Napi::Object exports);
        static void keyDown(const Napi::CallbackInfo& info);
        static void keyUp(const Napi::CallbackInfo& info);
        static void type(const Napi::CallbackInfo& info);
};

#endif
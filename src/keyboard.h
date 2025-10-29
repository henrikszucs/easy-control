#pragma once
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <napi.h>

class Keyboard {
    public:
        static Napi::Object Init(Napi::Env env, Napi::Object exports);
};

#endif
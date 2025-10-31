#pragma once
#ifndef SCREEN_H
#define SCREEN_H

#include <napi.h>

class Screen {
    public:
        static Napi::Object Init(Napi::Env env, Napi::Object exports);
        static Napi::Array list(const Napi::CallbackInfo& info);
};

#endif
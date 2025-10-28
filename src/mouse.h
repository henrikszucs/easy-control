#pragma once
#ifndef MOUSE_H
#define MOUSE_H

#include <napi.h>

class Mouse {
    public:
        static Napi::Object Init(Napi::Env env, Napi::Object exports);
};

#endif
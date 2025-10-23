#include "mouse.h"



Napi::Number Fn(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    // ...
    return Napi::Number::New(env, 12.0);
}

Napi::Object Mouse::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set(Napi::String::New(env, "getX"), Napi::Function::New(env, Fn));
    return obj;
}
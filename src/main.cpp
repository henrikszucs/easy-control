// addon.cc
#include <napi.h>

#include "mouse.h"

#if defined(USE_X11)
    #include "xdisplay.h"
#endif

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set(Napi::String::New(env, "Mouse"), Mouse::Init(env, exports));

    return obj;
}

NODE_API_MODULE(addon, InitAll);
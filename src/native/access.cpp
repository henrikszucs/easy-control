#include "access.h"
#include "platform.h"

#if defined(IS_MACOS)
    #include <ApplicationServices/ApplicationServices.h>
#elif defined(IS_LINUX)
    #include "wayland.h"
    #include <unistd.h>
#endif


// true when input sent now reaches applications:
//   macOS   the Accessibility permission of the app running this (without it
//           every synthesized event is dropped, without an error)
//   Linux   Wayland: /dev/uinput can be written (the virtual devices need it);
//           X11: the X display can be opened
//   Windows always (it asks nothing; see the README's limits)
static bool HasAccess() {
    #if defined(IS_MACOS)
        return AXIsProcessTrusted();
    #elif defined(IS_LINUX)
        if (IsWaylandSession()) {
            return access("/dev/uinput", W_OK) == 0 || access("/dev/input/uinput", W_OK) == 0;
        }
        return XGetMainDisplay() != nullptr;
    #else
        return true;
    #endif
}

Napi::Boolean InputAccess::hasInputAccess(const Napi::CallbackInfo& info) {
    return Napi::Boolean::New(info.Env(), HasAccess());
}

// Platform.requestInputAccess(): macOS shows its prompt pointing the user to
// the Accessibility settings (once per app, while the permission is not
// given); the Promise resolves with the access there is now, which on macOS
// stays false until the user has allowed it - check again later
Napi::Value InputAccess::requestInputAccess(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    bool hasAccess = false;
    #if defined(IS_MACOS)
        const void* keys[] = { kAXTrustedCheckOptionPrompt };
        const void* values[] = { kCFBooleanTrue };
        CFDictionaryRef options = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 1,
            &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        hasAccess = AXIsProcessTrustedWithOptions(options);
        if (options != NULL) {
            CFRelease(options);
        }
    #else
        hasAccess = HasAccess();
    #endif
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    deferred.Resolve(Napi::Boolean::New(env, hasAccess));
    return deferred.Promise();
}

Napi::Object InputAccess::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set(Napi::String::New(env, "hasInputAccess"), Napi::Function::New(env, InputAccess::hasInputAccess));
    obj.Set(Napi::String::New(env, "requestInputAccess"), Napi::Function::New(env, InputAccess::requestInputAccess));
    return obj;
}

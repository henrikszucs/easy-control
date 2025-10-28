// General addons
#include <napi.h>


// Check platform
#ifndef PyModule_AddIntMacro
    #define PyModule_AddIntMacro(module, macro) PyModule_AddIntConstant(module, #macro, macro)
#endif

#if !defined(IS_WINDOWS) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__))
    #define IS_WINDOWS
#endif

#if !defined(IS_MACOSX) && defined(__APPLE__) && defined(__MACH__)
    #define IS_MACOSX
#endif

#if !defined(USE_X11) && !defined(NUSE_X11) && !defined(IS_MACOSX) && !defined(IS_WINDOWS)
    #define USE_X11
#endif 
    
#if !defined(IS_WINDOWS) && !defined(IS_MACOSX) && !defined(USE_X11)
    #error "Unsupported platform"
#endif

// Include general alignments
#ifndef BYTE_ALIGN
    /* Interval to align by for large buffers (e.g. bitmaps). */
    /* Must be a power of 2. */
    #define BYTE_ALIGN 4 /* Bytes to align pixel buffers to. */
    /* #include <stddef.h> */
    /* #define BYTE_ALIGN (sizeof(size_t)) */
#endif

#if BYTE_ALIGN == 0
    /* No alignment needed. */
    #define ADD_PADDING(width) (width)
#else
    /* Aligns given width to padding. */
    #define ADD_PADDING(width) (BYTE_ALIGN + (((width) - 1) & ~(BYTE_ALIGN - 1)))
#endif

#if !defined(M_SQRT2)
	#define M_SQRT2 1.4142135623730950488016887 /* Fix for MSVC. */
#endif

// Include modules
#include "mouse.h"



Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set(Napi::String::New(env, "Mouse"), Mouse::Init(env, exports));

    return obj;
}

NODE_API_MODULE(addon, InitAll);
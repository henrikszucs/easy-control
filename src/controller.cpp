#include "controller.h"

#include <uv.h>

#if defined(IS_WINDOWS)
    #include <windows.h>
    
    #include "vJoyInterface.h"
    #include <setupapi.h>
    #include <shellapi.h>
    #include <stdio.h>
    #include <string.h>
    #pragma comment(lib, "setupapi.lib")
    #pragma comment(lib, "shell32.lib")
#elif defined(IS_MACOS)
    #include <IOKit/IOKitLib.h>
    #include <IOKit/hid/IOHIDLib.h>
    #include <CoreFoundation/CoreFoundation.h>
#elif defined(IS_LINUX)
    #include <unistd.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <linux/uinput.h>
    #include <string.h>
#endif





Napi::Boolean Controller::isSupported(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    bool supported = false;

    #if defined(IS_WINDOWS)
        int vJoyVersion = GetvJoyVersion();
        if (vJoyVersion != 0) {
            supported = true;
        }

    #elif defined(IS_MACOS)
        // On macOS, check if we can access IOKit HID services
        // macOS has native support for virtual HID devices through IOKit
        
        // Check if IOKit HID Manager is available
        IOHIDManagerRef hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
        if (hidManager) {
            // IOKit is available, which means we can create virtual controllers
            supported = true;
            CFRelease(hidManager);
        }
        
        // Alternative: Check if we have the required permissions
        // (Virtual HID devices may require special entitlements on newer macOS versions)
        io_iterator_t iterator = IO_OBJECT_NULL;
        kern_return_t result = IOServiceGetMatchingServices(
            kIOMasterPortDefault,
            IOServiceMatching(kIOHIDDeviceKey),
            &iterator
        );
        
        if (result == KERN_SUCCESS && iterator != IO_OBJECT_NULL) {
            supported = true;
            IOObjectRelease(iterator);
        }

    #elif defined(IS_LINUX)
        // On Linux, check if uinput kernel module is available
        // uinput allows creating virtual input devices
        
        // Check if /dev/uinput exists and is accessible
        int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (fd < 0) {
            // Try alternative path
            fd = open("/dev/input/uinput", O_WRONLY | O_NONBLOCK);
        }
        
        if (fd >= 0) {
            // uinput is accessible
            supported = true;
            close(fd);
        } else {
            // Check if uinput module exists but needs to be loaded
            struct stat st;
            if (stat("/dev/uinput", &st) == 0 || stat("/dev/input/uinput", &st) == 0) {
                // Device node exists but we don't have permission
                // User needs to add themselves to input group or run with sudo
                supported = true; // Module is available, just needs permissions
            }
        }

    #endif

    return Napi::Boolean::New(env, supported);
}



Napi::Boolean Controller::install(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    bool success = true;

    #if defined(IS_WINDOWS)
    /*
        // Check if vJoy is already installed
        HMODULE vJoyDll = LoadVJoyDLL();
        if (vJoyDll != NULL) {
            // vJoy is already installed
            UnloadVJoyDLL();
            return Napi::Boolean::New(env, true);
        }
        
        // Get the path to the vJoy installer
        char modulePath[MAX_PATH];
        HMODULE hModule = NULL;
        
        // Get the module handle for this DLL
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&install,
            &hModule
        );
        
        if (GetModuleFileNameA(hModule, modulePath, MAX_PATH) > 0) {
            // Get the directory of the module
            char* lastSlash = strrchr(modulePath, '\\');
            if (lastSlash != NULL) {
                *lastSlash = '\0';
            }
            
            // Construct the path to vJoy installer
            char installerPath[MAX_PATH];
            const char* possibleInstallers[] = {
                "\\vjoy_driver\\vJoyInstall.exe"
            };
            
            bool installerFound = false;
            for (int i = 0; i < sizeof(possibleInstallers) / sizeof(possibleInstallers[0]); i++) {
                snprintf(installerPath, MAX_PATH, "%s%s", modulePath, possibleInstallers[i]);
                
                // Check if installer exists
                DWORD fileAttr = GetFileAttributesA(installerPath);
                if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
                    installerFound = true;
                    break;
                }
            }
            
            if (installerFound) {
                // Run the installer with elevated privileges using ShellExecute
                SHELLEXECUTEINFOA sei = { 0 };
                sei.cbSize = sizeof(SHELLEXECUTEINFOA);
                sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
                sei.lpVerb = "runas"; // Request UAC elevation
                sei.lpFile = installerPath;
                sei.lpParameters = "/S /SILENT /VERYSILENT /SUPPRESSMSGBOXES /NORESTART";
                sei.nShow = SW_HIDE; // Hide installer window
                sei.hProcess = NULL;
                
                if (ShellExecuteExA(&sei)) {
                    if (sei.hProcess != NULL) {
                        // Wait for the installer to complete (timeout: 5 minutes)
                        DWORD waitResult = WaitForSingleObject(sei.hProcess, 300000);
                        
                        if (waitResult == WAIT_OBJECT_0) {
                            // Installer completed, check exit code
                            DWORD exitCode = 0;
                            if (GetExitCodeProcess(sei.hProcess, &exitCode)) {
                                if (exitCode == 0) {
                                    success = true;
                                }
                            }
                        }
                        
                        CloseHandle(sei.hProcess);
                    }
                    
                    // Verify installation by checking for vJoyInterface.dll
                    Sleep(2000); // Wait for system to finalize installation
                    vJoyDll = LoadLibraryA("vJoyInterface.dll");
                    if (vJoyDll != NULL) {
                        success = true;
                        FreeLibrary(vJoyDll);
                    }
                } else {
                    // ShellExecuteEx failed
                    DWORD error = GetLastError();
                    if (error == ERROR_CANCELLED) {
                        // User cancelled UAC prompt
                        success = false;
                    }
                }
            } else {
                // Installer not found - throw error
                Napi::Error::New(env, "vJoy installer not found in win32-x64\\vjoy driver directory")
                    .ThrowAsJavaScriptException();
                return Napi::Boolean::New(env, false);
            }
        } else {
            // Failed to get module path
            Napi::Error::New(env, "Failed to locate module path")
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }
            */
    #endif

    return Napi::Boolean::New(env, success);
}

Napi::Object Controller::list(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    Napi::Array displays = Napi::Array::New(env);

    #if defined(IS_WINDOWS)
        // Windows implementation to list displays
        // Placeholder: Add actual implementation here

    #elif defined(IS_MACOS)
        // MacOS implementation to list displays
        // Placeholder: Add actual implementation here

    #elif defined(IS_LINUX)
        // X11 implementation to list displays
        // Placeholder: Add actual implementation here

    #endif

    return displays;
}

Napi::Object Controller::create(const Napi::CallbackInfo& info) {
    return Controller::NewInstance(info.Env());
}

Napi::Object Controller::NewInstance(Napi::Env env) {
    Napi::EscapableHandleScope scope(env);
    Napi::Object obj = env.GetInstanceData<Napi::FunctionReference>()->New({});
    return scope.Escape(napi_value(obj)).ToObject();
}

Controller::Controller(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Controller>(info) {

}



void Controller::keyDown(const Napi::CallbackInfo& info) {

}

void Controller::keyUp(const Napi::CallbackInfo& info) {

}

void Controller::setAxes(const Napi::CallbackInfo& info) {

}

void Controller::disconnect(const Napi::CallbackInfo& info) {

}

void Controller::deleteController(const Napi::CallbackInfo& info) {

}


Napi::Object Controller::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object obj = Napi::Object::New(env);

    // static functions
    obj.Set(Napi::String::New(env, "isSupported"), Napi::Function::New(env, Controller::isSupported));
    obj.Set(Napi::String::New(env, "install"), Napi::Function::New(env, Controller::install));
    obj.Set(Napi::String::New(env, "list"), Napi::Function::New(env, Controller::list));

    // instance creation functions
    Napi::Object new_exports = Napi::Function::New(env, Controller::create, "create");
    Napi::Function func = DefineClass(env,
        "Controller",
        {
            InstanceMethod("keyDown", &Controller::keyDown),
            InstanceMethod("keyUp", &Controller::keyUp),
            InstanceMethod("setAxes", &Controller::setAxes),
            InstanceMethod("disconnect", &Controller::disconnect),
            InstanceMethod("delete", &Controller::deleteController)
        }
    );
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);
    new_exports.Set("Controller", func);
    obj.Set(Napi::String::New(env, "create"), new_exports);

    return obj;
}
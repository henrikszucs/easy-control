#include "controller.h"

#include <uv.h>

#if defined(IS_WINDOWS)
    #include <windows.h>

    #include <setupapi.h>
    #include <shellapi.h>
    #include <stdio.h>
    #include <string.h>
    #pragma comment(lib, "setupapi.lib")
    #pragma comment(lib, "shell32.lib")

    #include "vJoyInterface.h"
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

// Driver installation async implementation
InstallDriver::InstallDriver(const Napi::Env& env) : Napi::AsyncWorker{env, "InstallDriver"}, m_deferred{env} {
    
}

Napi::Promise InstallDriver::GetPromise() {
    return m_deferred.Promise();
}

// Perform installation steps here
void InstallDriver::Execute() {
    
    #if defined(IS_WINDOWS)
        // Check if vJoy is already installed
        int vJoyVersion = GetvJoyVersion();
        if (vJoyVersion != 0) {
            m_result = true;
            return;
        }

        // Get absolute path .node file
        char modulePath[MAX_PATH];
        HMODULE hModule = NULL;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)GetvJoyVersion,
            &hModule
        );
        GetModuleFileNameA(hModule, modulePath, MAX_PATH);
        char* lastSlash = strrchr(modulePath, '\\');
        if (lastSlash != NULL) {
            *lastSlash = '\0';
        }
        
        // Setup installer path
        char installerPath[MAX_PATH];
        snprintf(installerPath, MAX_PATH, "%s\\vjoy_driver\\vJoyInstall.exe", modulePath);
        char workingDir[MAX_PATH];
        snprintf(workingDir, MAX_PATH, "%s\\vjoy_driver", modulePath);
        
        // Use ShellExecuteEx to run installer
        SHELLEXECUTEINFOA sei = { 0 };
        sei.cbSize = sizeof(SHELLEXECUTEINFOA);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = "runas";
        sei.lpFile = installerPath;
        sei.lpDirectory = workingDir;
        if (ShellExecuteExA(&sei)) {
            if (sei.hProcess != NULL) {
                // Wait for installer to finish (max 5 minutes)
                DWORD waitResult = WaitForSingleObject(sei.hProcess, 300000);
                
                if (waitResult == WAIT_OBJECT_0) {
                    // Get exit code
                    DWORD exitCode = 0;
                    GetExitCodeProcess(sei.hProcess, &exitCode);
                    
                    CloseHandle(sei.hProcess);
                    
                    // Wait a bit for system to register the driver
                    for (int i = 0; i < 10; i++) {
                        Sleep(500);
                        // Verify installation
                        int version = GetvJoyVersion();
                        if (version != 0) {
                            m_result = true;
                            break;
                        } else {
                            m_result = false;
                        }
                    }
                } else {
                    // Timeout or error
                    CloseHandle(sei.hProcess);
                    m_result = false;
                }
            } else {
                // No process handle - may happen with runas
                // Just wait and verify
                Sleep(60000);
                int version = GetvJoyVersion();
                if (version != 0) {
                    m_result = true;
                } else {
                    m_result = false;
                }
            }
        } else {
            m_result = false;
        }
    #elif defined(IS_MACOS)
        m_result = true; // Assume driver is pre-installed on macOS
    #elif defined(IS_LINUX)
        // Check if uinput module is loaded
        FILE* procModules = fopen("/proc/modules", "r");
        if (procModules != NULL) {
            char line[256];
            bool found = false;
            while (fgets(line, sizeof(line), procModules)) {
                if (strncmp(line, "uinput", 6) == 0) {
                    found = true;
                    break;
                }
            }
            fclose(procModules);
            if (found) {
                m_result = true;
            } else {
                // Try to load uinput module
                int ret = system("modprobe uinput");
                if (ret == 0) {
                    m_result = true;
                } else {
                    m_result = false;
                }
            }
        } else {
            m_result = false;
        }
    #endif
}

void InstallDriver::OnOK() {
    Napi::Boolean val = Napi::Boolean::New(Env(), m_result);
    m_deferred.Resolve(val);
}

void InstallDriver::OnError(const Napi::Error& err) {
    m_deferred.Reject(err.Value());
}




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



Napi::Promise Controller::install(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    bool success = true;

    InstallDriver* worker = new InstallDriver(env);
    worker->Queue();
    return worker->GetPromise();
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
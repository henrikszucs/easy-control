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
        // Check if vJoy is already installed and version matches
        int vJoyVersion = GetvJoyVersion();
        if (vJoyVersion != 0) {
            WORD VerDll, VerDrv;
            if (DriverMatch(&VerDll, &VerDrv)) {
                m_result = true;
                return;
            }
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
            WORD VerDll, VerDrv;
            if (DriverMatch(&VerDll, &VerDrv)) {
                supported = true;
            }
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

    InstallDriver* worker = new InstallDriver(env);
    worker->Queue();
    return worker->GetPromise();
}


Napi::Array Controller::list(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    Napi::Array controllers = Napi::Array::New(env);

    for (size_t i = 0; i < Controller::controllers().size(); i++) {
        controllers.Set(i, Controller::controllers().at(i).Value());
    }

    return controllers;
}

Napi::Object Controller::create(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object new_controller = Controller::NewInstance(env);
    Controller::controllers().push_back(Napi::Persistent(new_controller));
    return new_controller;
}

Napi::Object Controller::NewInstance(Napi::Env env) {
    Napi::EscapableHandleScope scope(env);
    Napi::Object obj = env.GetInstanceData<Napi::FunctionReference>()->New({});
    return scope.Escape(napi_value(obj)).ToObject();
}

std::vector<Napi::ObjectReference>& Controller::controllers() {
    static std::vector<Napi::ObjectReference> m_controllers;
    return m_controllers;
}

Controller::Controller(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Controller>(info) {
    this->m_id = Controller::controllers().size();
    
    #if defined(IS_WINDOWS)
        while (this->m_id < 16 && GetVJDStatus(this->m_id) != VJD_STAT_FREE) {
            this->m_id++;
        }
        if (this->m_id >= 16) {
            Napi::TypeError::New(info.Env(), "No free virtual controller slots available").ThrowAsJavaScriptException();
            return;
        }
        BOOL isGetted = AcquireVJD(this->m_id);
        if (!isGetted) {
            Napi::TypeError::New(info.Env(), "Failed to acquire virtual controller").ThrowAsJavaScriptException();
            return;
        }
        ResetVJD(this->m_id);

    #elif defined(IS_MACOS)
        // Initialize IOKit HID resources if needed
    #elif defined(IS_LINUX)
        // Initialize uinput file descriptor if needed
        // TODO: Implement Linux uinput initialization
    #endif
    
    this->m_active = true;
}

Controller::~Controller() {
    if (!this->m_active) {
        return;
    }
    this->m_active = false;

    #if defined(IS_WINDOWS)
        RelinquishVJD(this->m_id);
    #elif defined(IS_MACOS)
        // Release IOKit resources if any
        // TODO: Implement macOS cleanup
    #elif defined(IS_LINUX)
        // Close uinput file descriptor if open
        // TODO: Implement Linux cleanup
    #endif
    
    // Remove from controllers list
    auto& controllers = Controller::controllers();
    for (size_t i = 0; i < controllers.size(); i++) {
        Napi::Object obj = controllers[i].Value();
        Controller* ctrl = Controller::Unwrap(obj);
        if (ctrl == this) {
            controllers.erase(controllers.begin() + i);
            break;
        }
    }
}


Napi::Value Controller::isActive(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, this->m_active);
}

void Controller::buttonDown(const Napi::CallbackInfo& info) {
    if (!this->m_active) {
        return;
    }

    Napi::Env env = info.Env();
    
    // Validate parameters
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Button ID is required").ThrowAsJavaScriptException();
        return;
    }
    
    if (!info[0].IsNumber()) {
        Napi::TypeError::New(env, "Button ID must be a number").ThrowAsJavaScriptException();
        return;
    }


    #if defined(IS_WINDOWS)
        int buttonId = info[0].As<Napi::Number>().Int32Value();
        
        // Validate button ID (vJoy supports buttons 1-128)
        if (buttonId < 1 || buttonId > 128) {
            Napi::RangeError::New(env, "Button ID must be between 1 and 128").ThrowAsJavaScriptException();
            return;
        }
        
        // Check if button exists on this device
        int maxButtons = GetVJDButtonNumber(this->m_id);
        if (buttonId > maxButtons) {
            Napi::RangeError::New(env, "Button ID exceeds available buttons on this controller").ThrowAsJavaScriptException();
            return;
        }
        
        // Press the button (TRUE = pressed)
        BOOL result = SetBtn(TRUE, this->m_id, buttonId);
        
        if (!result) {
            Napi::Error::New(env, "Failed to press button").ThrowAsJavaScriptException();
            return;
        }

    #elif defined(IS_MACOS)

    #elif defined(IS_LINUX)

    #endif
}

void Controller::buttonUp(const Napi::CallbackInfo& info) {
    if (!this->m_active) {
        return;
    }

    Napi::Env env = info.Env();
    
    // Validate parameters
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Button ID is required").ThrowAsJavaScriptException();
        return;
    }
    
    if (!info[0].IsNumber()) {
        Napi::TypeError::New(env, "Button ID must be a number").ThrowAsJavaScriptException();
        return;
    }


    #if defined(IS_WINDOWS)
        int buttonId = info[0].As<Napi::Number>().Int32Value();
        
        // Validate button ID (vJoy supports buttons 1-128)
        if (buttonId < 1 || buttonId > 128) {
            Napi::RangeError::New(env, "Button ID must be between 1 and 128").ThrowAsJavaScriptException();
            return;
        }
        
        // Check if button exists on this device
        int maxButtons = GetVJDButtonNumber(this->m_id);
        if (buttonId > maxButtons) {
            Napi::RangeError::New(env, "Button ID exceeds available buttons on this controller").ThrowAsJavaScriptException();
            return;
        }
        
        // Press the button (FALSE = released)
        BOOL result = SetBtn(FALSE, this->m_id, buttonId);
        
        if (!result) {
            Napi::Error::New(env, "Failed to press button").ThrowAsJavaScriptException();
            return;
        }

    #elif defined(IS_MACOS)

    #elif defined(IS_LINUX)

    #endif
}
    

void Controller::setAxis(const Napi::CallbackInfo& info) {
    if (!this->m_active) {
        return;
    }

    Napi::Env env = info.Env();
    
    // Validate parameters
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Axis ID and value are required").ThrowAsJavaScriptException();
        return;
    }
    
    if (!info[0].IsNumber()) {
        Napi::TypeError::New(env, "Axis ID must be a number").ThrowAsJavaScriptException();
        return;
    }
    
    if (!info[1].IsNumber()) {
        Napi::TypeError::New(env, "Axis value must be a number").ThrowAsJavaScriptException();
        return;
    }

    #if defined(IS_WINDOWS)
        int axisId = info[0].As<Napi::Number>().Int32Value();
        double axisValue = info[1].As<Napi::Number>().DoubleValue();
        
        // Validate axis value (0.0 to 1.0, where 0.5 is center)
        if (axisValue < 0.0 || axisValue > 1.0) {
            Napi::RangeError::New(env, "Axis value must be between 0.0 and 1.0").ThrowAsJavaScriptException();
            return;
        }
        
        // Convert normalized value (0.0-1.0) to vJoy range (0x1-0x8000)
        // vJoy uses range 0x1 (1) to 0x8000 (32768)
        LONG vJoyValue = (LONG)(axisValue * 0x7FFF) + 0x1;
        
        // Map axis ID to vJoy HID usage
        UINT vJoyAxis;
        switch (axisId) {
            case 0: vJoyAxis = HID_USAGE_X; break;
            case 1: vJoyAxis = HID_USAGE_Y; break;
            case 2: vJoyAxis = HID_USAGE_Z; break;
            case 3: vJoyAxis = HID_USAGE_RX; break;
            case 4: vJoyAxis = HID_USAGE_RY; break;
            case 5: vJoyAxis = HID_USAGE_RZ; break;
            case 6: vJoyAxis = HID_USAGE_SL0; break; // Slider
            case 7: vJoyAxis = HID_USAGE_SL1; break; // Dial
            default:
                Napi::RangeError::New(env, "Axis ID must be between 0 and 7").ThrowAsJavaScriptException();
                return;
        }
        
        // Check if axis exists on this device
        if (!GetVJDAxisExist(this->m_id, vJoyAxis)) {
            Napi::Error::New(env, "Axis does not exist on this controller").ThrowAsJavaScriptException();
            return;
        }
        
        // Set the axis value
        BOOL result = SetAxis(vJoyValue, this->m_id, vJoyAxis);
        
        if (!result) {
            Napi::Error::New(env, "Failed to set axis value").ThrowAsJavaScriptException();
            return;
        }

    #elif defined(IS_MACOS)

    #elif defined(IS_LINUX)

    #endif
}

void Controller::disconnect(const Napi::CallbackInfo& info) {
    // Call destructor
    this->~Controller();
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
            InstanceMethod("isActive", &Controller::isActive),
            InstanceMethod("buttonDown", &Controller::buttonDown),
            InstanceMethod("buttonUp", &Controller::buttonUp),
            InstanceMethod("setAxis", &Controller::setAxis),
            InstanceMethod("disconnect", &Controller::disconnect)
        }
    );
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);
    new_exports.Set("Controller", func);
    obj.Set(Napi::String::New(env, "create"), new_exports);

    return obj;
}
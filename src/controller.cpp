#include "controller.h"

#include <uv.h>
#include <stdio.h>
#include <string.h>

#if defined(IS_WINDOWS)
    #include <windows.h>

    #include <setupapi.h>
    #include <shellapi.h>
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
#endif

// Driver installation async implementation
InstallDriver::InstallDriver(const Napi::Env& env) : Napi::AsyncWorker{env, "InstallDriver"}, m_deferred{env} {}

Napi::Promise InstallDriver::GetPromise() {
    return m_deferred.Promise();
}

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


// EnableGamepad async implementation
EnableGamepad::EnableGamepad(const Napi::Env& env, uint32_t gamepadId, uint32_t maxButton, uint32_t maxAxis) : Napi::AsyncWorker{env, "EnableGamepad"}, m_deferred{env}, m_gamepadId{gamepadId}, m_maxButton{maxButton}, m_maxAxis{maxAxis} {}

Napi::Promise EnableGamepad::GetPromise() {
    return m_deferred.Promise();
}

void EnableGamepad::Execute() {
    #if defined(IS_WINDOWS)
        // Get absolute path to vJoyConfig.exe
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
        
        // Setup vJoyConfig.exe path
        char configPath[MAX_PATH];
        snprintf(configPath, MAX_PATH, "%s\\vJoyConfig.exe", modulePath);
        
        // Build command line arguments
        // Format: vJoyConfig.exe <deviceId> -f -b <numButtons> [-a <axesMask>]
        // Note: vJoy device IDs are 1-based
        uint32_t deviceId = m_gamepadId + 1;
        
        char parameters[512];
        
        if (m_maxAxis > 0 && m_maxAxis <= 8) {
            // Build axes mask string
            // Accepted values: x, y, z, rx, ry, rz, sl0, sl1 (space-separated)
            char axesMask[64] = "";
            const char* axisNames[] = {"x", "y", "z", "rx", "ry", "rz", "sl0", "sl1"};
            
            for (uint32_t i = 0; i < m_maxAxis; i++) {
                if (i > 0) {
                    strcat(axesMask, " ");
                }
                strcat(axesMask, axisNames[i]);
            }
            
            snprintf(parameters, sizeof(parameters), "%u -f -b %u -a %s", 
                     deviceId, m_maxButton, axesMask);
        } else {
            // No axes - don't include -a parameter
            snprintf(parameters, sizeof(parameters), "%u -f -b %u", deviceId, m_maxButton);
        }
        
        // Use ShellExecuteEx to run with elevated privileges
        SHELLEXECUTEINFOA sei = { 0 };
        sei.cbSize = sizeof(SHELLEXECUTEINFOA);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
        sei.lpVerb = "runas";  // Run as administrator
        sei.lpFile = configPath;
        sei.lpParameters = parameters;
        sei.nShow = SW_HIDE;
        
        if (ShellExecuteExA(&sei)) {
            if (sei.hProcess != NULL) {
                // Wait for the process to complete (max 30 seconds)
                DWORD waitResult = WaitForSingleObject(sei.hProcess, 30000);
                
                if (waitResult == WAIT_OBJECT_0) {
                    // Get exit code
                    DWORD exitCode = 0;
                    GetExitCodeProcess(sei.hProcess, &exitCode);
                    
                    CloseHandle(sei.hProcess);
                    
                    // Exit code 0 means success
                    if (exitCode == 0) {
                        // Wait a moment for the device to be ready
                        Sleep(500);
                        
                        // Verify the device is now available
                        VjdStat status = GetVJDStatus(deviceId);
                        if (status == VJD_STAT_FREE || status == VJD_STAT_OWN) {
                            m_result = true;
                        } else {
                            m_result = false;
                        }
                    } else {
                        m_result = false;
                    }
                } else {
                    // Timeout or error
                    CloseHandle(sei.hProcess);
                    m_result = false;
                }
            } else {
                // No process handle - may happen with runas on some systems
                // Wait and verify
                Sleep(5000);
                VjdStat status = GetVJDStatus(deviceId);
                if (status == VJD_STAT_FREE || status == VJD_STAT_OWN) {
                    m_result = true;
                } else {
                    m_result = false;
                }
            }
        } else {
            // ShellExecuteEx failed
            DWORD error = GetLastError();
            if (error == ERROR_CANCELLED) {
                // User cancelled UAC prompt
                m_result = false;
            } else {
                m_result = false;
            }
        }
    #elif defined(IS_MACOS)
        // Implementation to enable gamepad on macOS
    #elif defined(IS_LINUX)
        // Implementation to enable gamepad on Linux
    #endif
    m_result = true; // Placeholder
}

void EnableGamepad::OnOK() {
    Napi::Boolean val = Napi::Boolean::New(Env(), m_result);
    m_deferred.Resolve(val);
}

void EnableGamepad::OnError(const Napi::Error& err) {
    m_deferred.Reject(err.Value());
}


// DisableGamepad async implementation
DisableGamepad::DisableGamepad(const Napi::Env& env, uint32_t gamepadId) : Napi::AsyncWorker{env, "DisableGamepad"}, m_deferred{env}, m_gamepadId{gamepadId} {}

Napi::Promise DisableGamepad::GetPromise() {
    return m_deferred.Promise();
}

void DisableGamepad::Execute() {
    #if defined(IS_WINDOWS)
        // Get absolute path to vJoyConfig.exe
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
        
        // Setup vJoyConfig.exe path
        char configPath[MAX_PATH];
        snprintf(configPath, MAX_PATH, "%s\\vJoyConfig.exe", modulePath);
        
        // Build command line arguments
        // Format: vJoyConfig.exe <deviceId> -f -b <numButtons> [-a <axesMask>]
        // Note: vJoy device IDs are 1-based
        uint32_t deviceId = m_gamepadId + 1;
        
        char parameters[512];
        snprintf(parameters, sizeof(parameters), "-d %u", deviceId);
        
        // Use ShellExecuteEx to run with elevated privileges
        SHELLEXECUTEINFOA sei = { 0 };
        sei.cbSize = sizeof(SHELLEXECUTEINFOA);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
        sei.lpVerb = "runas";  // Run as administrator
        sei.lpFile = configPath;
        sei.lpParameters = parameters;
        sei.nShow = SW_HIDE;
        
        if (ShellExecuteExA(&sei)) {
            if (sei.hProcess != NULL) {
                // Wait for the process to complete (max 30 seconds)
                DWORD waitResult = WaitForSingleObject(sei.hProcess, 30000);
                
                if (waitResult == WAIT_OBJECT_0) {
                    // Get exit code
                    DWORD exitCode = 0;
                    GetExitCodeProcess(sei.hProcess, &exitCode);
                    
                    CloseHandle(sei.hProcess);
                    
                    // Exit code 0 means success
                    if (exitCode == 0) {
                        // Wait a moment for the device to be ready
                        Sleep(500);
                        
                        // Verify the device is now available
                        VjdStat status = GetVJDStatus(deviceId);
                        if (status == VJD_STAT_FREE || status == VJD_STAT_OWN) {
                            m_result = true;
                        } else {
                            m_result = false;
                        }
                    } else {
                        m_result = false;
                    }
                } else {
                    // Timeout or error
                    CloseHandle(sei.hProcess);
                    m_result = false;
                }
            } else {
                // No process handle - may happen with runas on some systems
                // Wait and verify
                Sleep(5000);
                VjdStat status = GetVJDStatus(deviceId);
                if (status != VJD_STAT_OWN) {
                    if (deviceId != 1 && status != VJD_STAT_FREE) {
                        m_result = false;
                    } else {
                        m_result = true;
                    }
                    
                } else {
                    m_result = false;
                }
            }
        } else {
            // ShellExecuteEx failed
            DWORD error = GetLastError();
            if (error == ERROR_CANCELLED) {
                // User cancelled UAC prompt
                m_result = false;
            } else {
                m_result = false;
            }
        }
    #elif defined(IS_MACOS)
        // Implementation to disable gamepad on macOS
    #elif defined(IS_LINUX)
        // Implementation to disable gamepad on Linux
    #endif
    m_result = true; // Placeholder
}

void DisableGamepad::OnOK() {
    Napi::Boolean val = Napi::Boolean::New(Env(), m_result);
    m_deferred.Resolve(val);
}

void DisableGamepad::OnError(const Napi::Error& err) {
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

    #if defined(IS_WINDOWS)
        
        for (size_t i = 1; i < 17; i++) {
            VjdStat gamepadStatus = GetVJDStatus(i);
            if (gamepadStatus == VJD_STAT_FREE || gamepadStatus == VJD_STAT_OWN) {
                controllers.Set(controllers.Length(), Napi::Number::New(env, i-1));
            }
        }
    #elif defined(IS_MACOS)
        // macOS implementation to list available virtual gamepads
    #elif defined(IS_LINUX)
        // Linux implementation to list available virtual gamepads
    #endif
    return controllers;
}


Napi::Value Controller::enable(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate parameters
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "gamepadId parameter is required").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (!info[0].IsNumber()) {
        Napi::TypeError::New(env, "gamepadId must be a number").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    uint32_t gamepadId = info[0].As<Napi::Number>().Uint32Value();

    uint32_t maxButtons = 16; // Default max buttons
    if (info.Length() >= 2) {
        if (!info[1].IsNumber()) {
            Napi::TypeError::New(env, "maxButtons must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        maxButtons = info[1].As<Napi::Number>().Uint32Value();
    }

    uint32_t maxAxis = 4; // Default max axis
    if (info.Length() >= 3) {
        if (!info[2].IsNumber()) {
            Napi::TypeError::New(env, "maxAxis must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        maxAxis = info[2].As<Napi::Number>().Uint32Value();
    }

    EnableGamepad* worker = new EnableGamepad(env, gamepadId, maxButtons, maxAxis);
    worker->Queue();
    return worker->GetPromise();
}

Napi::Value Controller::disable(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate parameters
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "gamepadId parameter is required").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (!info[0].IsNumber()) {
        Napi::TypeError::New(env, "gamepadId must be a number").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    uint32_t gamepadId = info[0].As<Napi::Number>().Uint32Value();

    DisableGamepad* worker = new DisableGamepad(env, gamepadId);
    worker->Queue();
    return worker->GetPromise();
}

void Controller::buttonDown(const Napi::CallbackInfo& info) {
    // Validate parameters
    Napi::Env env = info.Env();

    if (info.Length() < 2) {
        Napi::TypeError::New(env, "2 parameter are requires").ThrowAsJavaScriptException();
        return;
    }
    
    if (!info[0].IsNumber()) {
        Napi::TypeError::New(env, "Gamepad ID must be a number").ThrowAsJavaScriptException();
        return;
    }
    uint32_t gamepadId = info[0].As<Napi::Number>().Uint32Value();

    if (!info[1].IsNumber()) {
        Napi::TypeError::New(env, "Button ID must be a number").ThrowAsJavaScriptException();
        return;
    }
    uint32_t buttonId = info[1].As<Napi::Number>().Uint32Value();

    
    #if defined(IS_WINDOWS)
        gamepadId += 1; // vJoy IDs are 1-based
        // Check vJoy owner
        VjdStat gamepadStatus = GetVJDStatus(gamepadId);
        if (gamepadStatus != VJD_STAT_FREE && gamepadStatus != VJD_STAT_OWN) {
            Napi::TypeError::New(info.Env(), "Controller not available").ThrowAsJavaScriptException();
            return;
        }

        // Acquire the vJoy device if not already owned
        if (gamepadStatus == VJD_STAT_FREE) {
            BOOL isGetted = AcquireVJD(gamepadId);
            if (!isGetted) {
                Napi::TypeError::New(info.Env(), "Failed to acquire virtual controller").ThrowAsJavaScriptException();
                return;
            }
        }

        // Check if button exists on this device
        uint32_t maxButtons = GetVJDButtonNumber(gamepadId);
        if (buttonId > maxButtons) {
            Napi::RangeError::New(env, "Button ID exceeds available buttons on this controller").ThrowAsJavaScriptException();
            return;
        }
        
        // Press the button (TRUE = pressed)
        BOOL result = SetBtn(TRUE, gamepadId, buttonId);
        
        if (!result) {
            Napi::Error::New(env, "Failed to press button").ThrowAsJavaScriptException();
            return;
        }

    #elif defined(IS_MACOS)
        // Initialize IOKit HID resources if needed
    #elif defined(IS_LINUX)
        // Initialize uinput file descriptor if needed
        // TODO: Implement Linux uinput initialization
    #endif
}

void Controller::buttonUp(const Napi::CallbackInfo& info) {
    // Validate parameters
    Napi::Env env = info.Env();

    if (info.Length() < 2) {
        Napi::TypeError::New(env, "2 parameter are requires").ThrowAsJavaScriptException();
        return;
    }
    
    if (!info[0].IsNumber()) {
        Napi::TypeError::New(env, "Gamepad ID must be a number").ThrowAsJavaScriptException();
        return;
    }
    uint32_t gamepadId = info[0].As<Napi::Number>().Uint32Value();

    if (!info[1].IsNumber()) {
        Napi::TypeError::New(env, "Button ID must be a number").ThrowAsJavaScriptException();
        return;
    }
    uint32_t buttonId = info[1].As<Napi::Number>().Uint32Value();

    
    #if defined(IS_WINDOWS)
        gamepadId += 1; // vJoy IDs are 1-based
        // Check vJoy owner
        VjdStat gamepadStatus = GetVJDStatus(gamepadId);
        if (gamepadStatus != VJD_STAT_FREE && gamepadStatus != VJD_STAT_OWN) {
            Napi::TypeError::New(info.Env(), "Controller not available").ThrowAsJavaScriptException();
            return;
        }

        // Acquire the vJoy device if not already owned
        if (gamepadStatus == VJD_STAT_FREE) {
            BOOL isGetted = AcquireVJD(gamepadId);
            if (!isGetted) {
                Napi::TypeError::New(info.Env(), "Failed to acquire virtual controller").ThrowAsJavaScriptException();
                return;
            }
        }

        // Check if button exists on this device
        uint32_t maxButtons = GetVJDButtonNumber(gamepadId);
        if (buttonId > maxButtons) {
            Napi::RangeError::New(env, "Button ID exceeds available buttons on this controller").ThrowAsJavaScriptException();
            return;
        }
        
        // Press the button (FALSE = released)
        BOOL result = SetBtn(FALSE, gamepadId, buttonId);
        
        if (!result) {
            Napi::Error::New(env, "Failed to release button").ThrowAsJavaScriptException();
            return;
        }

    #elif defined(IS_MACOS)
        // Initialize IOKit HID resources if needed
    #elif defined(IS_LINUX)
        // Initialize uinput file descriptor if needed
        // TODO: Implement Linux uinput initialization
    #endif
}

void Controller::setAxis(const Napi::CallbackInfo& info) {
    // Validate parameters
    Napi::Env env = info.Env();

    if (info.Length() < 3) {
        Napi::TypeError::New(env, "3 parameter are requires").ThrowAsJavaScriptException();
        return;
    }
    
    if (!info[0].IsNumber()) {
        Napi::TypeError::New(env, "Gamepad ID must be a number").ThrowAsJavaScriptException();
        return;
    }
    uint32_t gamepadId = info[0].As<Napi::Number>().Uint32Value();

    if (!info[1].IsNumber()) {
        Napi::TypeError::New(env, "Axis ID must be a number").ThrowAsJavaScriptException();
        return;
    }
    uint32_t axisId = info[1].As<Napi::Number>().Uint32Value();

    if (!info[2].IsNumber()) {
        Napi::TypeError::New(env, "Axis value must be a number").ThrowAsJavaScriptException();
        return;
    }
    double axisValue = info[2].As<Napi::Number>().DoubleValue();

    #if defined(IS_WINDOWS)
        gamepadId += 1; // vJoy IDs are 1-based
        // Check vJoy owner
        VjdStat gamepadStatus = GetVJDStatus(gamepadId);
        if (gamepadStatus != VJD_STAT_FREE && gamepadStatus != VJD_STAT_OWN) {
            Napi::TypeError::New(info.Env(), "Controller not available").ThrowAsJavaScriptException();
            return;
        }

        // Acquire the vJoy device if not already owned
        if (gamepadStatus == VJD_STAT_FREE) {
            BOOL isGetted = AcquireVJD(gamepadId);
            if (!isGetted) {
                Napi::TypeError::New(info.Env(), "Failed to acquire virtual controller").ThrowAsJavaScriptException();
                return;
            }
        }

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
        if (!GetVJDAxisExist(gamepadId, vJoyAxis)) {
            Napi::Error::New(env, "Axis does not exist on this controller").ThrowAsJavaScriptException();
            return;
        }
        
        // Set the axis value
        // Convert normalized value (0.0-1.0) to vJoy range (0x1-0x8000)
        LONG vJoyValue = (LONG)(axisValue * 0x7FFF) + 0x1;
        
        BOOL result = SetAxis(vJoyValue, gamepadId, vJoyAxis);
        
        if (!result) {
            Napi::Error::New(env, "Failed to set axis value").ThrowAsJavaScriptException();
            return;
        }
    #elif defined(IS_MACOS)
        
    #elif defined(IS_LINUX)

    #endif

}


Napi::Object Controller::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object obj = Napi::Object::New(env);

    obj.Set(Napi::String::New(env, "isSupported"), Napi::Function::New(env, Controller::isSupported));
    obj.Set(Napi::String::New(env, "install"), Napi::Function::New(env, Controller::install));
    obj.Set(Napi::String::New(env, "list"), Napi::Function::New(env, Controller::list));
    obj.Set(Napi::String::New(env, "enable"), Napi::Function::New(env, Controller::enable));
    obj.Set(Napi::String::New(env, "disable"), Napi::Function::New(env, Controller::disable));
    obj.Set(Napi::String::New(env, "buttonDown"), Napi::Function::New(env, Controller::buttonDown));
    obj.Set(Napi::String::New(env, "buttonUp"), Napi::Function::New(env, Controller::buttonUp));
    obj.Set(Napi::String::New(env, "setAxis"), Napi::Function::New(env, Controller::setAxis));
   
    return obj;
}
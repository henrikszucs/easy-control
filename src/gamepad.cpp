#include "gamepad.h"

#include <uv.h>
#include <vector>
#include <stdio.h>
#include <string.h>

#if defined(IS_WINDOWS)
    #include <windows.h>

    #include <shellapi.h>
    #pragma comment(lib, "shell32.lib")

    #include <setupapi.h>
    #pragma comment(lib, "setupapi.lib")

    #include <Xinput.h>
    #include <ViGEm/Client.h>
    #pragma comment(lib, "ViGEmClient.lib")
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
    
}

void InstallDriver::OnOK() {
    Napi::Boolean val = Napi::Boolean::New(Env(), m_result);
    m_deferred.Resolve(val);
}

void InstallDriver::OnError(const Napi::Error& err) {
    m_deferred.Reject(err.Value());
}



std::vector<Napi::ObjectReference> gamepads;

Napi::Value Gamepad::list(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    Napi::Array gamepadsArr = Napi::Array::New(env);
    for (size_t i = 0; i < gamepads.size(); i++) {
        gamepadsArr.Set(i, gamepads[i].Value());
    }
    return gamepadsArr;
}

Napi::Object Gamepad::CreateObject(const Napi::CallbackInfo& info) {
    Napi::Object gamepad = Gamepad::NewInstance(info.Env());
    
    Napi::ObjectReference ref = Napi::Persistent(gamepad);
    gamepads.push_back(std::move(ref));

    return gamepad;
}

Napi::Object Gamepad::NewInstance(Napi::Env env) {
    Napi::EscapableHandleScope scope(env);
    Napi::Object obj = env.GetInstanceData<Napi::FunctionReference>()->New({});
    Napi::Object ret = scope.Escape(napi_value(obj)).ToObject();
    return ret;
}

Gamepad::Gamepad(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Gamepad>(info) {
    #if defined(IS_WINDOWS)
        // allocate memory
        this->m_client = vigem_alloc();
        if (this->m_client == nullptr) {
            this->m_active = false;
            return;
        }

        // connect to the driver
        const auto retval = vigem_connect(this->m_client);
        if (!VIGEM_SUCCESS(retval)) {
            vigem_free(this->m_client);
            this->m_active = false;
            return;
        }

        // create a new Xbox 360 controller target
        this->m_pad = vigem_target_x360_alloc();
        const auto pir = vigem_target_add(this->m_client, this->m_pad);
        if (!VIGEM_SUCCESS(pir)) {
            vigem_target_remove(this->m_client, this->m_pad);
            vigem_target_free(this->m_pad);
            vigem_disconnect(this->m_client);
            vigem_free(this->m_client);
            this->m_active = false;
            return;
        }

        // Allocate the report structure
        this->m_report = new XUSB_REPORT();
        ZeroMemory(this->m_report, sizeof(XUSB_REPORT));

        this->m_active = true;

    #elif defined(MACOS)
        // macOS specific initialization

    #elif defined(IS_LINUX)
        // Linux specific initialization

    #endif

    
};

Gamepad::~Gamepad() {
    this->m_active = false;
    #if defined(IS_WINDOWS)
        if (this->m_pad != nullptr) {
            vigem_target_remove(this->m_client, this->m_pad);
            vigem_target_free(this->m_pad);
            this->m_pad = nullptr;
        }
        if (this->m_client != nullptr) {
            vigem_disconnect(this->m_client);
            vigem_free(this->m_client);
            this->m_client = nullptr;
        }
        if (this->m_report != nullptr) {
            delete this->m_report;
            this->m_report = nullptr;
        }
    #elif defined(MACOS)
        // macOS specific cleanup
    #elif defined(IS_LINUX)
        // Linux specific cleanup
    #endif
}

Napi::Value Gamepad::IsActive(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, this->m_active);
}

void Gamepad::Destroy(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object thisObj = info.This().As<Napi::Object>();

    // call destructor
    this->~Gamepad();

    // Find and remove from the gamepads vector
    for (auto it = gamepads.begin(); it != gamepads.end(); ++it) {
        if (it->Value() == thisObj) {
            it->Reset();  // Release the persistent reference
            gamepads.erase(it);
            break;
        }
    }

    
}

void Gamepad::ButtonDown(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!this->m_active) {
        Napi::Error::New(env, "Gamepad is not active").ThrowAsJavaScriptException();
        return;
    }

    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Button index expected").ThrowAsJavaScriptException();
        return;
    }
    int btnIndex = info[0].As<Napi::Number>().Int32Value();

    if (btnIndex < 0 || btnIndex > 31) {
        Napi::RangeError::New(env, "Button index out of range (0-31)").ThrowAsJavaScriptException();
        return;
    }

    #if defined(IS_WINDOWS)
        USHORT buttonMask = 0;
        switch (btnIndex) {
            case 0: buttonMask = XUSB_GAMEPAD_A; break;
            case 1: buttonMask = XUSB_GAMEPAD_B; break;
            case 2: buttonMask = XUSB_GAMEPAD_X; break;
            case 3: buttonMask = XUSB_GAMEPAD_Y; break;
            case 4: buttonMask = XUSB_GAMEPAD_LEFT_SHOULDER; break;
            case 5: buttonMask = XUSB_GAMEPAD_RIGHT_SHOULDER; break;
            case 8: buttonMask = XUSB_GAMEPAD_BACK; break;
            case 9: buttonMask = XUSB_GAMEPAD_START; break;
            case 10: buttonMask = XUSB_GAMEPAD_LEFT_THUMB; break;
            case 11: buttonMask = XUSB_GAMEPAD_RIGHT_THUMB; break;
            case 12: buttonMask = XUSB_GAMEPAD_DPAD_UP; break;
            case 13: buttonMask = XUSB_GAMEPAD_DPAD_DOWN; break;
            case 14: buttonMask = XUSB_GAMEPAD_DPAD_LEFT; break;
            case 15: buttonMask = XUSB_GAMEPAD_DPAD_RIGHT; break;
            case 16: buttonMask = XUSB_GAMEPAD_GUIDE; break;
            case 6:
            case 7:
            default:
                Napi::RangeError::New(env, "Invalid button index").ThrowAsJavaScriptException();
                return;
        }

        // Set the button bit in current state
        this->m_report->wButtons |= buttonMask;

        // Update the gamepad
        const auto result = vigem_target_x360_update(this->m_client, this->m_pad, *this->m_report);
        if (!VIGEM_SUCCESS(result)) {
            Napi::Error::New(env, "Failed to update gamepad state").ThrowAsJavaScriptException();
            return;
        }
    #elif defined(MACOS)
        // macOS specific button down implementation
    #elif defined(IS_LINUX)
        // Linux specific button down implementation
    #endif
}

void Gamepad::ButtonUp(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!this->m_active) {
        Napi::Error::New(env, "Gamepad is not active").ThrowAsJavaScriptException();
        return;
    }

    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Button index expected").ThrowAsJavaScriptException();
        return;
    }
    int btnIndex = info[0].As<Napi::Number>().Int32Value();

    if (btnIndex < 0 || btnIndex > 31) {
        Napi::RangeError::New(env, "Button index out of range (0-31)").ThrowAsJavaScriptException();
        return;
    }


    #if defined(IS_WINDOWS)
        // Windows specific button down implementation
        // Map button index to Xbox 360 button
        USHORT buttonMask = 0;
        switch (btnIndex) {
            case 0: buttonMask = XUSB_GAMEPAD_A; break;
            case 1: buttonMask = XUSB_GAMEPAD_B; break;
            case 2: buttonMask = XUSB_GAMEPAD_X; break;
            case 3: buttonMask = XUSB_GAMEPAD_Y; break;
            case 4: buttonMask = XUSB_GAMEPAD_LEFT_SHOULDER; break;
            case 5: buttonMask = XUSB_GAMEPAD_RIGHT_SHOULDER; break;
            case 8: buttonMask = XUSB_GAMEPAD_BACK; break;
            case 9: buttonMask = XUSB_GAMEPAD_START; break;
            case 10: buttonMask = XUSB_GAMEPAD_LEFT_THUMB; break;
            case 11: buttonMask = XUSB_GAMEPAD_RIGHT_THUMB; break;
            case 12: buttonMask = XUSB_GAMEPAD_DPAD_UP; break;
            case 13: buttonMask = XUSB_GAMEPAD_DPAD_DOWN; break;
            case 14: buttonMask = XUSB_GAMEPAD_DPAD_LEFT; break;
            case 15: buttonMask = XUSB_GAMEPAD_DPAD_RIGHT; break;
            case 16: buttonMask = XUSB_GAMEPAD_GUIDE; break;
            case 6:
            case 7:
            default:
                Napi::RangeError::New(env, "Invalid button index").ThrowAsJavaScriptException();
                return;
        }

        // Clear the button bit (use -> for pointer)
        this->m_report->wButtons &= ~buttonMask;

        // Update the gamepad (dereference pointer with *)
        const auto result = vigem_target_x360_update(this->m_client, this->m_pad, *this->m_report);
        if (!VIGEM_SUCCESS(result)) {
            Napi::Error::New(env, "Failed to update gamepad state").ThrowAsJavaScriptException();
            return;
        }
    #elif defined(MACOS)
        // macOS specific button down implementation
    #elif defined(IS_LINUX)
        // Linux specific button down implementation
    #endif
}

void Gamepad::SetAxis(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Axis index and direction expected").ThrowAsJavaScriptException();
        return;
    }
    int axisIndex = info[0].As<Napi::Number>().Int32Value();
    if (axisIndex < 0 || axisIndex > 7) {
        Napi::RangeError::New(env, "Axis index out of range (0-7)").ThrowAsJavaScriptException();
        return;
    }
    double axisValue = info[1].As<Napi::Number>().DoubleValue();
    if (axisValue < -1.0 || axisValue > 1.0) {
        Napi::RangeError::New(env, "Axis value out of range (-1.0 to 1.0)").ThrowAsJavaScriptException();
        return;
    }

    #if defined(IS_WINDOWS)
        // Convert normalized value (-1.0 to 1.0) to Xbox 360 range
        SHORT value = (SHORT)(axisValue * 32767.0);

        // Update the appropriate axis in the report (use -> for pointer)
        switch (axisIndex) {
            case 0: // Left Stick X
                this->m_report->sThumbLX = value;
                break;
            case 1: // Left Stick Y
                this->m_report->sThumbLY = -value;
                break;
            case 2: // Right Stick X
                this->m_report->sThumbRX = value;
                break;
            case 3: // Right Stick Y
                this->m_report->sThumbRY = -value;
                break;
            case 4: // Left Trigger
                // Triggers use 0-255 range
                this->m_report->bLeftTrigger = (BYTE)((axisValue + 1.0) * 127.5);
                break;
            case 5: // Right Trigger
                // Triggers use 0-255 range
                this->m_report->bRightTrigger = (BYTE)((axisValue + 1.0) * 127.5);
                break;
            default:
                Napi::RangeError::New(env, "Invalid axis index").ThrowAsJavaScriptException();
                return;
        }

        // Update the gamepad (dereference pointer with *)
        const auto result = vigem_target_x360_update(this->m_client, this->m_pad, *this->m_report);
        if (!VIGEM_SUCCESS(result)) {
            Napi::Error::New(env, "Failed to update gamepad state").ThrowAsJavaScriptException();
            return;
        }
    #elif defined(MACOS)
        // macOS specific button down implementation
    #elif defined(IS_LINUX)
        // Linux specific button down implementation
    #endif
}



Napi::Object Gamepad::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object obj = Napi::Object::New(env);

    obj.Set(Napi::String::New(env, "list"), Napi::Function::New(env, Gamepad::list));
    
    // object create
    Napi::Object new_exports = Napi::Function::New(env, Gamepad::CreateObject);

    obj.Set(Napi::String::New(env, "create"), new_exports);

    Napi::Function func = DefineClass(env,
        "Gamepad",
        {
            InstanceMethod("isActive", &Gamepad::IsActive),
            InstanceMethod("destroy", &Gamepad::Destroy),
            InstanceMethod("buttonDown", &Gamepad::ButtonDown),
            InstanceMethod("buttonUp", &Gamepad::ButtonUp),
            InstanceMethod("setAxis", &Gamepad::SetAxis)
        }
    );

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    new_exports.Set("Gamepad", func);
    return obj;
}
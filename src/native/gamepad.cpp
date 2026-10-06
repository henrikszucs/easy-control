#include "gamepad.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if defined(IS_WINDOWS)
    #include "gamepad_win.h"
#elif defined(IS_MACOS)
    #import <Foundation/Foundation.h>
    #include "GamepadBridge.h"
#elif defined(IS_LINUX)
    #include "uinput.h"
    #include <cerrno>
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <linux/uinput.h>
#endif


// What the addon keeps per JS environment (main thread, each worker): the
// Gamepad class and the gamepads created and not yet destroyed.
struct GamepadAddonData {
    Napi::FunctionReference constructor;
    std::vector<Napi::ObjectReference> gamepads;
};

static const int BUTTON_COUNT = 17;
static const int AXIS_COUNT = 6;

#if defined(IS_LINUX)
// writes one change and the report that closes it
static bool Emit(int fd, unsigned short type, unsigned short code, int value) {
    struct input_event ev[2];
    memset(ev, 0, sizeof(ev));
    ev[0].type = type;
    ev[0].code = code;
    ev[0].value = value;
    ev[1].type = EV_SYN;
    ev[1].code = SYN_REPORT;
    ev[1].value = 0;
    return write(fd, ev, sizeof(ev)) == (ssize_t)sizeof(ev);
}
#endif


Napi::Value Gamepad::list(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GamepadAddonData* data = env.GetInstanceData<GamepadAddonData>();

    Napi::Array gamepadsArr = Napi::Array::New(env);
    for (size_t i = 0; i < data->gamepads.size(); i++) {
        gamepadsArr.Set((uint32_t)i, data->gamepads[i].Value());
    }
    return gamepadsArr;
}

// What plugging in a gamepad gives: the platform's handle to it.
struct PadHandle {
    #if defined(IS_WINDOWS)
        WinPad* pad = nullptr;
    #elif defined(IS_MACOS)
        int gamepadId = -1;
    #elif defined(IS_LINUX)
        int fd = -1;
    #endif
};

// Plugs in a virtual gamepad; false, with a message and an error code, when
// it cannot. Runs off the JS thread, so it touches no JS value.
static bool OpenPad(PadHandle& handle, std::string& message, std::string& code) {
    code = "EASYCONTROL_CREATE_FAILED";
    #if defined(IS_WINDOWS)
        WinPadError error;
        handle.pad = WinPadCreate(error);
        if (handle.pad == nullptr) {
            message = error.message;
            code = error.code;
            return false;
        }
        return true;

    #elif defined(IS_MACOS)
        handle.gamepadId = [GamepadBridge createGamepad];
        if (handle.gamepadId < 0) {
            message = "Failed to create the virtual gamepad (needs macOS 26 and the com.apple.developer.hid.virtual.device entitlement)";
            return false;
        }
        return true;

    #elif defined(IS_LINUX)
        int fd = OpenUinput(message);
        if (fd < 0) {
            return false;
        }

        // Enable event types
        if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0 ||
            ioctl(fd, UI_SET_EVBIT, EV_ABS) < 0) {
            message = std::string("Failed to set up the virtual gamepad: ") + strerror(errno);
            close(fd);
            return false;
        }

        // Enable buttons (BTN_GAMEPAD + standard Xbox buttons)
        ioctl(fd, UI_SET_KEYBIT, BTN_SOUTH);      // A
        ioctl(fd, UI_SET_KEYBIT, BTN_EAST);       // B
        ioctl(fd, UI_SET_KEYBIT, BTN_NORTH);      // X
        ioctl(fd, UI_SET_KEYBIT, BTN_WEST);       // Y
        ioctl(fd, UI_SET_KEYBIT, BTN_TL);         // LB
        ioctl(fd, UI_SET_KEYBIT, BTN_TR);         // RB
        ioctl(fd, UI_SET_KEYBIT, BTN_SELECT);     // Back
        ioctl(fd, UI_SET_KEYBIT, BTN_START);      // Start
        ioctl(fd, UI_SET_KEYBIT, BTN_MODE);       // Guide
        ioctl(fd, UI_SET_KEYBIT, BTN_THUMBL);     // Left Stick
        ioctl(fd, UI_SET_KEYBIT, BTN_THUMBR);     // Right Stick

        // Enable axes
        ioctl(fd, UI_SET_ABSBIT, ABS_X);          // Left stick X
        ioctl(fd, UI_SET_ABSBIT, ABS_Y);          // Left stick Y
        ioctl(fd, UI_SET_ABSBIT, ABS_RX);         // Right stick X
        ioctl(fd, UI_SET_ABSBIT, ABS_RY);         // Right stick Y
        ioctl(fd, UI_SET_ABSBIT, ABS_Z);          // Left trigger
        ioctl(fd, UI_SET_ABSBIT, ABS_RZ);         // Right trigger
        ioctl(fd, UI_SET_ABSBIT, ABS_HAT0X);      // D-pad X
        ioctl(fd, UI_SET_ABSBIT, ABS_HAT0Y);      // D-pad Y

        // Setup device
        struct uinput_setup usetup;
        memset(&usetup, 0, sizeof(usetup));
        usetup.id.bustype = BUS_USB;
        usetup.id.vendor = 0x045e;  // Microsoft
        usetup.id.product = 0x028e; // Xbox 360 Controller
        usetup.id.version = 1;
        strncpy(usetup.name, "Virtual Xbox 360 Controller", UINPUT_MAX_NAME_SIZE - 1);

        // Configure axis ranges
        struct uinput_abs_setup abs_setup;
        memset(&abs_setup, 0, sizeof(abs_setup));

        // Sticks
        abs_setup.absinfo.minimum = -32768;
        abs_setup.absinfo.maximum = 32767;
        abs_setup.absinfo.value = 0;
        const unsigned short sticks[] = {ABS_X, ABS_Y, ABS_RX, ABS_RY};
        for (unsigned short stick : sticks) {
            abs_setup.code = stick;
            ioctl(fd, UI_ABS_SETUP, &abs_setup);
        }

        // Triggers (0-255)
        abs_setup.absinfo.minimum = 0;
        abs_setup.absinfo.maximum = 255;
        abs_setup.code = ABS_Z;
        ioctl(fd, UI_ABS_SETUP, &abs_setup);
        abs_setup.code = ABS_RZ;
        ioctl(fd, UI_ABS_SETUP, &abs_setup);

        // D-pad (-1, 0, 1)
        abs_setup.absinfo.minimum = -1;
        abs_setup.absinfo.maximum = 1;
        abs_setup.code = ABS_HAT0X;
        ioctl(fd, UI_ABS_SETUP, &abs_setup);
        abs_setup.code = ABS_HAT0Y;
        ioctl(fd, UI_ABS_SETUP, &abs_setup);

        // Create the device. It takes the system a moment to announce it, so
        // applications may miss input sent right after this returns.
        if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0 ||
            ioctl(fd, UI_DEV_CREATE) < 0) {
            message = std::string("Failed to create the virtual gamepad: ") + strerror(errno);
            close(fd);
            return false;
        }
        handle.fd = fd;
        return true;
    #endif
}

// Unplugs a gamepad that never got its JS object.
static void ClosePad(PadHandle& handle) {
    #if defined(IS_WINDOWS)
        WinPadDestroy(handle.pad);
        handle.pad = nullptr;
    #elif defined(IS_MACOS)
        if (handle.gamepadId >= 0) {
            [GamepadBridge destroyGamepad:handle.gamepadId];
            handle.gamepadId = -1;
        }
    #elif defined(IS_LINUX)
        if (handle.fd >= 0) {
            ioctl(handle.fd, UI_DEV_DESTROY);
            close(handle.fd);
            handle.fd = -1;
        }
    #endif
}

// plugs a gamepad in off the JS thread (on Windows that starts the service
// and waits for the devices) and resolves with its object
class CreatePadWorker : public Napi::AsyncWorker {
    public:
        explicit CreatePadWorker(Napi::Env env)
            : Napi::AsyncWorker(env), m_deferred(Napi::Promise::Deferred::New(env)) {}

        Napi::Promise Promise() {
            return this->m_deferred.Promise();
        }

        void Execute() override {
            this->m_isOpen = OpenPad(this->m_handle, this->m_message, this->m_code);
        }

        void OnOK() override {
            Napi::Env env = this->Env();
            if (!this->m_isOpen) {
                Napi::Error error = Napi::Error::New(env, this->m_message);
                error.Set("code", Napi::String::New(env, this->m_code));
                this->m_deferred.Reject(error.Value());
                return;
            }
            // the object takes the handle over
            GamepadAddonData* data = env.GetInstanceData<GamepadAddonData>();
            Napi::Object gamepad = data->constructor.New({ Napi::External<PadHandle>::New(env, &this->m_handle) });
            if (env.IsExceptionPending()) {
                const Napi::Error error = env.GetAndClearPendingException();
                ClosePad(this->m_handle);
                this->m_deferred.Reject(error.Value());
                return;
            }
            data->gamepads.push_back(Napi::Persistent(gamepad));
            this->m_deferred.Resolve(gamepad);
        }

    private:
        Napi::Promise::Deferred m_deferred;
        PadHandle m_handle;
        bool m_isOpen = false;
        std::string m_message;
        std::string m_code;
};

// Gamepad.create(): a Promise of a new virtual gamepad; rejects with an Error
// saying why, and a code (EASYCONTROL_DRIVER_MISSING, ...), when it cannot
Napi::Value Gamepad::CreateObject(const Napi::CallbackInfo& info) {
    CreatePadWorker* worker = new CreatePadWorker(info.Env());
    Napi::Promise promise = worker->Promise();
    worker->Queue();
    return promise;
}

// made only by Gamepad.create(), which hands over the plugged-in gamepad
Gamepad::Gamepad(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Gamepad>(info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsExternal()) {
        Napi::TypeError::New(env, "Use Gamepad.create() to make a gamepad").ThrowAsJavaScriptException();
        return;
    }
    PadHandle* handle = info[0].As<Napi::External<PadHandle>>().Data();
    #if defined(IS_WINDOWS)
        this->m_pad = handle->pad;
        handle->pad = nullptr;
    #elif defined(IS_MACOS)
        this->m_gamepad_id = handle->gamepadId;
        handle->gamepadId = -1;
    #elif defined(IS_LINUX)
        this->m_uinput_fd = handle->fd;
        handle->fd = -1;
    #endif
    this->m_active = true;
}

Gamepad::~Gamepad() {
    this->Release();
}

void Gamepad::Release() {
    this->m_active = false;
    #if defined(IS_WINDOWS)
        WinPadDestroy(this->m_pad);
        this->m_pad = nullptr;
    #elif defined(IS_MACOS)
        if (this->m_gamepad_id >= 0) {
            [GamepadBridge destroyGamepad:this->m_gamepad_id];
            this->m_gamepad_id = -1;
        }
    #elif defined(IS_LINUX)
        if (this->m_uinput_fd >= 0) {
            ioctl(this->m_uinput_fd, UI_DEV_DESTROY);
            close(this->m_uinput_fd);
            this->m_uinput_fd = -1;
        }
        this->m_dpad = 0;
    #endif
}

Napi::Value Gamepad::IsActive(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, this->m_active);
}

void Gamepad::Destroy(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object thisObj = info.This().As<Napi::Object>();

    // unplug it now; the object itself is freed when JS lets go of it
    this->Release();

    // Find and remove from the gamepads list
    GamepadAddonData* data = env.GetInstanceData<GamepadAddonData>();
    for (auto it = data->gamepads.begin(); it != data->gamepads.end(); ++it) {
        if (it->Value() == thisObj) {
            it->Reset();  // Release the persistent reference
            data->gamepads.erase(it);
            break;
        }
    }
}

void Gamepad::SetButton(const Napi::CallbackInfo& info, bool isDown) {
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

    if (btnIndex < 0 || btnIndex >= BUTTON_COUNT) {
        Napi::RangeError::New(env, "Button index out of range (0-16)").ThrowAsJavaScriptException();
        return;
    }

    #if defined(IS_WINDOWS)
        if (!WinPadSetButton(this->m_pad, btnIndex, isDown)) {
            Napi::Error::New(env, "Failed to update gamepad state").ThrowAsJavaScriptException();
            return;
        }

    #elif defined(IS_MACOS)
        BOOL result = isDown
            ? [GamepadBridge buttonDown:this->m_gamepad_id button:btnIndex]
            : [GamepadBridge buttonUp:this->m_gamepad_id button:btnIndex];
        if (!result) {
            Napi::Error::New(env, "Failed to update gamepad state").ThrowAsJavaScriptException();
            return;
        }

    #elif defined(IS_LINUX)
        bool isWritten = true;
        if (btnIndex >= 12 && btnIndex <= 15) {
            // the D-pad is a hat: each axis is the net of the directions held
            const uint8_t bit = (uint8_t)(1u << (btnIndex - 12));
            if (isDown) {
                this->m_dpad |= bit;
            } else {
                this->m_dpad &= (uint8_t)~bit;
            }
            const bool isVertical = (btnIndex <= 13);
            const int value = isVertical
                ? ((this->m_dpad & 0x2) ? 1 : 0) - ((this->m_dpad & 0x1) ? 1 : 0)
                : ((this->m_dpad & 0x8) ? 1 : 0) - ((this->m_dpad & 0x4) ? 1 : 0);
            isWritten = Emit(this->m_uinput_fd, EV_ABS, isVertical ? ABS_HAT0Y : ABS_HAT0X, value);
        } else if (btnIndex == 6 || btnIndex == 7) {
            // the triggers are axes: fully pulled or released
            isWritten = Emit(this->m_uinput_fd, EV_ABS, btnIndex == 6 ? ABS_Z : ABS_RZ, isDown ? 255 : 0);
        } else {
            unsigned short buttonCode = BTN_SOUTH;
            switch (btnIndex) {
                case 0: buttonCode = BTN_SOUTH; break;
                case 1: buttonCode = BTN_EAST; break;
                case 2: buttonCode = BTN_NORTH; break;
                case 3: buttonCode = BTN_WEST; break;
                case 4: buttonCode = BTN_TL; break;
                case 5: buttonCode = BTN_TR; break;
                case 8: buttonCode = BTN_SELECT; break;
                case 9: buttonCode = BTN_START; break;
                case 10: buttonCode = BTN_THUMBL; break;
                case 11: buttonCode = BTN_THUMBR; break;
                case 16: buttonCode = BTN_MODE; break;
            }
            isWritten = Emit(this->m_uinput_fd, EV_KEY, buttonCode, isDown ? 1 : 0);
        }
        if (!isWritten) {
            Napi::Error::New(env, "Failed to update gamepad state").ThrowAsJavaScriptException();
            return;
        }
    #endif
}

void Gamepad::ButtonDown(const Napi::CallbackInfo& info) {
    this->SetButton(info, true);
}

void Gamepad::ButtonUp(const Napi::CallbackInfo& info) {
    this->SetButton(info, false);
}

void Gamepad::SetAxis(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (!this->m_active) {
        Napi::Error::New(env, "Gamepad is not active").ThrowAsJavaScriptException();
        return;
    }

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Axis index and direction expected").ThrowAsJavaScriptException();
        return;
    }
    int axisIndex = info[0].As<Napi::Number>().Int32Value();
    if (axisIndex < 0 || axisIndex >= AXIS_COUNT) {
        Napi::RangeError::New(env, "Axis index out of range (0-5)").ThrowAsJavaScriptException();
        return;
    }
    double axisValue = info[1].As<Napi::Number>().DoubleValue();
    if (!std::isfinite(axisValue) || axisValue < -1.0 || axisValue > 1.0) {
        Napi::RangeError::New(env, "Axis value out of range (-1.0 to 1.0)").ThrowAsJavaScriptException();
        return;
    }

    #if defined(IS_WINDOWS)
        if (!WinPadSetAxis(this->m_pad, axisIndex, axisValue)) {
            Napi::Error::New(env, "Failed to update gamepad state").ThrowAsJavaScriptException();
            return;
        }

    #elif defined(IS_MACOS)
        // Convert normalized value (-1.0 to 1.0) to int16 range (-32768 to 32767)
        int value = (int)(axisValue * 32767.0);

        BOOL result = [GamepadBridge setAxis:this->m_gamepad_id axis:axisIndex value:value];
        if (!result) {
            Napi::Error::New(env, "Failed to update gamepad state").ThrowAsJavaScriptException();
            return;
        }

    #elif defined(IS_LINUX)
        unsigned short code = ABS_X;
        int value = 0;
        switch (axisIndex) {
            case 0: code = ABS_X;  value = (int)(axisValue * 32767.0); break;  // Left Stick X
            case 1: code = ABS_Y;  value = (int)(axisValue * 32767.0); break;  // Left Stick Y
            case 2: code = ABS_RX; value = (int)(axisValue * 32767.0); break;  // Right Stick X
            case 3: code = ABS_RY; value = (int)(axisValue * 32767.0); break;  // Right Stick Y
            case 4: code = ABS_Z;  value = (int)((axisValue + 1.0) * 127.5); break;  // Left Trigger
            case 5: code = ABS_RZ; value = (int)((axisValue + 1.0) * 127.5); break;  // Right Trigger
        }
        if (!Emit(this->m_uinput_fd, EV_ABS, code, value)) {
            Napi::Error::New(env, "Failed to update gamepad state").ThrowAsJavaScriptException();
            return;
        }
    #endif
}


// Gamepad.getDriverStatus(): { isInstalled, version, required, isOutdated }.
// Only Windows needs a driver installed; elsewhere it reports one is.
Napi::Value Gamepad::GetDriverStatus(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);
    #if defined(IS_WINDOWS)
        const WinDriverStatus status = WinDriverGetStatus();
        result.Set("isInstalled", status.isInstalled);
        result.Set("version", status.isInstalled ? Napi::Value(Napi::Number::New(env, status.version)) : env.Null());
        result.Set("required", status.required);
        result.Set("isOutdated", status.isInstalled && status.version < status.required);
    #else
        result.Set("isInstalled", true);
        result.Set("version", env.Null());
        result.Set("required", env.Null());
        result.Set("isOutdated", false);
    #endif
    return result;
}

#if defined(IS_WINDOWS)
// runs the driver setup off the JS thread: it waits for the UAC prompt
class DriverSetupWorker : public Napi::AsyncWorker {
    public:
        DriverSetupWorker(Napi::Env env, const wchar_t* action)
            : Napi::AsyncWorker(env), m_action(action), m_deferred(Napi::Promise::Deferred::New(env)) {}

        Napi::Promise Promise() {
            return this->m_deferred.Promise();
        }

        void Execute() override {
            this->m_isDone = WinDriverRunSetup(this->m_action, this->m_error);
        }

        void OnOK() override {
            Napi::Env env = this->Env();
            if (this->m_isDone) {
                this->m_deferred.Resolve(env.Undefined());
                return;
            }
            Napi::Error error = Napi::Error::New(env, this->m_error.message);
            error.Set("code", Napi::String::New(env, this->m_error.code));
            this->m_deferred.Reject(error.Value());
        }

    private:
        const wchar_t* m_action;
        Napi::Promise::Deferred m_deferred;
        bool m_isDone = false;
        WinPadError m_error;
};
#endif

static Napi::Value RunDriverSetup(const Napi::CallbackInfo& info, const wchar_t* action) {
    Napi::Env env = info.Env();
    #if defined(IS_WINDOWS)
        DriverSetupWorker* worker = new DriverSetupWorker(env, action);
        Napi::Promise promise = worker->Promise();
        worker->Queue();
        return promise;
    #else
        (void)action;
        Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
        deferred.Resolve(env.Undefined());
        return deferred.Promise();
    #endif
}

// Gamepad.installDriver(): installs or updates the driver, behind one UAC prompt
Napi::Value Gamepad::InstallDriver(const Napi::CallbackInfo& info) {
    return RunDriverSetup(info, L"install");
}

// Gamepad.uninstallDriver(): removes it again, behind one UAC prompt
Napi::Value Gamepad::UninstallDriver(const Napi::CallbackInfo& info) {
    return RunDriverSetup(info, L"uninstall");
}


Napi::Object Gamepad::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object obj = Napi::Object::New(env);

    obj.Set(Napi::String::New(env, "list"), Napi::Function::New(env, Gamepad::list));
    obj.Set(Napi::String::New(env, "getDriverStatus"), Napi::Function::New(env, Gamepad::GetDriverStatus));
    obj.Set(Napi::String::New(env, "installDriver"), Napi::Function::New(env, Gamepad::InstallDriver));
    obj.Set(Napi::String::New(env, "uninstallDriver"), Napi::Function::New(env, Gamepad::UninstallDriver));

    // object create
    Napi::Function create = Napi::Function::New(env, Gamepad::CreateObject);
    obj.Set(Napi::String::New(env, "create"), create);

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

    GamepadAddonData* data = new GamepadAddonData();
    data->constructor = Napi::Persistent(func);
    env.SetInstanceData(data);

    create.Set("Gamepad", func);
    return obj;
}

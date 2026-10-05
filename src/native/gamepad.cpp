#include "gamepad.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if defined(IS_WINDOWS)
    #include <windows.h>

    #include <Xinput.h>
    #include <ViGEm/Client.h>
    #pragma comment(lib, "ViGEmClient.lib")
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

// Gamepad.create(): plugs in a new virtual gamepad, or throws why it cannot
Napi::Value Gamepad::CreateObject(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    GamepadAddonData* data = env.GetInstanceData<GamepadAddonData>();

    Napi::Object gamepad = data->constructor.New({});
    if (env.IsExceptionPending()) {
        return env.Undefined();
    }
    data->gamepads.push_back(Napi::Persistent(gamepad));
    return gamepad;
}

Gamepad::Gamepad(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Gamepad>(info) {
    Napi::Env env = info.Env();

    #if defined(IS_WINDOWS)
        // allocate memory
        this->m_client = vigem_alloc();
        if (this->m_client == nullptr) {
            Napi::Error::New(env, "Failed to allocate the ViGEm client").ThrowAsJavaScriptException();
            return;
        }

        // connect to the driver
        const VIGEM_ERROR connected = vigem_connect(this->m_client);
        if (!VIGEM_SUCCESS(connected)) {
            vigem_free(this->m_client);
            this->m_client = nullptr;
            char message[160];
            if (connected == VIGEM_ERROR_BUS_NOT_FOUND) {
                snprintf(message, sizeof(message), "ViGEmBus driver is not installed (https://github.com/nefarius/ViGEmBus/releases)");
            } else if (connected == VIGEM_ERROR_BUS_VERSION_MISMATCH) {
                snprintf(message, sizeof(message), "ViGEmBus driver version is not supported, update it (https://github.com/nefarius/ViGEmBus/releases)");
            } else if (connected == VIGEM_ERROR_BUS_ACCESS_FAILED) {
                snprintf(message, sizeof(message), "Access to the ViGEmBus driver failed");
            } else {
                snprintf(message, sizeof(message), "Failed to connect to the ViGEmBus driver (0x%08X)", (unsigned int)connected);
            }
            Napi::Error::New(env, message).ThrowAsJavaScriptException();
            return;
        }

        // create a new Xbox 360 controller target
        PVIGEM_TARGET pad = vigem_target_x360_alloc();
        if (pad == nullptr) {
            this->Release();
            Napi::Error::New(env, "Failed to allocate the virtual controller").ThrowAsJavaScriptException();
            return;
        }
        const VIGEM_ERROR added = vigem_target_add(this->m_client, pad);
        if (!VIGEM_SUCCESS(added)) {
            // never plugged in, so only freed
            vigem_target_free(pad);
            this->Release();
            char message[96];
            if (added == VIGEM_ERROR_NO_FREE_SLOT) {
                snprintf(message, sizeof(message), "No free slot for another virtual controller");
            } else {
                snprintf(message, sizeof(message), "Failed to plug in the virtual controller (0x%08X)", (unsigned int)added);
            }
            Napi::Error::New(env, message).ThrowAsJavaScriptException();
            return;
        }
        this->m_pad = pad;

        // the state every update sends whole
        this->m_report = new XUSB_REPORT();
        XUSB_REPORT_INIT(this->m_report);

        this->m_active = true;

    #elif defined(IS_MACOS)
        // Create gamepad via GamepadBridge
        this->m_gamepad_id = [GamepadBridge createGamepad];
        if (this->m_gamepad_id < 0) {
            Napi::Error::New(env, "Failed to create the virtual gamepad (needs macOS 26 and the com.apple.developer.hid.virtual.device entitlement)").ThrowAsJavaScriptException();
            return;
        }
        this->m_active = true;

    #elif defined(IS_LINUX)
        std::string openError;
        this->m_uinput_fd = OpenUinput(openError);
        if (this->m_uinput_fd < 0) {
            Napi::Error::New(env, openError).ThrowAsJavaScriptException();
            return;
        }

        // Enable event types
        if (ioctl(this->m_uinput_fd, UI_SET_EVBIT, EV_KEY) < 0 ||
            ioctl(this->m_uinput_fd, UI_SET_EVBIT, EV_ABS) < 0) {
            const int error = errno;
            this->Release();
            Napi::Error::New(env, std::string("Failed to set up the virtual gamepad: ") + strerror(error)).ThrowAsJavaScriptException();
            return;
        }

        // Enable buttons (BTN_GAMEPAD + standard Xbox buttons)
        ioctl(this->m_uinput_fd, UI_SET_KEYBIT, BTN_SOUTH);      // A
        ioctl(this->m_uinput_fd, UI_SET_KEYBIT, BTN_EAST);       // B
        ioctl(this->m_uinput_fd, UI_SET_KEYBIT, BTN_NORTH);      // X
        ioctl(this->m_uinput_fd, UI_SET_KEYBIT, BTN_WEST);       // Y
        ioctl(this->m_uinput_fd, UI_SET_KEYBIT, BTN_TL);         // LB
        ioctl(this->m_uinput_fd, UI_SET_KEYBIT, BTN_TR);         // RB
        ioctl(this->m_uinput_fd, UI_SET_KEYBIT, BTN_SELECT);     // Back
        ioctl(this->m_uinput_fd, UI_SET_KEYBIT, BTN_START);      // Start
        ioctl(this->m_uinput_fd, UI_SET_KEYBIT, BTN_MODE);       // Guide
        ioctl(this->m_uinput_fd, UI_SET_KEYBIT, BTN_THUMBL);     // Left Stick
        ioctl(this->m_uinput_fd, UI_SET_KEYBIT, BTN_THUMBR);     // Right Stick

        // Enable axes
        ioctl(this->m_uinput_fd, UI_SET_ABSBIT, ABS_X);          // Left stick X
        ioctl(this->m_uinput_fd, UI_SET_ABSBIT, ABS_Y);          // Left stick Y
        ioctl(this->m_uinput_fd, UI_SET_ABSBIT, ABS_RX);         // Right stick X
        ioctl(this->m_uinput_fd, UI_SET_ABSBIT, ABS_RY);         // Right stick Y
        ioctl(this->m_uinput_fd, UI_SET_ABSBIT, ABS_Z);          // Left trigger
        ioctl(this->m_uinput_fd, UI_SET_ABSBIT, ABS_RZ);         // Right trigger
        ioctl(this->m_uinput_fd, UI_SET_ABSBIT, ABS_HAT0X);      // D-pad X
        ioctl(this->m_uinput_fd, UI_SET_ABSBIT, ABS_HAT0Y);      // D-pad Y

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
        for (unsigned short code : sticks) {
            abs_setup.code = code;
            ioctl(this->m_uinput_fd, UI_ABS_SETUP, &abs_setup);
        }

        // Triggers (0-255)
        abs_setup.absinfo.minimum = 0;
        abs_setup.absinfo.maximum = 255;
        abs_setup.code = ABS_Z;
        ioctl(this->m_uinput_fd, UI_ABS_SETUP, &abs_setup);
        abs_setup.code = ABS_RZ;
        ioctl(this->m_uinput_fd, UI_ABS_SETUP, &abs_setup);

        // D-pad (-1, 0, 1)
        abs_setup.absinfo.minimum = -1;
        abs_setup.absinfo.maximum = 1;
        abs_setup.code = ABS_HAT0X;
        ioctl(this->m_uinput_fd, UI_ABS_SETUP, &abs_setup);
        abs_setup.code = ABS_HAT0Y;
        ioctl(this->m_uinput_fd, UI_ABS_SETUP, &abs_setup);

        // Create the device. It takes the system a moment to announce it, so
        // applications may miss input sent right after this returns.
        if (ioctl(this->m_uinput_fd, UI_DEV_SETUP, &usetup) < 0 ||
            ioctl(this->m_uinput_fd, UI_DEV_CREATE) < 0) {
            const int error = errno;
            close(this->m_uinput_fd);
            this->m_uinput_fd = -1;
            Napi::Error::New(env, std::string("Failed to create the virtual gamepad: ") + strerror(error)).ThrowAsJavaScriptException();
            return;
        }

        this->m_active = true;
    #endif
}

Gamepad::~Gamepad() {
    this->Release();
}

void Gamepad::Release() {
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
        USHORT buttonMask = 0;
        switch (btnIndex) {
            case 0: buttonMask = XUSB_GAMEPAD_A; break;
            case 1: buttonMask = XUSB_GAMEPAD_B; break;
            case 2: buttonMask = XUSB_GAMEPAD_X; break;
            case 3: buttonMask = XUSB_GAMEPAD_Y; break;
            case 4: buttonMask = XUSB_GAMEPAD_LEFT_SHOULDER; break;
            case 5: buttonMask = XUSB_GAMEPAD_RIGHT_SHOULDER; break;
            case 6: this->m_report->bLeftTrigger = isDown ? 255 : 0; break;
            case 7: this->m_report->bRightTrigger = isDown ? 255 : 0; break;
            case 8: buttonMask = XUSB_GAMEPAD_BACK; break;
            case 9: buttonMask = XUSB_GAMEPAD_START; break;
            case 10: buttonMask = XUSB_GAMEPAD_LEFT_THUMB; break;
            case 11: buttonMask = XUSB_GAMEPAD_RIGHT_THUMB; break;
            case 12: buttonMask = XUSB_GAMEPAD_DPAD_UP; break;
            case 13: buttonMask = XUSB_GAMEPAD_DPAD_DOWN; break;
            case 14: buttonMask = XUSB_GAMEPAD_DPAD_LEFT; break;
            case 15: buttonMask = XUSB_GAMEPAD_DPAD_RIGHT; break;
            case 16: buttonMask = XUSB_GAMEPAD_GUIDE; break;
        }

        if (isDown) {
            this->m_report->wButtons |= buttonMask;
        } else {
            this->m_report->wButtons &= ~buttonMask;
        }

        const VIGEM_ERROR result = vigem_target_x360_update(this->m_client, this->m_pad, *this->m_report);
        if (!VIGEM_SUCCESS(result)) {
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
        // Convert normalized value (-1.0 to 1.0) to Xbox 360 range
        SHORT value = (SHORT)(axisValue * 32767.0);

        switch (axisIndex) {
            case 0: // Left Stick X
                this->m_report->sThumbLX = value;
                break;
            case 1: // Left Stick Y (XInput counts up as positive)
                this->m_report->sThumbLY = -value;
                break;
            case 2: // Right Stick X
                this->m_report->sThumbRX = value;
                break;
            case 3: // Right Stick Y (XInput counts up as positive)
                this->m_report->sThumbRY = -value;
                break;
            case 4: // Left Trigger (0-255)
                this->m_report->bLeftTrigger = (BYTE)((axisValue + 1.0) * 127.5);
                break;
            case 5: // Right Trigger (0-255)
                this->m_report->bRightTrigger = (BYTE)((axisValue + 1.0) * 127.5);
                break;
        }

        const VIGEM_ERROR result = vigem_target_x360_update(this->m_client, this->m_pad, *this->m_report);
        if (!VIGEM_SUCCESS(result)) {
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


Napi::Object Gamepad::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object obj = Napi::Object::New(env);

    obj.Set(Napi::String::New(env, "list"), Napi::Function::New(env, Gamepad::list));

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

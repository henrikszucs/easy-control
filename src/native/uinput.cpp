#include "uinput.h"

#if defined(IS_LINUX)

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/uinput.h>


int OpenUinput(std::string& error) {
    // the uinput device lives at one of two paths
    const char* paths[] = {"/dev/uinput", "/dev/input/uinput"};
    int openError = 0;
    for (const char* path : paths) {
        int fd = open(path, O_WRONLY | O_NONBLOCK);
        if (fd >= 0) {
            return fd;
        }
        if (openError == 0 || errno != ENOENT) {
            openError = errno;
        }
    }
    if (openError == EACCES || openError == EPERM) {
        error = "Permission denied opening /dev/uinput, add the user to the input group (see README)";
    } else if (openError == ENOENT) {
        error = "/dev/uinput not found, load the uinput module (sudo modprobe uinput)";
    } else {
        error = std::string("Failed to open /dev/uinput: ") + strerror(openError);
    }
    return -1;
}


namespace {

// the fractions PointerMoveTo takes are spread over this range
const int POINTER_RANGE = 65535;

std::mutex deviceMutex;
int pointerFd = -1;
int keyboardFd = -1;

bool Write(int fd, const struct input_event* events, size_t count) {
    const ssize_t size = (ssize_t)(sizeof(struct input_event) * count);
    return write(fd, events, size) == size;
}

struct input_event Event(unsigned short type, unsigned short code, int value) {
    struct input_event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code;
    event.value = value;
    return event;
}

// Plugs in the device the setup function describes; -1 and why on failure.
template <typename Setup>
int CreateDevice(const char* name, unsigned short product, Setup setup, std::string& error) {
    int fd = OpenUinput(error);
    if (fd < 0) {
        return -1;
    }
    if (!setup(fd)) {
        error = std::string("Failed to set up the virtual ") + name + ": " + strerror(errno);
        close(fd);
        return -1;
    }

    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_VIRTUAL;
    usetup.id.vendor = 0x1209;      // pid.codes, the vendor ID for open projects
    usetup.id.product = product;
    usetup.id.version = 1;
    snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "easy-control virtual %s", name);

    if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0 || ioctl(fd, UI_DEV_CREATE) < 0) {
        error = std::string("Failed to create the virtual ") + name + ": " + strerror(errno);
        close(fd);
        return -1;
    }

    // The compositor opens the new device only once udev has announced it;
    // events sent before that are lost, so the first use waits for it, once.
    usleep(200000);
    return fd;
}

// An absolute pointer with buttons and wheels. udev counts a device with
// absolute X/Y and mouse buttons (and no touch or pen) as a mouse - the way VM
// "tablet" mice are made - and compositors map its range onto the bounding box
// of all outputs.
bool EnsurePointer(std::string& error) {
    if (pointerFd >= 0) {
        return true;
    }
    pointerFd = CreateDevice("pointer", 0x0002, [](int fd) {
        if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0 ||
            ioctl(fd, UI_SET_EVBIT, EV_REL) < 0 ||
            ioctl(fd, UI_SET_EVBIT, EV_ABS) < 0) {
            return false;
        }
        const int buttons[] = {BTN_LEFT, BTN_RIGHT, BTN_MIDDLE, BTN_SIDE, BTN_EXTRA};
        for (int button : buttons) {
            ioctl(fd, UI_SET_KEYBIT, button);
        }
        ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
        ioctl(fd, UI_SET_RELBIT, REL_HWHEEL);

        struct uinput_abs_setup absSetup;
        memset(&absSetup, 0, sizeof(absSetup));
        absSetup.absinfo.minimum = 0;
        absSetup.absinfo.maximum = POINTER_RANGE;
        absSetup.code = ABS_X;
        if (ioctl(fd, UI_ABS_SETUP, &absSetup) < 0) {
            return false;
        }
        absSetup.code = ABS_Y;
        return ioctl(fd, UI_ABS_SETUP, &absSetup) >= 0;
    }, error);
    return pointerFd >= 0;
}

// A keyboard with every key of the evdev range the key tables use.
bool EnsureKeyboard(std::string& error) {
    if (keyboardFd >= 0) {
        return true;
    }
    keyboardFd = CreateDevice("keyboard", 0x0003, [](int fd) {
        if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0) {
            return false;
        }
        for (int key = KEY_ESC; key <= KEY_MICMUTE; key++) {
            ioctl(fd, UI_SET_KEYBIT, key);
        }
        return true;
    }, error);
    return keyboardFd >= 0;
}

// writes the events and the report closing them; on a write error the device
// is dropped, to be made again on the next call
bool Send(int& fd, std::initializer_list<struct input_event> events, std::string& error) {
    struct input_event buffer[4];
    size_t count = 0;
    for (const struct input_event& event : events) {
        buffer[count++] = event;
    }
    buffer[count++] = Event(EV_SYN, SYN_REPORT, 0);
    if (!Write(fd, buffer, count)) {
        error = std::string("Failed to send input: ") + strerror(errno);
        ioctl(fd, UI_DEV_DESTROY);
        close(fd);
        fd = -1;
        return false;
    }
    return true;
}

int ToRange(double fraction) {
    if (!(fraction > 0)) {
        return 0;
    }
    const long value = std::lround(fraction * (POINTER_RANGE + 1));
    return value > POINTER_RANGE ? POINTER_RANGE : (int)value;
}

}  // namespace


namespace VirtualInput {

bool PointerMoveTo(double x, double y, std::string& error) {
    std::lock_guard<std::mutex> lock(deviceMutex);
    if (!EnsurePointer(error)) {
        return false;
    }
    return Send(pointerFd, {Event(EV_ABS, ABS_X, ToRange(x)), Event(EV_ABS, ABS_Y, ToRange(y))}, error);
}

bool PointerButton(unsigned short button, bool isDown, std::string& error) {
    std::lock_guard<std::mutex> lock(deviceMutex);
    if (!EnsurePointer(error)) {
        return false;
    }
    return Send(pointerFd, {Event(EV_KEY, button, isDown ? 1 : 0)}, error);
}

bool PointerScroll(unsigned short axis, int amount, std::string& error) {
    std::lock_guard<std::mutex> lock(deviceMutex);
    if (!EnsurePointer(error)) {
        return false;
    }
    return Send(pointerFd, {Event(EV_REL, axis, amount)}, error);
}

bool KeyboardKey(unsigned short key, bool isDown, std::string& error) {
    std::lock_guard<std::mutex> lock(deviceMutex);
    if (!EnsureKeyboard(error)) {
        return false;
    }
    return Send(keyboardFd, {Event(EV_KEY, key, isDown ? 1 : 0)}, error);
}

}  // namespace VirtualInput

#endif

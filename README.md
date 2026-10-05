# easy-control
Simple and easy to use node.js module to simulate mouse, keyboard and gamepad input for Windows, MacOS and Linux.



## Usage

```js
// Module import
import { Mouse, Keyboard,  Gamepad, Screen } from "./dist/easy-control.mjs";
// CommonJS import
const { Mouse, Keyboard,  Gamepad, Screen } = require("./dist/easy-control.cjs");
// Electron import example (need OS absolute route to .node file)
const absolutePath = "/tmp/dist/easy-control.node"
const { Mouse, Keyboard, Gamepad, Screen } = require(absolutePath);

/*
    !!! Function calculate with X and Y positions logical scaled value (not the native resolution)!!!

    Example: 1920 x 1080 screen with 1.25 (125% scale) >> the sreen size will be 1536 x 864 and right bottom corner coorinates will be X:1536; Y:864

Note:
    - If you want convert normal pixel to scaled just divide by scale e.g. 1920/1.25 = 1536
    - If you want convert scaled pixel to normal just multiply is by scale e.g. 1536*1.25 = 1920
    - Mouse.getX/getY, Mouse.setX/setY/setPosition and Screen.list all use the same coordinates,
      so a position read can be set back, and a point inside a screen from Screen.list lands on that screen
    - Windows: with screens of different scales, each screen is placed against its neighbour (as Electron's
      screen API does), so the logical space has no gaps or overlaps
    - macOS: points, origin at the top left of the main display
    - Linux (X11): pixels, scaleFactor is always 1.0 - X11 has no logical coordinates
    - Linux (Wayland): the compositor's logical coordinates, see "Linux: X11 and Wayland" below
*/
```

### Mouse
```js
const x = Mouse.getX();
const y = Mouse.getY();

const icon = Mouse.getIcon();
/*
{
    "width": 32,        // icon width
    "height": 32,       // icon height
    "data": Uint8Array, // image in rgba data, row by row from the top (width * height * 4 bytes)
    "xOffset": 0,       // pointer X offset from icon
    "yOffset": 0        // pointer Y offset from icon
}
*/

const iconId = Mouse.getIconId();   // a number that changes when the pointer shape changes, 0 while it is hidden.
                                    // Cheap on Windows and Linux: poll it and call getIcon only when it changes.
                                    // On macOS it is a hash of the picture, about as costly as getIcon.

Mouse.setPosition(x, y);    // move in one step
Mouse.setX(x);
Mouse.setY(y);

Mouse.buttonDown(btn);  // "right" | "middle" | "left" | "back" | "forward"
Mouse.buttonUp(btn);    // "right" | "middle" | "left" | "back" | "forward"

Mouse.scrollDown(amount=1, isHorizontal=false); // down or right scroll
Mouse.scrollUp(amount=1, isHorizontal=false);   // up or left scroll
```

### Keyboard
```js

const isKeySupported = Keyboard.isKeySupported(key=""); // check if special key is supported.

Keyboard.keyDown(key="");   // key value is a KeyboardEvent "code" property string (https://developer.mozilla.org/en-US/docs/Web/API/UI_Events/Keyboard_event_code_values)
Keyboard.keyUp(key="");

Keyboard.type(char="");     // Character to type. "keyDown" with "keyUp" methods does with physical keyboard keys but if you want input layout dependent characters like ő,ú,ű on english keyboard, use this.

const layout = Keyboard.GetLayout();    // Get the current layout settings in string
Keyboard.SetLayout(layout="");  // Set the keyboard language setting, this affect keyDown, keyUp characters, Windows:https://learn.microsoft.com/en-us/windows-hardware/manufacture/desktop/windows-language-pack-default-values?view=windows-11


```


### Gamepad
```js
/*
--------------------
On Windows for gamepad support need the latest ViGEm Bus Driver (https://github.com/nefarius/ViGEmBus/releases)
Note: ViGEmBus is retired by its author (no further updates); it still works, but it is a dependency with no future.

--------------------
On macOS the virtual gamepad needs macOS 26 and an app signed with the
com.apple.developer.hid.virtual.device entitlement (CoreHID HIDVirtualDevice), so it does not work from a plain
`node` process. It appears as a generic HID gamepad ("easy-control Virtual Gamepad"), not as an Xbox controller.

--------------------
On Linux run few configuration to enable UInput (the gamepad needs it, and on Wayland the mouse and keyboard too)
# Add your user to the input group
sudo usermod -a -G input $USER

# Create udev rule for uinput permissions
sudo tee /etc/udev/rules.d/99-uinput.rules << EOF
KERNEL=="uinput", MODE="0660", GROUP="input", OPTIONS+="static_node=uinput"
EOF

# Reload udev rules
sudo udevadm control --reload-rules && sudo udevadm trigger

# Load uinput module
sudo modprobe uinput

# Make it load on boot
echo "uinput" | sudo tee -a /etc/modules

--------------------



*/


const gamepads = Gamepad.list(); // return gamepad objects in array

const gamepad1 = Gamepad.create();  // throws an Error saying why when no virtual gamepad can be made
                                    // (driver missing, no permission, ...)
gamepad1.isActive();

gamepad1.buttonDown(btn=0);
gamepad1.buttonUp(btn=0);
gamepad1.setAxis(axis=0, direction=0);

gamepad1.destroy();     // unplugs it; isActive() is false after, and its methods throw

/*
btn - number from 0 to 16
axis - number from 0 to 5
direction - number from -1 to 1

Indices follow the W3C Standard Gamepad layout (navigator.getGamepads()), so a browser
gamepad's buttons and axes can be passed through as they are.
Values for a standard Xbox360 controller:
    btn=0 - A button
    btn=1 - B button
    btn=2 - X button
    btn=3 - Y button
    btn=4 - left button
    btn=5 - right button
    btn=6 - left trigger (pulled fully)
    btn=7 - right trigger (pulled fully)
    btn=8 - select button
    btn=9 - start button
    btn=10 - left stick button
    btn=11 - right stick button
    btn=12 - d-pad up button
    btn=13 - d-pad down button
    btn=14 - d-pad left button
    btn=15 - d-pad right button
    btn=16 - home button

    axis=0 - left stick horizontal direction: from -1 left to 1 right
    axis=1 - left stick vertical direction: from -1 up to 1 down
    axis=2 - right stick horizontal direction: from -1 left to 1 right
    axis=3 - right stick vertical direction: from -1 up to 1 down
    axis=4 - left trigger, analog: from -1 released to 1 pulled
    axis=5 - right trigger, analog: from -1 released to 1 pulled

*/



```


### Screen
```js
const screens = Screen.list();
/*
[
    {
        "isPrimary": true,
        "width": 1536,
        "height": 864,
        "x": 0,
        "y": 0,
        "scaleFactor": 1.25
    },
    {
        "isPrimary": false,
        "width": 1680,
        "height": 900,
        "x": -1680,
        "y": 0,
        "scaleFactor": 1
    }
]
*/
```

## Linux: X11 and Wayland

In an X11 session, mouse and keyboard go through XTest and the screens come from XRandR.

In a Wayland session (`WAYLAND_DISPLAY` set, or `XDG_SESSION_TYPE=wayland`) the input goes through two virtual
devices made with uinput, "easy-control virtual pointer" and "easy-control virtual keyboard", so it reaches every
application, native Wayland ones too. The compositor reads them like any other mouse and keyboard (GNOME, KDE, sway
and other wlroots compositors, ...).

- uinput needs the same setup as the gamepad (the input group and the udev rule in the Gamepad section); without it
  the mouse and keyboard functions throw, saying so.
- The devices are made on the first call that needs them, which waits 0.2 s once so the compositor has picked them up.
- `Screen.list()` asks the compositor (wl_output, xdg-output): positions and sizes are logical, `scaleFactor` is the
  output's pixels per logical pixel, and the output at 0,0 is reported as primary - Wayland has no primary output.
  libwayland-client is loaded at run time, so it is no build dependency; without it XRandR (XWayland) is used.
- `Mouse.setPosition/setX/setY` move an absolute pointer over the bounding box of all screens.
- `Mouse.getX/getY` - Wayland tells no client where the pointer is: they return the position last set through
  easy-control, and XWayland's idea of it before the first move.
- `Mouse.getIcon/getIconId` see only the pointer shapes of XWayland applications.
- `Keyboard.keyDown/keyUp` press physical keys, as on the other platforms.
- `Keyboard.type` presses the keys of the current layout, read from XWayland, so it needs XWayland. A character the
  layout has no key for is skipped, and the call throws naming them once the rest is typed - the compositor's keymap
  is not a client's to change.
- `Keyboard.GetLayout` reads the layout from XWayland; `Keyboard.SetLayout` throws - every desktop has its own way.

`EASY_CONTROL_BACKEND=x11` or `EASY_CONTROL_BACKEND=wayland` in the environment overrides the detection.

## Testing

The Windows gamepad needs the ViGEmBus driver; without it `Gamepad.create()` throws and the test app reports that and skips the gamepad tests.


The tests run in electron enviroment. Copy ./dev/test folder to electron app and run.

## Building

```
npm install
npm run build
```

`npm install` brings `node-gyp`, `node-addon-api` and `esbuild` in as dev dependencies, so none has to be installed globally.

`npm run build` compiles the addon for the running platform, then minifies the JS loaders (`src/easy-control.cjs`, `src/easy-control.mjs`) into `dist/` with esbuild. The native sources are in `src/native/`, the build script of the loaders is `src/build.js`. To rebuild only the loaders, without a C++ toolchain:

```
npm run build:js
```

### Clean
```
npm run clean
```

> [!CAUTION]
> For building need additional tools. It is different in every operating system.

### Dependencies
#### Windows
- install Visual Studio [https://visualstudio.microsoft.com/vs/community/](https://visualstudio.microsoft.com/vs/community/) and select "Desktop development with C++" bundle
- install Python 3.6+ [https://apps.microsoft.com/detail/9ncvdn91xzqp](https://apps.microsoft.com/detail/9ncvdn91xzqp)
- install CMake [https://cmake.org/download/](https://cmake.org/download/)

#### MacOS
- install Xcode [https://apps.apple.com/us/app/xcode/id497799835](https://apps.apple.com/us/app/xcode/id497799835)

#### Linux
- ```sudo apt-get install libx11-dev libxtst-dev libxfixes-dev libxrandr-dev```



## License

[LGPL-3.0-only](./LICENSE) — see also the referenced [GPL-3.0](./LICENSE.GPL-3.0).

The Windows build bundles [ViGEmClient](https://github.com/nefarius/ViGEmClient) (`dist/win32-x64/ViGEmClient.dll`), MIT License, Copyright (c) 2017-2019 Nefarius Software Solutions e.U. and Contributors; its notice ships beside it as `dist/win32-x64/ViGEmClient.LICENSE`.

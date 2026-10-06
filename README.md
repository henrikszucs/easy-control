# easy-control
Simple and easy to use node.js module to simulate mouse, keyboard and gamepad input for Windows, MacOS and Linux.



## Usage

```js
// ES module
import { Mouse, Keyboard, Gamepad, Screen, Platform } from "easy-control";
// CommonJS
const { Mouse, Keyboard, Gamepad, Screen, Platform } = require("easy-control");

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

TypeScript definitions come with the package, for both.

### Platform
```js
Platform.target;            // "win32-x64": this process's platform and CPU
Platform.supportedTargets;  // ["darwin-arm64", "darwin-x64", "linux-arm64", "linux-x64", "win32-arm64", "win32-x64"]
Platform.isSupported;       // the build for this target is there and loaded
Platform.loadError;         // why not, when isSupported is false; null otherwise

// Importing never throws: where isSupported is false, every other function throws (or, if it returns a
// Promise, rejects) an Error with code "EASYCONTROL_UNSUPPORTED_PLATFORM" and Platform.loadError as message.

Platform.hasInputAccess();  // whether input sent now reaches applications: on macOS the app needs the
                            // Accessibility permission, on Wayland /dev/uinput must be writable; true on Windows
await Platform.requestInputAccess();
// macOS: shows the system prompt pointing to System Settings > Privacy & Security > Accessibility, and
// resolves with the access there is now - false until the user allows it, so check hasInputAccess() again
// later. Elsewhere it resolves with hasInputAccess() at once.
```

Keys and mouse buttons pressed through easy-control and still down when the process ends are released (on exit,
and on SIGINT and SIGTERM when the app does not handle those itself), so a closing app leaves none stuck.

### Electron

Use it in the main process (or a preload or renderer with Node integration) like any other dependency. Native files
cannot be loaded from inside an asar archive, so when packaging, unpack the addon and the Windows gamepad driver
files, which the driver setup must find as real files:

```json
"asarUnpack": ["**/node_modules/easy-control/dist/*/*.node", "**/node_modules/easy-control/dist/*/gamepad/**"]
```

(electron-builder; Electron Forge's `@electron-forge/plugin-auto-unpack-natives` covers the `.node` files, add the
`gamepad` folder to its `unpack` option.)

### Mouse
```js
const x = Mouse.getX();
const y = Mouse.getY();

const icon = Mouse.getIcon();
/*
The same format on every platform:
{
    "width": 32,        // in physical pixels (on a Retina Mac, 2 per point)
    "height": 32,
    "data": Uint8Array, // RGBA, row by row from the top (width * height * 4 bytes), straight (not
                        // premultiplied) alpha; fully transparent pixels are black
    "xOffset": 0,       // the hot spot (the pixel that points), in physical pixels
    "yOffset": 0
}
While the pointer is hidden: width and height 0, data empty. Windows draws the inverting pixels of black-and-
white cursors (the text I-beam) by the screen under them; here they are mid-grey.
*/

const iconId = Mouse.getIconId();   // a number that changes when the pointer shape changes, 0 while it is hidden.
                                    // Cheap on Windows and Linux: poll it and call getIcon only when it changes.
                                    // On macOS it is a hash of the picture, about as costly as getIcon.

Mouse.setPosition(x, y);    // move in one step
Mouse.setX(x);
Mouse.setY(y);

Mouse.buttonDown(btn);  // "right" | "middle" | "left" | "back" | "forward"
Mouse.buttonUp(btn);    // "right" | "middle" | "left" | "back" | "forward"
Mouse.releaseAll();     // releases every button buttonDown pressed and buttonUp did not release yet

Mouse.scrollDown(amount, isHorizontal);    // down, or right when isHorizontal is true
Mouse.scrollUp(amount, isHorizontal);      // up, or left when isHorizontal is true
// Both arguments are required. amount is in wheel notches; a fraction is dropped, a negative amount scrolls
// the other way, 0 does nothing. Pixel deltas (e.g. from a browser's wheel event) can be added up in JS and
// sent a notch at a time.
```

### Keyboard
```js

// a physical key, named by its KeyboardEvent "code" value ("KeyA", "ShiftLeft", "Numpad5", ...):
// https://developer.mozilla.org/en-US/docs/Web/API/UI_Events/Keyboard_event_code_values
const isKeySupported = Keyboard.isKeySupported(code);  // true when this platform can press that key

Keyboard.keyDown(code);
Keyboard.keyUp(code);
Keyboard.releaseAll();      // releases every key keyDown pressed and keyUp did not release yet,
                            // e.g. when the remote side of a session is gone
// keyDown/keyUp press a key by its place on the keyboard (its scan code), the same key whatever the layout;
// the layout only decides which character that key makes. "KeyY" is the key right of "KeyT": it types "y"
// on a US layout and "z" on a German or Hungarian one.

Keyboard.type(text);
// Enters text as it is, whatever the layout: "ő€日本😀" types the same on every layout. Use it for text,
// keyDown/keyUp for keys and shortcuts.
// Linux types each character with the key that makes it on the current layout, Shift and AltGr included
// (on a Hungarian layout "@" is AltGr+V). A character the layout has no key for is put on a spare key on X11;
// on Wayland it is skipped and the call throws naming it, once the rest is typed, unless:
Keyboard.type(text, { "unicodeFallback": true });
// enters those characters with Ctrl+Shift+U and their code point, which GTK and IBus applications
// understand - other applications get stray characters or shortcuts, hence it is off by default.

const layout = Keyboard.GetLayout();
Keyboard.SetLayout(layout);
// The current keyboard layout, as the platform names it:
//   Windows: the layout ID (KLID) of the window the input goes to, e.g. "00000409" (US), "0000040E" (Hungarian),
//            see https://learn.microsoft.com/windows-hardware/manufacture/desktop/windows-language-pack-default-values
//   macOS:   the input source ID, e.g. "com.apple.keylayout.US"
//   Linux:   the XKB group name, e.g. "English (US)"; SetLayout throws on Wayland
// SetLayout switches it: on Windows for the window the input goes to, on macOS the input source, on X11 the XKB
// group. It changes what characters keys make, so what keyDown/keyUp type - not which keys they press.

```


### Gamepad
```js
/*
--------------------
On Windows the virtual gamepad uses easy-control's own driver, installed once per machine (see "Windows gamepad
driver" below). Without it Gamepad.create() throws an Error with code "EASYCONTROL_DRIVER_MISSING".

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

const gamepad1 = await Gamepad.create();    // a Promise: plugging a gamepad in takes a moment (on Windows
                                            // the service starts and the devices come up). It rejects
                                            // with an Error saying why it cannot (driver missing, no
                                            // permission, ...), with a code, see below
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

// the Windows driver; on macOS and Linux there is nothing to install, so these report it
// installed and resolve at once
const status = Gamepad.getDriverStatus();
// { isInstalled: true, version: 1, required: 1, isOutdated: false }
await Gamepad.installDriver();      // installs or updates it, behind one UAC prompt
await Gamepad.uninstallDriver();    // removes it, behind one UAC prompt
```

#### Windows gamepad driver

A virtual gamepad on Windows is two software devices served by easy-control's own user-mode (UMDF 2) driver: one
XInput reads, so games, browsers, SDL and Windows.Gaming.Input see an Xbox 360 controller, and one HID gamepad for
DirectInput and Raw Input. A small service, started on demand, plugs them in for applications that are not
administrators and unplugs them when the application destroys the gamepad or exits. Up to 4 gamepads (XInput's
limit). Windows 10 1903 or later, x64 and ARM64. Windows Server editions lack Microsoft's Xbox controller filter
(`xinputhid.sys`): there the gamepad works for XInput, but Windows.Gaming.Input - and so Chromium and Electron -
do not see it.

An application can offer the install when it is missing:

```js
let gamepad;
try {
    gamepad = await Gamepad.create();
} catch (error) {
    if (error.code === "EASYCONTROL_DRIVER_MISSING" || error.code === "EASYCONTROL_DRIVER_OUTDATED") {
        await Gamepad.installDriver();  // rejects with code EASYCONTROL_SETUP_CANCELLED if UAC is declined
        gamepad = await Gamepad.create();
    } else {
        throw error;
    }
}
```

`installDriver()` runs `dist/win32-<x64 or arm64>/gamepad/easy-control-gamepad-setup.ps1 install` as administrator. It copies
the driver and the service to `%ProgramFiles%\easy-control\gamepad`, creates a code signing certificate for this
machine, trusts it, signs the driver package with it and deletes its private key, so the certificate can sign
nothing else; then it installs the driver package and the service. No test-signing mode and no reboot are needed,
because the driver runs in user mode. Its log is `%ProgramData%\easy-control\gamepad-setup.log`; the script can also
be run by hand (`install`, `uninstall`, `status`). `uninstallDriver()` removes all of it, the certificate included.

The service logs the pads it plugs in and out, and failures with their reason, to
`%ProgramData%easy-controlgamepad-service.log` (at most 256 KB, the one before as `.log.old`); it starts when an
application asks for a gamepad and stops itself after a minute with none.

`Gamepad.create()` rejects with an Error with one of these `code`s: `EASYCONTROL_DRIVER_MISSING`,
`EASYCONTROL_DRIVER_OUTDATED`, `EASYCONTROL_NO_SLOT` (4 gamepads already), `EASYCONTROL_SERVICE_FAILED`,
`EASYCONTROL_CREATE_FAILED`.


### Screen
```js
const screens = Screen.list();
/*
[
    {
        "id": "\\.\DISPLAY1",    // stable while the screen stays connected, see below
        "name": "DELL U2720Q",      // the monitor's name for people, "" when the system has none
        "isPrimary": true,
        "width": 1536,
        "height": 864,
        "x": 0,
        "y": 0,
        "scaleFactor": 1.25
    },
    {
        "id": "\\.\DISPLAY2",
        "name": "SyncMaster",
        "isPrimary": false,
        "width": 1680,
        "height": 900,
        "x": -1680,
        "y": 0,
        "scaleFactor": 1
    }
]

id: Windows the GDI device name ("\.DISPLAY1"), macOS the CGDirectDisplayID ("69733248"), Linux the output name
("HDMI-1"). On macOS it equals Electron's display.id; elsewhere match a screen to Electron's displays or to a
desktopCapturer source by their bounds, which are the same to the pixel on Windows (screens are rounded and laid out
as Electron does).
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

## Limits

What the operating systems do not let a program do, or do only in part.

### Windows
- **Elevated windows**: input does not reach a window of a process running as administrator unless the process
  sending it runs as administrator too (User Interface Privilege Isolation). It is dropped without an error.
- **The secure desktop**: nothing reaches UAC prompts, the lock and sign-in screens, or Ctrl+Alt+Del. A user
  controlling a machine remotely cannot approve a UAC prompt - `Gamepad.installDriver()`'s included - so install
  the driver while someone is at the machine.
- **Raw Input**: `Mouse.setPosition`/`setX`/`setY` move the pointer, which applications reading Raw Input do not see
  as mouse movement: many games and pointer-locked web pages. Clicks, scrolling and keys do reach them.
- **`Keyboard.type`** sends the characters as Unicode packets, which some games and remote-desktop clients ignore;
  `keyDown`/`keyUp` reach them.
- **Gamepads**: at most 4 (XInput's limit); kernel-level anti-cheat may refuse virtual ones; on Windows Server
  editions XInput only (see "Windows gamepad driver").

### macOS
- Input needs the **Accessibility** permission (System Settings > Privacy & Security > Accessibility) for the app
  that runs easy-control - the terminal for `node`, the app itself for Electron. Without it macOS drops every event,
  without an error.
- The virtual gamepad needs macOS 26 and an app signed with the `com.apple.developer.hid.virtual.device` entitlement.

### Linux
- X11 coordinates are pixels; there is no logical (scaled) coordinate space.
- Wayland: see "Linux: X11 and Wayland" above.

## Testing

The tests use the built `dist/`, so build first.

```
npm test              # unit tests (node:test), test/unit/
npm run test:types    # the TypeScript definitions against test/types/ (tsc)
npm run test:native   # C++ tests of code without system calls, test/native/ (the Windows screen layout)
npm run test:e2e      # end-to-end tests in an Electron window, test/e2e/
npm run test:driver   # Windows: uninstalls and reinstalls the gamepad driver, test/driver/
```

On Linux, `test/unit/typing-x11.test.js` types into a window of its own with a Hungarian layout, to check characters
that need AltGr; as it types for real, it runs only with `EASYCONTROL_TYPING_TEST=1` (CI sets it under Xvfb).
CI (`.github/workflows/build.yml`) builds every target and runs these on Windows, macOS and Linux.

`npm test` checks the loaders, every function's results and argument checks, and the gamepad lifecycle. It moves the
pointer and puts it back, but never clicks, scrolls or presses keys.

`npm run test:e2e` opens a window and drives it with real input: clicks with every button, drags, scrolling,
`getIcon`/`getIconId` against two pointer shapes, every common key, modifiers, `Keyboard.type`, `Screen.list` against
Electron's screen API, and a virtual gamepad against `navigator.getGamepads()`. It takes the mouse and keyboard for
about 5 seconds; keys are only pressed while its window has the focus.

The gamepad tests need the platform's virtual gamepad support (the driver on Windows, uinput on Linux, see Gamepad);
without it they are skipped, with the reason `Gamepad.create()` gave.

`npm run test:driver` checks `uninstallDriver()` and `installDriver()` for real: after the uninstall no service,
driver package, certificate, Program Files folder or device of the driver is left, and after the install a gamepad
works again. Each step asks for administrator rights, so it shows two UAC prompts (three when the driver was not
installed, as it leaves the machine as it found it). It is not part of `npm test`.

## Building

```
npm install
npm run build
```

`npm install` brings `node-gyp`, `node-addon-api` and `esbuild` in as dev dependencies, so none has to be installed globally. Building needs Node `^22.22.2`, `^24.15.0` or `>=26` (what node-gyp 13 runs on); using the built package needs Node 22 or later, or Electron 21 or later (it brings its own Node).

`npm run build` compiles the addon for the running platform, then minifies the JS loaders (`src/easy-control.cjs`, `src/easy-control.mjs`) into `dist/` with esbuild. The native sources are in `src/native/`, the build script of the loaders is `src/build.js`. Another CPU is given with `--arch`; on Windows x64 this builds `dist/win32-arm64/easy-control.node` (needs Visual Studio's "MSVC ARM64 build tools"):

```
npm run build -- --arch arm64
```

To rebuild only the loaders, without a C++ toolchain:

```
npm run build:js
```

The Windows gamepad driver, its service and setup script (`src/native/windows-gamepad/`) are built separately into
`dist/win32-x64/gamepad/` and `dist/win32-arm64/gamepad/`:

```
npm run build:gamepad
```

It needs Visual Studio's C++ tools only, and builds ARM64 too when the "MSVC ARM64 build tools" are installed
(it says so when it skips it). The parts of the Windows Driver Kit it uses (UMDF headers and libraries, InfVerif,
Inf2Cat) come from Microsoft's WDK NuGet packages, downloaded once into `build_wdk/`.

Only the results in `dist/` are committed; `build/` and `build_wdk/` hold intermediate files and are ignored.

### Clean
```
npm run clean            # build/, build_swift/, tmp/
npm run clean -- --all   # also node_modules/ and build_wdk/ (the WDK download cache)
```

> [!CAUTION]
> For building need additional tools. It is different in every operating system.

### Dependencies
#### Windows
- install Visual Studio [https://visualstudio.microsoft.com/vs/community/](https://visualstudio.microsoft.com/vs/community/) and select "Desktop development with C++" bundle
- install Python 3.6+ [https://apps.microsoft.com/detail/9ncvdn91xzqp](https://apps.microsoft.com/detail/9ncvdn91xzqp)

#### MacOS
- install Xcode [https://apps.apple.com/us/app/xcode/id497799835](https://apps.apple.com/us/app/xcode/id497799835)

#### Linux
- ```sudo apt-get install libx11-dev libxtst-dev libxfixes-dev libxrandr-dev```



## License

[LGPL-3.0-only](./LICENSE) — see also the referenced [GPL-3.0](./LICENSE.GPL-3.0).

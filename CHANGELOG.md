# Changelog

## 0.12.0

What a remote desktop needs from the library: movement games and pointer-locked pages read, smooth scrolling,
batched gamepad state and rumble, and knowing why input does not land.

### Added

- `Mouse.moveBy(dx, dy)`: relative movement in mouse counts, which pointer-locked pages (`movementX`/`movementY`)
  and games read - `setPosition` never reaches them. On Wayland through a relative virtual mouse of its own.
- `Mouse.scroll(x, y)`: scrolling by fractions of a notch, with `WheelEvent`'s signs (Windows and Wayland: 120ths
  of a notch, macOS: points, X11: added up to whole notches).
- `Keyboard.getLockState()`: `{ capsLock, numLock, scrollLock }`.
- `Keyboard.getLayout()` / `setLayout()`, the same functions as `GetLayout` / `SetLayout`, which stay.
- `Platform.getInputBlock()`: why input would not arrive now - `"secure-desktop"`, `"elevated-window"` (Windows),
  `"secure-input"` (macOS), `"no-permission"` (macOS, Wayland) - or null.
- `gamepad.setState({ buttons, axes })`: many changes in one report; a browser `Gamepad` can be passed as it is,
  analog triggers included.
- `gamepad.onRumble`: what games set the rumble motors to, on Windows and Linux.
- Driver versions (Windows): `getDriverStatus()` has `available` (the driver in this package) and
  `isUpdateAvailable`; `create()` rejects with `EASYCONTROL_DRIVER_RESTART_NEEDED` when an older driver or service
  than the installed one still runs; `installDriver({ force })`.
- The gamepad driver 3: tells rumble as it changes (driver 2 is asked every 16 ms) and its own version.

### Fixed

- macOS: two quick clicks are a double click (the events carry the click count); drags carry it too.
- `Keyboard.type`: `"\n"`, `"\r\n"` (once), `"\t"`, `"\b"` and `"\x1b"` press Enter, Tab, Backspace and
  Escape - before, Windows and macOS sent them as characters (macOS on the A key's code), so applications saw no
  Enter. `type("")` does nothing instead of throwing.
- Releasing held keys and buttons when a worker thread ended released the main thread's too.
- `scrollDown`/`scrollUp` with a huge amount hung X11 and overflowed on Windows; more than 10000 notches is now a
  `RangeError`.
- Linux: input sent to a new gamepad right after `create()` was lost.
- Wayland: `Mouse.setPosition` made two round trips to the compositor for the screen layout each time; it now
  reads what changed without waiting.
- macOS: moves carry their distance in the event's delta fields.

### Behaviour changes

- Windows: input the system refuses (`SendInput` sending nothing, the pointer not to be read or set - the secure
  desktop) throws an Error with code `EASYCONTROL_INPUT_BLOCKED` instead of passing silently; `getX`/`getY` throw
  instead of returning 0, on every platform.
- `getDriverStatus().required` is now the oldest driver this version works with (2), not the newest; a newer
  driver serves older versions, so 0.12.0 needs no reinstall. `installDriver()` no longer replaces a newer
  installed driver without `{ force: true }`.
- The Linux gamepad takes force feedback (rumble) requests.

## 0.11.0

The first release whose builds of every target come from CI and pass its tests: 0.10.0's macOS and Linux builds
in `dist/` were from before it, and its macOS sources did not compile.

### Added

- Builds for macOS on Intel (`darwin-x64`) and Linux on ARM (`linux-arm64`); all six builds in `dist/` are now
  made and tested by CI.

### Fixed

- macOS: the addon builds again (the Swift part used macOS 26 types outside availability checks once built for
  macOS 10.15, the oldest it supports).
- macOS `Keyboard.isKeySupported("KeyA")` was false: the A key's code is 0, which counted as "no key".
- macOS `Mouse.getX`/`getY` right after a move read the previous position; the pointer is now there at once.
- Windows Server editions, which lack Microsoft's `xinputhid` filter: the virtual gamepad starts there and works
  for XInput (not for Windows.Gaming.Input); before, it did not start.
- `Gamepad.uninstallDriver()` left the HID collections of removed gamepads behind as phantom devices.

### Other

- CI shows why a step failed in an annotation, readable without signing in; it uses Node 24 actions.

## 0.10.0

### Breaking

- **Windows gamepad**: easy-control's own driver replaces ViGEmBus, which was retired in 2023. It is installed once
  per machine with `Gamepad.installDriver()` (one UAC prompt); until then `Gamepad.create()` rejects with
  `code: "EASYCONTROL_DRIVER_MISSING"`. ViGEmBus is no longer used and can be uninstalled. Windows 10 1903 or later,
  x64 and ARM64.
- **`Gamepad.create()` returns a Promise** on every platform: plugging a gamepad in takes a moment, which no longer
  blocks the event loop. Its errors carry a `code`.
- **Node 22 or later** (`engines`); Electron 21 or later.
- `Screen.list()` reports sizes on Windows as Electron does: a size that does not divide by the scale is rounded up
  (1024 px at 125% is 820, was 819), and screens of different scales are placed as Electron places them.

### Added

- `Platform`: the running `target`, the `supportedTargets`, `isSupported` and `loadError`; importing no longer
  throws where there is no build - every function then throws (or rejects) with `code:
  "EASYCONTROL_UNSUPPORTED_PLATFORM"`. `hasInputAccess()` and `requestInputAccess()`: macOS's Accessibility
  permission, without which macOS drops all input silently.
- `Keyboard.releaseAll()` and `Mouse.releaseAll()`; keys and buttons still down are also released when the
  process ends.
- `Gamepad.getDriverStatus()`, `Gamepad.installDriver()`, `Gamepad.uninstallDriver()`.
- `Screen.list()` objects have an `id` and a `name`.
- `Keyboard.type(text, { unicodeFallback: true })` for characters a Wayland layout has no key for.
- TypeScript definitions.
- Windows ARM64 builds.

### Fixed

- Linux `Keyboard.type` types characters that need AltGr (`@`, `[`, `{` ... on many European layouts), and finds
  characters the layout holds as legacy keysyms (Hungarian `ő`), which Wayland reported as not on the layout.
- `Mouse.getIcon()` gives the same data on every platform: straight alpha (Linux and macOS gave it premultiplied),
  physical pixels (macOS gave Retina cursors at half size), an empty picture while the pointer is hidden (Windows
  gave the hidden cursor's).
- Linux `Mouse.scrollDown`/`scrollUp` with a negative amount scroll the other way, as on Windows and macOS; a
  non-finite amount throws a `TypeError` everywhere instead of scrolling 0.
- Linux X11: when no output is set as primary (Xvfb, some desktops), the screen at 0,0 is reported primary.
- A build older than its loader is reported in `Platform.loadError` instead of failing on first use.

### Other

- Builds of every target come from CI (`.github/workflows/build.yml`).
- `npm run clean` removes only build output (`-- --all` also `node_modules/` and the WDK cache); it no longer
  deletes `.vscode/` and `package-lock.json`, which is now committed.
- The README documents the platforms' limits (UIPI, the secure desktop, Raw Input, macOS permissions).

# Changelog

## 0.10.0

### Breaking

- **Windows gamepad**: easy-control's own driver replaces ViGEmBus, which was retired in 2023. It is installed once
  per machine with `Gamepad.installDriver()` (one UAC prompt); until then `Gamepad.create()` rejects with
  `code: "EASYCONTROL_DRIVER_MISSING"`. ViGEmBus is no longer used and can be uninstalled. Windows 10 1903 or later,
  x64 and ARM64; on Windows Server editions, which lack Microsoft's `xinputhid` filter, XInput only.
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
- macOS `Keyboard.isKeySupported("KeyA")` was false: the A key's code is 0, which counted as "no key".
- macOS `Mouse.getX`/`getY` right after a move read the previous position; the pointer is now there at once.
- A build older than its loader is reported in `Platform.loadError` instead of failing on first use.

### Other

- Builds of every target come from CI (`.github/workflows/build.yml`).
- `npm run clean` removes only build output (`-- --all` also `node_modules/` and the WDK cache); it no longer
  deletes `.vscode/` and `package-lock.json`, which is now committed.
- The README documents the platforms' limits (UIPI, the secure desktop, Raw Input, macOS permissions).

/*! easy-control v0.11.0 | LGPL-3.0-only | https://github.com/henrikszucs/easy-control */
// Type definitions for easy-control. src/build.js copies them to
// dist/easy-control.d.cts (require) and dist/easy-control.d.mts (import, which
// also has the default export).

/**
 * The `code` of the Errors easy-control throws or rejects with, beside the
 * TypeErrors and RangeErrors of wrong arguments.
 */
export type ErrorCode =
    /** No build for this platform and CPU, or it did not load: see `Platform.loadError`. */
    | "EASYCONTROL_UNSUPPORTED_PLATFORM"
    /** Windows: the gamepad driver is not installed; `Gamepad.installDriver()` installs it. */
    | "EASYCONTROL_DRIVER_MISSING"
    /** Windows: the installed gamepad driver is older than this version needs; `Gamepad.installDriver()` updates it. */
    | "EASYCONTROL_DRIVER_OUTDATED"
    /** Windows: 4 virtual gamepads are plugged in already (XInput's limit). */
    | "EASYCONTROL_NO_SLOT"
    /** Windows: the gamepad service could not be started or did not answer. */
    | "EASYCONTROL_SERVICE_FAILED"
    /** The virtual gamepad could not be made; the message says why. */
    | "EASYCONTROL_CREATE_FAILED"
    /** Windows: the driver files are missing beside the addon (not unpacked from an asar archive?). */
    | "EASYCONTROL_SETUP_MISSING"
    /** Windows: the user declined the UAC prompt of the driver setup. */
    | "EASYCONTROL_SETUP_CANCELLED"
    /** Windows: the driver setup failed; its log is %ProgramData%\easy-control\gamepad-setup.log. */
    | "EASYCONTROL_SETUP_FAILED";

/** An Error with one of easy-control's codes. */
export interface EasyControlError extends Error {
    code: ErrorCode;
}

/** A mouse button. */
export type MouseButton = "left" | "middle" | "right" | "back" | "forward";

/** The pointer's picture. */
export interface MouseIcon {
    /** In physical pixels; 0 while the pointer is hidden. */
    width: number;
    /** In physical pixels; 0 while the pointer is hidden. */
    height: number;
    /**
     * RGBA, 8 bits per channel, rows from the top, straight (not premultiplied) alpha;
     * `width * height * 4` bytes, empty while the pointer is hidden.
     */
    data: Uint8Array;
    /** The hot spot (the pixel that points), from the left, in physical pixels. */
    xOffset: number;
    /** The hot spot (the pixel that points), from the top, in physical pixels. */
    yOffset: number;
}

/**
 * The mouse. Coordinates are logical: on a 125% display, physical pixels divided by 1.25
 * (see the README); `getX`/`getY`, `setX`/`setY`/`setPosition` and `Screen.list()` share them.
 */
export interface Mouse {
    getX(): number;
    getY(): number;
    /** The pointer's picture; empty while it is hidden. */
    getIcon(): MouseIcon;
    /**
     * A number that changes when the pointer's shape does; 0 while it is hidden. Cheap on
     * Windows and Linux (poll it, call `getIcon` when it changes); a hash of the picture on macOS.
     */
    getIconId(): number;
    setX(x: number): void;
    setY(y: number): void;
    /** Moves the pointer in one step. */
    setPosition(x: number, y: number): void;
    buttonDown(button: MouseButton): void;
    buttonUp(button: MouseButton): void;
    /** Releases every button `buttonDown` pressed and `buttonUp` did not release; also done when the process ends. */
    releaseAll(): void;
    /**
     * Scrolls down, or right when `isHorizontal` is true. `amount` is in wheel notches:
     * a fraction is dropped, a negative amount scrolls the other way, 0 does nothing.
     */
    scrollDown(amount: number, isHorizontal: boolean): void;
    /**
     * Scrolls up, or left when `isHorizontal` is true. `amount` is in wheel notches:
     * a fraction is dropped, a negative amount scrolls the other way, 0 does nothing.
     */
    scrollUp(amount: number, isHorizontal: boolean): void;
}

/**
 * A physical key, by its `KeyboardEvent.code` value: `"KeyA"`, `"ShiftLeft"`, `"Numpad5"`, ...
 * (https://developer.mozilla.org/docs/Web/API/UI_Events/Keyboard_event_code_values).
 */
export type KeyCode = string;

/** Options of `Keyboard.type`. */
export interface TypeOptions {
    /** Linux Wayland: enter characters the layout has no key for with Ctrl+Shift+U (GTK, IBus). Default false; ignored elsewhere. */
    unicodeFallback?: boolean;
}

/** The keyboard. */
export interface Keyboard {
    /** Presses a physical key, the same one whatever the layout; the layout decides what it types. Throws when the key is not supported. */
    keyDown(code: KeyCode): void;
    keyUp(code: KeyCode): void;
    /** Releases every key `keyDown` pressed and `keyUp` did not release; also done when the process ends. */
    releaseAll(): void;
    /** Whether this platform can press the key. */
    isKeySupported(code: KeyCode): boolean;
    /**
     * Enters text as it is, whatever the layout. On Linux a character is typed with the key that
     * makes it, Shift and AltGr included; one the layout has no key for goes on a spare key (X11)
     * or, on Wayland, throws once the rest is typed - unless `unicodeFallback` enters it with
     * Ctrl+Shift+U, which GTK and IBus applications understand (others type stray characters).
     */
    type(text: string, options?: TypeOptions): void;
    /**
     * The current layout, as the platform names it: a KLID like `"0000040E"` (Windows),
     * an input source ID like `"com.apple.keylayout.US"` (macOS), an XKB group name (Linux).
     */
    GetLayout(): string;
    /** Switches the layout (throws on Wayland). It changes what keys type, not which keys `keyDown` presses. */
    SetLayout(layout: string): void;
}

/**
 * A virtual gamepad. Buttons and axes follow the W3C Standard Gamepad layout
 * (`navigator.getGamepads()`), so a browser gamepad's indices can be passed as they are.
 */
export declare class VirtualGamepad {
    private constructor();
    /** False once destroyed; its other methods then throw. */
    isActive(): boolean;
    /** Unplugs it. */
    destroy(): void;
    /**
     * Presses a button, 0-16: 0 A, 1 B, 2 X, 3 Y, 4 left shoulder, 5 right shoulder,
     * 6 left trigger (fully), 7 right trigger (fully), 8 back/select, 9 start, 10 left stick,
     * 11 right stick, 12 D-pad up, 13 down, 14 left, 15 right, 16 home/guide.
     */
    buttonDown(button: number): void;
    buttonUp(button: number): void;
    /**
     * Sets an axis, 0-5, to -1..1: 0 left stick x (-1 left), 1 left stick y (-1 up),
     * 2 right stick x, 3 right stick y, 4 left trigger, 5 right trigger (-1 released, 1 pulled).
     */
    setAxis(axis: number, value: number): void;
}

/** The Windows gamepad driver's state; elsewhere there is nothing to install, so it reports one installed. */
export interface DriverStatus {
    isInstalled: boolean;
    /** The installed driver's version; null when none is installed or not on Windows. */
    version: number | null;
    /** The version this easy-control needs; null when not on Windows. */
    required: number | null;
    /** An older driver than `required` is installed; `installDriver()` updates it. */
    isOutdated: boolean;
}

/** Virtual gamepads. */
export interface Gamepad {
    /** The gamepads made and not yet destroyed. */
    list(): VirtualGamepad[];
    /**
     * Plugs in a new virtual gamepad. Rejects with an `EasyControlError` saying why it cannot:
     * on Windows `EASYCONTROL_DRIVER_MISSING` when the driver must be installed first.
     */
    create: (() => Promise<VirtualGamepad>) & { readonly Gamepad: typeof VirtualGamepad };
    getDriverStatus(): DriverStatus;
    /** Windows: installs or updates the driver behind one UAC prompt. Elsewhere resolves at once. */
    installDriver(): Promise<void>;
    /** Windows: removes the driver behind one UAC prompt. Elsewhere resolves at once. */
    uninstallDriver(): Promise<void>;
}

/** A screen, in the same logical coordinates as the mouse. */
export interface ScreenInfo {
    /**
     * Stable while the screen stays connected: the GDI device name (`"\\.\DISPLAY1"`, Windows),
     * the CGDirectDisplayID (macOS, equal to Electron's `display.id` there), the output name (Linux).
     */
    id: string;
    /** The monitor's name for people; `""` when the system has none. */
    name: string;
    isPrimary: boolean;
    x: number;
    y: number;
    width: number;
    height: number;
    /** Physical pixels per logical pixel (1.25 for 125%); always 1 on X11. */
    scaleFactor: number;
}

/** The screens. */
export interface Screen {
    list(): ScreenInfo[];
}

/** The platform easy-control runs on. Works also where there is no build. */
export interface Platform {
    /** `"<platform>-<arch>"` of this process, e.g. `"win32-x64"`. */
    readonly target: string;
    /** The targets easy-control has builds for. */
    readonly supportedTargets: readonly string[];
    /** The build for this target is there and loaded; when false, every other function throws `EASYCONTROL_UNSUPPORTED_PLATFORM`. */
    readonly isSupported: boolean;
    /** Why `isSupported` is false; null when it is true. */
    readonly loadError: string | null;
    /**
     * Whether input sent now reaches applications: macOS needs the Accessibility permission,
     * Wayland a writable /dev/uinput. False where there is no build.
     */
    hasInputAccess(): boolean;
    /**
     * macOS: shows the system prompt pointing to the Accessibility settings, and resolves with the
     * access there is now (false until the user allows it). Elsewhere resolves with `hasInputAccess()`.
     */
    requestInputAccess(): Promise<boolean>;
}

export declare const Mouse: Mouse;
export declare const Keyboard: Keyboard;
export declare const Gamepad: Gamepad;
export declare const Screen: Screen;
export declare const Platform: Platform;

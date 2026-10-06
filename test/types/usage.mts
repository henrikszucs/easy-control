// Type-checked by `npm run test:types` (tsc --noEmit), never run: it uses
// every function through the package name, as an ES module user does, and
// the lines marked @ts-expect-error must stay errors.

import easyControl, { Mouse, Keyboard, Gamepad, Screen, Platform } from "easy-control";
import type { EasyControlError, ErrorCode, MouseIcon, ScreenInfo, DriverStatus, VirtualGamepad } from "easy-control";

const x: number = Mouse.getX();
const y: number = Mouse.getY();
const icon: MouseIcon = Mouse.getIcon();
const pixels: Uint8Array = icon.data;
const iconId: number = Mouse.getIconId();
Mouse.setX(x);
Mouse.setY(y);
Mouse.setPosition(x, y);
Mouse.buttonDown("left");
Mouse.buttonUp("forward");
Mouse.releaseAll();
Mouse.scrollDown(1, false);
Mouse.scrollUp(-2, true);
// @ts-expect-error: not a button
Mouse.buttonDown("wheel");
// @ts-expect-error: both scroll arguments are required
Mouse.scrollDown(1);

Keyboard.keyDown("KeyA");
Keyboard.keyUp("KeyA");
Keyboard.releaseAll();
const isSupported: boolean = Keyboard.isKeySupported("Numpad5");
Keyboard.type("ő€");
Keyboard.type("日本", { unicodeFallback: true });
// @ts-expect-error: no such option
Keyboard.type("x", { fallback: true });
const layout: string = Keyboard.GetLayout();
Keyboard.SetLayout(layout);

const status: DriverStatus = Gamepad.getDriverStatus();
const version: number | null = status.version;
await Gamepad.installDriver();
try {
    const gamepad: VirtualGamepad = await Gamepad.create();
    gamepad.buttonDown(0);
    gamepad.buttonUp(0);
    gamepad.setAxis(4, 1);
    const isActive: boolean = gamepad.isActive();
    gamepad.destroy();
    const isOurs: boolean = gamepad instanceof Gamepad.create.Gamepad;
} catch (error) {
    const code: ErrorCode = (error as EasyControlError).code;
}
const gamepads: VirtualGamepad[] = Gamepad.list();
await Gamepad.uninstallDriver();
// @ts-expect-error: only Gamepad.create() makes one
new Gamepad.create.Gamepad();

const screens: ScreenInfo[] = Screen.list();
const first: { id: string, name: string, scaleFactor: number } = screens[0];

const target: string = Platform.target;
const targets: readonly string[] = Platform.supportedTargets;
const loadError: string | null = Platform.loadError;
if (Platform.isSupported && !Platform.hasInputAccess()) {
    const hasAccess: boolean = await Platform.requestInputAccess();
}
// @ts-expect-error: read-only
Platform.isSupported = true;

const viaDefault: number = easyControl.Mouse.getX();

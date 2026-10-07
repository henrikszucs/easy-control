"use strict";

// Sends no key: a press would go to whatever window has the focus. The
// end-to-end suite (test/e2e) presses keys into a window of its own.

import { test } from "node:test";
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import os from "node:os";

import { Keyboard, inputBlock, isWayland } from "./helpers.js";

const commonKeys = [
    "KeyA", "KeyM", "KeyZ", "Digit0", "Digit5", "Digit9",
    "Enter", "Escape", "Space", "Tab", "Backspace", "Delete",
    "ShiftLeft", "ShiftRight", "ControlLeft", "ControlRight", "AltLeft", "AltRight", "MetaLeft",
    "ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight", "Home", "End", "PageUp", "PageDown",
    "F1", "F12", "Minus", "Equal", "BracketLeft", "Semicolon", "Comma", "Period", "Slash",
    "Numpad0", "NumpadAdd"
];

test("isKeySupported knows the common KeyboardEvent.code values", function() {
    const missing = commonKeys.filter(function(key) {
        return Keyboard.isKeySupported(key) !== true;
    });
    assert.deepEqual(missing, []);
});

test("isKeySupported is false for names that are no key code", function() {
    for (const key of ["NotAKey", "a", "A", "keya", "Key", "Shift"]) {
        assert.equal(Keyboard.isKeySupported(key), false, key);
    }
});

test("isKeySupported rejects a missing, non-string or empty argument", function() {
    assert.throws(function() { Keyboard.isKeySupported(); }, { "name": "TypeError", "message": "Expected 1 argument" });
    assert.throws(function() { Keyboard.isKeySupported(65); }, { "name": "TypeError", "message": "Argument 1 must be a string" });
    assert.throws(function() { Keyboard.isKeySupported(""); }, { "name": "TypeError", "message": "Argument 1 must not be empty" });
});

test("keyDown and keyUp throw for an unsupported key, without sending anything", function() {
    for (const fn of [Keyboard.keyDown, Keyboard.keyUp]) {
        assert.throws(function() { fn("NotAKey"); }, { "message": "Key not supported" });
        assert.throws(function() { fn(); }, TypeError);
        assert.throws(function() { fn(65); }, TypeError);
        assert.throws(function() { fn(""); }, TypeError);
    }
});

test("type rejects a missing or non-string argument, and an empty one types nothing", function() {
    assert.throws(function() { Keyboard.type(); }, TypeError);
    assert.throws(function() { Keyboard.type(1); }, TypeError);
    assert.equal(Keyboard.type(""), undefined);
});

test("getLockState tells which lock keys are on", function() {
    const state = Keyboard.getLockState();
    assert.deepEqual(Object.keys(state).sort(), ["capsLock", "numLock", "scrollLock"]);
    for (const key of Object.keys(state)) {
        assert.equal(typeof state[key], "boolean", key);
    }
    if (os.platform() === "darwin") {
        assert.equal(state["numLock"], false);
        assert.equal(state["scrollLock"], false);
    }
});

test("getLayout and setLayout are GetLayout and SetLayout", function() {
    assert.equal(Keyboard.getLayout, Keyboard.GetLayout);
    assert.equal(Keyboard.setLayout, Keyboard.SetLayout);
});

test("GetLayout returns a non-empty layout name", function() {
    const layout = Keyboard.GetLayout();
    assert.equal(typeof layout, "string");
    assert.ok(layout.length > 0);
    if (os.platform() === "win32") {
        // a keyboard layout identifier, e.g. 00000409
        assert.match(layout, /^[0-9A-F]{8}$/i);
    }
});

test("SetLayout rejects bad arguments and unknown layouts", function() {
    assert.throws(function() { Keyboard.SetLayout(); }, TypeError);
    assert.throws(function() { Keyboard.SetLayout(""); }, TypeError);
    assert.throws(function() { Keyboard.SetLayout("no-such-layout-easy-control"); }, Error);
});

// the user's keyboard layouts, as "language:KLID" (Windows)
const installedLayouts = function() {
    return execFileSync("powershell.exe", ["-NoProfile", "-NonInteractive", "-Command",
        "(Get-WinUserLanguageList).InputMethodTips -join ','"], { "encoding": "utf8" }).trim();
};

test("SetLayout on Windows does not install a layout the user does not have", {
    "skip": (os.platform() !== "win32" && "Windows only") ||
        (inputBlock === "secure-desktop" && "no foreground window on the secure desktop: SetLayout throws before looking")
}, function() {
    // a layout Windows has, which is in none of the user's languages
    const before = installedLayouts();
    const installed = before.toUpperCase();
    const known = execFileSync("reg.exe", ["query", "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts"], { "encoding": "utf8" })
        .split(/\r?\n/).map(function(line) {
            return line.trim().split("\\").pop();
        }).filter(function(klid) {
            return /^[0-9A-F]{8}$/i.test(klid) && !installed.includes(":" + klid.toUpperCase());
        });
    assert.ok(known.length > 0, "every layout Windows has is installed");
    assert.throws(function() { Keyboard.SetLayout(known[0]); }, { "message": "Layout not found" });
    assert.equal(installedLayouts(), before, "the user's layouts did not change");
});

test("SetLayout to the current layout keeps it", {
    "skip": (isWayland && "SetLayout is not supported on Wayland") ||
        (inputBlock === "secure-desktop" && "no foreground window on the secure desktop")
}, function() {
    const layout = Keyboard.GetLayout();
    Keyboard.SetLayout(layout);
    assert.equal(Keyboard.GetLayout(), layout);
});

test("SetLayout on the secure desktop throws EASYCONTROL_INPUT_BLOCKED", {
    "skip": !(os.platform() === "win32" && inputBlock === "secure-desktop") && "no secure desktop showing"
}, function() {
    assert.throws(function() { Keyboard.SetLayout(Keyboard.GetLayout()); }, { "code": "EASYCONTROL_INPUT_BLOCKED" });
});

test("SetLayout throws on Wayland", { "skip": !isWayland && "not a Wayland session" }, function() {
    assert.throws(function() { Keyboard.SetLayout(Keyboard.GetLayout()); }, { "message": /not supported on Wayland/ });
});

test("releaseAll with no key down does nothing and throws nothing", function() {
    assert.equal(Keyboard.releaseAll(), undefined);
    assert.equal(Keyboard.releaseAll(), undefined);
});

test("a key that failed to go down is not released later", function() {
    assert.throws(function() { Keyboard.keyDown("NotAKey"); }, { "message": "Key not supported" });
    // releaseAll would throw if it tried to release the unsupported key
    assert.equal(Keyboard.releaseAll(), undefined);
});

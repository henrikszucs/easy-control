"use strict";

// Sends no key: a press would go to whatever window has the focus. The
// end-to-end suite (test/e2e) presses keys into a window of its own.

import { test } from "node:test";
import assert from "node:assert/strict";
import os from "node:os";

import { Keyboard, isWayland } from "./helpers.js";

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
    assert.throws(function() { Keyboard.isKeySupported(); }, TypeError);
    assert.throws(function() { Keyboard.isKeySupported(65); }, TypeError);
    assert.throws(function() { Keyboard.isKeySupported(""); }, { "name": "TypeError", "message": "Expected non empty string" });
});

test("keyDown and keyUp throw for an unsupported key, without sending anything", function() {
    for (const fn of [Keyboard.keyDown, Keyboard.keyUp]) {
        assert.throws(function() { fn("NotAKey"); }, { "message": "Key not supported" });
        assert.throws(function() { fn(); }, TypeError);
        assert.throws(function() { fn(65); }, TypeError);
        assert.throws(function() { fn(""); }, TypeError);
    }
});

test("type rejects a missing, non-string or empty argument", function() {
    assert.throws(function() { Keyboard.type(); }, TypeError);
    assert.throws(function() { Keyboard.type(1); }, TypeError);
    assert.throws(function() { Keyboard.type(""); }, TypeError);
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
    if (os.platform() !== "win32") {
        // Windows loads any well-formed identifier, so only the others refuse
        assert.throws(function() { Keyboard.SetLayout("no-such-layout-easy-control"); }, Error);
    }
});

test("SetLayout to the current layout keeps it", { "skip": isWayland && "SetLayout is not supported on Wayland" }, function() {
    const layout = Keyboard.GetLayout();
    Keyboard.SetLayout(layout);
    assert.equal(Keyboard.GetLayout(), layout);
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

"use strict";

// The lifecycle tests plug in a real virtual gamepad, so they need the
// platform's driver (easy-control's own on Windows, uinput access on Linux, the CoreHID
// entitlement on macOS). Without it they are skipped, and create() is checked
// to say why instead.

import { test, after } from "node:test";
import assert from "node:assert/strict";
import os from "node:os";

import { Gamepad } from "./helpers.js";

const BUTTON_COUNT = 17;
const AXIS_COUNT = 6;

// one probe decides which half of this file runs
let unavailableReason = "";
try {
    (await Gamepad.create()).destroy();
} catch (error) {
    unavailableReason = error.message;
}
const skipNoDriver = unavailableReason !== "" && ("no virtual gamepad: " + unavailableReason);

after(function() {
    for (const gamepad of Gamepad.list()) {
        gamepad.destroy();
    }
});

test("Gamepad.list starts empty", function() {
    assert.deepEqual(Gamepad.list(), []);
});

test("Gamepad.create returns a Promise", function() {
    const created = Gamepad.create();
    assert.ok(created instanceof Promise);
    // whatever it gives, nothing is left plugged in
    return created.then(function(gamepad) { gamepad.destroy(); }, function() {});
});

test("Gamepad.create rejects with an Error that says why", { "skip": !skipNoDriver && "a virtual gamepad can be made here" }, async function() {
    await assert.rejects(Gamepad.create(), function(error) {
        return error instanceof Error && error.message.length > 0 && typeof error.code === "string" && error.code.startsWith("EASYCONTROL_");
    });
    assert.deepEqual(Gamepad.list(), [], "a failed create leaves nothing in the list");
});

test("a gamepad is only made by Gamepad.create", function() {
    assert.throws(function() { new Gamepad.create.Gamepad(); }, { "name": "TypeError", "message": /Gamepad\.create\(\)/ });
});

test("getDriverStatus reports the driver", function() {
    const status = Gamepad.getDriverStatus();
    assert.deepEqual(Object.keys(status).sort(), ["isInstalled", "isOutdated", "required", "version"]);
    assert.equal(typeof status["isInstalled"], "boolean");
    assert.equal(typeof status["isOutdated"], "boolean");
    if (os.platform() === "win32") {
        assert.ok(Number.isInteger(status["required"]) && status["required"] >= 1);
        if (status["isInstalled"]) {
            assert.ok(Number.isInteger(status["version"]));
            assert.equal(status["isOutdated"], status["version"] < status["required"]);
        } else {
            assert.equal(status["version"], null);
            assert.equal(status["isOutdated"], false);
        }
    } else {
        // nothing to install elsewhere
        assert.equal(status["isInstalled"], true);
        assert.equal(status["isOutdated"], false);
    }
});

test("without the driver, create rejects with EASYCONTROL_DRIVER_MISSING", {
    "skip": (os.platform() !== "win32" && "the driver is Windows only") ||
        (Gamepad.getDriverStatus()["isInstalled"] && "the driver is installed")
}, async function() {
    await assert.rejects(Gamepad.create(), { "code": "EASYCONTROL_DRIVER_MISSING", "message": /installDriver/ });
});

test("installDriver and uninstallDriver resolve at once where there is nothing to install", {
    // on Windows they ask for administrator rights, which a test must not
    "skip": os.platform() === "win32" && "would show a UAC prompt"
}, async function() {
    assert.equal(await Gamepad.installDriver(), undefined);
    assert.equal(await Gamepad.uninstallDriver(), undefined);
});

test("create plugs in an active gamepad that list reports", { "skip": skipNoDriver }, async function() {
    const gamepad = await Gamepad.create();
    try {
        assert.equal(gamepad.isActive(), true);
        assert.ok(gamepad instanceof Gamepad.create.Gamepad);
        assert.deepEqual(Gamepad.list(), [gamepad]);
    } finally {
        gamepad.destroy();
    }
});

test("every button and axis takes input", { "skip": skipNoDriver }, async function() {
    const gamepad = await Gamepad.create();
    try {
        for (let btn = 0; btn < BUTTON_COUNT; btn++) {
            gamepad.buttonDown(btn);
            gamepad.buttonUp(btn);
        }
        for (let axis = 0; axis < AXIS_COUNT; axis++) {
            for (const value of [-1, -0.5, 0, 0.5, 1]) {
                gamepad.setAxis(axis, value);
            }
            gamepad.setAxis(axis, axis >= 4 ? -1 : 0);   // triggers rest at -1
        }
    } finally {
        gamepad.destroy();
    }
});

test("buttons and axes reject bad indices and values", { "skip": skipNoDriver }, async function() {
    const gamepad = await Gamepad.create();
    try {
        for (const fn of ["buttonDown", "buttonUp"]) {
            assert.throws(function() { gamepad[fn](); }, TypeError);
            assert.throws(function() { gamepad[fn]("0"); }, TypeError);
            assert.throws(function() { gamepad[fn](-1); }, RangeError);
            assert.throws(function() { gamepad[fn](BUTTON_COUNT); }, { "name": "RangeError", "message": "Button index out of range (0-16)" });
        }
        assert.throws(function() { gamepad.setAxis(); }, TypeError);
        assert.throws(function() { gamepad.setAxis(0); }, TypeError);
        assert.throws(function() { gamepad.setAxis(0, "1"); }, TypeError);
        assert.throws(function() { gamepad.setAxis(-1, 0); }, RangeError);
        assert.throws(function() { gamepad.setAxis(AXIS_COUNT, 0); }, { "name": "RangeError", "message": "Axis index out of range (0-5)" });
        assert.throws(function() { gamepad.setAxis(0, 1.01); }, RangeError);
        assert.throws(function() { gamepad.setAxis(0, -1.01); }, RangeError);
        assert.throws(function() { gamepad.setAxis(0, NaN); }, RangeError);
    } finally {
        gamepad.destroy();
    }
});

test("several gamepads can be plugged in at once", { "skip": skipNoDriver }, async function() {
    const [first, second] = await Promise.all([Gamepad.create(), Gamepad.create()]);
    try {
        assert.notEqual(first, second);
        assert.equal(Gamepad.list().length, 2);
        assert.ok(Gamepad.list().includes(first) && Gamepad.list().includes(second));
        first.destroy();
        assert.deepEqual(Gamepad.list(), [second]);
        assert.equal(second.isActive(), true, "destroying one leaves the other active");
    } finally {
        first.destroy();
        second.destroy();
    }
});

test("destroy unplugs: inactive, out of the list, and its methods throw", { "skip": skipNoDriver }, async function() {
    const gamepad = await Gamepad.create();
    gamepad.destroy();
    assert.equal(gamepad.isActive(), false);
    assert.deepEqual(Gamepad.list(), []);
    assert.throws(function() { gamepad.buttonDown(0); }, { "message": "Gamepad is not active" });
    assert.throws(function() { gamepad.buttonUp(0); }, { "message": "Gamepad is not active" });
    assert.throws(function() { gamepad.setAxis(0, 0); }, { "message": "Gamepad is not active" });
    gamepad.destroy();   // a second destroy is harmless
    assert.equal(gamepad.isActive(), false);
});

"use strict";

// Moves the pointer (and puts it back), but never clicks or scrolls: those
// would land on whatever window is under it. The end-to-end suite
// (test/e2e) checks clicks and scrolling against a window of its own.

import { test, after } from "node:test";
import assert from "node:assert/strict";

import { Mouse, Screen, pixelTolerance } from "./helpers.js";

const startX = Mouse.getX();
const startY = Mouse.getY();
after(function() {
    Mouse.setPosition(startX, startY);
});

const assertNear = function(actual, expected, tolerance, message) {
    assert.ok(Math.abs(actual - expected) <= tolerance, message + ": " + actual + " is not within " + tolerance + " of " + expected);
};

test("getX and getY return finite numbers", function() {
    assert.ok(Number.isFinite(Mouse.getX()));
    assert.ok(Number.isFinite(Mouse.getY()));
});

test("a position read can be set back unchanged", function() {
    const x = Mouse.getX();
    const y = Mouse.getY();
    Mouse.setPosition(x, y);
    assertNear(Mouse.getX(), x, 0.01, "x");
    assertNear(Mouse.getY(), y, 0.01, "y");
});

test("setPosition reaches points on every screen", function() {
    const screens = Screen.list();
    const tolerance = pixelTolerance(screens);
    for (const s of screens) {
        const points = [
            [s["x"] + s["width"] / 2, s["y"] + s["height"] / 2],    // centre
            [s["x"] + 10, s["y"] + 10],                             // near top left
            [s["x"] + s["width"] - 10, s["y"] + s["height"] - 10]   // near bottom right
        ];
        for (const [x, y] of points) {
            Mouse.setPosition(x, y);
            assertNear(Mouse.getX(), x, tolerance, "x on screen at " + s["x"] + "," + s["y"]);
            assertNear(Mouse.getY(), y, tolerance, "y on screen at " + s["x"] + "," + s["y"]);
        }
    }
});

test("setX moves only horizontally and setY only vertically", function() {
    const primary = Screen.list().find(function(s) {
        return s["isPrimary"];
    });
    const tolerance = pixelTolerance([primary]);
    const x0 = primary["x"] + 100;
    const y0 = primary["y"] + 100;
    Mouse.setPosition(x0, y0);

    Mouse.setX(x0 + 50);
    assertNear(Mouse.getX(), x0 + 50, tolerance, "setX moved x");
    assertNear(Mouse.getY(), y0, tolerance, "setX kept y");

    Mouse.setY(y0 + 40);
    assertNear(Mouse.getX(), x0 + 50, tolerance, "setY kept x");
    assertNear(Mouse.getY(), y0 + 40, tolerance, "setY moved y");
});

test("getIcon returns an RGBA picture of the documented shape", function() {
    const icon = Mouse.getIcon();
    assert.deepEqual(Object.keys(icon).sort(), ["data", "height", "width", "xOffset", "yOffset"]);
    assert.ok(icon["data"] instanceof Uint8Array);
    assert.ok(Number.isInteger(icon["width"]) && icon["width"] >= 0);
    assert.ok(Number.isInteger(icon["height"]) && icon["height"] >= 0);
    assert.equal(icon["data"].length, icon["width"] * icon["height"] * 4);
    if (icon["width"] > 0) {
        // the hot spot lies inside the picture
        assert.ok(icon["xOffset"] >= 0 && icon["xOffset"] < icon["width"], "xOffset inside");
        assert.ok(icon["yOffset"] >= 0 && icon["yOffset"] < icon["height"], "yOffset inside");
        // a pointer picture is never fully transparent
        let isVisible = false;
        for (let i = 3; i < icon["data"].length; i += 4) {
            if (icon["data"][i] !== 0) {
                isVisible = true;
                break;
            }
        }
        assert.ok(isVisible, "some pixel has alpha");
    }
});

test("getIconId is a non-negative integer, stable while the shape does not change", function() {
    const id = Mouse.getIconId();
    assert.ok(Number.isInteger(id) && id >= 0);
    assert.equal(Mouse.getIconId(), id);
});

test("setPosition, setX and setY reject bad coordinates", function() {
    assert.throws(function() { Mouse.setPosition(); }, TypeError);
    assert.throws(function() { Mouse.setPosition(10); }, TypeError);
    assert.throws(function() { Mouse.setPosition("10", 10); }, TypeError);
    assert.throws(function() { Mouse.setPosition(10, NaN); }, { "name": "TypeError", "message": "Expected finite number argument" });
    assert.throws(function() { Mouse.setPosition(Infinity, 10); }, TypeError);
    assert.throws(function() { Mouse.setX(); }, TypeError);
    assert.throws(function() { Mouse.setX(null); }, TypeError);
    assert.throws(function() { Mouse.setY(); }, TypeError);
    assert.throws(function() { Mouse.setY(-Infinity); }, TypeError);
});

test("buttonDown and buttonUp reject unknown buttons", function() {
    for (const fn of [Mouse.buttonDown, Mouse.buttonUp]) {
        assert.throws(function() { fn(); }, { "name": "TypeError", "message": "Expected 1 argument" });
        assert.throws(function() { fn(0); }, { "name": "TypeError", "message": "Expected string argument" });
        assert.throws(function() { fn("LEFT"); }, TypeError);
        assert.throws(function() { fn("wheel"); }, TypeError);
    }
});

test("scrollDown and scrollUp need an amount and a direction flag", function() {
    for (const fn of [Mouse.scrollDown, Mouse.scrollUp]) {
        assert.throws(function() { fn(); }, TypeError);
        assert.throws(function() { fn(1); }, TypeError);
        assert.throws(function() { fn("1", false); }, TypeError);
        assert.throws(function() { fn(1, "no"); }, TypeError);
    }
});

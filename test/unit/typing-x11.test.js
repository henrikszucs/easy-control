"use strict";

// Keyboard.type on X11 with a layout whose characters need AltGr (Hungarian):
// a window of x11-key-window.c reports the keysyms it gets. It types for real
// into the focused window, so it runs only where EASYCONTROL_TYPING_TEST=1
// says that is fine - CI sets it under Xvfb.

import { test, before, after } from "node:test";
import assert from "node:assert/strict";
import { execFileSync, spawn } from "node:child_process";
import os from "node:os";
import path from "node:path";

import { Keyboard, isX11 } from "./helpers.js";

const hasTool = function(command, args) {
    try {
        execFileSync(command, args, { "stdio": "ignore" });
        return true;
    } catch {
        return false;
    }
};

const skip = (!isX11 && "X11 only") ||
    (process.env["EASYCONTROL_TYPING_TEST"] !== "1" && "types for real; set EASYCONTROL_TYPING_TEST=1 to run it (CI does, under Xvfb)") ||
    (!hasTool("cc", ["--version"]) && "no C compiler") ||
    (!hasTool("setxkbmap", ["-query"]) && "no setxkbmap");

const windowExe = path.join(os.tmpdir(), "easy-control-x11-key-window");
let previousLayout = "us";

before(function() {
    if (skip) {
        return;
    }
    execFileSync("cc", ["-o", windowExe, path.join(import.meta.dirname, "x11-key-window.c"), "-lX11"]);
    const query = execFileSync("setxkbmap", ["-query"], { "encoding": "utf8" });
    previousLayout = (query.match(/^layout:\s*(\S+)/m) || [null, "us"])[1];
    execFileSync("setxkbmap", ["hu"]);
});

after(function() {
    if (!skip) {
        execFileSync("setxkbmap", [previousLayout]);
    }
});

// the keysyms the window gets while `type` runs
const typeIntoWindow = async function(text) {
    const window = spawn(windowExe, [], { "stdio": ["ignore", "pipe", "inherit"] });
    let output = "";
    window.stdout.on("data", function(chunk) {
        output += chunk;
    });
    await new Promise(function(resolve, reject) {
        const timer = setTimeout(function() { reject(new Error("the key window did not start")); }, 5000);
        window.stdout.on("data", function() {
            if (output.includes("ready\n")) {
                clearTimeout(timer);
                resolve();
            }
        });
    });
    Keyboard.type(text);
    await new Promise(function(resolve) {
        window.on("exit", resolve);
    });
    return output.split("\n").slice(1).filter(Boolean).map(function(hex) {
        return parseInt(hex, 16);
    });
};

test("type: plain, Shift and AltGr characters of a Hungarian layout", { skip, "timeout": 20000 }, async function() {
    // @ [ ] { } \ need AltGr on it, ő and é are keys of their own, Q needs Shift
    const keysyms = await typeIntoWindow("@[]{}\\őéQz");
    assert.deepEqual(keysyms.map(function(keysym) { return keysym.toString(16); }),
        [0x40, 0x5b, 0x5d, 0x7b, 0x7d, 0x5c, 0x1f5, 0xe9, 0x51, 0x7a].map(function(keysym) { return keysym.toString(16); }));
});

test("type: a character on no key of the layout comes through a borrowed key", { skip, "timeout": 20000 }, async function() {
    // the Unicode keysym of 日 (U+65E5), put on a spare keycode for the call
    const keysyms = await typeIntoWindow("日");
    assert.deepEqual(keysyms, [0x01000000 | 0x65e5]);
});

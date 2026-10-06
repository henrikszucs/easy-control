"use strict";

// A virtual gamepad on Linux gets a game's rumble: ff-client.c uploads and
// plays force feedback effects on its event device, as a game does, and
// onRumble reports them. It needs uinput and read-write access to the pad's
// event device, so it runs only where EASYCONTROL_RUMBLE_TEST=1 says those
// are set up - CI does (.github/workflows/build.yml).

import { test, before } from "node:test";
import assert from "node:assert/strict";
import { execFile, execFileSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";

import { Gamepad } from "./helpers.js";

const hasCompiler = function() {
    try {
        execFileSync("cc", ["--version"], { "stdio": "ignore" });
        return true;
    } catch {
        return false;
    }
};

const skip = (os.platform() !== "linux" && "Linux only") ||
    (process.env["EASYCONTROL_RUMBLE_TEST"] !== "1" && "needs uinput and the pad's event device; set EASYCONTROL_RUMBLE_TEST=1 to run it (CI does)") ||
    (!hasCompiler() && "no C compiler");

const clientExe = path.join(os.tmpdir(), "easy-control-ff-client");

before(function() {
    if (!skip) {
        execFileSync("cc", ["-o", clientExe, path.join(import.meta.dirname, "ff-client.c")]);
    }
});

// the event device of the newest virtual pad, once udev has made it usable
const findEventDevice = async function() {
    const end = Date.now() + 5000;
    while (Date.now() < end) {
        for (const entry of fs.readdirSync("/sys/class/input")) {
            if (!entry.startsWith("event")) {
                continue;
            }
            let name = "";
            try {
                name = fs.readFileSync(path.join("/sys/class/input", entry, "device", "name"), "utf8").trim();
            } catch {
                continue;
            }
            const device = path.join("/dev/input", entry);
            if (name === "Virtual Xbox 360 Controller") {
                try {
                    fs.accessSync(device, fs.constants.R_OK | fs.constants.W_OK);
                    return device;
                } catch {
                    // udev has not set its access yet
                }
            }
        }
        await new Promise(function(resolve) { setTimeout(resolve, 50); });
    }
    throw new Error("No usable event device of the virtual gamepad");
};

test("onRumble reports a game's rumble effects, their end by length and their stop", { skip }, async function() {
    const gamepad = await Gamepad.create();
    try {
        const rumbles = [];
        gamepad.onRumble = function(rumble) {
            rumbles.push(rumble);
        };
        const device = await findEventDevice();
        // the client runs apart, as a game would: its uploads wait for the pad's answers
        await new Promise(function(resolve, reject) {
            execFile(clientExe, [device], function(error, stdout, stderr) {
                error ? reject(new Error(stderr || error.message)) : resolve();
            });
        });
        await new Promise(function(resolve) { setTimeout(resolve, 100); });
        const expected = [
            { "strong": 0xC000 / 65535, "weak": 0x4000 / 65535 },
            { "strong": 0, "weak": 0 },     // ended by its length
            { "strong": 1, "weak": 1 },
            { "strong": 0, "weak": 0 }      // stopped
        ];
        assert.equal(rumbles.length, expected.length, JSON.stringify(rumbles));
        rumbles.forEach(function(rumble, i) {
            assert.ok(Math.abs(rumble["strong"] - expected[i]["strong"]) < 1e-6 && Math.abs(rumble["weak"] - expected[i]["weak"]) < 1e-6,
                "rumble " + i + ": " + JSON.stringify(rumble) + ", expected " + JSON.stringify(expected[i]));
        });
    } finally {
        gamepad.destroy();
    }
});

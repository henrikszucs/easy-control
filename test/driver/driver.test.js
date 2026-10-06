"use strict";

// The Windows gamepad driver's install and uninstall, for real:
//
//     npm run test:driver
//
// Each install and uninstall asks for administrator rights, so this shows
// UAC prompts to approve (two, or three when the driver was not installed
// before). It leaves the driver as it found it. Not part of `npm test`.

import { test, before, after } from "node:test";
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";

import { Gamepad, distPath, platformDir } from "../unit/helpers.js";

const isWindows = os.platform() === "win32";
const skip = !isWindows && "the driver is Windows only";

const SERVICE_NAME = "EasyControlGamepad";
const INSTALL_DIR = path.join(process.env["ProgramFiles"] || "C:\\Program Files", "easy-control", "gamepad");
const SETUP_SCRIPT = path.join(distPath, platformDir, "gamepad", "easy-control-gamepad-setup.ps1");

// a UAC prompt waits for a person
const UAC_TIMEOUT = 5 * 60 * 1000;

const powershell = function(script) {
    return execFileSync("powershell.exe", ["-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-Command", script],
        { "encoding": "utf8" }).trim();
};

// what the setup script finds: packages in the driver store, certificates
const setupStatus = function() {
    return JSON.parse(execFileSync("powershell.exe",
        ["-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-File", SETUP_SCRIPT, "status"], { "encoding": "utf8" }));
};

const serviceExists = function(name) {
    return powershell("[bool](Get-Service -Name '" + name + "' -ErrorAction SilentlyContinue)") === "True";
};

// every device node of ours Windows knows, plugged in or remembered
const deviceNodes = function() {
    const ids = powershell("Get-PnpDevice -ErrorAction SilentlyContinue | Where-Object { $_.InstanceId -like 'SWD\\EASYCONTROL*' } | " +
        "ForEach-Object { $_.InstanceId }");
    return ids ? ids.split(/\r?\n/) : [];
};

const assertInstalled = function() {
    const status = Gamepad.getDriverStatus();
    assert.equal(status["isInstalled"], true);
    assert.equal(status["version"], status["required"]);
    assert.equal(serviceExists(SERVICE_NAME), true, "the service is registered");
    const setup = setupStatus();
    assert.equal(setup["driverPackages"].length, 2, "the HID and the XUSB driver packages: " + setup["driverPackages"]);
    // the certificate is trusted, and its private key is gone with it from the personal store
    const stores = setup["certificates"].map(function(entry) {
        return entry.split(":")[0];
    }).sort();
    assert.deepEqual(stores, ["Root", "TrustedPublisher"], "one certificate, trusted: " + setup["certificates"]);
    for (const file of ["easy-control-gamepad-service.exe", "driver\\easycontrol_gamepad.dll"]) {
        assert.ok(fs.existsSync(path.join(INSTALL_DIR, file)), file + " in Program Files");
    }
};

let wasInstalled = false;

before(async function() {
    if (!isWindows) {
        return;
    }
    wasInstalled = Gamepad.getDriverStatus()["isInstalled"];
    if (!wasInstalled) {
        // something to remove first
        await Gamepad.installDriver();
    }
});

after(async function() {
    // as it was found
    if (isWindows && !wasInstalled && Gamepad.getDriverStatus()["isInstalled"]) {
        await Gamepad.uninstallDriver();
    }
});

test("uninstallDriver removes the driver, its service, certificate, files and devices", { skip, "timeout": UAC_TIMEOUT }, async function() {
    // a pad plugged in while it goes
    const gamepad = Gamepad.create();
    assert.equal(gamepad.isActive(), true);
    gamepad.destroy();

    assert.equal(await Gamepad.uninstallDriver(), undefined);

    const status = Gamepad.getDriverStatus();
    assert.equal(status["isInstalled"], false);
    assert.equal(status["version"], null);
    assert.equal(status["isOutdated"], false);
    assert.equal(serviceExists(SERVICE_NAME), false, "the service is gone");
    const setup = setupStatus();
    assert.deepEqual(setup["driverPackages"], [], "no driver package left in the driver store");
    assert.deepEqual(setup["certificates"], [], "no certificate left in any store");
    assert.equal(fs.existsSync(INSTALL_DIR), false, "Program Files folder is gone");
    assert.deepEqual(deviceNodes(), [], "Windows remembers no device of ours");
    // Windows' own filter driver stays
    assert.equal(serviceExists("xinputhid"), true, "xinputhid is left in place");

    assert.throws(function() { Gamepad.create(); }, { "code": "EASYCONTROL_DRIVER_MISSING" });
});

test("installDriver puts it back, and a gamepad works again", { skip, "timeout": UAC_TIMEOUT }, async function() {
    assert.equal(await Gamepad.installDriver(), undefined);
    assertInstalled();

    const gamepad = Gamepad.create();
    try {
        assert.equal(gamepad.isActive(), true);
        gamepad.buttonDown(0);
        gamepad.setAxis(0, 0.5);
    } finally {
        gamepad.destroy();
    }
});

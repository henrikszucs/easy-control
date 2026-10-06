"use strict";

import { test } from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs/promises";
import path from "node:path";
import { createRequire } from "node:module";

import { Control, distPath, platformDir, rootPath } from "./helpers.js";

const require = createRequire(import.meta.url);
const pkg = JSON.parse(await fs.readFile(path.join(rootPath, "package.json"), "utf8"));

test("dist holds the native addon for the running platform", async function() {
    const stat = await fs.stat(path.join(distPath, platformDir, "easy-control.node"));
    assert.ok(stat.size > 0);
});

test("dist loaders carry the version banner", async function() {
    const banner = "/*! " + pkg["name"] + " v" + pkg["version"] + " | " + pkg["license"];
    for (const file of ["easy-control.cjs", "easy-control.mjs"]) {
        const code = await fs.readFile(path.join(distPath, file), "utf8");
        assert.ok(code.startsWith(banner), file + " starts with " + banner);
    }
});

test("CommonJS loader exports Mouse, Keyboard, Gamepad and Screen", function() {
    assert.deepEqual(Object.keys(Control).sort(), ["Gamepad", "Keyboard", "Mouse", "Screen"]);
});

test("ES module loader has the same objects as named and default exports", async function() {
    const esm = await import("../../dist/easy-control.mjs");
    assert.equal(esm.default, Control);
    for (const name of ["Mouse", "Keyboard", "Gamepad", "Screen"]) {
        assert.equal(esm[name], Control[name], name);
    }
});

test("package exports resolve to the dist loaders", function() {
    assert.equal(pkg["exports"]["."]["require"], "./dist/easy-control.cjs");
    assert.equal(pkg["exports"]["."]["import"], "./dist/easy-control.mjs");
    assert.equal(require(path.join(rootPath, pkg["main"])), Control);
});

test("the addon can be required directly by absolute path (Electron use)", function() {
    const direct = require(path.join(distPath, platformDir, "easy-control.node"));
    assert.equal(direct, Control);
});

test("every API member is a function", function() {
    const api = {
        "Mouse": ["getX", "getY", "getIcon", "getIconId", "setX", "setY", "setPosition", "buttonDown", "buttonUp", "scrollDown", "scrollUp"],
        "Keyboard": ["keyDown", "keyUp", "isKeySupported", "type", "GetLayout", "SetLayout"],
        "Gamepad": ["list", "create", "getDriverStatus", "installDriver", "uninstallDriver"],
        "Screen": ["list"]
    };
    for (const [object, members] of Object.entries(api)) {
        for (const member of members) {
            assert.equal(typeof Control[object][member], "function", object + "." + member);
        }
    }
});

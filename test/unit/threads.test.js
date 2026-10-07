"use strict";

// The addon from several worker threads at once: the X11 connection (Linux)
// and the shared state (macOS) are used under locks, so calls from four
// threads at once neither crash nor make Xlib report errors. Sends no input.

import { test } from "node:test";
import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import os from "node:os";
import path from "node:path";

import { distPath, inputBlock } from "./helpers.js";

test("four workers reading the pointer, its icon, the layout and the screens at once", {
    "skip": os.platform() === "win32" && inputBlock === "secure-desktop" && "the pointer cannot be read on the secure desktop",
    "timeout": 120000
}, function() {
    // in a child process, whose stderr shows what Xlib reports
    const script = [
        "const { Worker } = require('node:worker_threads');",
        "const loader = " + JSON.stringify(path.join(distPath, "easy-control.cjs")) + ";",
        "const calls = ['Mouse.getX()', 'Mouse.getIcon()', 'Keyboard.getLayout()', 'Screen.list()'];",
        "Promise.all(calls.map(function(call) {",
        "    const worker = new Worker([",
        "        \"const { parentPort } = require('node:worker_threads');\",",
        "        \"const { Mouse, Keyboard, Screen } = require(\" + JSON.stringify(loader) + \");\",",
        "        \"for (let i = 0; i < 2000; i++) { \" + call + \"; }\",",
        "        \"parentPort.postMessage('done');\"",
        "    ].join('\\n'), { eval: true });",
        "    return new Promise(function(resolve, reject) {",
        "        worker.once('message', resolve);",
        "        worker.once('error', reject);",
        "    });",
        "})).then(function(results) {",
        "    console.log(JSON.stringify(results));",
        "}, function(error) {",
        "    console.error('worker failed: ' + error.message);",
        "    process.exit(1);",
        "});"
    ].join("\n");
    const result = spawnSync(process.execPath, ["-e", script], { "encoding": "utf8", "timeout": 110000 });
    assert.equal(result.stderr, "", "nothing on stderr (Xlib reports its errors there)");
    assert.equal(result.status, 0, "the process ended normally (signal " + result.signal + ")");
    assert.deepEqual(JSON.parse(result.stdout), ["done", "done", "done", "done"]);
});

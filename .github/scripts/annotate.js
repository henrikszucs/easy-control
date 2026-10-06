"use strict";

// Puts a failed step's output into an error annotation, which the run's page
// shows and the GitHub API gives without signing in (unlike the job logs):
//
//     node .github/scripts/annotate.js "<title>" <log file>...
//
// Test logs come first and every file gets its share - the lines that look
// like errors and its last lines - so a long build log cannot push the test
// failures out of the annotation.

import fs from "node:fs";
import path from "node:path";

const ERROR_LINES = 30;
const LAST_LINES = 20;
const MAX_LENGTH = 60000;

const isError = function(line) {
    return /\berror\b|error:|fatal|failed|not ok|✖|AssertionError|Exception|Cannot |undefined reference/i.test(line);
};

// test logs, then what the failure step collected, then build logs
const rank = function(file) {
    const name = path.basename(file);
    return /test/.test(name) ? 0 : /build/.test(name) ? 2 : 1;
};

const [title, ...files] = process.argv.slice(2);
const parts = [];
for (const file of [...files].sort(function(a, b) { return rank(a) - rank(b); })) {
    let lines;
    try {
        lines = fs.readFileSync(file, "utf8").split(/\r?\n/);
    } catch {
        parts.push("== " + path.basename(file) + ": missing");
        continue;
    }
    const errors = lines.filter(isError).slice(0, ERROR_LINES);
    const last = lines.filter(Boolean).slice(-LAST_LINES);
    parts.push("== " + path.basename(file), ...errors, "-- last lines:", ...last);
}

// workflow commands end at a line break, so line breaks are escaped
const escape = function(text) {
    return text.replaceAll("%", "%25").replaceAll("\r", "%0D").replaceAll("\n", "%0A");
};
const message = parts.join("\n").slice(0, MAX_LENGTH);
console.log("::error title=" + escape(title).replaceAll(",", "%2C").replaceAll(":", "%3A") + "::" + escape(message));

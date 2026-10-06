"use strict";

// Puts a failed step's output into an error annotation, which the run's page
// shows and the GitHub API gives without signing in (unlike the job logs):
//
//     node .github/scripts/annotate.js "<title>" <log file>...
//
// Of each file: the lines that look like errors and the last lines, as much as
// fits one annotation.

import fs from "node:fs";

const MAX_LINES = 120;
const LAST_LINES = 40;

const [title, ...files] = process.argv.slice(2);
const parts = [];
for (const file of files) {
    let lines;
    try {
        lines = fs.readFileSync(file, "utf8").split(/\r?\n/);
    } catch {
        parts.push("== " + file + ": missing");
        continue;
    }
    const isError = function(line) {
        return /\berror\b|error:|fatal|failed|not ok|✖|AssertionError|Exception|Cannot |undefined reference/i.test(line);
    };
    const errors = lines.filter(isError).slice(0, MAX_LINES - LAST_LINES);
    const last = lines.filter(Boolean).slice(-LAST_LINES);
    parts.push("== " + file, ...errors, "-- last lines:", ...last);
}

// workflow commands end at a line break, so line breaks are escaped
const escape = function(text) {
    return text.replaceAll("%", "%25").replaceAll("\r", "%0D").replaceAll("\n", "%0A");
};
const message = parts.join("\n").slice(0, 60000);
console.log("::error title=" + escape(title).replaceAll(",", "%2C").replaceAll(":", "%3A") + "::" + escape(message));

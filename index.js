"use strict";

// npm run build                      the addon for this machine, then the JS loaders
// npm run build -- --arch arm64      the addon for another CPU (cross-compiled)
// npm run clean                      removes the build output
// npm run clean -- --all             also node_modules/ and the WDK download cache

import os from "node:os";
import process from "node:process";
import { spawn } from "node:child_process";
import { createRequire } from "node:module";
import fs from "node:fs/promises";

const require = createRequire(import.meta.url);


// search in parameters
const getArg = function(args, argName, isKeyValue=false, isInline=false) {
    for (let i = 0, length=args.length; i < length; i++) {
        const arg = args[i];
        if (isKeyValue) {
            if (isInline) {
                if (arg.startsWith(argName + "=")) {
                    return arg.slice(argName.length + 1);
                }
            } else {
                if (arg === argName) {
                    return args[i + 1];
                }
            }
        } else {
            if (arg === argName) {
                return true;
            }
        }
    }
    return undefined;
};


// build function; arch is the CPU to build for, the running one unless
// given (npm run build -- --arch arm64 builds the Windows ARM64 addon on x64)
const build = async (arch) => {
    const distDir = "./dist/" + os.platform() + "-" + arch + "/";

    // run node-gyp, the project's own, through this Node: no shell, and no
    // node-gyp needed on the PATH
    // nothing of a build for another CPU may be reused (on macOS the Swift
    // part is built into build_swift/)
    await fs.rm("./build/", { "recursive": true, "force": true });
    await fs.rm("./build_swift/", { "recursive": true, "force": true });
    const nodeGyp = require.resolve("node-gyp/bin/node-gyp.js");
    const ls = spawn(process.execPath, [nodeGyp, "configure", "build", "--arch=" + arch], {
        "cwd": process.cwd(),
        "stdio": "inherit"
    });
    let code = await new Promise((resolve) => {
        ls.on("close", (code) => {
            console.log(`Building end with: ${code}`);
            resolve(code);
        });
    });

    if (code !== 0) {
        throw new Error("Build process failed");
    }

    // copy built files
    process.stdout.write("Copying built files...   ");
    await fs.mkdir(distDir, { "recursive": true });

    if (os.platform() === "win32") {
        await fs.copyFile("./build/Release/easy-control.node", distDir + "easy-control.node");
        // the gamepad uses easy-control's own driver now (npm run build:gamepad
        // builds it into dist/<platform>/gamepad); drop the ViGEm client
        // earlier builds shipped beside the addon
        for (const stale of ["ViGEmClient.dll", "ViGEmClient.lib", "ViGEmClient.LICENSE"]) {
            await fs.rm(distDir + stale, { "force": true });
        }
    } else if (os.platform() === "darwin") {
        await fs.copyFile("./build/Release/easy-control.node", distDir + "easy-control.node");
        // the Swift code is linked into the .node now; drop the libraries
        // earlier builds needed beside it
        for (const stale of ["GamepadImplement.a", "nothing.a"]) {
            await fs.rm(distDir + stale, { "force": true });
        }
    } else if (os.platform() === "linux") {
        await fs.copyFile("./build/Release/easy-control.node", distDir + "easy-control.node");
    }
    process.stdout.write("done\n");

    // minify the JS loaders into dist
    await import("./src/build.js");
};


// clean function: the build output only; with all, also what npm install
// and the gamepad build downloaded
const clean = async (isAll) => {
    process.stdout.write("Removing build output... ");
    const pathList = [
        "./build",
        "./build_swift",
        "./tmp"
    ];
    if (isAll) {
        pathList.push("./node_modules", "./build_wdk");
    }
    for (const dir of pathList) {
        try {
            await fs.rm(dir, { "recursive": true, "force": true });
        } catch (error) {
            console.error(`Error removing ${dir}:`, error);
        }
    }
    process.stdout.write("done\n");
};


// start main function
const main = async () => {
    if (getArg(process.argv, "--clean", false)) {
        await clean(getArg(process.argv, "--all", false) || false);
    } else {
        await build(getArg(process.argv, "--arch", true) || os.arch());
    }
};
main();

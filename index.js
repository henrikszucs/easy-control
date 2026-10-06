"use strict";

import os from "node:os";
import process from "node:process";
import { spawn } from "node:child_process";
import fs from "node:fs/promises";
import path from "node:path";


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

    // run node-gyp
    await fs.rm("./build/", { "recursive": true, "force": true });  //for safety
    const ls = spawn("node-gyp", ["configure", "build", "--arch=" + arch], {
        "cwd": process.cwd(),
        "shell": true,
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
    //await fs.rm("./dist/", { "recursive": true, "force": true });   // for dev
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

    // test environment copy
    /*
    const distSrc = "./dist/";
    const distDest = "./dev/test/resources/app/dist/";
    await fs.mkdir(distDest, { "recursive": true });
    const distFiles = await fs.readdir(distSrc, {"recursive": true});
    for (const file of distFiles) {
        const fileSrc = path.join(distSrc, file);
        const fileDest = path.join(distDest, file);
        const isDir = (await fs.stat(fileSrc)).isDirectory();
        if (isDir) {
            await fs.mkdir(fileDest, {"recursive": true});
        } else {
            await fs.cp(fileSrc, fileDest), { "recursive": true };
        }
    }*/
};


// uninstall function
const uninstall = async () => {
    process.stdout.write("Removing built files...  ");
    const pathList = [
        "./package-lock.json",
        "./node_modules",
        "./build",
        "./.vscode",
        "./tmp",
        "./build_swift"
    ];
    for (const dir of pathList) {
        try {
            await fs.rm(dir, { "recursive": true, "force": true });
        } catch (error) {
            console.error(`Error removing ${dir}:`, error);
        }
    }

    /*
    const distSrc = "./dev/test";
    const distFiles = await fs.readdir(distSrc);
    for (const file of distFiles) {
        const fileSrc = path.join(distSrc, file);
        const isDir = (await fs.stat(fileSrc)).isDirectory();
        if (!isDir) {
            await fs.rm(fileSrc);
        }
    }*/

    process.stdout.write("done\n");
};


// start main function
const main = async () => {
    const uninstallFlag = getArg(process.argv, "--uninstall", false) || false;
    if (uninstallFlag) {
        await uninstall();
    } else {
        await build(getArg(process.argv, "--arch", true) || os.arch());
    }
};
main();
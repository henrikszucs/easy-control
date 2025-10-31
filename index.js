"use strict";

import os from "node:os";
import process from "node:process";
import { spawn } from "node:child_process";
import fs from "node:fs/promises";


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


// build function
const build = async () => {
    // run node-gyp
    await fs.rm("./build/", { "recursive": true, "force": true });  //for safety
    const ls = spawn("node-gyp", ["configure", "build"], {
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
    process.stdout.write("Coping built files...    ");
    await fs.rm("./dist/", { "recursive": true, "force": true });   // for dev
    await fs.mkdir("./dist/" + os.platform() + "-" + os.arch() + "/", { "recursive": true });

    await fs.copyFile("./src/easy-control.cjs", "./dist/easy-control.cjs");
    
    if (os.platform() === "win32") {
        await fs.copyFile("./build/Release/easy-control.node", "./dist/" + os.platform() + "-" + os.arch() + "/easy-control.node");
        await fs.cp("./src/vjoy_driver/", "./dist/" + os.platform() + "-" + os.arch() + "/vjoy_driver/", { "recursive": true });
    } else if (os.platform() === "darwin") {
        await fs.copyFile("./build/Release/easy-control.node", "./dist/" + os.platform() + "-" + os.arch() + "/easy-control.node");
    } else if (os.platform() === "linux") {
        await fs.copyFile("./build/Release/easy-control.node", "./dist/" + os.platform() + "-" + os.arch() + "/easy-control.node");
    }
    process.stdout.write("done\n");
};


// uninstall function
const uninstall = async () => {
    process.stdout.write("Removing built files...  ");
    const pathList = [
        "./package-lock.json",
        "./node_modules",
        "./build",
        "./.vscode"
    ];
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
    const uninstallFlag = getArg(process.argv, "--uninstall", false) || false;
    if (uninstallFlag) {
        await uninstall();
    } else {
        await build();
    }
};
main();
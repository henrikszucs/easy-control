"use strict";

const path = require("node:path");
const os = require("node:os");
const { ipcRenderer } = require("electron");

ipcRenderer.on("api", function(event, ...args) {
    console.log(args[1]);
});


const main = async function() {
    // load native module
    const appPath = await ipcRenderer.invoke("api", "path-app");
    let packagePath = path.join(appPath, "../../../../dist", os.platform() + "-" + os.arch(), "easy-control.node");
    const Control = require(packagePath);
    console.log(Control);

    document.getElementById("start-btn").addEventListener("click", async function() {
        
        // place Window
        ipcRenderer.send("api", "set-primary-monitor");
        ipcRenderer.send("api", "maximize-window");
        const bounds = await ipcRenderer.invoke("api", "get-content-bounds");
        console.log("Window bounds:", bounds);
        await new Promise(resolve => setTimeout(resolve, 500));

        //test Screen API
        const screenData = Control.Screen.list();
        const screenDataElectron = await ipcRenderer.invoke("api", "list-screens");
        console.log("Screens:", screenData);
        console.log("Screens:", screenDataElectron);


        //test Mouse API



        //test Keyboard API



        //test Controller API
        


    });

};
main();
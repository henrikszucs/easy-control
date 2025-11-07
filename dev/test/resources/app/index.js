"use strict";

const path = require("node:path");
const os = require("node:os");
const { ipcRenderer } = require("electron");
const { log } = require("node:console");

ipcRenderer.on("api", function(event, ...args) {
    console.log(args[1]);
});


const main = async function() {
    // load native module
    const appPath = await ipcRenderer.invoke("api", "path-app");
    let packagePath = path.join(appPath, "../../../../dist", os.platform() + "-" + os.arch(), "easy-control.node");
    globalThis.Control = require(packagePath);
    console.log(Control);

    window.addEventListener("keydown", function(event) {
        console.log("Key down event (window listener):", event.key);
    });
    window.addEventListener("keyup", function(event) {
        console.log("Key up event (window listener):", event.key);
    });
    /*
    setTimeout( function() {Control.Keyboard.keyDown("a");Control.Keyboard.keyUp("a");}, 1000);
    */

    document.getElementById("start-btn").addEventListener("click", async function() {
        const logElem = document.getElementById("log");
        logElem.innerHTML = "";
/*
        // place Window
        ipcRenderer.send("api", "set-primary-monitor");
        ipcRenderer.send("api", "maximize-window");
        const bounds = await ipcRenderer.invoke("api", "get-content-bounds");
        console.log("Window bounds:", bounds);
        await new Promise(resolve => setTimeout(resolve, 500));

        //test Screen API
        const screenData = Control.Screen.list();
        const screenDataElectron = await ipcRenderer.invoke("api", "list-screens");
        
        if (screenData.length !== screenDataElectron.length) {
            console.log("Screens:", screenData);
            console.log("Screens:", screenDataElectron);
            throw new Error("Screen count mismatch!");
        }
        for (let i = 0; i < screenData.length; i++) {
            if (screenDataElectron[i]["bounds"]["width"] !== screenData[i]["width"] ||
                screenDataElectron[i]["bounds"]["height"] !== screenData[i]["height"] ||
                screenDataElectron[i]["bounds"]["x"] !== screenData[i]["x"] ||
                screenDataElectron[i]["bounds"]["y"] !== screenData[i]["y"] ||
                screenDataElectron[i]["scaleFactor"] !== screenData[i]["scaleFactor"]) {
                    console.log("Screens:", screenData);
                    console.log("Screens:", screenDataElectron);
                throw new Error("Screen data mismatch!");
            }
        }
        logElem.innerHTML += "<li style='color:green'>Screen API test passed.</li>";

        let mainMonitor = {};
        for (let i = 0; i < screenData.length; i++) {
            if (screenData[i]["isPrimary"]) {
                mainMonitor = screenData[i];
                break;
            }
        }

        //test Mouse API
        const testMouse = async function(x=Control.Mouse.getX(), y=Control.Mouse.getY()) {
            return new Promise((resolve, reject) => {
                let timeoutId = -1;
                const timeoutFn = function() {
                    clearTimeout(timeoutId);
                    window.removeEventListener("click", clickListener);
                    reject(new Error("Mouse click timeout!"));
                };
                const clickListener = async function (event) {
                    clearTimeout(timeoutId);
                    window.removeEventListener("click", clickListener);

                    const mousePosX = event.clientX;
                    const mousePosY = event.clientY;
                    console.log("Mouse position:", mousePosX, mousePosY);
                    const mouseAPIPosX = Math.floor(Control.Mouse.getX())-bounds["x"];
                    const mouseAPIPosY = Math.floor(Control.Mouse.getY())-bounds["y"];
                    console.log("Mouse position from API:", mouseAPIPosX, mouseAPIPosY);

                    if (Math.abs(mousePosX - mouseAPIPosX) > 1 || Math.abs(mousePosY - mouseAPIPosY) > 1) {
                        console.log("Expected position:", x, y);
                        reject(new Error("Mouse position mismatch!"));
                    }
                    resolve(true);
                };
                window.addEventListener("click", clickListener);
                Control.Mouse.setX(x);
                Control.Mouse.setY(y);
                Control.Mouse.buttonDown("left");
                Control.Mouse.buttonUp("left");
                timeoutId = setTimeout(timeoutFn, 1000);
            });
            
        };
        const testMouse2 = async function(x=Control.Mouse.getX(), y=Control.Mouse.getY()) {
            return new Promise((resolve, reject) => {
                let timeoutId = -1;
                const timeoutFn = function() {
                    clearTimeout(timeoutId);
                    window.removeEventListener("mouseup", clickListener);
                    reject(new Error("Mouse click timeout!"));
                };
                const btns = ["left", "middle", "right", "back", "forward"];
                const btnIndex = Math.floor(Math.random() * btns.length);
                const btn = btns[btnIndex];

                const clickListener = async function (event) {
                    clearTimeout(timeoutId);
                    window.removeEventListener("mouseup", clickListener);

                    const mousePosX = event.clientX;
                    const mousePosY = event.clientY;
                    console.log("Mouse position:", mousePosX, mousePosY);
                    const mouseAPIPosX = Math.floor(Control.Mouse.getX())-bounds["x"];
                    const mouseAPIPosY = Math.floor(Control.Mouse.getY())-bounds["y"];
                    console.log("Mouse position from API:", mouseAPIPosX, mouseAPIPosY);

                    if (Math.abs(mousePosX - mouseAPIPosX) > 1 || Math.abs(mousePosY - mouseAPIPosY) > 1) {
                        console.log("Expected position:", x, y);
                        reject(new Error("Mouse position mismatch!"));
                    }
                    if (event.button !== btnIndex) {
                        console.log("Expected button:", btnIndex, btn);
                        console.log("Received button:", event.button);
                        reject(new Error("Mouse button mismatch!"));
                    }
                    resolve(true);
                };
                
                window.addEventListener("mouseup", clickListener);

                Control.Mouse.setX(x);
                Control.Mouse.setY(y);
                Control.Mouse.buttonDown(btn);
                Control.Mouse.buttonUp(btn);
                timeoutId = setTimeout(timeoutFn, 1000);
            });
        };
        const getRandomPos = function() {
            const x = Math.floor(Math.random() * bounds["width"]) + bounds["x"] + 10;
            const y = Math.floor(Math.random() * bounds["height"]) + bounds["y"] + 10;
            return {
                "x": x,
                "y": y
            };
        };
        await testMouse(100, 100);
        await new Promise(resolve => setTimeout(resolve, 100));
        await testMouse(200, 150);
        await new Promise(resolve => setTimeout(resolve, 100));
        await testMouse(300, 200);
        await new Promise(resolve => setTimeout(resolve, 100));
        for (let i = 0; i < 20; i++) {
            await testMouse(getRandomPos()["x"], getRandomPos()["y"]);
            await new Promise(resolve => setTimeout(resolve, 25));
        }
        for (let i = 0; i < 20; i++) {
            await testMouse2(getRandomPos()["x"], getRandomPos()["y"]);
            await new Promise(resolve => setTimeout(resolve, 25));
        }
        
        const testMouseWheel = async function(isUp=true, delta=1, isHorizontal=false) {
            return new Promise((resolve, reject) => {
                let timeoutId = -1;
                const timeoutFn = function() {
                    clearTimeout(timeoutId);
                    window.removeEventListener("scroll", wheelListener);
                    console.log("Mouse wheel test:", isUp, delta, isHorizontal);
                    reject(new Error("Mouse wheel timeout!"));
                };
                const wheelListener = async function (event) {
                    clearTimeout(timeoutId);
                    window.removeEventListener("scroll", wheelListener);
                    resolve(true);
                };
                window.scrollTo(100, 100);
                window.addEventListener("scroll", wheelListener);
                if (isUp) {
                    Control.Mouse.scrollUp(delta, isHorizontal);
                } else {
                    Control.Mouse.scrollDown(delta, isHorizontal);
                }
                timeoutId = setTimeout(timeoutFn, 1000);
            });
        };
        for (let i = 0; i < 20; i++) {
            const isUp = Math.random() >= 0.5;
            const delta = Math.round(Math.random() * 5) + 1;
            const isHorizontal = Math.random() >= 0.5;
            await testMouseWheel(isUp, delta, isHorizontal);
            await new Promise(resolve => setTimeout(resolve, 25));
        }

        logElem.innerHTML += "<li style='color:green'>Mouse API test passed.</li>";
*/

        //test Keyboard API
        const testKey = async function(keyBtn) {
            return new Promise((resolve, reject) => {
                let timeoutId = -1;
                const timeoutFn = () => {
                    clearTimeout(timeoutId);
                    window.removeEventListener("keydown", keyListener);
                    window.removeEventListener("keyup", keyListener2);
                    reject(new Error("Keydown timeout!"));
                };

                let isDown = false;
                const keyListener = (event) => {
                    console.log("Key down event:", event.key);
                    if (event.key === keyBtn) {
                        window.removeEventListener("keydown", keyListener);
                        isDown = true;
                    }
                };
                const keyListener2 = (event) => {
                    clearTimeout(timeoutId);
                    window.removeEventListener("keyup", keyListener2);
                    console.log("Key up event:", event.key);
                    if (event.key !== keyBtn) {
                        console.log("Expected key:", keyBtn, "; Received key:", event.key);
                        reject(new Error("Received key does not match!"));
                        return;
                    }
                    
                    if (!isDown) {
                        reject(new Error("Key was not registered as down!"));
                        return;
                    }
                    console.log("Expected key:", keyBtn, "; Received key:", event.key);
                    resolve(true);
                };
                window.addEventListener("keydown", keyListener);
                window.addEventListener("keyup", keyListener2);
                Control.Keyboard.keyDown(keyBtn);
                Control.Keyboard.keyUp(keyBtn);
                console.log("Sent key:", keyBtn);
                timeoutId = setTimeout(timeoutFn, 1000);
            });
        };
        const keysToTest = ["a", "b", "c", "A", "B", "C", "1", "2", "3", "!", "@", "#", "Enter", "Escape", "ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight", " "];
        await testKey("h");
        await new Promise(resolve => setTimeout(resolve, 1000));
        await testKey("g");
        await new Promise(resolve => setTimeout(resolve, 1000));
        
        logElem.innerHTML += "<li style='color:green'>Keyboard API test passed.</li>";

        //test Controller API
        


    });

};
main();
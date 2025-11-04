"use strict";

import Control from "../dist/easy-control.cjs";


const main = async () => {
    /*
    console.log(Control);
    console.log(Control.Mouse.getX());
    console.log(Control.Mouse.getY());
    console.log(Control.Mouse.getIcon());
    console.log(Control.Mouse.setX(1500));
    
   
    */
 console.log(Control.Screen.list());
    console.log(Control.Controller.list());
    console.log(Control.Controller.isSupported());
    if (!Control.Controller.isSupported()) {
        console.log(await Control.Controller.install());
    }
    const controller = Control.Controller.create();
    console.log(controller);
    console.log(Control.Controller.list());
    for (let i = 0; i < 4; i++) {
        console.log("press button 1");
        controller.buttonDown(1);
        await new Promise((resolve) => setTimeout(resolve, 500));
        controller.buttonUp(1);
        await new Promise((resolve) => setTimeout(resolve, 500));
    }
    
    console.log(controller.isActive());
    console.log("turn off");
    controller.disconnect();
    console.log(controller.isActive());
    console.log(controller);
    console.log(Control.Controller.list());
};
main();

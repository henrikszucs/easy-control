"use strict";

import Control from "../dist/easy-control.cjs";

console.log(Control);
console.log(Control.Mouse.getX());
console.log(Control.Mouse.getY());
console.log(Control.Mouse.getIcon());
console.log(Control.Mouse.setX(1500));

console.log(Control.Screen.list());

console.log(Control.Controller.list());
console.log(Control.Controller.isSupported());
console.log(Control.Controller.install());
console.log(Control.Controller.create());

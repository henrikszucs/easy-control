# easy-control
Simple and easy to use node.js module to handle mouse, keyboard and controller input for Windows, MacOS and Linux.

## Building

### Automatic build
```
npm install
```

### Manual build
```
npm install
npm run build
```

### Uninstall
```
npm run uninstall
```

> [!CAUTION]
> For building need additional tools. It differs in every operating system.

### Dependencies
#### Windows
- install Visual Studio [https://visualstudio.microsoft.com/vs/community/](https://visualstudio.microsoft.com/vs/community/) and select "Desktop development with C++" bundle
- install Python 3.6+ [https://apps.microsoft.com/detail/9ncvdn91xzqp](https://apps.microsoft.com/detail/9ncvdn91xzqp)
- npm install -g node-gyp

#### MacOS
- install Xcode

#### Linux
- sudo apt-get install libxtst-dev libpng++-dev gcc

## Usage

```js
import { Mouse, Keyboard,  Controller, Screen } from "./dist/easy-control.js";
```

### Mouse
```js
const x = Mouse.getX();
const y = Mouse.getY();
const icon = Mouse.getIcon();

Mouse.setX(x);
Mouse.setY(y);

Mouse.buttonDown(btn);
Mouse.buttonUp(btn);

Mouse.scrollDown(btn);
Mouse.scrollUp(btn);
```

### Keyboard
```js
Keyboard.keyDown(key);
Keyboard.keyUp(key);

Keyboard.type(text);
```


### Controller
```js
const isSupported = Controller.isSupport();

const contollers = Controller.list();
const controller1 = Controller.create(name);

const status = controller1.isActive();

controller1.keyDown(key);
controller1.keyUp(key);

controller1.setAxes(type, axes);

controller1.vibrate(ms);

controller1.disconnect();
controller1.delete();
```


### Screen
```js
const screens = Screen.list();
```







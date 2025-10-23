# easy-control
Simple and easy to use node.js module to handle mouse, keyboard and controller input for Windows, MacOS and Linux.

## Building

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
- ```npm install -g node-gyp```

#### MacOS
- install Xcode [https://apps.apple.com/us/app/xcode/id497799835](https://apps.apple.com/us/app/xcode/id497799835)

#### Linux
- ```sudo apt-get install libxtst-dev libpng++-dev gcc```

## Usage

```js
import { Mouse, Keyboard,  Controller, Screen } from "./dist/easy-control.cjs";
```

### Mouse
```js
const x = Mouse.getX();
const y = Mouse.getY();

const icon = Mouse.getIcon();

Mouse.setX(x);
Mouse.setY(y);

Mouse.buttonDown(btn);  // right | middle | left | button_1 | button_2 | button_3
Mouse.buttonUp(btn);

Mouse.scrollDown(length=1);
Mouse.scrollUp(length=1);
```

### Keyboard
```js
Keyboard.keyDown(key);  //https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/key
Keyboard.keyUp(key);

Keyboard.type(text="");
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







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

/*
    !!! Function calculate and return with X and Y positions with scaled value !!!

    Example: 1920 x 1080 with 1.25 (125% scale) >> right bottom corner >> 1536 x 864

    If you want convert normal pixel to scaled just divide by scale e.g. 1920/1.25 = 1536
    
    If you want convert scaled pixel to normal just multiply is by scale e.g. 1536*1.25 = 1920
*/
```

### Mouse
```js
const x = Mouse.getX();
const y = Mouse.getY();

const icon = Mouse.getIcon();
/*
{
    "width": 32,
    "height": 32,
    "data": [0,1,2...], //argb data
    "xOffset": 0,
    "yOffset": 0
}
*/

Mouse.setX(x);
Mouse.setY(y);

Mouse.buttonDown(btn);  // right | middle | left | back | forward
Mouse.buttonUp(btn);    // right | middle | left | back | forward

Mouse.scrollDown(amount=1, isHorizontal=false); // down or right scroll
Mouse.scrollUp(amount=1, isHorizontal=false);   // up or left scroll
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

const isSuccess = Controller.install();

const contollers = Controller.list();

const controller1 = Controller.create(name);

const status = controller1.isActive();

controller1.keyDown(key);
controller1.keyUp(key);

controller1.setAxes(type, axes);

controller1.disconnect();
controller1.delete();
```


### Screen
```js
const screens = Screen.list();
/*
[
    {
        "isPrimary": true,
        "width": 1536,
        "height": 864,
        "xOffset": 0,
        "yOffset": 0,
        "scaleFactor": 1.25
    },
    {
        "isPrimary": false,
        "width": 1680,
        "height": 900,
        "xOffset": -1680,
        "yOffset": 0,
        "scaleFactor": 1
    }
]
*/
```







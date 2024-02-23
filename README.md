# TR2D Engine

TR2D is a game engine made by TimRat for **God's Star** project.

## Table of contents

1. [Building the engine](#building-the-engine)
2. [Startup arguments](#startup-arguments)
3. [Writing UI](#interface-creation)

### Building the engine

To build the engine you need:

- CMake
- C++ compiler(preferably MinGW)
- SFML: Graphics library(can be found at sfml-dev.org)
- OpenAL: Audio library(can be found at openal.org)
- Box2D: Physics library(can be found at github.com/erincatto/box2d)
- PugiXML: XML parser(can be found at pugixml.org)

After that just load the CMake project and compile it. The engine uses static linking by default.

### Startup arguments

`.\TR2D.exe [args: name=type=value]`
The `args` parameter is a list of variables of window that you want to change before the startup. You can define them here or change in the `res/global/window.trconf` file.

Syntax:

1. *Name*: the variable name;
2. *Type*: the type of variable, either `str` or `num`;
3. *Value*: the value itself.

**Warning: there has to be no spaces between parts, otherwise you will get the parse error!**

**Example:** `.\TR2D.exe debug=num=0`

### Interface creation

1. [Basis](#main-elements)
2. [Additional tags](#additional-elements)

#### Main elements

The first thing every player will see is the main menu. It is hardcoded in the engine and is loading from the `res/ui/mainMenu.trui` file.

Actually, for the engine every menu is just a fullscreen UI object, so we write them in the following files. Every file with XML syntax has to start with `<?xml version="1.0"?>`.

Frame is the root object for any UI so it is the first thing we declare:
`<frame name="some name">[all the parts...]</frame>`

The next thing is object - a container for sprites, texts and actions. It is declared by the following tag:
`<object name="some name">[all the details...]</object>`

#### Additional elements

If you need all the sprites and texts have the same position, you can define it for the object:

`<position [type]>x1 y1 x2 y2</position>`

Position type may be:

- *relative*: A number between 0 and 1, where 0 is the left(up) side of the window, 0.5 is the center and 1 is the right(down) side of the window;
- *absolute*: Pixel coordinates.

x1, y1, x2 and y2 stand for X and Y coordinates for idle and active states.

To use this position, the `position` tag in sprite/text has to be with type `inherit`, after which you can define the offset from object's position in pixels.
`<position type="inherit">x1 y1 x2 y2</position>`

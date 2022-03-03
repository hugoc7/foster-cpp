# Foster

## Description
Foster is a 2D online multiplayer plateformer where you shoot your enemies to win the battle. This project is still under development. Author: Hugo Caroubalos.

## Game instructions
When the game begins, type 1 for launching a server or 2 to launch the client. Then choose a UDP port.

Controls:
- Jump : UP
- Left : LEFT
- Right : RIGHT
- Chat message : ENTER
- Scoreboard / Connected players : TAB

## Download the game
 You can directly download the game for windows on the Github page of the project : https://github.com/hugoc7/foster-cpp/releases/tag/Foster-0.0.1-win64

## Development

Foster is written in C++ and uses the SDL library. The game engine is created from scratch, reinventing the wheel is a way to learn concepts.
The big parts of this projects are :
- Collisions and physics system
- Entity Component System
- Messaging Service (Chat) using TCP protocol
- Networked entities using UDP protocol 

The game logic is not implemented yet.

## Dependancies

Foster uses the following libraries:
- SDL : https://www.libsdl.org/
- SDL_net : https://www.libsdl.org/projects/SDL_net/
- SDL_ttf : https://www.libsdl.org/projects/SDL_ttf/
- SDL_FontCache : https://github.com/grimfang4/SDL_FontCache
- ReaderWriterQueue : https://github.com/cameron314/readerwriterqueue
- Struct: https://github.com/svperbeast/struct
- CRCpp: https://github.com/d-bahr/CRCpp

## Build and Debug

Foster uses [Xmake](https://xmake.io) for building, project generation and library management. You can download Xmake to easily build the game, although I haven't tested my script on many plateforms.

**Build the game** 
```
xmake
xmake install
```
If everything goes fine you will get a **bin** folder with Foster inside. To run the game you will also need an **assets** folder containing the following files (images, fonts and map file) :
```
assets  
└───img
│   │   player.bmp
│   │   plateform.bmp
└───maps
│   │   map.txt
└───fonts
    │   Sansation_Regular.ttf
```
You can [download the assets](https://github.com/hugoc7/foster-cpp/releases/tag/Assets) on the github page of the project, under Releases. Then unzip the archive and put the **assets** folder in the same folder than the Foster executable.

**Generate a VisualStudio project**  
```
xmake project -k vsxmake -m "debug,release"
```
If you use a different IDE, see [Xmake documentation](https://xmake.io/#/plugin/builtin_plugins?id=generate-ide-project-files)


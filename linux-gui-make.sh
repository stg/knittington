#!/bin/sh
set -e
echo [bin-linux/gui] building command-line-interface binary...
gcc gui/*.c lib/src/*.c -I lib/include/ -lfreeimage -lstdc++ -lSDLmain -lSDL -o bin-linux/gui_sdl12

echo [bin-linux/gui] building SDL2 gui binary...
gcc gui/*.c lib/src/*.c -I lib/include/ -lfreeimage -lstdc++ -lSDL2main -lSDL2 -DSDL2 -o bin-linux/gui
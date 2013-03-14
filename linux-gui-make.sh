#!/bin/sh
set -e
#echo [bin-linux/gui] building SDL1.2 gui binary..
#gcc gui/*.c lib/src/*.c -I lib/include/ -lfreeimage -lstdc++ -lSDLmain -lSDL -o bin-linux/gui_sdl12

echo [bin-linux/gui] building SDL2 gui binary...
objcopy -B i386 -I binary -O elf32-i386 res/ui.bmp ui_bmp.o
gcc ui_bmp.o gui/*.c lib/src/*.c -I lib/include/ -lfreeimage -lstdc++ -lSDL2main -lSDL2 -DSDL2 -o bin-linux/gui
rm ui_bmp.o

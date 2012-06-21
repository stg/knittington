#!/bin/sh
set -e
echo [bin-osx/gui] building command-line-interface binary...
gcc gui/*.c lib/src/*.c -framework Cocoa -I lib/include/ -lfreeimage -lstdc++ -lSDLmain -lSDL -o bin-osx/gui_sdl12

echo [bin-osx/gui] building SDL2 gui binary...
gcc gui/*.c lib/src/*.c -framework Cocoa -Fbin-osx/kh940knit.app/Contents/Frameworks -framework SDL2 -I lib/include/ -lfreeimage -lstdc++ -DSDL2 -o bin-osx/kh940knit.app/Contents/MacOS/kh940knit
install_name_tool -change @rpath/SDL2.framework/Versions/A/SDL2 @executable_path/../Frameworks/SDL2.framework/Versions/A/SDL2 bin-osx/kh940knit.app/Contents/MacOS/kh940knit

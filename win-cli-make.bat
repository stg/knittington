@echo off
echo [bin-win\knit.exe] building command-line-interface executable...
gcc cli\src\knit.c lib\src\*.c -I lib\include\ -o bin-win\knit.exe
strip bin-win\knit.exe
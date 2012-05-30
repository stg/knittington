@echo off
echo [bin-win\cli.exe] building command-line-interface executable...
gcc cli\src\knit.c lib\src\*.c -Ilib\include\ -lsetupapi -o bin-win\cli.exe
strip bin-win\cli.exe
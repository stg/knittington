#!/bin/sh
echo [bin-linux/cli] building command-line-interface binary...
set -e
gcc cli/src/main.c lib/src/*.c -I lib/include/ -o bin-linux/cli

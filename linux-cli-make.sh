#!/bin/sh
echo [bin-linux/knit] building command-line-interface binary...
set -e
gcc cli/src/knit.c lib/src/*.c -I lib/include/ -o bin-linux/knit

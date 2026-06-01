#! /bin/bash
set -xe
export PKG_CONFIG_PATH=/home/ve/opt/lib/raylib/lib/pkgconfig


make 

[ ! -e "build/libraycandle.so" ] && ln -s --relative "build/libraycandle.so.1" "build/libraycandle.so"
RAYCANDLE_LIB="./build"

if [ "$#" -lt 1 ]; then
    echo "Usage: ./run.sh file1.c [file2.c ...] [optional_flags]"
    exit 1
fi


LAST_ARG="${@: -1}"
EXE_NAME=$(basename "$LAST_ARG" .c)


gcc "$@" -o "$EXE_NAME" `pkg-config --cflags raylib`  -L./build -lraycandle `pkg-config --libs raylib`  -lm 

 LD_LIBRARY_PATH=$RAYCANDLE_LIB ./$EXE_NAME

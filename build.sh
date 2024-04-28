#!/bin/bash

set -xe
NAME=gson
CCFLAGS="-DDEBUG=0 -Wall -g -O3"

if [ ! -z "$1" ] && [ "$1" == "clean" ]
then
    rm -vf lib/lib$NAME.a lib/$NAME.o ./$NAME
else
    gcc $CCFLAGS -c src/$NAME.c -o lib/$NAME.o
    ar rcs lib/lib$NAME.a lib/$NAME.o
    gcc -L./lib -I./include main.c -o $NAME -l$NAME
fi


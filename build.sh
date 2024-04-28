#!/bin/bash

set -xe
NAME=gson
CCFLAGS="-DDEBUG=0 -Wall -g"

compile_lib() {
    mkdir -pv lib
    gcc $CCFLAGS -c src/$NAME.c  -o lib/$NAME.o
    gcc $CCFLAGS -c src/debug.c  -o lib/debug.o
    ar rcs lib/lib$NAME.a lib/$NAME.o lib/debug.o
}

run_tests() {
    compile_lib
    gcc -g -L./lib -I./include tests/tests.c -o "$NAME"_tests -l$NAME
    ./"$NAME"_tests
}

if [ ! -z "$1" ] && [ "$1" == "clean" ]
then
    rm -vf lib/lib$NAME.a lib/$NAME.o lib/debug.o ./$NAME ./"$NAME"_tests
elif [ ! -z "$1" ] && [ "$1" == "test" ]
then
    run_tests 2>/dev/null
else
    compile_lib
    gcc -g -L./lib -I./include main.c -o $NAME -l$NAME
fi


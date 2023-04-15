#!/bin/sh

set -xe

CC=g++
CXXFLAGS="-Wall -std=c++11 `pkg-config sdl2 --cflags`"
LIBS=`pkg-config sdl2 --libs`
SRC="main.cpp display.cpp chip.cpp"

$CC $CXXFLAGS $SRC -o chip8 $LIBS

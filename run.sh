#!/bin/sh

set -xe

g++ main.cpp -I/usr/local/include/SDL2 -std=c++11 -L/usr/local/lib -lSDL2 -o chip8

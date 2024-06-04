#!/bin/sh
make -f Makefile.original cube
cc -O2 -Wall -Werror -g -I/usr/include -I./ -Iicons/ `sh -c 'pkg-config gtk+-2.0 $0 2>/dev/null || gtk-config $0' --cflags`  -c sdl.c -o sdl.o
cc -lm -ldl -o cube cube.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf

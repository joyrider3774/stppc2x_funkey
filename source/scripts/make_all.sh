#!/bin/sh
make -f Makefile.original
cc -O2 -Wall -Werror -g -I/usr/include -I./ -Iicons/ `sh -c 'pkg-config gtk+-2.0 $0 2>/dev/null || gtk-config $0' --cflags`  -c sdl.c -o sdl.o
cc -lm -ldl -o blackbox blackbox.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o bridges bridges.o drawing.o dsf.o malloc.o midend.o misc.o sdl.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o cube cube.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o dominosa dominosa.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o fifteen fifteen.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o filling filling.o drawing.o dsf.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o flip flip.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o tree234.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o galaxies galaxies.o drawing.o dsf.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o guess guess.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o inertia inertia.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o lightup lightup.o combi.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o loopy loopy.o drawing.o dsf.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o tree234.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o map map.o drawing.o dsf.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o mines mines.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o tree234.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o net net.o drawing.o dsf.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o tree234.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o netslide netslide.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o tree234.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o pattern pattern.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o pegs pegs.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o tree234.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o rect rect.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o samegame samegame.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o sixteen sixteen.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o slant slant.o drawing.o dsf.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o solo solo.o divvy.o drawing.o dsf.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o tents tents.o drawing.o sdl.o malloc.o maxflow.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o twiddle twiddle.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o unequal unequal.o drawing.o sdl.o latin.o malloc.o maxflow.o midend.o misc.o printing.o ps.o random.o tree234.o version.o -lSDL -lSDL_gfx -lSDL_ttf
cc -lm -ldl -o untangle untangle.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o tree234.o version.o -lSDL -lSDL_gfx -lSDL_ttf

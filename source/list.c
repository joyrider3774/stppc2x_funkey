/*
 * list.c: List of pointers to puzzle structures, for monolithic
 * platforms.
 *
 */
#include "puzzles.h"
#define GAMELIST(A) \
    A(blackbox) \
    A(bridges) \
    A(cube) \
    A(dominosa) \
    A(fifteen) \
    A(filling) \
    A(flip) \
    A(galaxies) \
    A(guess) \
    A(inertia) \
    A(lightup) \
    A(loopy) \
    A(map) \
    A(maze3d) \
    A(mines) \
    A(mosco) \
    A(net) \
    A(netslide) \
    A(pattern) \
    A(pegs) \
    A(rect) \
    A(samegame) \
    A(sixteen) \
    A(slant) \
    A(slide) \
    A(sokoban) \
    A(solo) \
    A(tents) \
    A(twiddle) \
    A(unequal) \
    A(untangle) \

#define DECL(x) extern const game x;
#define REF(x) &x,
GAMELIST(DECL)
const game *gamelist[] = { GAMELIST(REF) };
const int gamecount = lenof(gamelist);

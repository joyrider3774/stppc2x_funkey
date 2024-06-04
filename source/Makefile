# Makefile for GP2X cross-compilation using the Open2x toolchain and libraries
#
# This Makefile statically compiles all the games into a single executable per game and then
# strips them.  This way, they don't need particular SDL libraries loaded on the GP2X.
# It can also compile the against Paeryn's hardware-accelerated SDL libraries by replacing
# libSDL.a with Paeryn's one.
#
# I've used the default Open2x directories.  I've also removed some of the object files
# such as those used for external solver programs etc.

CROSS_COMPILE = /opt/open2x/gcc-4.1.1-glibc-2.3.6/bin/arm-open2x-linux-
SDL_BASE = /opt/open2x/gcc-4.1.1-glibc-2.3.6/bin/arm-open2x-linux-
LDFLAGS = -static -L/opt/open2x/gcc-4.1.1-glibc-2.3.6/lib/ -L/opt/open2x/gcc-4.1.1-glibc-2.3.6/lib/gcc/arm-open2x-linux/4.1.1/ -L/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/lib/ -lSDL -lSDL_ttf -lSDL -lSDL_gfx -lfreetype -lm -lgcc -lpthread -lc

CC=$(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
STRIP = $(CROSS_COMPILE)strip

CXXFLAGS = -I /opt/open2x/gcc-4.1.1-glibc-2.3.6/include/ -DTARGET_GP2X -O2 -Wall -Werror 
#CXXFLAGS = -I /opt/open2x/gcc-4.1.1-glibc-2.3.6/include/ -DTARGET_GP2X -O2 -Wall -Werror
LIBS = -L"/opt/open2x/gcc-4.1.1-glibc-2.3.6/lib" -lSDL -lSDL_gfx --start-group -lSDL_ttf -lfreetype -lSDL --end-group -lSDL_image -ljpeg -lpng12 -lz --start-group -lSDL_mixer -lvorbisidec -lmikmod -lsmpeg -lmad -lSDL --end-group -lgcc -lm -lc -lpthread -ldl

ALL_TARGETS = $(TARGET)

all: blackbox.gpe bridges.gpe cube.gpe dominosa.gpe fifteen.gpe filling.gpe flip.gpe galaxies.gpe guess.gpe inertia.gpe lightup.gpe loopy.gpe map.gpe mines.gpe net.gpe netslide.gpe pattern.gpe pegs.gpe rect.gpe samegame.gpe sixteen.gpe slant.gpe solo.gpe tents.gpe twiddle.gpe unequal.gpe untangle.gpe

clean:
		rm *.o

# GAMES
# =====

blackbox.gpe: blackbox.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o
	$(CXX) -o $@ blackbox.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o $(LDFLAGS)
	$(STRIP) $@

bridges.gpe: bridges.o drawing.o dsf.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o
	$(CXX) -o $@ bridges.o drawing.o dsf.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o $(LDFLAGS)
	$(STRIP) $@

cube.gpe: cube.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o
	$(CXX) -o $@ cube.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o $(LDFLAGS)
	$(STRIP) $@

dominosa.gpe: dominosa.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o
	$(CXX) -o $@ dominosa.o drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o $(LDFLAGS)
	$(STRIP) $@

fifteen.gpe: drawing.o fifteen.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o
	$(CXX) -o $@ drawing.o fifteen.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o $(LDFLAGS)
	$(STRIP) $@

filling.gpe: drawing.o dsf.o filling.o sdl.o malloc.o midend.o 	misc.o printing.o ps.o random.o version.o
	$(CXX) -o $@ drawing.o dsf.o filling.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o $(LDFLAGS)
	$(STRIP) $@

flip.gpe: drawing.o flip.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o tree234.o version.o
	$(CXX) -o $@ drawing.o flip.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o tree234.o version.o $(LDFLAGS)
	$(STRIP) $@

galaxies.gpe: drawing.o dsf.o galaxies.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o
	$(CXX) -o $@ drawing.o dsf.o galaxies.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o version.o $(LDFLAGS)
	$(STRIP) $@

guess.gpe: drawing.o sdl.o guess.o malloc.o midend.o misc.o printing.o ps.o random.o version.o
	$(CXX) -o $@ drawing.o sdl.o guess.o malloc.o midend.o misc.o printing.o ps.o random.o version.o $(LDFLAGS)
	$(STRIP) $@

inertia.gpe: drawing.o sdl.o inertia.o malloc.o midend.o misc.o printing.o ps.o random.o version.o
	$(CXX) -o $@ drawing.o sdl.o inertia.o malloc.o midend.o misc.o printing.o ps.o random.o version.o $(LDFLAGS)
	$(STRIP) $@

lightup.gpe: combi.o drawing.o sdl.o lightup.o malloc.o midend.o misc.o printing.o ps.o random.o version.o
	$(CXX) -o $@ combi.o drawing.o sdl.o lightup.o malloc.o midend.o misc.o printing.o ps.o random.o version.o $(LDFLAGS)
	$(STRIP) $@

loopy.gpe: drawing.o dsf.o sdl.o loopy.o malloc.o midend.o misc.o printing.o ps.o random.o tree234.o version.o
	$(CXX) -o $@ drawing.o dsf.o sdl.o loopy.o malloc.o midend.o misc.o printing.o ps.o random.o tree234.o version.o $(LDFLAGS)
	$(STRIP) $@

map.gpe: drawing.o dsf.o sdl.o malloc.o map.o midend.o misc.o printing.o ps.o random.o version.o
	$(CXX) -o $@ drawing.o dsf.o sdl.o malloc.o map.o midend.o misc.o printing.o ps.o random.o version.o $(LDFLAGS)
	$(STRIP) $@

mines.gpe: drawing.o sdl.o malloc.o midend.o mines.o misc.o printing.o ps.o random.o tree234.o version.o
	$(CXX) -o $@ drawing.o sdl.o malloc.o midend.o mines.o misc.o printing.o ps.o random.o tree234.o version.o $(LDFLAGS)
	$(STRIP) $@

net.gpe: drawing.o dsf.o sdl.o malloc.o midend.o misc.o net.o printing.o ps.o random.o tree234.o version.o
	$(CXX) -o $@ drawing.o dsf.o sdl.o malloc.o midend.o misc.o net.o printing.o ps.o random.o tree234.o version.o $(LDFLAGS)
	$(STRIP) $@

netslide.gpe: drawing.o sdl.o malloc.o midend.o misc.o netslide.o printing.o ps.o random.o tree234.o version.o
	$(CXX) -o $@ drawing.o sdl.o malloc.o midend.o misc.o netslide.o printing.o ps.o random.o tree234.o version.o $(LDFLAGS)
	$(STRIP) $@

pattern.gpe: drawing.o sdl.o malloc.o midend.o misc.o pattern.o printing.o ps.o random.o version.o
	$(CXX) -o $@ drawing.o sdl.o malloc.o midend.o misc.o pattern.o printing.o ps.o random.o version.o $(LDFLAGS)
	$(STRIP) $@

pegs.gpe: drawing.o sdl.o malloc.o midend.o misc.o pegs.o printing.o ps.o random.o tree234.o version.o
	$(CXX) -o $@ drawing.o sdl.o malloc.o midend.o misc.o pegs.o printing.o ps.o random.o tree234.o version.o $(LDFLAGS)
	$(STRIP) $@

rect.gpe: drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o rect.o version.o
	$(CXX) -o $@ drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o rect.o version.o $(LDFLAGS)
	$(STRIP) $@

samegame.gpe: drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o samegame.o version.o
	$(CXX) -o $@ drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o samegame.o version.o $(LDFLAGS)
	$(STRIP) $@

sixteen.gpe: drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o sixteen.o version.o
	$(CXX) -o $@ drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o sixteen.o version.o $(LDFLAGS)
	$(STRIP) $@

slant.gpe: drawing.o dsf.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o slant.o version.o
	$(CXX) -o $@ drawing.o dsf.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o slant.o version.o $(LDFLAGS)
	$(STRIP) $@

solo.gpe: divvy.o drawing.o dsf.o malloc.o midend.o misc.o printing.o ps.o random.o sdl.o solo.o version.o
	$(CXX) -o $@ divvy.o drawing.o dsf.o malloc.o midend.o misc.o printing.o ps.o random.o sdl.o solo.o version.o $(LDFLAGS)
	$(STRIP) $@

tents.gpe: drawing.o sdl.o malloc.o maxflow.o midend.o misc.o printing.o ps.o random.o tents.o version.o
	$(CXX) -o $@ drawing.o sdl.o malloc.o maxflow.o midend.o misc.o printing.o ps.o random.o tents.o version.o $(LDFLAGS)
	$(STRIP) $@

twiddle.gpe: drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o twiddle.o version.o
	$(CXX) -o $@ drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o twiddle.o version.o $(LDFLAGS)
	$(STRIP) $@

unequal.gpe: drawing.o sdl.o latin.o malloc.o maxflow.o midend.o misc.o printing.o ps.o random.o tree234.o unequal.o version.o
	$(CXX) -o $@ drawing.o sdl.o latin.o malloc.o maxflow.o midend.o misc.o printing.o ps.o random.o tree234.o unequal.o version.o $(LDFLAGS)
	$(STRIP) $@

untangle.gpe: drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o tree234.o untangle.o version.o
	$(CXX) -o $@ drawing.o sdl.o malloc.o midend.o misc.o printing.o ps.o random.o tree234.o untangle.o version.o $(LDFLAGS)
	$(STRIP) $@

# OBJECTS
# =======

blackbox.o: ./blackbox.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

bridges.o: ./bridges.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

combi.o: ./combi.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

cube.o: ./cube.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

divvy.o: ./divvy.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

dominosa.o: ./dominosa.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

drawing.o: ./drawing.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

dsf.o: ./dsf.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

fifteen.o: ./fifteen.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

filling.o: ./filling.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

flip.o: ./flip.c ./puzzles.h ./tree234.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

galaxies.o: ./galaxies.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

guess.o: ./guess.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

inertia.o: ./inertia.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

latin.o: ./latin.c ./puzzles.h ./tree234.h ./maxflow.h ./latin.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

lightup.o: ./lightup.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

list.o: ./list.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

loopy.o: ./loopy.c ./puzzles.h ./tree234.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

malloc.o: ./malloc.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

map.o: ./map.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

maxflow.o: ./maxflow.c ./maxflow.h ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

midend.o: ./midend.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

mines.o: ./mines.c ./tree234.h ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

misc.o: ./misc.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

net.o: ./net.c ./puzzles.h ./tree234.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

netslide.o: ./netslide.c ./puzzles.h ./tree234.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

no-icon.o: ./no-icon.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

pattern.o: ./pattern.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

pegs.o: ./pegs.c ./puzzles.h ./tree234.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

printing.o: ./printing.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

ps.o: ./ps.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

random.o: ./random.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

rect.o: ./rect.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

samegame.o: ./samegame.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

sdl.o: ./sdl.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

sixteen.o: ./sixteen.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

slant.o: ./slant.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

solo.o: ./solo.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

tents.o: ./tents.c ./puzzles.h ./maxflow.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

tree234.o: ./tree234.c ./tree234.h ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

twiddle.o: ./twiddle.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

unequal.o: ./unequal.c ./puzzles.h ./latin.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

untangle.o: ./untangle.c ./puzzles.h ./tree234.h
	$(CXX) $(CXXFLAGS) -c $< -o $@


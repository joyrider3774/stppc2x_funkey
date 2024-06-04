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
UPX = upx -9
#UPX = echo

CXXFLAGS = -I /opt/open2x/gcc-4.1.1-glibc-2.3.6/include/ -DTARGET_GP2X -O2 -Wall -Werror 
#CXXFLAGS = -fsigned-bitfields -fsigned-char -I /opt/open2x/gcc-4.1.1-glibc-2.3.6/include/ -DTARGET_GP2X -O2 -Wall -Werror
LIBS = -L"/opt/open2x/gcc-4.1.1-glibc-2.3.6/lib" -lSDL -lSDL_gfx --start-group -lSDL_ttf -lfreetype -lSDL --end-group -lSDL_image -ljpeg -lpng12 -lz --start-group -lSDL_mixer -lvorbisidec -lmikmod -lsmpeg -lmad -lSDL --end-group -lgcc -lm -lc -lpthread -ldl

ALL_TARGETS = $(TARGET)

all: blackbox.gpe bridges.gpe cube.gpe dominosa.gpe fifteen.gpe filling.gpe flip.gpe galaxies.gpe guess.gpe inertia.gpe lightup.gpe loopy.gpe map.gpe mines.gpe net.gpe netslide.gpe pattern.gpe pegs.gpe rect.gpe samegame.gpe sixteen.gpe slant.gpe solo.gpe tents.gpe twiddle.gpe unequal.gpe untangle.gpe

clean:
		rm *.gpo

# GAMES
# =====

blackbox.gpe: blackbox.gpo dictionary.gpo drawing.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo sdl.gpo version.gpo
	$(CXX) -o $@ blackbox.gpo dictionary.gpo drawing.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo sdl.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

bridges.gpe: bridges.gpo dictionary.gpo drawing.gpo dsf.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo sdl.gpo version.gpo
	$(CXX) -o $@ bridges.gpo dictionary.gpo drawing.gpo dsf.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo sdl.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

cube.gpe: cube.gpo dictionary.gpo drawing.gpo iniparser.gpo sdl.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo
	$(CXX) -o $@ cube.gpo dictionary.gpo drawing.gpo iniparser.gpo sdl.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

dominosa.gpe: dictionary.gpo dominosa.gpo drawing.gpo iniparser.gpo sdl.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo dominosa.gpo drawing.gpo iniparser.gpo sdl.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

fifteen.gpe: dictionary.gpo drawing.gpo fifteen.gpo iniparser.gpo sdl.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo fifteen.gpo iniparser.gpo sdl.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

filling.gpe: dictionary.gpo drawing.gpo dsf.gpo filling.gpo iniparser.gpo sdl.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo dsf.gpo filling.gpo iniparser.gpo sdl.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

flip.gpe: dictionary.gpo drawing.gpo flip.gpo iniparser.gpo sdl.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo tree234.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo flip.gpo iniparser.gpo sdl.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo tree234.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

galaxies.gpe: dictionary.gpo drawing.gpo dsf.gpo galaxies.gpo iniparser.gpo sdl.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo dsf.gpo galaxies.gpo iniparser.gpo sdl.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

guess.gpe: dictionary.gpo drawing.gpo sdl.gpo guess.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo sdl.gpo guess.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

inertia.gpe: dictionary.gpo drawing.gpo sdl.gpo inertia.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo sdl.gpo inertia.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

lightup.gpe: combi.gpo dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo lightup.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo
	$(CXX) -o $@ combi.gpo dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo lightup.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

loopy.gpe: dictionary.gpo drawing.gpo dsf.gpo sdl.gpo iniparser.gpo loopy.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo tree234.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo dsf.gpo sdl.gpo iniparser.gpo loopy.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo tree234.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

map.gpe: dictionary.gpo drawing.gpo dsf.gpo sdl.gpo iniparser.gpo malloc.gpo map.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo dsf.gpo sdl.gpo iniparser.gpo malloc.gpo map.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

mines.gpe: dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo mines.gpo misc.gpo printing.gpo ps.gpo random.gpo tree234.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo mines.gpo misc.gpo printing.gpo ps.gpo random.gpo tree234.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

net.gpe: dictionary.gpo drawing.gpo dsf.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo net.gpo printing.gpo ps.gpo random.gpo tree234.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo dsf.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo net.gpo printing.gpo ps.gpo random.gpo tree234.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

netslide.gpe: dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo netslide.gpo printing.gpo ps.gpo random.gpo tree234.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo netslide.gpo printing.gpo ps.gpo random.gpo tree234.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

pattern.gpe: dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo pattern.gpo printing.gpo ps.gpo random.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo pattern.gpo printing.gpo ps.gpo random.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

pegs.gpe: dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo pegs.gpo printing.gpo ps.gpo random.gpo tree234.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo pegs.gpo printing.gpo ps.gpo random.gpo tree234.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

rect.gpe: dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo rect.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo rect.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

samegame.gpe: dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo samegame.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo samegame.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

sixteen.gpe: dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo sixteen.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo sixteen.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

slant.gpe: dictionary.gpo drawing.gpo dsf.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo slant.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo dsf.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo slant.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

solo.gpe: dictionary.gpo divvy.gpo drawing.gpo dsf.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo sdl.gpo solo.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo divvy.gpo drawing.gpo dsf.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo sdl.gpo solo.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

tents.gpe: dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo maxflow.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo tents.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo maxflow.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo tents.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

twiddle.gpe: dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo twiddle.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo twiddle.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

unequal.gpe: dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo latin.gpo malloc.gpo maxflow.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo tree234.gpo unequal.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo latin.gpo malloc.gpo maxflow.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo tree234.gpo unequal.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

untangle.gpe: dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo tree234.gpo untangle.gpo version.gpo
	$(CXX) -o $@ dictionary.gpo drawing.gpo sdl.gpo iniparser.gpo malloc.gpo midend.gpo misc.gpo printing.gpo ps.gpo random.gpo tree234.gpo untangle.gpo version.gpo $(LDFLAGS)
	$(STRIP) $@
	$(UPX) $@

# OBJECTS
# =======

blackbox.gpo: ./blackbox.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

bridges.gpo: ./bridges.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

combi.gpo: ./combi.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

cube.gpo: ./cube.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

dictionary.gpo: ./iniparser/src/dictionary.c ./iniparser/src/dictionary.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

divvy.gpo: ./divvy.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

dominosa.gpo: ./dominosa.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

drawing.gpo: ./drawing.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

dsf.gpo: ./dsf.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

fifteen.gpo: ./fifteen.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

filling.gpo: ./filling.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

flip.gpo: ./flip.c ./puzzles.h ./tree234.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

galaxies.gpo: ./galaxies.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

guess.gpo: ./guess.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

inertia.gpo: ./inertia.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

iniparser.gpo: ./iniparser/src/iniparser.c ./iniparser/src/iniparser.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

latin.gpo: ./latin.c ./puzzles.h ./tree234.h ./maxflow.h ./latin.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

lightup.gpo: ./lightup.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

list.gpo: ./list.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

loopy.gpo: ./loopy.c ./puzzles.h ./tree234.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

malloc.gpo: ./malloc.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

map.gpo: ./map.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

maxflow.gpo: ./maxflow.c ./maxflow.h ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

midend.gpo: ./midend.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

mines.gpo: ./mines.c ./tree234.h ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

misc.gpo: ./misc.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

net.gpo: ./net.c ./puzzles.h ./tree234.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

netslide.gpo: ./netslide.c ./puzzles.h ./tree234.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

no-icon.gpo: ./no-icon.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

pattern.gpo: ./pattern.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

pegs.gpo: ./pegs.c ./puzzles.h ./tree234.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

printing.gpo: ./printing.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

ps.gpo: ./ps.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

random.gpo: ./random.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

rect.gpo: ./rect.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

samegame.gpo: ./samegame.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

sdl.gpo: ./sdl.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

sixteen.gpo: ./sixteen.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

slant.gpo: ./slant.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

solo.gpo: ./solo.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

tents.gpo: ./tents.c ./puzzles.h ./maxflow.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

tree234.gpo: ./tree234.c ./tree234.h ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

twiddle.gpo: ./twiddle.c ./puzzles.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

unequal.gpo: ./unequal.c ./puzzles.h ./latin.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

untangle.gpo: ./untangle.c ./puzzles.h ./tree234.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

version.gpo: ./version.c
	$(CXX) $(CXXFLAGS) -c $< -o $@


# Makefile for GP2X cross-compilation using the Open2x toolchain and libraries
#
# This Makefile statically compiles all the games into a single executable per game and then
# strips them.  This way, they don't need particular SDL libraries loaded on the GP2X.
# It can also compile the against Paeryn's hardware-accelerated SDL libraries by replacing
# libSDL.a with Paeryn's one.
#
# I've used the default Open2x directories.  I've also removed some of the object files
# such as those used for external solver programs etc.

COMBINED = -DCOMBINED -DSLOW_SYSTEM

CROSS_COMPILE_DIR= /opt/open2x/gcc-4.1.1-glibc-2.3.6
CROSS_COMPILE = $(CROSS_COMPILE_DIR)/bin/arm-open2x-linux-
#LDFLAGS = $(COMBINED) -static -L$(CROSS_COMPILE_DIR)/lib/ -L$(CROSS_COMPILE_DIR)/lib/gcc/arm-open2x-linux/4.1.1/ -L$(CROSS_COMPILE_DIR)/arm-open2x-linux/lib/ -lSDL -lSDL_ttf -lSDL -lSDL_gfx -lSDL_image -lSDL -ljpeg -lpng12 -lz -lfreetype -lm -lgcc -lpthread -lc
LDFLAGS = $(COMBINED) -static -L$(CROSS_COMPILE_DIR)/lib/ \
-L$(CROSS_COMPILE_DIR)/lib/gcc/arm-open2x-linux/4.1.1/ \
-L$(CROSS_COMPILE_DIR)/arm-open2x-linux/lib/ \-lSDL -lSDLmain \
-lSDL_mixer -lsmpeg -lmad \
-lpthread -lSDL -lSDL_ttf -lSDL -lSDL_gfx -lSDL_image -lSDL -ljpeg \
-lpng12 -lz -lfreetype -lm -lgcc -lpthread -lc

#LDFLAGS = $(COMBINED) -static -L$(CROSS_COMPILE_DIR)/lib/ \
#-L$(CROSS_COMPILE_DIR)/lib/gcc/arm-open2x-linux/4.1.1/ \
#-L$(CROSS_COMPILE_DIR)/arm-open2x-linux/lib/ \-lSDL -lSDLmain \
#-lSDL_mixer -lvorbisfile -lvorbisidec -logg -lmikmod -lsmpeg -lmad \
#-lpthread -lSDL -lSDL_ttf -lSDL -lSDL_gfx -lSDL_image -lSDL -ljpeg \
#-lpng12 -lz -lfreetype -lm -lgcc -lpthread -lc

CC=$(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++
LD=$(CROSS_COMPILE)ld
STRIP = $(CROSS_COMPILE)strip
UPX = /usr/local/sbin/upx -9
#UPX = echo

OPTIMISATION=-O3

CXXFLAGS = $(COMBINED) -fsigned-char -I $(CROSS_COMPILE_DIR)/include/ -DTARGET_GP2X $(OPTIMISATION) -Wall -Werror 
LIBS = -L$(CROSS_COMPILE_DIR)/lib -lSDL -lSDL_gfx --start-group -lSDL_ttf \
-lfreetype -lSDL --end-group -lSDL_image -ljpeg -lpng12 -lz --start-group -lSDL_mixer \
-lvorbisfile -lvorbisidec -l ogg -lmikmod -lsmpeg -lmad -lSDL --end-group -lgcc -lm -lc -lpthread -ldl $(COMBINED)

ALL_TARGETS = $(TARGET)

all: stppc2x
#blackbox.gpe bridges.gpe cube.gpe dominosa.gpe fifteen.gpe filling.gpe flip.gpe galaxies.gpe guess.gpe inertia.gpe lightup.gpe loopy.gpe map.gpe maze3d.gpe mines.gpe net.gpe netslide.gpe pattern.gpe pegs.gpe rectangles.gpe samegame.gpe sixteen.gpe slant.gpe slide.gpe sokoban.gpe solo.gpe tents.gpe twiddle.gpe unequal.gpe untangle.gpe 

stppc2x: blackbox.gpo bridges.gpo combi.gpo cube.gpo dictionary.gpo divvy.gpo dominosa.gpo drawing.gpo dsf.gpo \
                fastevents.gpo fifteen.gpo filling.gpo flip.gpo galaxies.gpo grid.gpo guess.gpo inertia.gpo \
                iniparser.gpo latin.gpo lightup.gpo list.gpo loopy.gpo malloc.gpo map.gpo maxflow.gpo maze3d.gpo \
                maze3dc.gpo midend.gpo mines.gpo misc.gpo mosco.gpo net.gpo netslide.gpo pattern.gpo pegs.gpo printing.gpo \
                ps.gpo random.gpo rect.gpo samegame.gpo sdl.gpo sixteen.gpo slant.gpo slide.gpo sokoban.gpo \
                solo.gpo tents.gpo tree234.gpo twiddle.gpo unequal.gpo untangle.gpo version.gpo
	$(CPP) -o stppc2x.gpe blackbox.gpo bridges.gpo combi.gpo cube.gpo dictionary.gpo \
		divvy.gpo dominosa.gpo drawing.gpo dsf.gpo fastevents.gpo fifteen.gpo filling.gpo flip.gpo \
		galaxies.gpo grid.gpo guess.gpo inertia.gpo iniparser.gpo latin.gpo lightup.gpo list.gpo loopy.gpo \
		malloc.gpo map.gpo maxflow.gpo maze3d.gpo maze3dc.gpo midend.gpo mines.gpo misc.gpo mosco.gpo net.gpo \
		netslide.gpo pattern.gpo pegs.gpo printing.gpo ps.gpo random.gpo rect.gpo samegame.gpo sdl.gpo \
		sixteen.gpo slant.gpo slide.gpo sokoban.gpo solo.gpo tents.gpo tree234.gpo twiddle.gpo unequal.gpo \
		untangle.gpo version.gpo $(LDFLAGS)
	$(STRIP) stppc2x.gpe
	$(UPX) stppc2x.gpe

clean:
		rm *.gpo

# OBJECTS
# =======

blackbox.gpo: ./blackbox.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

bridges.gpo: ./bridges.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

combi.gpo: ./combi.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

cube.gpo: ./cube.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

dictionary.gpo: ./dictionary.c ./dictionary.h
	$(CC) $(CXXFLAGS) -c $< -o $@

divvy.gpo: ./divvy.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

dominosa.gpo: ./dominosa.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

drawing.gpo: ./drawing.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

dsf.gpo: ./dsf.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

fastevents.gpo: ./fastevents.c ./fastevents.h
	$(CC) $(CXXFLAGS) -c $< -o $@     

fifteen.gpo: ./fifteen.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

filling.gpo: ./filling.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

flip.gpo: ./flip.c ./puzzles.h ./tree234.h
	$(CC) $(CXXFLAGS) -c $< -o $@

galaxies.gpo: ./galaxies.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

grid.gpo: ./grid.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

guess.gpo: ./guess.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

inertia.gpo: ./inertia.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

iniparser.gpo: ./iniparser.c ./iniparser.h
	$(CC) $(CXXFLAGS) -c $< -o $@

latin.gpo: ./latin.c ./puzzles.h ./tree234.h ./maxflow.h ./latin.h
	$(CC) $(CXXFLAGS) -c $< -o $@

lightup.gpo: ./lightup.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

list.gpo: ./list.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

loopy.gpo: ./loopy.c ./grid.h ./puzzles.h ./tree234.h
	$(CC) $(CXXFLAGS) -c $< -o $@

malloc.gpo: ./malloc.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

map.gpo: ./map.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

maxflow.gpo: ./maxflow.c ./maxflow.h ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

maze3d.gpo: maze3d.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

maze3dc.gpo: maze3dc.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

midend.gpo: ./midend.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

mines.gpo: ./mines.c ./tree234.h ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

misc.gpo: ./misc.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

mosco.gpo: ./mosco.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

net.gpo: ./net.c ./puzzles.h ./tree234.h
	$(CC) $(CXXFLAGS) -c $< -o $@

netslide.gpo: ./netslide.c ./puzzles.h ./tree234.h
	$(CC) $(CXXFLAGS) -c $< -o $@

no-icon.gpo: ./no-icon.c
	$(CC) $(CXXFLAGS) -c $< -o $@

pattern.gpo: ./pattern.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

pegs.gpo: ./pegs.c ./puzzles.h ./tree234.h
	$(CC) $(CXXFLAGS) -c $< -o $@

printing.gpo: ./printing.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

ps.gpo: ./ps.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

random.gpo: ./random.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

rect.gpo: ./rect.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

samegame.gpo: ./samegame.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

sdl.gpo: ./sdl.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

sdl_large.gpo: ./sdl.c ./puzzles.h
	$(CC) $(CXXFLAGS) -DLARGE_SCREEN -c $< -o $@

sixteen.gpo: ./sixteen.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

slant.gpo: ./slant.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

slide.gpo: ./slide.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

sokoban.gpo: ./sokoban.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

solo.gpo: ./solo.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

tents.gpo: ./tents.c ./puzzles.h ./maxflow.h
	$(CC) $(CXXFLAGS) -c $< -o $@

tree234.gpo: ./tree234.c ./tree234.h ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

twiddle.gpo: ./twiddle.c ./puzzles.h
	$(CC) $(CXXFLAGS) -c $< -o $@

unequal.gpo: ./unequal.c ./puzzles.h ./latin.h
	$(CC) $(CXXFLAGS) -c $< -o $@

untangle.gpo: ./untangle.c ./puzzles.h ./tree234.h
	$(CC) $(CXXFLAGS) -c $< -o $@

version.gpo: ./version.c
	$(CC) $(CXXFLAGS) `cat version.def` -c $< -o $@;


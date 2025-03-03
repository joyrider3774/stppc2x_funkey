DEBUG = 1
SRC_DIR = source
OBJ_DIR = ./obj
EXE=stppc2x

SRCF=puzzles.c blackbox.c bridges.c combi.c cube.c dictionary.c divvy.c dominosa.c drawing.c dsf.c \
		fastevents.c fifteen.c filling.c flip.c galaxies.c grid.c guess.c inertia.c \
		iniparser.c latin.c lightup.c list.c loopy.c malloc.c map.c maxflow.c maze3d.c \
		maze3dc.c midend.c mines.c misc.c mosco.c net.c netslide.c pattern.o pegs.c \
		random.c rect.c samegame.c sdl.c sixteen.c slant.c slide.c sokoban.c \
		solo.c tents.c tree234.c twiddle.c unequal.c untangle.c version.c

SRC=$(addprefix $(SRC_DIR)/, $(SRCF))
OBJS=$(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))


CC ?= gcc
SDLCONFIG ?= sdl-config
CFLAGS ?= -Os -Wall -Wextra -DCOMBINED -DSLOW_SYSTEM
LDFLAGS ?= -lSDL_image -lSDL_ttf -lSDL_mixer -lmikmod -lSDL_gfx -lm

ifdef DEBUG
CFLAGS += -g
endif

ifdef TARGET
include $(TARGET).mk
endif

CFLAGS += `$(SDLCONFIG) --cflags`
LDFLAGS += `$(SDLCONFIG) --libs`

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $(TARGET_ARCH) $^ $(LDFLAGS) -o $@ 

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

clean:
	$(RM) -rv *~ $(OBJS) $(EXE)
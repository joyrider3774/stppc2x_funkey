DEBUG = 1
SRC_DIR = source
OBJ_DIR = obj
EXE = stppc2x

SRCF = puzzles.c blackbox.c bridges.c combi.c cube.c dictionary.c divvy.c dominosa.c drawing.c dsf.c \
       fastevents.c fifteen.c filling.c flip.c galaxies.c grid.c guess.c inertia.c \
       iniparser.c latin.c lightup.c list.c loopy.c malloc.c map.c maxflow.c maze3d.c \
       maze3dc.c midend.c mines.c misc.c mosco.c net.c netslide.c pattern.c pegs.c \
       random.c rect.c samegame.c sdl.c sixteen.c slant.c slide.c sokoban.c \
       solo.c tents.c tree234.c twiddle.c unequal.c untangle.c version.c

# Create object file names directly without using source paths
OBJF = $(SRCF:.c=.o)
OBJECTS = $(addprefix $(OBJ_DIR)/, $(OBJF))

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

all: prepare $(EXE)

prepare:
	mkdir -p $(OBJ_DIR)

$(EXE): $(OBJECTS)
	$(CC) $(CFLAGS) $(TARGET_ARCH) $^ $(LDFLAGS) -o $@

# Direct and explicit rule for each object file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(EXE)
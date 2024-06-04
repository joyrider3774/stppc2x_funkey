/*
 * mosco.c: implementation of a game from Ivan Moscovich's books called
 *           "Magic Arrow Tile Puzzles"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include "puzzles.h"

#define PREFERRED_TILE_SIZE 32
#define NO_OF_BASIC_TILES 8
#define TILE_BORDER_SIZE 3
#define NO_OF_TILE_SETS (w*h / NO_OF_BASIC_TILES)
#define ARROW_LARGE 1
#define ARROW_SMALL 0
enum {UP, UP_LEFT, LEFT, DOWN_LEFT, DOWN, DOWN_RIGHT, RIGHT, UP_RIGHT, NO_ARROW};
enum {HUP=1, HUP_LEFT=2, HLEFT=4, HDOWN_LEFT=8, HDOWN=16, HDOWN_RIGHT=32, HRIGHT=64, HUP_RIGHT=128};
char description[NO_OF_BASIC_TILES][3]={"U ", "UL", "L ", "DL", "D ", "DR", "R ", "UR"};

struct game_params
{
    int w, h;
};

struct affected_tiles_struct
{
    int x, y;
    int direction_to_affect;
};

struct clues_struct
{
    int x, y;
    struct affected_tiles_struct *affected_tiles;
};

struct game_state
{
    int w, h;
    unsigned char *grid; /* (w+2)x(h+2) */
    unsigned char *hints_grid; /* (w+2)x(h+2) */
    game_params *params; 
    int completed;
};

static void free_game(game_state *state)
{
    sfree(state->grid);
    sfree(state->hints_grid);
    sfree(state);
}

static game_state *dup_game(game_state *state)
{
    game_state *ret = snew(game_state);

    ret->w=state->w; 
    ret->h=state->h;
    ret->completed=state->completed;

    ret->grid = snewn((ret->w+2)*(ret->h+2)+1, unsigned char);
    memcpy(ret->grid, state->grid, (ret->w+2)*(ret->h+2) * sizeof(unsigned char));

    ret->hints_grid = snewn((ret->w+2)*(ret->h+2)+1, unsigned char);
    memcpy(ret->hints_grid, state->hints_grid, (ret->w+2)*(ret->h+2) * sizeof(unsigned char));

    ret->params=state->params;

    return ret;
}

void print_grid(unsigned char *grid, int w, int h)
{
    int i,j;
    for(j=0;j<(h+2);j++)
    {
        for(i=0;i<(w+2);i++)
        {
            printf("%u, ", grid[i + j *(h+2)]);
        };
        printf("\n");
    };    
};

#define GRIDTILE(x,y) state->grid[x + y *(state->h+2)]
void regenerate_clues_for_grid(game_state *state)
{
    int i;
    int j;

    // Blank the existing clues.

    for(j=0;j<(state->h+2);j++)
    {
        for(i=0;i<(state->w+2);i++)
        {
            if((i==0) || (i==(state->w+1)) || (j==0) || (j==(state->h+1)))
            {
                GRIDTILE(i,j)=0;
            };
        };
    };
            
    for(j=1;j<(state->h+1);j++)
    {
        for(i=1;i<(state->w+1);i++)
        {
            int x=0,y=0;
            if(GRIDTILE(i,j) < NO_ARROW)
            {
                switch(GRIDTILE(i,j))
                {
                    case UP_LEFT:
                        x=i;
                        y=j;

                        while( (x > 0) && (y > 0))
                        {
                           x--;
                           y--;
                        };
                        break;
    
                    case UP:
                        x=i;
                        y=j;
 
                    while( (y > 0))
                    {
                       y--;
                    };

                    break;

                case UP_RIGHT:
                    x=i;
                    y=j;

                    while( (x < (state->w+1)) && (y > 0))
                    {
                       x++;
                       y--;
                    };

                    break;

                case LEFT:
                    x=i;
                    y=j;

                    while( (x > 0))
                    {
                       x--;
                    };

                    break;

                case RIGHT:
                    x=i;
                    y=j;

                    while(x < (state->w+1))
                    {
                       x++;
                    };

                    break;

                case DOWN_LEFT: 
                    x=i;
                    y=j;

                    while( (x > 0) && (y < state->h+1))
                    {
                       x--;
                       y++;
                    };

                    break;

                case DOWN:
                    x=i;
                    y=j;

                    while(y < (state->h+1))
                    {
                       y++;
                    };

                    break;

                case DOWN_RIGHT:
                    x=i;
                    y=j;

                    while( (x < (state->w+1)) && (y < (state->h+1)))
                    {
                       x++;
                       y++;
                    };

                    break;
                };
                GRIDTILE(x,y)++;
             };
        };
    };

}
#undef GRIDTILE

#define GRIDTILE(x,y) grid[x + y *(h+2)]

int clue_is_satisfied(game_state *state, int x, int y)
{
    int ret;
    game_state *recalc_game_state;

    assert( (x==0) || (x==(state->w+1)) || (y==0) || (y==(state->h+1)) );

    recalc_game_state=dup_game(state);
    regenerate_clues_for_grid(recalc_game_state);

    if(recalc_game_state->grid[x + y *(state->h+2)] == state->grid[x+y*(state->h+2)])
    {
        ret = 0;
    }
    else if(recalc_game_state->grid[x + y *(state->h+2)] > state->grid[x+y*(state->h+2)])
    {
        ret = 1;
    }
    else
    {
        ret=-1;
    };
    
    free_game(recalc_game_state);

    return(ret);
};

//returns TRUE if the game is won
//returns FALSE with x,y if there is an error in the clues
int check_if_finished(game_state *state, int *x, int *y)
{
    int i, j;
    int clues_correct=0;
    int arrows_filled=0;
    game_state *recalc_game_state;
    recalc_game_state=dup_game(state);
    regenerate_clues_for_grid(recalc_game_state);

#ifdef DEBUG
    printf("Regenerated\n");
    print_grid(recalc_game_state->grid, recalc_game_state->w, recalc_game_state->h);
    printf("\n");
    printf("Original \n");
    print_grid(state->grid, state->w, state->h);
    printf("\n");
#endif

    *x=-1;
    *y=-1;

    for(i=0; i<(state->w+2); i++)
    {
        for(j=0;j<(state->h+2);j++)
        {
            if((i==0) || (i==(state->w+1)) || (j==0) || (j==(state->h+1)))
            {
                if(recalc_game_state->grid[i + j *(state->h+2)] > state->grid[i+j*(state->h+2)])
                {
                    *x=i;
                    *y=j;
#ifdef DEBUG
    printf("%i arrows pointing at %i,%i (should be %i)\n", recalc_game_state->grid[i + j *(state->h+2)], *x, *y, state->grid[i+j*(state->h+2)]);
#endif
                }
                else if(recalc_game_state->grid[i + j *(state->h+2)] == state->grid[i+j*(state->h+2)])
                {
                    clues_correct++;
                };
            }
            else
            {
                if(state->grid[i+j*(state->h+2)] != NO_ARROW)
                    arrows_filled++;
            };
        };
    };

    free_game(recalc_game_state);

    if((*x >= 0) || (*y >= 0))
    {
#ifdef DEBUG
        printf("Grid has a clue error at %i, %i\n", *x, *y);
#endif
        return(FALSE);
    };

    if(clues_correct != (2*state->w + 2*state->h +4))
    {
#ifdef DEBUG
        printf("Grid only has %i clues satisfied\n", clues_correct);
#endif
        return(FALSE);
    };

    if(arrows_filled != (state->w * state->h))
    {
#ifdef DEBUG
        printf("Grid only has %i arrows\n", arrows_filled);
#endif
        return(FALSE);
    };

#ifdef DEBUG
    printf("Grid is completed and correct\n");
#endif
    state->completed=TRUE;
    return(TRUE);
};

#undef GRIDTILE


static game_params *default_params(void)
{
    game_params *ret = snew(game_params);
    ret->w = ret->h = 4;
    return ret;
}

// Any area that is a multiple of NO_OF_BASIC_TILES that can fit inside a grid.
// {4,2} is valid but there's a bug somewhere that stops it playing ball.
static const game_params moscow_presets[] = {
    {4, 4},
    {8, 8},
    {16, 16},
};

static int game_fetch_preset(int i, char **name, game_params **params)
{
    char str[80];
    game_params *ret;

    if (i < 0 || i >= lenof(moscow_presets))
        return FALSE;

    ret = snew(game_params);
    *ret = moscow_presets[i];

    sprintf(str, "%dx%d", ret->w, ret->h);

    *name = dupstr(str);
    *params = ret;
    return TRUE;
}

static void free_params(game_params *params)
{
    sfree(params);
}

static game_params *dup_params(game_params *params)
{
    game_params *ret = snew(game_params);
    *ret = *params;		       /* structure copy */
    return ret;
}

static void decode_params(game_params *params, char const *string)
{
    char const *p = string;
    game_params *defs = default_params();

    *params = *defs; free_params(defs);

    while (*p) {
        switch (*p++) {
        case 'w':
            params->w = atoi(p);
            while (*p && isdigit((unsigned char)*p)) p++;
            break;

        case 'h':
            params->h = atoi(p);
            while (*p && isdigit((unsigned char)*p)) p++;
            break;

        default:
            ;
        }
    }
}

static char *encode_params(game_params *params, int full)
{
    char str[256];

    sprintf(str, "w%dh%d", params->w, params->h);
    return dupstr(str);
}

static config_item *game_configure(game_params *params)
{
    config_item *ret;
    char buf[80];

    ret = snewn(3, config_item);

    ret[0].name = "Width";
    ret[0].type = C_STRING;
    sprintf(buf, "%d", params->w);
    ret[0].sval = dupstr(buf);
    ret[0].ival = 0;

    ret[1].name = "Height";
    ret[1].type = C_STRING;
    sprintf(buf, "%d", params->h);
    ret[1].sval = dupstr(buf);
    ret[1].ival = 0;

    ret[2].name = NULL;
    ret[2].type = C_END;
    ret[2].sval = NULL;
    ret[2].ival = 0;

    return ret;
}

static game_params *custom_params(config_item *cfg)
{
    game_params *ret = snew(game_params);

    ret->w = atoi(cfg[0].sval);
    ret->h = atoi(cfg[1].sval);

    return ret;
}

static char *validate_params(game_params *params, int full)
{
    if (params->w < 4 || params->h < 4)
        return "Width and height must both be at least four";
    if (((params->w * params->h) % NO_OF_BASIC_TILES)!=0)
        return "w * h must be a multiple of "STR(NO_OF_BASIC_TILES)".";
    if (params->w > 255 || params->h > 255)
        return "Widths and heights greater than 255 are not supported";
    return NULL;
}

#define GRIDTILE(x,y) grid[x + y *(h+2)]
#define ARENATILE(x,y) grid[(x+1) + (y+1)*(h+2)]

static char *new_game_desc(game_params *params, random_state *rs, char **aux, int interactive)
{
    int i,j;
    int w,h;
#ifdef DEBUG
    unsigned int total_tiles=0;
#endif
    unsigned char *grid;
    unsigned char *tileset;
    unsigned char *game_desc;
    char *game_desc_string;

    w=params->w;
    h=params->h;

    tileset = snewn(w * h, unsigned char);
    memset(tileset,0, w*h*sizeof(unsigned char));

    for(j=0;j<NO_OF_TILE_SETS;j++)
        for(i=0;i<NO_OF_BASIC_TILES;i++)
            tileset[i + j*NO_OF_BASIC_TILES] = i;
#ifdef DEBUG
    printf("Tiles: ");
    for(i=0;i<w*h;i++)
    {
        printf("%u, ", tileset[i]);
    };
    printf("\n");
#endif

    shuffle(tileset, w*h, sizeof(unsigned char), rs);

#ifdef DEBUG
    printf("Shuffled Tiles: ");
    for(i=0;i<w*h;i++)
    {
        printf("%u, ", tileset[i]);
    };
    printf("\n");
#endif

    grid = snewn((w +2) * (h + 2) + 1, unsigned char);
    memset(grid, 0, ((w + 2) * (h + 2) + 1) *sizeof(unsigned char));

    for(i=0;i<w;i++)
    {
        for(j=0;j<h;j++)
        {
            ARENATILE(i,j) = tileset[i + j*h];
#ifdef DEBUG
            printf("%u, ", ARENATILE(i,j));
#endif
        };
#ifdef DEBUG
        printf("\n");
#endif
    };

#ifdef DEBUG
    printf("\n");
    printf("\n");
    print_grid(grid, w, h);
#endif

    for(j=1;j<(h+1);j++)
    {
        for(i=1;i<(w+1);i++)
        {
            int x=0,y=0;
            switch(GRIDTILE(i,j))
            {
                case UP_LEFT:
                    x=i;
                    y=j;
#ifdef DEBUG
                    printf("UL: x, y: %u, %u - ", x, y);
#endif

                    while( (x > 0) && (y > 0))
                    {
                       x--;
                       y--;
                    };
#ifdef DEBUG
                    printf("%u, %u  -  ", x, y);
#endif
                    break;

                case UP:
                    x=i;
                    y=j;
#ifdef DEBUG
                    printf("U : x, y: %u, %u - ", x, y);
#endif

                    while( (y > 0))
                    {
                       y--;
                    };

#ifdef DEBUG
                    printf("%u, %u  -  ", x, y);
#endif
                    break;

                case UP_RIGHT:
                    x=i;
                    y=j;
#ifdef DEBUG
                    printf("UR: x, y: %u, %u - ", x, y);
#endif

                    while( (x < (w+1)) && (y > 0))
                    {
                       x++;
                       y--;
                    };

#ifdef DEBUG
                    printf("%u, %u  -  ", x, y);
#endif
                    break;

                case LEFT:
                    x=i;
                    y=j;

#ifdef DEBUG
                    printf("L : x, y: %u, %u - ", x, y);
#endif

                    while( (x > 0))
                    {
                       x--;
                    };

#ifdef DEBUG
                    printf("%u, %u  -  ", x, y);
#endif
                    break;

                case RIGHT:
                    x=i;
                    y=j;
#ifdef DEBUG
                    printf("R : x, y: %u, %u - ", x, y);
#endif

                    while(x < (w+1))
                    {
                       x++;
                    };
#ifdef DEBUG
                    printf("%u, %u  -  ", x, y);
#endif
                    break;

                case DOWN_LEFT: 
                    x=i;
                    y=j;
#ifdef DEBUG
                    printf("DL: x, y: %u, %u - ", x, y);
#endif

                    while( (x > 0) && (y < h+1))
                    {
                       x--;
                       y++;
                    };
#ifdef DEBUG
                    printf("%u, %u  -  ", x, y);
#endif
                    break;

                case DOWN:
                    x=i;
                    y=j;
#ifdef DEBUG
                    printf("D : x, y: %u, %u - ", x, y);
#endif

                    while(y < (h+1))
                    {
                       y++;
                    };
#ifdef DEBUG
                    printf("%u, %u  -  ", x, y);
#endif
                    break;

                case DOWN_RIGHT:
                    x=i;
                    y=j;
#ifdef DEBUG
                    printf("DR: x, y: %u, %u - ", x, y);
#endif

                    while( (x < (w+1)) && (y < (h+1)))
                    {
                       x++;
                       y++;
                    };
#ifdef DEBUG
                    printf("%u, %u  -  ", x, y);
#endif
                    break;

                default:
                    fatal("Hit a invalid arrow type\n");
            };
            GRIDTILE(x,y)++;
        };
#ifdef DEBUG
        printf("\n");
#endif
    };

#ifdef DEBUG
    printf("\n");
    printf("\n");
   
    for(j=0;j<(h+2);j++)
    {
        for(i=0;i<(w+2);i++)
        {
            if((i>0) && (i<(w+1)) && (j>0) && (j<(h+1)))
                printf("%s, ", description[GRIDTILE(i,j)]);
            else
            {
                printf("%u , ", GRIDTILE(i,j));
                total_tiles+=GRIDTILE(i,j);
            };
        };
        printf("\n");
    };
    printf("Total tiles: %u\n",total_tiles);

    // The total count of the game hints at the edges should alway equal the size of the game grid.
    // If they don't, we've buggered up generation of the hints.
    assert(total_tiles == (w * h)); 
   
#endif

    game_desc = snewn((w+2)*(h+2), unsigned char);
    for(i=0; i < (w+2)*(h+2);i++)
        game_desc[i]=grid[i];

    game_desc_string=bin2hex(game_desc, (w+2)*(h+2));
    *aux=dupstr(game_desc_string);

#ifdef DEBUG
    printf("Game description: %s\n", game_desc_string);
#endif

    sfree(tileset);
    sfree(grid);
    sfree(game_desc);

    return(game_desc_string);
}

#undef GRIDTILE

static char *validate_desc(game_params *params, char *desc)
{
    char *ret;
    ret = NULL;
    return ret;
}

#define GRIDTILE(x,y) state->grid[x + y *(params->h+2)]
#define HINTSGRIDTILE(x,y) state->hints_grid[x + y *(params->h+2)]

static game_state *new_game(midend *me, game_params *params, char *desc)
{
    int i,j;
    game_state *state = snew(game_state);
    unsigned char *game_desc;
    state->grid = snewn((params->w+2)*(params->h+2)+1, unsigned char);
    state->hints_grid = snewn((params->w+2)*(params->h+2)+1, unsigned char);

    state->w=params->w;
    state->h=params->h;
    state->completed=FALSE;

    game_desc=hex2bin(desc, (params->w+2)*(params->h+2));

    for(i=0; i < (params->w+2)*(params->h+2);i++)
        state->grid[i]=game_desc[i];
    sfree(game_desc);

    for(i=0;i<(params->w+2);i++)
        for(j=0;j<(params->h+2);j++)
        {
            if((i>0) && (i<(params->w+1)) && (j>0) && (j<(params->h+1)))
            {
                GRIDTILE(i,j)=NO_ARROW;
                HINTSGRIDTILE(i,j)=0;
            };
        };

    state->grid[(params->w+2)*(params->h+2)]=0;
    state->hints_grid[(params->w+2)*(params->h+2)]=0;

    state->params=params;
    return state;
}
#undef HINTSGRIDTILE
#undef GRIDTILE



#define GRIDTILE(x,y) state->grid[x + y *(state->h+2)]
static char *solve_game(game_state *state, game_state *currstate,char *aux, char **error)
{
    char *move_string;

    if(state->completed)
    {
        *error = "Game already complete.";
        return(NULL);
    };

    move_string=snewn(32768, char);
    memset(move_string, 0, 32768);

#ifdef SOLVER_DEBUG
    int i,j,n;
    clues_struct clues=snewn((state->w*2) + (state->h+2), struct clues_struct);

    n=0;
    for(i=0;i<state->w+2;i++)
    {
        for(j=0;j<state->h+2;j++)
        {
            if((i==0) || (i==state->w+2) || (j==0) || (j==(state->h+2)))
            {
                affected_tiles[n]->x=i;
                affected_tiles[n]->y=j;
            };
        };
    };

#endif
    // Cheat - send a string copy of the game as it was generated to execute_move.
    strcat(move_string, "S");
    strcat(move_string, aux);

    return(move_string);
}
#undef GRIDTILE

static int game_can_format_as_text_now(game_params *params)
{
    return TRUE;
}

static char *game_text_format(game_state *state)
{
    return NULL;
}

struct game_ui {
    int errors;
};

static game_ui *new_ui(game_state *state)
{
    game_ui *ui = snew(game_ui);
    ui->errors = 0;
    return ui;
}

static void free_ui(game_ui *ui)
{
    sfree(ui);
}

static char *encode_ui(game_ui *ui)
{
    char buf[80];
    /*
     * The error counter needs preserving across a serialisation.
     */
    sprintf(buf, "E%d", ui->errors);
    return dupstr(buf);
}

static void decode_ui(game_ui *ui, char *encoding)
{
    sscanf(encoding, "E%d", &ui->errors);
}

static void game_changed_state(game_ui *ui, game_state *oldstate,
                               game_state *newstate)
{

}

struct game_drawstate
{
    int tilesize, w, h;
    unsigned int *grid;          /* as the game_state grid */
};

#define GRIDTILE(x,y) state->grid[x + y *(state->h+2)]
#define HINTSGRIDTILE(x,y) state->hints_grid[x + y *(state->h+2)]

static char *interpret_move(game_state *state, game_ui *ui, game_drawstate *ds,
			    int x, int y, int button)
{
    char *ret;
    ret=snewn(255, char);
    ret[0]='\0';

    if(button == LEFT_BUTTON || button == MIDDLE_BUTTON || button == RIGHT_BUTTON)
    {
        int x_square = x / ds->tilesize;
        int y_square = y / ds->tilesize;
        int x_offset = x - (x_square * ds->tilesize);
        int y_offset = y - (y_square * ds->tilesize);
        int up, down, left, right;
        int direction=0;

        up = (y_offset < (ds->tilesize/3));
        down = (y_offset > (ds->tilesize*2/3));
        left = (x_offset < (ds->tilesize/3));
        right = (x_offset > (ds->tilesize*2/3));

        if((x_square>0) && (x_square<(state->w+1)) && (y_square>0) && (y_square<(state->h+1)))
        {
            switch(button)
            {
                case LEFT_BUTTON:
                    sprintf(ret, "G%i-%i", x_square, y_square);
                    return(ret);
                    break;

                case MIDDLE_BUTTON:
                    sprintf(ret, "Z%i-%i", x_square, y_square);
                    return(ret);
                    break;
          
                case RIGHT_BUTTON:
                    if(up)
                        direction=HUP;
                    if(up && left)
                        direction=HUP_LEFT;
                    if(up && right)
                        direction=HUP_RIGHT;
                    if(down)
                        direction=HDOWN;
                    if(down && left)
                        direction=HDOWN_LEFT;
                    if(down && right)
                        direction=HDOWN_RIGHT;
                    if((direction == 0) && left)
                        direction=HLEFT;
                    if((direction == 0) && right)
                        direction=HRIGHT;

                    sprintf(ret, "H%i-%i-%i", x_square, y_square,direction);
                    return(ret);
            };
#ifdef DEBUG
            printf("Move: %s\n", ret);
#endif
        };
    };
    sfree(ret);
    return(NULL);
}

#undef HINTSGRIDTILE
#undef GRIDTILE

#define GRIDTILE(x,y) ret->grid[x + y *(ret->h+2)]
#define HINTSGRIDTILE(x,y) ret->hints_grid[x + y *(ret->h+2)]

static game_state *execute_move(game_state *from, char *move)
{
    int x_square, y_square;
    int x,y;
    int direction;
    game_state *ret;

    ret=dup_game(from);

    // Clear a square.
    if(move[0] == 'Z')
    {
        if(sscanf(move, "Z%i-%i", &x_square, &y_square) < 2)
        {
            return(NULL);
        }
        else
        {
            GRIDTILE(x_square,y_square)=NO_ARROW;
            HINTSGRIDTILE(x_square,y_square)=0;
            check_if_finished(ret, &x, &y);
            return(ret);
        };
    };

    // Add a arrow to a square.
    if(move[0] == 'G')
    {
        if(sscanf(move, "G%i-%i", &x_square, &y_square) < 2)
        {
            return(NULL);
        }
        else
        {
            GRIDTILE(x_square,y_square)=(GRIDTILE(x_square,y_square)+1) % NO_OF_BASIC_TILES;
            HINTSGRIDTILE(x_square,y_square)=0;
            check_if_finished(ret, &x, &y);
            return(ret);
        };
    };

    // Add a hint to a square.
    if(move[0] == 'H')
    {
        if(sscanf(move, "H%i-%i-%i", &x_square, &y_square, &direction) < 3)
        {
            return(NULL);
        }
        else
        {
            GRIDTILE(x_square,y_square)=NO_ARROW;
            if(direction > 0)
                HINTSGRIDTILE(x_square,y_square)^=direction;
            check_if_finished(ret, &x, &y);
            return(ret);
        };
    };

/*
    // Multiple moves in one string!
    if(move[0] == 'M')
    {
        char *next_string;

        next_string=strtok(move, ",");
        while(next_string != NULL)
        {
            execute_move(next_string);
            next_string=strtok(move, NULL);
        };
    };
*/

    // Solve the game.
    if(move[0] == 'S')
    {
        int i;
        char *game_desc = dupstr(&move[1]);
        unsigned char *decoded_game_desc;

        decoded_game_desc=hex2bin(game_desc, (ret->w+2)*(ret->h+2));

        for(i=0; i < (ret->w+2)*(ret->h+2);i++)
        {
            ret->grid[i]=decoded_game_desc[i];
            ret->hints_grid[i]=0;
        };
        sfree(decoded_game_desc);
        sfree(game_desc);
        return(ret);
    };
    return(NULL);
}
#undef HINTSGRIDTILE
#undef GRIDTILE

/* ----------------------------------------------------------------------
 * Drawing routines.
 */

static void game_compute_size(game_params *params, int tilesize, int *x, int *y)
{
    *x = (params->w+2) * tilesize;
    *y = (params->h+2) * tilesize;
}

static void game_set_size(drawing *dr, game_drawstate *ds,
			  game_params *params, int tilesize)
{
    ds->tilesize = tilesize;
}

static float *game_colours(frontend *fe, int *ncolours)
{
    float *ret;
    *ncolours=11;

    ret=snewn(*ncolours * 3, float);

// Colour 0 - Background
    frontend_default_colour(fe, &ret[0]);

// Colour 1, 2 - high/lowlights
    game_mkhighlight(fe, ret, 0, 1, 2);

// Colour 3 - Black
    ret[9] = 0.0F;
    ret[10] = 0.0F;
    ret[11] = 0.0F;

// Colour 4 - White
    ret[12] = 1.0F;
    ret[13] = 1.0F;
    ret[14] = 1.0F;

// Colour 5 - grid colour
    ret[15] = ret[0] * 0.9F;
    ret[16] = ret[1] * 0.9F;
    ret[17] = ret[2] * 0.9F;

// Colour 6 - Red square
    ret[18] = ret[0] * 0.9F;
    ret[19] = ret[1] * 0.5F;
    ret[20] = ret[2] * 0.5F;

// Colour 7 - Yellow square
    ret[21] = ret[0] * 0.9F;
    ret[22] = ret[1] * 0.9F;
    ret[23] = ret[2] * 0.5F;

// Colour 8 - Green square
    ret[24] = ret[0] * 0.5F;
    ret[25] = ret[1] * 0.9F;
    ret[26] = ret[2] * 0.3F;

// Colour 9 - Blue square
    ret[27] = ret[0] * 0.5F;
    ret[28] = ret[1] * 0.5F;
    ret[29] = ret[2] * 0.9F;

// Colour 10 - Red
    ret[30] = 1.0F;
    ret[31] = 0.0F;
    ret[32] = 0.0F;

    return ret;
}

static game_drawstate *game_new_drawstate(drawing *dr, game_state *state)
{
    struct game_drawstate *ds = snew(struct game_drawstate);

    ds->tilesize = PREFERRED_TILE_SIZE;
    ds->w = state->w;
    ds->h = state->h;
    ds->grid = snewn((state->w+2)*(state->h+2), unsigned int);
    memset(ds->grid, 0, (state->w+2)*(state->h+2)*sizeof(unsigned int));

    return ds;
}

static void game_free_drawstate(drawing *dr, game_drawstate *ds)
{
    sfree(ds->grid);
    sfree(ds);
}

void draw_arrow(drawing *dr, game_drawstate *ds, int x, int y, int size, int direction)
{
    int base_x=x*ds->tilesize;
    int base_y=y*ds->tilesize;

    clip(dr, x*ds->tilesize, y*ds->tilesize, ds->tilesize, ds->tilesize);

    if(size == ARROW_LARGE)
    {
#define TOP_LEFT base_x+1, base_y+1
#define TOP_MIDDLE base_x+(ds->tilesize/2), base_y+1
#define TOP_RIGHT base_x +ds->tilesize-1, base_y+1

#define MIDDLE_LEFT base_x+1, base_y+(ds->tilesize/2)
#define MIDDLE_MIDDLE base_x+(ds->tilesize/2), base_y+(ds->tilesize/2)
#define MIDDLE_RIGHT base_x +ds->tilesize-1, base_y+(ds->tilesize/2)

#define BOTTOM_LEFT base_x+1, base_y+ds->tilesize-1
#define BOTTOM_MIDDLE base_x+(ds->tilesize/2), base_y+ds->tilesize-1
#define BOTTOM_RIGHT base_x +ds->tilesize-1, base_y+ds->tilesize-1

    switch(direction)
    {
        case UP_LEFT:
            draw_rect(dr, x*ds->tilesize+TILE_BORDER_SIZE, y*ds->tilesize+TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, 8);
            draw_line(dr, TOP_LEFT, BOTTOM_RIGHT, 3);
            draw_line(dr, TOP_LEFT, MIDDLE_LEFT, 3);
            draw_line(dr, TOP_LEFT, TOP_MIDDLE, 3);
            break;

        case UP:
            draw_rect(dr, x*ds->tilesize+TILE_BORDER_SIZE, y*ds->tilesize+TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, 9);
            draw_line(dr, TOP_MIDDLE, BOTTOM_MIDDLE, 3);
            draw_line(dr, TOP_MIDDLE, MIDDLE_LEFT, 3);
            draw_line(dr, TOP_MIDDLE, MIDDLE_RIGHT, 3);
            break;

        case UP_RIGHT:
            draw_rect(dr, x*ds->tilesize+TILE_BORDER_SIZE, y*ds->tilesize+TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, 7);
            draw_line(dr, TOP_RIGHT, BOTTOM_LEFT, 3);
            draw_line(dr, TOP_RIGHT, TOP_MIDDLE, 3);
            draw_line(dr, TOP_RIGHT, MIDDLE_RIGHT, 3);
            break;

        case LEFT:
            draw_rect(dr, x*ds->tilesize+TILE_BORDER_SIZE, y*ds->tilesize+TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, 6);
            draw_line(dr, MIDDLE_LEFT, MIDDLE_RIGHT, 3);
            draw_line(dr, MIDDLE_LEFT, TOP_MIDDLE, 3);
            draw_line(dr, MIDDLE_LEFT, BOTTOM_MIDDLE, 3);
            break;

        case RIGHT:
            draw_rect(dr, x*ds->tilesize+TILE_BORDER_SIZE, y*ds->tilesize+TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, 6);
            draw_line(dr, MIDDLE_RIGHT, MIDDLE_LEFT, 3);
            draw_line(dr, MIDDLE_RIGHT, BOTTOM_MIDDLE, 3);
            draw_line(dr, MIDDLE_RIGHT, TOP_MIDDLE, 3);
            break;

        case DOWN_LEFT:
            draw_rect(dr, x*ds->tilesize+TILE_BORDER_SIZE, y*ds->tilesize+TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, 7);
            draw_line(dr, BOTTOM_LEFT, TOP_RIGHT, 3);
            draw_line(dr, BOTTOM_LEFT, BOTTOM_MIDDLE, 3);
            draw_line(dr, BOTTOM_LEFT, MIDDLE_LEFT, 3);
            break;

        case DOWN:
            draw_rect(dr, x*ds->tilesize+TILE_BORDER_SIZE, y*ds->tilesize+TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, 9);
            draw_line(dr, BOTTOM_MIDDLE, TOP_MIDDLE, 3);
            draw_line(dr, BOTTOM_MIDDLE, MIDDLE_LEFT, 3);
            draw_line(dr, BOTTOM_MIDDLE, MIDDLE_RIGHT, 3);
            break;

        case DOWN_RIGHT:
            draw_rect(dr, x*ds->tilesize+TILE_BORDER_SIZE, y*ds->tilesize+TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, ds->tilesize-2*TILE_BORDER_SIZE, 8);
            draw_line(dr, BOTTOM_RIGHT, TOP_LEFT, 3);
            draw_line(dr, BOTTOM_RIGHT, BOTTOM_MIDDLE, 3);
            draw_line(dr, BOTTOM_RIGHT, MIDDLE_RIGHT, 3);
            break;
     
        case NO_ARROW:
        default:
            break;
    };

#undef TOP_LEFT 
#undef TOP_MIDDLE
#undef TOP_RIGHT
#undef MIDDLE_LEFT
#undef MIDDLE_MIDDLE
#undef MIDDLE_RIGHT
#undef BOTTOM_LEFT
#undef BOTTOM_MIDDLE
#undef BOTTOM_RIGHT
    }
    else
    {
#define TOP_LEFT(x,y) base_x+x+1, base_y+y+1
#define TOP_MIDDLE(x,y) base_x+(ds->tilesize/2)+x, base_y+y+1
#define TOP_RIGHT(x,y) base_x+ds->tilesize+x-1, base_y+y+1

#define MIDDLE_LEFT(x,y) base_x+x+1, base_y+(ds->tilesize/2)+y
#define MIDDLE_MIDDLE(x,y) base_x+(ds->tilesize/2)+x, base_y+(ds->tilesize/2)+y
#define MIDDLE_RIGHT(x,y) base_x+ds->tilesize+x-1, base_y+(ds->tilesize/2)+y

#define BOTTOM_LEFT(x,y) base_x+x+1, base_y+y+ds->tilesize-1
#define BOTTOM_MIDDLE(x,y) base_x+(ds->tilesize/2)+x, base_y+ds->tilesize+y-1
#define BOTTOM_RIGHT(x,y) base_x +ds->tilesize+x-1, base_y+ds->tilesize+y-1

        if(direction & HUP_LEFT)
        {
            draw_line(dr, TOP_LEFT(0,0), TOP_LEFT(5,0), 3);
            draw_line(dr, TOP_LEFT(0,0), TOP_LEFT(0,5), 3);
            draw_line(dr, TOP_LEFT(0,0), TOP_LEFT(5,5), 3);
        };

        if(direction & HUP)
        {
            draw_line(dr, TOP_MIDDLE(0,0), TOP_MIDDLE(0,5), 3);
            draw_line(dr, TOP_MIDDLE(0,0), TOP_MIDDLE(-3,3), 3);
            draw_line(dr, TOP_MIDDLE(0,0), TOP_MIDDLE(3,3), 3);
        };

        if(direction & HUP_RIGHT)
        {
            draw_line(dr, TOP_RIGHT(0,0), TOP_RIGHT(-5,5), 3);
            draw_line(dr, TOP_RIGHT(0,0), TOP_RIGHT(0,5), 3);
            draw_line(dr, TOP_RIGHT(0,0), TOP_RIGHT(-5,0), 3);
        };

        if(direction & HLEFT)
        {
            draw_line(dr, MIDDLE_LEFT(0,0), MIDDLE_LEFT(5,0), 3);
            draw_line(dr, MIDDLE_LEFT(0,0), MIDDLE_LEFT(3,3), 3);
            draw_line(dr, MIDDLE_LEFT(0,0), MIDDLE_LEFT(3,-3), 3);
        };

        if(direction & HRIGHT)
        {
            draw_line(dr, MIDDLE_RIGHT(0,0), MIDDLE_RIGHT(-5,0), 3);
            draw_line(dr, MIDDLE_RIGHT(0,0), MIDDLE_RIGHT(-3,3), 3);
            draw_line(dr, MIDDLE_RIGHT(0,0), MIDDLE_RIGHT(-3,-3), 3);
        };

        if(direction & HDOWN_LEFT)
        {
            draw_line(dr, BOTTOM_LEFT(0,0), BOTTOM_LEFT(5,-5), 3);
            draw_line(dr, BOTTOM_LEFT(0,0), BOTTOM_LEFT(5,0), 3);
            draw_line(dr, BOTTOM_LEFT(0,0), BOTTOM_LEFT(0,-5), 3);
        };

        if(direction & HDOWN)
        {
            draw_line(dr, BOTTOM_MIDDLE(0,0), BOTTOM_MIDDLE(0,-5), 3);
            draw_line(dr, BOTTOM_MIDDLE(0,0), BOTTOM_MIDDLE(3,-3), 3);
            draw_line(dr, BOTTOM_MIDDLE(0,0), BOTTOM_MIDDLE(-3,-3), 3);
        };

        if(direction & HDOWN_RIGHT)
        {
            draw_line(dr, BOTTOM_RIGHT(0,0), BOTTOM_RIGHT(-5,-5), 3);
            draw_line(dr, BOTTOM_RIGHT(0,0), BOTTOM_RIGHT(-5,0), 3);
            draw_line(dr, BOTTOM_RIGHT(0,0), BOTTOM_RIGHT(0,-5), 3);
        };
    };
    unclip(dr);
};


#define GRIDTILE(x,y) state->grid[x + y *(state->h+2)]
#define HINTSGRIDTILE(x,y) state->hints_grid[x + y *(state->h+2)]
static void game_redraw(drawing *dr, game_drawstate *ds, game_state *oldstate,
			game_state *state, int dir, game_ui *ui,
			float animtime, float flashtime)
{
    int i,j;
    int error_clue_x, error_clue_y;
    unsigned int tile_count[NO_OF_BASIC_TILES+1];

    for(i=0;i<(NO_OF_BASIC_TILES+1);i++)
        tile_count[i]=0;

    for(i=0;i<state->w+2;i++)
    {
        for(j=0;j<state->h+2;j++)
        {
            if((i>0) && (i<(state->w+1)) && (j>0) && (j<(state->h+1)))
                tile_count[GRIDTILE(i,j)]++;
        };
    };

    for(i=0;i<state->w+2;i++)
    {
        for(j=0;j<state->h+2;j++)
        {
            draw_rect(dr, i*ds->tilesize, j*ds->tilesize, ds->tilesize+1, ds->tilesize+1, 5);
            draw_rect_outline(dr, i*ds->tilesize, j*ds->tilesize, ds->tilesize+1, ds->tilesize+1, 2);
            if((i>0) && (i<(state->w+1)) && (j>0) && (j<(state->h+1)))
            {
                draw_arrow(dr, ds, i, j, ARROW_LARGE, GRIDTILE(i,j));
                draw_arrow(dr, ds, i, j, ARROW_SMALL, HINTSGRIDTILE(i,j));

                if((GRIDTILE(i,j) < NO_ARROW) && (tile_count[GRIDTILE(i,j)] > (state->w*state->h / NO_OF_BASIC_TILES)))
                    draw_rect_outline(dr, i*ds->tilesize, j*ds->tilesize, ds->tilesize, ds->tilesize, 10);
            }
            else
            {
                char *number_as_string=snewn(3, char);
                int colour=3;
                int clue_correct = clue_is_satisfied(state, i, j);
 
                if(clue_correct == 0)
                {
                    colour=8;
                }
                else if(clue_correct >0)
                {
                    colour=10;
                };

                sprintf(number_as_string, "%u", GRIDTILE(i,j));

                draw_text(dr, (i+0.25)*ds->tilesize, (j+1)*ds->tilesize, FONT_VARIABLE, ds->tilesize /2, ALIGN_VCENTRE & ALIGN_HCENTRE, colour, number_as_string);
                sfree(number_as_string);
            };
        };
    };

    if(check_if_finished(state, &error_clue_x, &error_clue_y))
    {
        status_bar(dr, "Completed!");
    }
    else
    {
//        draw_rect_outline(dr, i*ds->tilesize, j*ds->tilesize, ds->tilesize, ds->tilesize, 10);
    };
}

#undef GRIDTILE

static float game_anim_length(game_state *oldstate, game_state *newstate,
			      int dir, game_ui *ui)
{
    return 0.0F;
}

static float game_flash_length(game_state *oldstate, game_state *newstate,
			       int dir, game_ui *ui)
{
    return 0.0F;
}

static int game_timing_state(game_state *state, game_ui *ui)
{
    return TRUE;
}


#ifdef COMBINED
#define thegame mosco
#endif

const struct game thegame = {
    "Mosco", "games.mosco", "mosco",
    default_params,
    game_fetch_preset,
    decode_params,
    encode_params,
    free_params,
    dup_params,
    TRUE, game_configure, custom_params,
    validate_params,
    new_game_desc,
    validate_desc,
    new_game,
    dup_game,
    free_game,
    TRUE, solve_game,
    FALSE, game_can_format_as_text_now, game_text_format,
    new_ui,
    free_ui,
    encode_ui,
    decode_ui,
    game_changed_state,
    interpret_move,
    execute_move,
    PREFERRED_TILE_SIZE, game_compute_size, game_set_size,
    game_colours,
    game_new_drawstate,
    game_free_drawstate,
    game_redraw,
    game_anim_length,
    game_flash_length,
    FALSE, FALSE, NULL, NULL,
    TRUE,			       /* wants_statusbar */
    FALSE, game_timing_state,
    0,		       /* flags */
};

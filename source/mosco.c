/*
 * mosco.c: implementation of a game from Ivan Moscovich's books called
 *           "Magic Arrow Tile Puzzles"
 */

/*
   The game is played on a (initially blank) grid with clues on the outer edges.
   These "clues" are numbers which indicate the number of arrow tiles pointing
   at the square they are in.  The arrow tiles are placed on the inner grid,
   indicated by -'s in this example:

   1 0 2 2 0 0
   1 - - - - 2
   1 - - - - 0
   0 - - - - 1
   0 - - - - 1
   1 2 0 2 0 0 

   The object of the game is to place sets of eight arrow tiles (UL, U, UR, L, R, 
   DL, D, DR) into the grid so that the clues are all exactly fullfilled, e.g. 
   a "3" has exactly three arrows pointing at it.

   Because the grid is only non-trivial when the player is limited to a certain
   number of each arrow tile, (how many complete "sets" of U, UL, etc.), the 
   internal grid area (not including the clues on the outer edges) must be a 
   multiple of 8.  

   For instance, on a 4x4 grid, two "sets" of arrow tiles are used, so the 
   player has two of each direction (so they can't use three up arrows).  
   On an 8x8 grid, they will have eight "sets" of tiles to insert.

   One solution to the above grid is:

   L , R , R , UL
   UR, U , L , DL
   D , U , UL, DR
   DL, DR, D , UR

   Although there are usually several possible answers, most of which are 
   permutations of other answers, or contain "loops" of arrow tiles which can be
   rearranged to provide answers which are also correct.

   Notice how there are two of each tile, because this is a 4x4 puzzle.

   Definitions:

       Grid: the set of tiles, including the outer edge tiles for clues.
       Arena: the set of inner tiles, not including the outer edge tiles for clues.
       Width: the number of tiles "across" the arena.
       Height: the number of tiles "down" the arena.

       Clues: The outer edge tiles, which each hold a number counting the arrow 
              tiles that are "pointing" at them.  These are not changeable
              directly by the user.  There are (w*2)+(h*2)+4 of these in a grid
              (four sides and four diagonal clues).

       Tiles: individual squares within the grid which can contain nothing
              or an arrow.  An arrow can point in any of eight directions:
              UL, U, UR, L, R, DL, D, DR.

       Tile sets: The sets of the eight possible arrow tiles which can fit into 
                  the arena.  There will be  (w * h) / 8) of these.  As such, 
                  there will be "tile set" amounts of each arrow, 
                  e.g. only (w*h)/8 "U"'s, (w*h)/8 "UL"'s, etc.

       Affecting tile: An arena tile who, with the right arrow, could affect the 
                       total of a certain clue (i.e. a tile that could "point" at 
                       the clue in question).  Corner clues have min(w,h) of 
                       these.  Other clues have w+h-1 of these.

       Game: a set of clues (and nothing else) inside a grid.
       Solution: A completed grid whose tiles fulfill all the clues and where
                 there are only (w * h) / 8 of each type of tile.
              
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

#include "puzzles.h"

#define PREFERRED_TILE_SIZE 32
#define NO_OF_BASIC_TILES 8
#define TILE_BORDER_SIZE 3
#define NO_OF_TILE_SETS (w*h / NO_OF_BASIC_TILES)
#define ARROW_LARGE 1
#define ARROW_SMALL 0
#define FLASH_FRAME 0.2F

enum {UP, UP_LEFT, LEFT, DOWN_LEFT, DOWN, DOWN_RIGHT, RIGHT, UP_RIGHT, NO_ARROW};
enum {HUP=1, HUP_LEFT=2, HLEFT=4, HDOWN_LEFT=8, HDOWN=16, HDOWN_RIGHT=32, HRIGHT=64, HUP_RIGHT=128};
char description[NO_OF_BASIC_TILES][3]={"U ", "UL", "L ", "DL", "D ", "DR", "R ", "UR"};

/* Helper macros. */
#define GRIDTILE(grid, h, x, y) grid[x*(h+2) + y]
#define ARENATILE(grid, h, x,y) GRIDTILE(grid, h, (x+1), (y+1))

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
    int clue;
    struct affected_tiles_struct *affected_tiles;   /* A list of tiles which can affect the outcome of 
                                                       this clue, i.e. those that "could" point to it. */
    int number_of_affected_tiles;
};

struct game_state
{
    int w, h;
    unsigned char *grid;       /* The main grid, including clues (w+2)x(h+2) */
    unsigned char *hints_grid; /* An array to hold player "hints" as bitmaps (w+2)x(h+2) */
    unsigned char *highlighted_grid;
    game_params *params; 
    int completed, used_solve;
};

static void free_game(game_state *state)
{
    sfree(state->grid);
    sfree(state->hints_grid);
    sfree(state->highlighted_grid);
    sfree(state);
}

static game_state *dup_game(game_state *state)
{
    game_state *ret = snew(game_state);

    ret->w=state->w; 
    ret->h=state->h;
    ret->completed=state->completed;
    ret->used_solve=state->used_solve;
    ret->grid = snewn((ret->w+2)*(ret->h+2)+1, unsigned char);
    memcpy(ret->grid, state->grid, (ret->w+2)*(ret->h+2) * sizeof(unsigned char));

    ret->hints_grid = snewn((ret->w+2)*(ret->h+2)+1, unsigned char);
    memcpy(ret->hints_grid, state->hints_grid, (ret->w+2)*(ret->h+2) * sizeof(unsigned char));

    ret->highlighted_grid = snewn((ret->w+2)*(ret->h+2)+1, unsigned char);
    memcpy(ret->highlighted_grid, state->highlighted_grid, (ret->w+2)*(ret->h+2) * sizeof(unsigned char));

    ret->params=state->params;

    return ret;
}

unsigned int count_binary_bits(unsigned int number)
{
    /*
       Brian Kernighan's (fast) method for counting the number of binary bits set in a number.
       It goes through as many iterations as there are set bits. So if we have a 32-bit word 
       with only the high bit set, then it will only go once through the loop. 

       Published in 1988, the C Programming Language 2nd Ed. (by Brian W. Kernighan and Dennis
       M. Ritchie) mentions this in exercise 2-9. On April 19, 2006 Don Knuth pointed out that 
       this method "was first published by Peter Wegner in CACM 3 (1960), 322. (Also discovered
       independently by Derrick Lehmer and published in 1964 in a book edited by Beckenbach.)"

       Text and actual code taken from http://www-graphics.stanford.edu/~seander/bithacks.html
    */

    unsigned int count;               /* accumulates the total bits set */
    for(count = 0; number; count++)
    {
        number &= number - 1; /* clear the least significant bit set */
    };
    return(count);
}

unsigned int b(unsigned int total, unsigned int start, unsigned int l)
{
    if (l < 0) return total;

    int n = 1 << l;
    int t = 0;
    while ((total&n) == 0 && t <= start)
    {
         t = b(total + n, start, l-1);
         n <<= 1;
    }

    return t;
}

unsigned int find_next_binary_iteration2(unsigned int start, unsigned int x, unsigned int size)
{
    return b(0, start, x-1);
}


unsigned int find_next_binary_iteration(unsigned int number, unsigned int number_of_bits_set, unsigned int size)
{
    /* Takes a number of binary length "size", with "number_of_bits_set" binary bits set (1) 
       in its expansion and finds the next highest number with that same property, or NULL 
       if none exist. */

    int i;

    for(i=number+1;i<=pow(2,size);i++)
    {
        if(count_binary_bits(i) == number_of_bits_set)
        {
            return(i);
        };
    };

    return(0);
}

unsigned int find_first_binary_iteration(unsigned int number_of_bits_set, unsigned int size)
{
    /* Returns the first number which has a binary expression of "size" bits, where there are
       "number_of_bits_set" bits being 1, or NULL if there is none. */
    int i=1;
    return(find_next_binary_iteration2(i, number_of_bits_set, size));
}

#ifdef DEBUG
void print_grid(unsigned char *grid, int w, int h)
{
    /* Debugging routine to print the grid out */
    int i,j;
    for(j=0;j<(h+2);j++)
    {
        for(i=0;i<(w+2);i++)
        {
            printf("%u, ", GRIDTILE(grid, h, i, j));
        };
        printf("\n");
    };    
}
#endif

void generate_affected_tiles(game_state *state, struct clues_struct *clue)
{
    int i,j;
    int x = clue->x;
    int y = clue->y;
    for(j=1;j<state->h+1;j++)
    {
        for(i=1;i<state->w+1;i++)
        {
            int affected=FALSE;
            int direction=0;

            if(abs( i - x) == abs(j - y))
            {
                affected = TRUE;
            };

            if((x==0 && j==y) || (y==0 && i==x) || ((x==state->w+1) && (j==y)) || ((y==state->h+1) && (i==x)))
            {
                affected = TRUE;
            };

            if(affected == TRUE)
            {
                int up=FALSE, down=FALSE, left=FALSE, right=FALSE;
                if(j > y)
                    up=TRUE;
                if(j < y)
                    down=TRUE;
                if(i > x)
                    left=TRUE;
                if(i < x)
                    right=TRUE;

                if(up)
                {
                    if(left)
                        direction=UP_LEFT;
                    else if(right)
                        direction=UP_RIGHT;
                    else
                        direction=UP;
                }
                else if(down)
                {
                    if(left)
                        direction=DOWN_LEFT;
                    else if(right)
                        direction=DOWN_RIGHT;
                    else
                        direction=DOWN;
                }
                else if(right)
                    direction=RIGHT;
                else if(left)
                    direction=LEFT;

                clue->number_of_affected_tiles++;
#ifdef DEBUG
                printf("will be affected by tile no. %u, at %u, %u if it points %s\n", clue, clue.clue, x, y, clue.number_of_affected_tiles, i, j, description[direction]);
#endif
                clue->affected_tiles = sresize(clue->affected_tiles, clue->number_of_affected_tiles, struct affected_tiles_struct);
                clue->affected_tiles[clue->number_of_affected_tiles-1].x = i;
                clue->affected_tiles[clue->number_of_affected_tiles-1].y = j;
                clue->affected_tiles[clue->number_of_affected_tiles-1].direction_to_affect = direction;
            };
        };
    };
}

struct clues_struct *generate_clue_lists(game_state *state, int *number_of_clues)
{
    int n, i, j;
    struct clues_struct *clues;

    /* There will ALWAYS be this many clues in any grid */
    int size=(state->w*2) + (state->h*2) + 4;
    clues=snewn(size, struct clues_struct);

    n=0;

    /* Find every clue tile in the grid and add them to a 
       dynamic list with their co-ordinates */
    for(j=0;j<state->h+2;j++)
    {
        for(i=0;i<state->w+2;i++)
        {
            if((i==0) || (i==(state->w+1)) || (j==0) || (j==(state->h+1)))
            {
                clues[n].x=i;
                clues[n].y=j;
                clues[n].clue=GRIDTILE(state->grid, state->h, i, j);
                clues[n].affected_tiles = snew(struct affected_tiles_struct);
                clues[n].number_of_affected_tiles=0;
#ifdef DEBUG
                printf("Clue %u (%u) found at %u, %u\n", n, clues[n]->clue, i, j);
#endif
                n++;
            };
        };
    };

    /* There will ALWAYS be this many clues in any grid */
    assert(n == size);
    *number_of_clues=size;
    return(clues);
}

void regenerate_clues_for_grid(game_state *state)
{
    /* 
       Using the currently placed arena tiles, determine the clues that they 
       would generate.  This is used to:-
       
       - Create the initial screen for the user (fill the inner arena with some 
         valid tiles, then use this to generate the clues, then clear the arena).
       - Check whether the tiles that the user has placed satisfy the criteria
         of the given clues. (By allowing this function to regenerate all the 
         clues for the user's placed tiles and then checking them against the 
         given clues to see if they match).
       - etc.
    */

    int i,n,c;
    struct clues_struct *clues;

    clues=generate_clue_lists(state, &n);

    /* 
       Loop over every clue and find every arena tile that could affect them, adding the
       tile's co-ordinates and the direction it would need to point to affect the clue 
       to a dynamic list.

       For corner clues, this are merely min(w,h) of these.  For other clues, there are
       (w+h-1) of them.      
    */

    for(c=0;c<n;c++)
    {
#ifdef DEBUG
        printf("Clue %u (%u) at %u, %u \n", c, clues[c].clue, clues[c].x, clues[c].y);
#endif
        generate_affected_tiles(state, &clues[c]);
    };      

    /* 
       Loop over every clue and every one of its "affecting" tiles and if a tile 
       which is actually inside the arena is pointing in the direction it needs to 
       be to affect this particular clue, count it.  When you have counted all 
       possible affecting tiles for a particular clue, the count *is* that clue.
    */

    for(c=0;c<n;c++)
    {
        int count=0;
        for(i=0;i<clues[c].number_of_affected_tiles;i++)
        {
            if(GRIDTILE(state->grid, state->h, clues[c].affected_tiles[i].x, clues[c].affected_tiles[i].y) == clues[c].affected_tiles[i].direction_to_affect)
            {
                count++;
            };
        };
        GRIDTILE(state->grid, state->h, clues[c].x, clues[c].y) = count;
        sfree(clues[c].affected_tiles);
    };
    sfree(clues);
}

int clue_is_satisfied(game_state *state, int x, int y)
{
    /*
       Returns 0 if a particular clue has the correct number of arena tile arrows
       pointing at it.  Returns +/- 1 if the clue has too many/not enough arrows
       pointing at it.

       We do this by duplicating the current arena tiles, regenerating 
       the clues for the new arena and then checking if this particular clue matches 
       in both the original games clueset, and the regenerated one.
    */

    int ret;
    game_state *recalc_game_state;

    /* Assert if we're not being given a clue co-ordinate */
    assert( (x==0) || (x==(state->w+1)) || (y==0) || (y==(state->h+1)) );

    recalc_game_state=dup_game(state);
    regenerate_clues_for_grid(recalc_game_state);

    if(GRIDTILE(recalc_game_state->grid, state->h, x, y) == GRIDTILE(state->grid, state->h, x, y))
    {
        ret = 0;
    }
    else if(GRIDTILE(recalc_game_state->grid, state->h, x, y) > GRIDTILE(state->grid, state->h, x, y))
    {
        ret = 1;
    }
    else
    {
        ret=-1;
    };
    
    free_game(recalc_game_state);

    return(ret);
}

int check_if_finished(game_state *state, int *x, int *y)
{
   /*
      Returns TRUE if the game is won (all clues are satisfied
      and the correct number of each type of tile has been used).

      Returns FALSE with x,y >=0 if there is an error in the clues, 
      indicating the clue that is incorrect.

      Returns FALSE with x,y < 0 if there is any other type of problem.
   */

    int i, j;
    int clues_correct=0;
    int arrows_filled=0;
    int too_many_arrows=FALSE;

    game_state *recalc_game_state;
    unsigned int tile_count[NO_OF_BASIC_TILES+1];

    for(i=0;i<(NO_OF_BASIC_TILES+1);i++)
        tile_count[i]=0;

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

                tile_count[GRIDTILE(state->grid, state->h, i,j)]++;
                if((tile_count[GRIDTILE(state->grid, state->h, i,j)] > (state->w*state->h / NO_OF_BASIC_TILES)))
                    too_many_arrows=TRUE;
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

    if(clues_correct != (2*state->w + 2*state->h + 4))
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

    if(too_many_arrows)
    {
#ifdef DEBUG
        printf("Grid has too many of at least one type of arrow\n");
#endif
        return(FALSE);
    };

#ifdef DEBUG
    printf("Grid is completed and correct\n");
#endif
    state->completed=TRUE;
    return(TRUE);
}

static game_params *default_params(void)
{
    game_params *ret = snew(game_params);
    ret->w = ret->h = 4;
    return ret;
}

/* Any size that gives an area that is a multiple of NO_OF_BASIC_TILES can
   be a valid grid.*/
static const game_params moscow_presets[] = {
    {4, 4},
    {6, 4},
    {8, 4},
    {8, 8},
    {16, 16},
};

static int game_fetch_preset(int i, char **name, game_params **params)
{
    char str[strlen("255x255")];
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
    char str[strlen("w255h255")];

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

    /* 255 is WAY too high because even a fast computer will struggle much
       past 64x64 */
    if (params->w > 255 || params->h > 255)
        return "Widths and heights greater than 255 are not supported";
    return NULL;
}

/*
char* itoa(int val, int base)
{
	static char buf[32] = {0};	
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = "0123456789abcdef"[val % base];
	return &buf[i+1];	
}
*/

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

/*
    int n=find_first_binary_iteration(4, 7);
    while(n != 0)
    {
        printf("%u, %s\n", n, itoa(n, 2));
        n=find_next_binary_iteration2(n, 4, 7);
    };
*/

    /*
        For now, we cheat.  Instead of building a "solveable" grid by logic, we
        simply get enough of the correct tiles, shuffle them about randomly, and
        then put them into the grid in any old order.

        We generate all of the clues from it, remove the tiles and, voila, we
        have a grid and (if we remember the tiles that we put in and in what order)
        we also have a solution.

        It's impossible to generate a grid which doesn't provide a correct set of
        clues this way, although whether the solution is logically deducible is tricky
        to answer.  I know for 4x4 that the problem is so trivial that it has been
        impossible to generate a puzzle that I haven't been able to solve by deduction
        and it may be that large puzzles just require more of the same.

        However, at some point, this needs to be made into a proper generator/solver
        by way of checking logical deducibility on a human level.
    */        

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
            ARENATILE(grid, h, i, j) = tileset[i*h + j];
#ifdef DEBUG
            printf("%u, ", ARENATILE(grid, h, i, j));
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
            switch(GRIDTILE(grid, h, i ,j))
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
                    printf("Hit an invalid arrow type: %u at %u,%u\n", GRIDTILE(grid, h, i ,j), i, j);
                    fatal("Hit an invalid arrow type\n");
            };
            GRIDTILE(grid, h, x,y)++;
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
                printf("%s, ", description[GRIDTILE(grid, h, i,j)]);
            else
            {
                printf("%u , ", GRIDTILE(grid, h, i,j));
                total_tiles+=GRIDTILE(grid, h, i,j);
            };
        };
        printf("\n");
    };
    printf("Total tiles: %u\n",total_tiles);

    /* The total count of the game clues at the edges should alway equal the size of the game arena.
     If they don't, we've buggered up generation of the clues. */
    assert(total_tiles == (w * h)); 
   
#endif


    /*
       Cheat and feed our "solution" that we started from straight into the aux string
       so that the solver can pull it out of the air later if it's ever asked to solve.  
       Cheating, yes.  Simple, yes.
    */      

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

static char *validate_desc(game_params *params, char *desc)
{
    char *ret;
    ret = NULL;
    return ret;
}

static game_state *new_game(midend *me, game_params *params, char *desc)
{
    int i,j;
    game_state *state = snew(game_state);
    unsigned char *game_desc;
    state->grid = snewn((params->w+2)*(params->h+2)+1, unsigned char);
    state->hints_grid = snewn((params->w+2)*(params->h+2)+1, unsigned char);
    state->highlighted_grid = snewn((params->w+2)*(params->h+2)+1, unsigned char);

    state->w=params->w;
    state->h=params->h;
    state->completed=FALSE;
    state->used_solve=FALSE;

    game_desc=hex2bin(desc, (params->w+2)*(params->h+2));

    for(i=0; i < (params->w+2)*(params->h+2);i++)
        state->grid[i]=game_desc[i];
    sfree(game_desc);

    /* Clear the arena if it's not already */
    for(i=0;i<(params->w+2);i++)
        for(j=0;j<(params->h+2);j++)
        {
            if((i>0) && (i<(params->w+1)) && (j>0) && (j<(params->h+1)))
            {
                GRIDTILE(state->grid, params->h, i,j)=NO_ARROW;
                GRIDTILE(state->hints_grid, params->h, i,j)=0;
            };
            GRIDTILE(state->highlighted_grid, params->h,i,j)=FALSE;
        };

    state->grid[(params->w+2)*(params->h+2)]=0;
    state->hints_grid[(params->w+2)*(params->h+2)]=0;

    state->params=params;
    return state;
}

static char *solve_game(game_state *state, game_state *currstate,char *aux, char **error)
{
    char *move_string;

    if(state->completed)
    {
        *error = "Game already complete.";
        return(NULL);
    };

/*
    game_state *recalc_game_state;
    int i;
    recalc_game_state=dup_game(state);
    regenerate_clues_for_grid(recalc_game_state);

    for(i=0;i<recalc_game_state.clues
*/

    move_string=snewn(strlen(aux)+1, char);
    memset(move_string, 0, strlen(aux)+1);

    /* Cheat - send a string copy of the game as it was generated to execute_move. */
    strcat(move_string, "S");
    strcat(move_string, aux);

    return(move_string);
}

static int game_can_format_as_text_now(game_params *params)
{
    return TRUE;
}

static char *game_text_format(game_state *state)
{
    return NULL;
}

struct game_ui 
{
    int errors;
    int cur_x, cur_y, cur_visible;
};

static game_ui *new_ui(game_state *state)
{
    game_ui *ui = snew(game_ui);
    ui->cur_x = ui->cur_y = ui->cur_visible = ui->errors = 0;
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

static char *interpret_move(game_state *state, game_ui *ui, game_drawstate *ds,
			    int x, int y, int button)
{
    char *ret;
    ret=snewn(255, char);
    ret[0]='\0';

    if(IS_CURSOR_MOVE(button))
    {
        if(button == CURSOR_LEFT)
            ui->cur_x = max(ui->cur_x - 1,0);
        if(button == CURSOR_RIGHT)
            ui->cur_x = min(ui->cur_x + 1,state->w + 1);
        if(button == CURSOR_UP)
            ui->cur_y = max(ui->cur_y - 1,0);
        if(button == CURSOR_DOWN)
            ui->cur_y = min(ui->cur_y + 1,state->h +1);
        ui->cur_visible = 1;
        return "";
    }
    else if(IS_CURSOR_SELECT(button))
    {
        if (!ui->cur_visible)
        {
            ui->cur_visible = 1;
            return "";
        }
        x = ui->cur_x * ds->tilesize;
        y = ui->cur_y * ds->tilesize;

        button = (button == CURSOR_SELECT2) ? MIDDLE_BUTTON : LEFT_BUTTON;
    };

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
                /* Add/rotate the arrow in the current tile */
                case LEFT_BUTTON:
                    sprintf(ret, "G%i-%i", x_square, y_square);
                    return(ret);
                    break;

                /* clear the current tile */
                case MIDDLE_BUTTON:
                    sprintf(ret, "Z%i-%i", x_square, y_square);
                    return(ret);
                    break;

                /* add a hint arrow in in the current tile */
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
        }
        else if( (x_square>=0) && (x_square <= (state->w+1)) && (y_square>=0) && (y_square<=(state->h+1)) )
        /* we're inside the clues but not outside the grid as a whole */
        {
            sprintf(ret, "A%i-%i", x_square, y_square);
            return(ret);
        };
    };
    sfree(ret);
    return(NULL);
}


static game_state *execute_move(game_state *from, char *move)
{
    int x_square, y_square;
    int x,y;
    int direction;
    game_state *ret;

    ret=dup_game(from);

    /* Clear a square. */
    if(move[0] == 'Z')
    {
        if(sscanf(move, "Z%i-%i", &x_square, &y_square) < 2)
        {
            return(NULL);
        }
        else
        {
            GRIDTILE(ret->grid, ret->h, x_square,y_square)=NO_ARROW;
            GRIDTILE(ret->hints_grid, ret->h, x_square,y_square)=0;
            check_if_finished(ret, &x, &y);
            return(ret);
        };
    };

    /* Add a arrow to a square.*/
    if(move[0] == 'G')
    {
        if(sscanf(move, "G%i-%i", &x_square, &y_square) < 2)
        {
            return(NULL);
        }
        else
        {
            GRIDTILE(ret->grid, ret->h, x_square,y_square)=(GRIDTILE(ret->grid, ret->h, x_square,y_square)+1) % NO_OF_BASIC_TILES;
            GRIDTILE(ret->hints_grid, ret->h, x_square,y_square)=0;
            check_if_finished(ret, &x, &y);
            return(ret);
        };
    };

    /* Add a hint to a square. */
    if(move[0] == 'H')
    {
        if(sscanf(move, "H%i-%i-%i", &x_square, &y_square, &direction) < 3)
        {
            return(NULL);
        }
        else
        {
            GRIDTILE(ret->grid, ret->h, x_square,y_square)=NO_ARROW;
            if(direction > 0)
                GRIDTILE(ret->hints_grid, ret->h, x_square,y_square)^=direction;
            check_if_finished(ret, &x, &y);
            return(ret);
        };
    };

    /* Reveal affected tiles for a clue */
    if(move[0] == 'A')
    {
        if(sscanf(move, "A%i-%i", &x_square, &y_square) < 2)
        {
            return(NULL);
        }
        else
        {
            struct clues_struct *clues;
            int clue_number=-1;
            int c=-1, i, j, n;
            int existing_highlight;
            clues=generate_clue_lists(from, &n);

            for(c=0;c<n;c++)
            {
                if((clues[c].x == x_square) && (clues[c].y == y_square))
                {
                    generate_affected_tiles(from, &clues[c]); 
                    clue_number=c;
                    break;
                };
            };
            
            if(c<0)
                return(ret);
            existing_highlight=GRIDTILE(ret->highlighted_grid, ret->h, clues[clue_number].x, clues[clue_number].y);
            
            for(i=0;i<ret->w+2;i++)
                for(j=0;j<ret->h+2;j++)
                    GRIDTILE(ret->highlighted_grid, ret->h, i, j) = FALSE;              

            if(existing_highlight == FALSE)
            {
                GRIDTILE(ret->highlighted_grid, ret->h, clues[clue_number].x, clues[clue_number].y) = TRUE;
                for(i=0;i<clues[clue_number].number_of_affected_tiles;i++)
                {
                    GRIDTILE(ret->highlighted_grid, ret->h, clues[clue_number].affected_tiles[i].x, clues[clue_number].affected_tiles[i].y) = TRUE;
                };
            };
        
            for(c=0;c<n;c++)
            {
                sfree(clues[c].affected_tiles);
            };
            sfree(clues);
            return(ret);
        };
    };

    /* Solve the game. */
    if(move[0] == 'S')
    {
        /*
           Cheat and take the string we are given, which is a copy of the
           grid as it was when it was generated.  Paste it straight into
           the current grid and, hey presto!, a "solved" grid.
        */
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
        ret->used_solve=TRUE;
        return(ret);
    };
    return(NULL);
}

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

/* Colour 0 - Background */
    frontend_default_colour(fe, &ret[0]);

/* Colour 1, 2 - high/lowlights */
    game_mkhighlight(fe, ret, 0, 1, 2);

/* Colour 3 - Black */
    ret[9] = 0.0F;
    ret[10] = 0.0F;
    ret[11] = 0.0F;

/* Colour 4 - White */
    ret[12] = 1.0F;
    ret[13] = 1.0F;
    ret[14] = 1.0F;

/* Colour 5 - grid colour */
    ret[15] = ret[0] * 0.9F;
    ret[16] = ret[1] * 0.9F;
    ret[17] = ret[2] * 0.9F;

/* Colour 6 - Red square */
    ret[18] = ret[0] * 0.9F;
    ret[19] = ret[1] * 0.5F;
    ret[20] = ret[2] * 0.5F;

/* Colour 7 - Yellow square */
    ret[21] = ret[0] * 0.9F;
    ret[22] = ret[1] * 0.9F;
    ret[23] = ret[2] * 0.5F;

/* Colour 8 - Green square */
    ret[24] = ret[0] * 0.5F;
    ret[25] = ret[1] * 0.9F;
    ret[26] = ret[2] * 0.3F;

/* Colour 9 - Blue square */
    ret[27] = ret[0] * 0.5F;
    ret[28] = ret[1] * 0.5F;
    ret[29] = ret[2] * 0.9F;

/* Colour 10 - Red */
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
    /* 
       Draws an arrow in the given position, either a full arrow (large, coloured ones)
       or a small arrow (small "hint" arrows inside a tile).
    */
    int base_x=x*ds->tilesize;
    int base_y=y*ds->tilesize;

    clip(dr, x*ds->tilesize, y*ds->tilesize, ds->tilesize, ds->tilesize);

    if(size == ARROW_LARGE)
    {

/* Some helpful defines for the corners of the selected tile
   We use these to draw arrows line by line, e.g. TOP_LEFT -> BOTTOM_RIGHT etc. */
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
/* 
   Some helpful defines for the corners of the selected tile, but taking into
   account that we need to sub-divide each tile into nine "mini-tiles" for the
   hint arrows.
 
   We use these to draw hint arrows line by line, e.g. TOP_LEFT -> BOTTOM_RIGHT etc. 
   in their correct portion of the given tile (e.g. UL arrow in top-left corner, etc.)
*/
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
    draw_update(dr, x*ds->tilesize, y*ds->tilesize, ds->tilesize, ds->tilesize);
}


static void game_redraw(drawing *dr, game_drawstate *ds, game_state *oldstate,
			game_state *state, int dir, game_ui *ui,
			float animtime, float flashtime)
{
    int i,j;
    int error_clue_x, error_clue_y;
    unsigned int tile_count[NO_OF_BASIC_TILES+1];

    int frame = -1;
    if (flashtime > 0)
    {
        /*
         * We're animating a completion flash. Find which frame
         * we're at.
         */
        frame = (int)(flashtime / FLASH_FRAME);
    }

    for(i=0;i<(NO_OF_BASIC_TILES+1);i++)
        tile_count[i]=0;

    /* Add up how many of each type of tile we have.
       We will use this later to highlight errors to the user */
    for(i=0;i<state->w+2;i++)
    {
        for(j=0;j<state->h+2;j++)
        {
            if((i>0) && (i<(state->w+1)) && (j>0) && (j<(state->h+1)))
                tile_count[GRIDTILE(state->grid, state->h, i,j)]++;
        };
    };

    for(i=0;i<state->w+2;i++)
    {
        for(j=0;j<state->h+2;j++)
        {
            int colour;

            /* if we're animating a flash, draw every other frame in red */
            if((frame % 2)==0)
                colour=10;  // Solid red
            else if(ui->cur_visible && (i == ui->cur_x) && (j == ui->cur_y))
                colour=1;  // Highlight for the cursor
            else if(GRIDTILE(state->highlighted_grid, state->h, i, j))
                colour=2;  // Lowlight
            else
                colour=5;  // Grid Colour
                
            draw_rect(dr, i*ds->tilesize, j*ds->tilesize, ds->tilesize+1, ds->tilesize+1, colour);
            draw_rect_outline(dr, i*ds->tilesize, j*ds->tilesize, ds->tilesize+1, ds->tilesize+1, 2);

            /* If this is an arena tile */
            if((i>0) && (i<(state->w+1)) && (j>0) && (j<(state->h+1)))
            {
                draw_arrow(dr, ds, i, j, ARROW_LARGE, GRIDTILE(state->grid, state->h, i, j));
                draw_arrow(dr, ds, i, j, ARROW_SMALL, GRIDTILE(state->hints_grid, state->h, i, j));

                /* If this is one of those tiles that there are too many of... */
                if((GRIDTILE(state->grid, state->h, i,j) < NO_ARROW) && (tile_count[GRIDTILE(state->grid, state->h, i,j)] > (state->w*state->h / NO_OF_BASIC_TILES)))
                {
                    draw_rect_outline(dr, i*ds->tilesize, j*ds->tilesize, ds->tilesize, ds->tilesize, 10);
                };
            }
            else
            {
                /* This is a clue tile */
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

                sprintf(number_as_string, "%u", GRIDTILE(state->grid, state->h, i,j));

                draw_text(dr, (i+0.25)*ds->tilesize, (j+1)*ds->tilesize, FONT_VARIABLE, ds->tilesize /2, ALIGN_VCENTRE & ALIGN_HCENTRE, colour, number_as_string);
                sfree(number_as_string);
            };
        };
    };

    draw_update(dr, 0, 0, (state->w+2)*ds->tilesize, (state->h+2)*ds->tilesize);

    if(check_if_finished(state, &error_clue_x, &error_clue_y))
    {
        if(state->used_solve)
        {
            status_bar(dr, "Completed (Auto-solve was used)!");
        }
        else
        {
            status_bar(dr, "Completed!");
            game_completed();
        };
    };

/*
        char *message;
        message=snewn(17, char);
        sprintf(message, "%u of each tile", (state->w*state->h / NO_OF_BASIC_TILES));
        status_bar(dr, message);
        sfree(message);
*/
}

static float game_anim_length(game_state *oldstate, game_state *newstate,
			      int dir, game_ui *ui)
{
    return 0.0F;
}

static float game_flash_length(game_state *oldstate, game_state *newstate,
			       int dir, game_ui *ui)
{
   /*
     * If the game has just been completed, we display a completion
     * flash.
     */
    if (!oldstate->completed && newstate->completed &&
        !oldstate->used_solve && !newstate->used_solve) {
        int size = 0;
        if (size < newstate->w)
            size = newstate->w;
        if (size < newstate->h)
            size = newstate->h;
        return FLASH_FRAME * (size+4);
    }

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

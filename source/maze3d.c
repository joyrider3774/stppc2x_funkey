/*
 * 3d Maze....
 *
 * Version 2 - 15-Jan-2006 - Better Type selections (and fi )
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include "puzzles.h"
#include "maze3dc.h"

#define PREFFERED_TILE_SIZE 450
#define ANIM_TIME 0.1
#define INSTRUCTION_TEXT_SIZE 13

void mazedraw_rect(drawing *dr, double sz, int ax, int ay, int bx, int by, int col);
void mazedraw_triangle(drawing *dr, double sz, int p1x, int p1y,  int p2x, int p2y, int p3x,int p3y, int col);
void mazedraw_romb(drawing *dr, double sz, int p1x, int p1y, int p2x, int p2y, char orient, int col);
void mazedraw_square(drawing *dr, double sz, int topleft,int col);
void mazedraw_romb(drawing *dr, double sz, int p1x, int p1y, int p2x, int p2y, char orient, int col);
void mazedraw_thread(drawing *dr, double sz, MAZEVIEW *view, int targ, int *d_l, int thread);
int col_wall(int dir, MAZEVIEW *view);
char encode_target(int targetd);
int decode_targetd(char targetc);
char encode_display(int displayi);
int decode_display(char displayc);

enum {
    COL_BACKGROUND,
    COL_WHITE,
    COL_LINE,
    COL_MARK,
    COL_TARGET,
    COL_THREAD,

    COL_N,
/*
    COL_S,
 */
/*
    COL_W,
 */
    COL_E,
    COL_U,
    COL_D,

/*
    COL_WALL,
    COL_FLOOR,
    COL_CIEL,
 */
  
    COL_FOG,

    NCOLOURS
};


enum {
  DISPLAY_NONE,
  DISPLAY_COORD,
  DISPLAY_DIRDIST
};
  

#define COL_S COL_N
#define COL_W COL_E

/*
#define COL_N COL_WALL
#define COL_S COL_WALL
#define COL_E COL_WALL
#define COL_W COL_WALL
#define COL_U COL_CIEL
#define COL_D COL_FLOOR
 */

/*
 *I do not use this at the moment 
 * It is required to force a call of changed_state()
 *
 */
struct game_ui {
	int dummy;	/* Dummy variable */
};

struct game_params {
    int xx, yy, zz;	/* Dimensions of maze */
    int targetd, isdouble; /* Extra parameters, target dest and doubles allowed */
    int display;   /* What to display */
    int seed;		    /* The maze seed */
    int valid;	    /* A trick I do to use decode_params as a validator */
};

struct game_state {
	MAZE *maze;
	/*
	 * Using chars here to save memory space in BIG games
	 * a 5 x 5 x 5 has well over 250 moves
	 */
	char x, y, z;		/* Position */
	char dir;		/* Direction I am facing */
	char mark;		/* If marked  - 1 is marked 0 */
	char target;		/* 0 is no target, 1 is */
	char move;		/* The move being performed.  To work outif I am undoing it */
	long moves, rotations;	/* The counts of these */
};

static game_params *default_params(void)
{
    game_params *ret = snew(game_params);

    ret->xx = ret->yy = ret->zz = 4;	/* Not too difficult */
    ret->targetd = MAZE_CORNER;		/* Seems as goods as place as any */
    ret->isdouble = FALSE;		/* Lets not do this by default :-) */
    ret->display = DISPLAY_DIRDIST;
    ret->valid = TRUE;

    ret->seed = 0;

    return ret;
}

static int game_fetch_preset(int i, char **name, game_params **params)
{

    game_params *pars;
    if(i < 0 || i > 6) return FALSE;

    pars = default_params();
    *params = pars;

    switch(i) {
    case 0:
	*name = dupstr("Easy (3x3x3)");
	pars->xx = 3;
	pars->yy = 3;
	pars->zz = 3;
	pars->targetd = MAZE_CORNER;
	pars->isdouble = FALSE;
        pars->display = DISPLAY_DIRDIST;
	break;
    case 1:
	*name = dupstr("Basic (4x4x4)");
	pars->xx = 4;
	pars->yy = 4;
	pars->zz = 4;
	pars->targetd = MAZE_CORNER;
	pars->isdouble = FALSE;
        pars->display = DISPLAY_DIRDIST;
	break;
    case 2:
	*name = dupstr("More Challenging (5x5x5)");
	pars->xx = 5;
	pars->yy = 5;
	pars->zz = 5;
	pars->targetd = MAZE_CORNER;
	pars->isdouble = FALSE;
        pars->display = DISPLAY_DIRDIST;
	break;
    case 3:
	*name = dupstr("Flat (8x8)");
	pars->xx = 8;
	pars->yy = 8;
	pars->zz = 1;
	pars->targetd = MAZE_CORNER;
	pars->isdouble = FALSE;
        pars->display = DISPLAY_DIRDIST;
	break;
    case 4:
	*name = dupstr("Big flat (20x20)");
	pars->xx = 20;
	pars->yy = 20;
	pars->zz = 1;
	pars->targetd = MAZE_CORNER;
	pars->isdouble = FALSE;
        pars->display = DISPLAY_COORD;
	break;
    case 5:
	*name = dupstr("Difficult (5x5x5 no hints)");
	pars->xx = 5;
	pars->yy = 5;
	pars->zz = 5;
	pars->targetd = MAZE_ANYWHERE;
	pars->isdouble = FALSE;
        pars->display = DISPLAY_COORD;
	break;
    case 6:
	*name = dupstr("Impossible (10x10x10 with doubles)");
	pars->xx = 10;
	pars->yy = 10;
	pars->zz = 10;
	pars->targetd = MAZE_ANYWHERE;
	pars->isdouble = TRUE;
        pars->display = DISPLAY_NONE;
	break;
   }

    return TRUE;
}

static void free_params(game_params *params)
{
    sfree(params);
}

static game_params *dup_params(game_params *params)
{
    game_params *ret = snew(game_params);
    ret->xx = params->xx;
    ret->yy = params->yy;
    ret->zz = params->zz;
    ret->targetd = params->targetd;
    ret->isdouble = params->isdouble;
    ret->valid = params->valid;
    ret->display = params->display;

    ret->seed = params->seed;

    return ret;
}

static void decode_params(game_params *params, char const *string)
{
	int mode;

	params->xx = 0;
	params->yy = 0;
	params->zz = 0;
	params->targetd = MAZE_CORNER;
	params->isdouble = FALSE;
        params->display = DISPLAY_DIRDIST;
	params->seed = 0;
	params->valid = TRUE;

	mode = 0;

	for(;*string;string++)
	{
		switch(mode) {
		case 0:
			if(tolower(*string) == 'x')
				mode = 1;
			else if(!isdigit(*string))
			{
				mode = 3;
				string--;
			}
			else
			{
				params->xx *= 10;
				params->xx += *string - '0';
			}
			break;
		case 1:
			if(tolower(*string) == 'x')
				mode = 2;
			else if(!isdigit(*string))
			{
				mode = 3;
				string--;
			}
			else
			{
				params->yy *= 10;
				params->yy += *string - '0';
			}
			break;
		case 2:
			if(!isdigit(*string))
			{
				mode = 3;
				string--;
			}
			else
			{
				params->zz *= 10;
				params->zz += *string - '0';
			}
			break;
		case 3:
			if(isdigit(*string))
			{
				mode = 4;
				string--;
			}
			else if (tolower(*string) == 'c')
				params->targetd = MAZE_CORNER;
			else if (tolower(*string) == 'e')
				params->targetd = MAZE_EDGE;
			else if (tolower(*string) == 'f')
				params->targetd = MAZE_FACE;
			else if (tolower(*string) == 'a')
				params->targetd = MAZE_ANYWHERE;
			else if (tolower(*string) == 'd')
				params->isdouble = TRUE;
			else if (tolower(*string) == 's')
				params->isdouble = FALSE;
			else if (tolower(*string) == 'n')
				params->display = DISPLAY_NONE;
			else if (tolower(*string) == 'l')
				params->display = DISPLAY_COORD;
			else if (tolower(*string) == 'w')
				params->display = DISPLAY_DIRDIST;;
			break;
		case 4:
			if(!isdigit(*string))
			{
				params->valid = FALSE;
				mode = 5;
			}
			else
			{
				params->seed *= 10;
				params->seed += *string - '0';
			}
			break;
		default:
			break;
		}
	}
	/*
	 * To place some defaults here
	 */

	if((!params->zz) && params->yy) params->zz = 1;
	if(!params->yy && params->xx)
	{
		if(params->zz)
			params->yy = 1;
		else
			params->yy = params->zz = params->xx;
	}
	if(!params->xx)
	{
		if(params->yy)
			params->xx = 1;
		else
			params->xx = params->yy = params->zz = 4;
	}
}

char encode_target(int targetd)
{
	char *s = "cefa";
	if(targetd < 0 || targetd > 3)
		return 'c';
	else
		return s[targetd];
	return'c';
}
int decode_target(char targetc)
{
	char *s = "cefa";
	int i = 0;
	targetc = tolower(targetc);
	while(*s)
	{
		if(*s == targetc) return i;
		i++;
		s++;
	}
	return 0;
}
	

char encode_display(int displayi)
{
	char *s = "nlw";   
	if(displayi < 0 || displayi > 3)
		return 'w';
	else
		return s[displayi];
	return'c';
}
int decode_display(char displayc)
{
	char *s = "nlw";
	int i = 0;
	displayc = tolower(displayc);
	while(*s)
	{
		if(*s == displayc) return i;
		i++;
		s++;
	}
	return DISPLAY_DIRDIST;
}
	

static char *encode_params(game_params *params, int full)
{
	char *ret = NULL;
	ret = snewn(20, char);
	sprintf(ret, "%dx%dx%d%c%c%c%d",
		params->xx % 100, params->yy % 100, params->zz % 100,
		encode_target(params->targetd),
		encode_display(params->display),
		params->isdouble ? 'd' : 's',
		full ? params->seed % 1000000000 : 0);

	return ret;
}

static config_item *game_configure(game_params *params)
{
    config_item *itms = snewn(7, config_item);
    config_item *itm = itms;
    char ws[32];

    itm->name = "Width";
    itm->type = C_STRING;
    sprintf(ws, "%d", params->xx);
    itm->sval = dupstr(ws);
    itm->ival = 0;

    itm++;

    itm->name = "Length";
    itm->type = C_STRING;
    sprintf(ws, "%d", params->yy);
    itm->sval = dupstr(ws);
    itm->ival = 0;

    itm++;

    itm->name = "Height";
    itm->type = C_STRING;
    sprintf(ws, "%d", params->zz);
    itm->sval = dupstr(ws);
    itm->ival = 0;

    itm++;

    itm->name = "Target location";
    itm->type = C_CHOICES;
    itm->ival = params->targetd;
    itm->sval = ":Corner:Edge:Face:Anywhere";

    itm++;

    itm->name = "Double Routes/Circles";
    itm->type = C_BOOLEAN;
    itm->ival = params->isdouble ? TRUE : FALSE;

    itm++;

    itm->name = "Display";
    itm->type = C_CHOICES;
    itm->ival = params->display;
    itm->sval = ":Nothing:Coordinates:Direction/Distance";

    itm++;

    itm->type = C_END;


    return itms;
}

static game_params *custom_params(config_item *cfg)
{
    game_params *pars = default_params();

    pars->xx = atoi(cfg[0].sval);
    pars->yy = atoi(cfg[1].sval);
    pars->zz = atoi(cfg[2].sval);

    pars->targetd  = cfg[3].ival;
    pars->isdouble = cfg[4].ival;
    pars->display  = cfg[5].ival;

    return pars;
}

static char *validate_params(game_params *params, int full)
{
    if( ((params->xx < 2) || (params->yy < 3)) && ((params->xx < 3) || (params->yy < 2)) )
        return("Minimum size is 3 x 2 x 1");
    if( params->xx > MAX_DIM)
        return("Width must not be >" STR(MAX_DIM));
    if( params->yy > MAX_DIM)
        return("Length must not be >" STR(MAX_DIM));
    if( params->zz > MAX_DIM)
        return("Height must not be >" STR(MAX_DIM));
    if( params->xx < 2)
        return("Width must be at least 2.");
    if( params->yy < 2)
        return("Length must be at least 2.");
    if( params->zz < 1)
        return("Height must be at least 1.");

    return NULL;
}

static char *new_game_desc(game_params *params, random_state *rs,
			   char **aux, int interactive)
{
	/*
	 * This does not create the maze
	 *
	 * instead I will try and use UI for that
	 */

	/*
	 * Format for desc - 2 char xx, t char yy, t char zz, 1 char targetd, 1 char isdouble, seed
	 */

	char *ret;


	/* Check for sanity */

	if(params->xx < 0) params->xx *= -1;
	if(params->yy < 0) params->yy *= -1;
	if(params->zz < 0) params->zz *= -1;


	/* If no seed then make one - use letters A-Z*/

	if(!params->seed)
	{
		params->seed = random_upto(rs, 0xfffffff);
	}

	ret = snewn(20, char);
	sprintf(ret, "%dx%dx%d%c%c%c%d",
		params->xx % 100, params->yy % 100, params->zz % 100,
		encode_target(params->targetd),
		encode_display(params->display),
		params->isdouble ? 'd' : 's',
		params->seed % 1000000000);


	return ret;


}

static char *validate_desc(game_params *params, char *desc)
{

	game_params *p = default_params();
	decode_params(p, desc);
	if(!p->valid)
	{
		return "Format needs to be something like '4x4x4AS12345' - see docs for details";
	}
	sfree(p);
	return NULL;
}


#define ng_dim_s(x) if(x < 1) x = 1; if (x > 20) x = 20

static game_state *new_game(midend *me, game_params *params, char *desc)
{

    int x, y, z, d, m, t;
    long moves, rots;
    game_state *state = snew(game_state);


    /*
     * I use the params to handle desc
     * So I will create my own copy  here
     */

    game_params *pars = default_params();

    decode_params(pars, desc);
    
    /*
     * To do some sanity checks and alterations
     */

    ng_dim_s(pars->xx);
    ng_dim_s(pars->yy);
    ng_dim_s(pars->zz);

    /* The minimum dimension is 1 * 2 * 3 */

    m = 0;
    if((pars->xx == 1 && pars->yy == 1) ||
	(pars->xx == 1 && pars->zz == 1) ||
	(pars->yy == 1 && pars->zz == 1)) m = -1;
    if(pars->xx <= 2 && pars->yy <= 2 && pars->zz <= 2) m = -1;
    if(m)
    {
	pars->xx = 3;
	pars->yy = 2;
	pars->zz = 1;
    }


    state->maze = maze_new(pars->xx, pars->yy, pars->zz,
			    pars->targetd, pars->isdouble,
			    pars->seed);


    /*
    maze_ascii(state->maze, stdout);
     */

    /*
     * getpos gets ints, I store chars, be careful! 
     */
    maze_getpos(state->maze, &x, &y, &z, &d, &m, &t, &moves, &rots);
    maze_setmode(state->maze, pars->display);
    state->x = x;
    state->y = y;
    state->z = z;
    state->mark = 0;	/* A special value I will use for the first state */
    state->target = t;
    state->dir = d;
    state->moves = moves;
    state->rotations = rots;

    free_params(pars);

    return state;
}

static game_state *dup_game(game_state *state)
{
    game_state *ret = snew(game_state);
   

    *ret = *state;	/* Structure copy */

    /* The following incrments reference count, and ret the same */
    ret->maze = maze_copy(state->maze);

    return ret;
}

static void free_game(game_state *state)
{
    /* This decrements the count and frees on zero */
    maze_free(state->maze);
    sfree(state);
}

static char *solve_game(game_state *state, game_state *currstate,
			char *aux, char **error)
{
	return dupstr("=");
}

static int game_can_format_as_text_now(game_params *params)
{
    return TRUE;
}

static char *game_text_format(game_state *state)
{
    return NULL;
}

/* game_ui needs to be defined else chnged_state is not called */
static game_ui *new_ui(game_state *state)
{
   game_ui *ui = snew(game_ui);
   return ui;
}

static void free_ui(game_ui *ui)
{
	sfree(ui);
}

static char *encode_ui(game_ui *ui)
{
	return dupstr("0");
}

static void decode_ui(game_ui *ui, char *encoding)
{
}

static void game_changed_state(game_ui *ui, game_state *oldstate,
                               game_state *newstate)
{
	/*
	 * this simply updates the maze accordingly
	 *
	 * I think restarts are doing by running new_game again
	 */
	maze_setpos(newstate->maze, newstate->x, newstate->y, newstate->z, newstate->dir, newstate->mark, newstate->moves,
			newstate->rotations);
}

struct game_drawstate {
    int tilesize;
    int targrad;
};

static char *interpret_move(game_state *state, game_ui *ui, game_drawstate *ds,
			    int x, int y, int button)
{
	char move[2];

	*move = -1;
	/*
	 * The tilesize is the entire screen
	 *
	 * This assumes screen of 450 x 450
	 */

	if(ds->tilesize)
	{
		x *=  450 / ds->tilesize;
		y *=  450 / ds->tilesize;
	}

	switch(button & ~MOD_MASK) {
	case LEFT_BUTTON:	/* Left button - but WHERE */
		if     (  0 <= x && x < 100 &&   0 <= y && y < 100) *move = MAZE_ANTICLOCK;
		else if(350 <= x && x < 450 && 350 <= y && y < 450) *move = MAZE_ANTICLOCK;
		else if(  0 <= x && x < 100 && 350 <= y && y < 450) *move = MAZE_CLOCK;
		else if(350 <= x && x < 450 &&   0 <= y && y < 100) *move = MAZE_CLOCK;
		else if(100 <= x && x < 350 &&   0 <= y && y < 100) *move = MAZE_UP;
		else if(350 <= x && x < 450 && 100 <= y && y < 350) *move = MAZE_RIGHT;
		else if(100 <= x && x < 350 && 350 <= y && y < 450) *move = MAZE_DOWN;
		else if(  0 <= x && x < 100 && 100 <= y && y < 350) *move = MAZE_LEFT;
		else if(100 <= x && x < 350 && 100 <= y && y < 350) *move = MAZE_FORWARD;
		break;
	case RIGHT_BUTTON:
	case 'M':
	case 'm':
	case '0':
			*move = MAZE_MARK;
			break;
	case '8':
	case CURSOR_UP:
			*move = MAZE_UP;
			break;
	case '6':
	case CURSOR_RIGHT:
			*move = MAZE_RIGHT;
			break;
	case '2':
	case CURSOR_DOWN:
			*move = MAZE_DOWN;
			break;
	case '4':
	case CURSOR_LEFT:
			*move = MAZE_LEFT;
			break;
	case CURSOR_SELECT:
			*move = MAZE_FORWARD;
			break;
	case CURSOR_SELECT2:
			*move = MAZE_MARK;
			break;
	case '9':
	case '1':
	case 'c':
	case 'C':
			*move = MAZE_CLOCK;
			break;
	case '7':
	case '3':
	case 'a':
	case 'A':
			*move = MAZE_ANTICLOCK;
			break;
	case ' ':
	case '+':
	case '5':
			*move = MAZE_FORWARD;
			break;
	case 'B':
	case 'b':
	case '-':
			*move = MAZE_BACK;
			break;
	case 'T':
	case 't':
	case '*':
			*move = MAZE_TURN;
			break;
	default:
		return NULL;
		break;
	}

	*move += 'A';	/* Better than '0' in cas I need > 9 here sometime */
	move[1] = 0;
	return dupstr(move);
}

static game_state *execute_move(game_state *state, char *move)
{
	/*
	 * I will execute the move here, even though I zap to it in changed_state
	 * It is a bit inneficient but I do not see how else I can easily
	 * do this
	 */
	char mv = -1;
	game_state *ret;
	int valmove = TRUE;
	int x, y, z, d, m, t;
	long moves, rots;
	int mode;

	if(strlen(move) != 1) return NULL;

	/*
	 * First - a special one to solve things
	 */

	if(*move == '=')
	{
		mode = maze_getmode(state->maze);
		maze_setmode(state->maze, (mode & 4) ? mode & ~4 : mode | 4);
		valmove = TRUE;
	}
	else
	{
		mv = *move - 'A';

		switch(mv) {
		case MAZE_FORWARD:
		case MAZE_TURN:
		case MAZE_LEFT:
		case MAZE_RIGHT:
		case MAZE_UP:
		case MAZE_DOWN:
		case MAZE_MARK:
		case MAZE_BACK:
		case MAZE_CLOCK:
		case MAZE_ANTICLOCK:
			if(!maze_move(state->maze, mv))
				valmove = TRUE;
		break;
		}
	}
	if(valmove)
	{
		ret = dup_game(state);
		/* Use temp ints because of type conflicts */
    		maze_getpos(state->maze, &x, &y, &z, &d, &m, &t, &moves, &rots);
    		ret->x = x;
    		ret->y = y;
    		ret->z = z;
    		ret->mark = m;
    		ret->dir = d;
		ret->target = t;
		ret->moves = moves;
		ret->rotations = rots;;
		return ret;
	}
	return NULL;
				
}
		
/* ----------------------------------------------------------------------
 * Drawing routines.
 */

static void game_compute_size(game_params *params, int tilesize,
			      int *x, int *y)
{

    *x = tilesize + (tilesize/3);
    *y = tilesize;
}

static void game_set_size(drawing *dr, game_drawstate *ds,
			  game_params *params, int tilesize)
{
    ds->tilesize = tilesize;
}

#define RGB_R(r,i) (r[(i) * 3])
#define RGB_G(r,i) (r[((i) * 3) + 1])
#define RGB_B(r,i) (r[((i) * 3) + 2])
static float *game_colours(frontend *fe, int *ncolours)
{
    float *ret = snewn(3 * NCOLOURS, float);

    frontend_default_colour(fe, &ret[COL_BACKGROUND * 3]);

    RGB_R(ret, COL_WHITE) = 1.0;
    RGB_G(ret, COL_WHITE) = 1.0;
    RGB_B(ret, COL_WHITE) = 1.0;

    RGB_R(ret, COL_LINE) = 0.0;
    RGB_G(ret, COL_LINE) = 0.0;
    RGB_B(ret, COL_LINE) = 0.0;

    RGB_R(ret, COL_MARK) = 1.0;
    RGB_G(ret, COL_MARK) = 0.5;
    RGB_B(ret, COL_MARK) = 0.5;

    RGB_R(ret, COL_TARGET) = 1.0;
    RGB_G(ret, COL_TARGET) = 1.0;
    RGB_B(ret, COL_TARGET) = 0.0;

    RGB_R(ret, COL_THREAD) = 1.0;
    RGB_G(ret, COL_THREAD) = 1.0;
    RGB_B(ret, COL_THREAD) = 0.0;

    RGB_R(ret, COL_E) = 1.0;
    RGB_G(ret, COL_E) = 0.7;
    RGB_B(ret, COL_E) = 0.7;

/*
    RGB_R(ret, COL_S) = 1.0;
    RGB_G(ret, COL_S) = 0.7;
    RGB_B(ret, COL_S) = 0.7;
 */

/*
    RGB_R(ret, COL_W) = 0.7;
    RGB_G(ret, COL_W) = 1.0;
    RGB_B(ret, COL_W) = 0.7;
 */

    RGB_R(ret, COL_N) = 0.7;
    RGB_G(ret, COL_N) = 1.0;
    RGB_B(ret, COL_N) = 0.7;

    RGB_R(ret, COL_D) = 0.7;
    RGB_G(ret, COL_D) = 0.7;
    RGB_B(ret, COL_D) = 1.0;

    RGB_R(ret, COL_U) = 0.7;
    RGB_G(ret, COL_U) = 0.7;
    RGB_B(ret, COL_U) = 1.0;
/*
    RGB_R(ret, COL_WALL) = 1.0;
    RGB_G(ret, COL_WALL) = 0.7;
    RGB_B(ret, COL_WALL) = 0.7;

    RGB_R(ret, COL_FLOOR) = 0.7;
    RGB_G(ret, COL_FLOOR) = 1.0;
    RGB_B(ret, COL_FLOOR) = 0.7;

    RGB_R(ret, COL_CIEL) = 0.7;
    RGB_G(ret, COL_CIEL) = 1.00;
    RGB_B(ret, COL_CIEL) = 0.7;
 */

    RGB_R(ret, COL_FOG) = 0.7;
    RGB_G(ret, COL_FOG) = 0.7;
    RGB_B(ret, COL_FOG) = 0.7;

    *ncolours = NCOLOURS;
    return ret;
}

static game_drawstate *game_new_drawstate(drawing *dr, game_state *state)
{
    struct game_drawstate *ds = snew(struct game_drawstate);

    ds->tilesize = 0;
    ds->targrad = 0;

    return ds;
}

static void game_free_drawstate(drawing *dr, game_drawstate *ds)
{
    sfree(ds);
}


void word_rect(drawing *dr, double sz, int left, int top, int width, int height, int col, char *word);
char  *letter_compass(int p);

#define SEE_DIST 8
#define scale(p) (int)((p) * sz)

static void game_redraw(drawing *dr, game_drawstate *ds, game_state *oldstate,
			game_state *state, int dir, game_ui *ui,
			float animtime, float flashtime)
{
    /*
     * The initial contents of the window are not guaranteed and
     * can vary with front ends. To be on the safe side, all games
     * should start by drawing a big background-colour rectangle
     * covering the whole window.
     */
   /*             0             1          2          3            4          5              6         7 */
   /*             25     15    25     12    22     10    18    8    15     7    13        6    12   6   11      6 */
    int d_l[] = {0, 25,      40,  65,    77,  99,    109, 127,   135, 150,   157, 170,   176, 188,  194, 205,  209} ;
    int d_h[] = {450, 425,   410, 385,   373, 351,   341, 323,   315, 300,   293, 280,   274, 262,  256, 245,  241} ;
    int i,  a, b;

    int wl, wr, wu, wd, wf;
    int mk;
    int colline;
    int thread = -1;
    MAZEVIEW view;
    int targ=0;
    char buf[32];
    double sz = ((double)ds->tilesize) / 450.0;
    int mkl, mkh;



/*
 * Instructions - maybe remove at some point
 */

   char *inst[] = {
	"Click on centre to",
	"move forward.",
	"Edges to rotate",
	"(pitch, yaw).",
	"Corners to spin",
	"(roll).",
	"Right click to mark."};

    maze_view(state->maze, &view);
   if(sz == 0) sz = 450;


    if(animtime != 0)
    {
    	if(ds->targrad)
	{
		int frame = (int)(animtime / ANIM_TIME);
		int radius = scale(225 - ds->targrad);
		if(frame % 2) 
			colline = COL_WHITE;
		else
			colline = COL_TARGET;
		draw_circle(dr, scale(225), scale(225), radius, colline, COL_LINE);
		
    		draw_update(dr, 225-radius, 225-radius, radius *2, radius*2);
	}
	return;
    }

    

    draw_rect(dr, 0, 0, scale(600), scale(450),  COL_BACKGROUND);
    draw_rect(dr, 0, 0, scale(450), scale(450),  COL_WHITE);

    draw_text(dr, scale(525), scale(20), FONT_VARIABLE, INSTRUCTION_TEXT_SIZE, ALIGN_HCENTRE | ALIGN_VNORMAL, COL_LINE, inst[0]);
    draw_text(dr, scale(525), scale(36), FONT_VARIABLE, INSTRUCTION_TEXT_SIZE, ALIGN_HCENTRE | ALIGN_VNORMAL, COL_LINE, inst[1]);
    draw_text(dr, scale(525), scale(56), FONT_VARIABLE, INSTRUCTION_TEXT_SIZE, ALIGN_HCENTRE | ALIGN_VNORMAL, COL_LINE, inst[2]);
    draw_text(dr, scale(525), scale(72), FONT_VARIABLE, INSTRUCTION_TEXT_SIZE, ALIGN_HCENTRE | ALIGN_VNORMAL, COL_LINE, inst[3]);
    draw_text(dr, scale(525), scale(92), FONT_VARIABLE, INSTRUCTION_TEXT_SIZE, ALIGN_HCENTRE | ALIGN_VNORMAL, COL_LINE, inst[4]);
    draw_text(dr, scale(525), scale(108), FONT_VARIABLE, INSTRUCTION_TEXT_SIZE, ALIGN_HCENTRE | ALIGN_VNORMAL, COL_LINE, inst[5]);
    draw_text(dr, scale(525), scale(128), FONT_VARIABLE, INSTRUCTION_TEXT_SIZE, ALIGN_HCENTRE | ALIGN_VNORMAL, COL_LINE, inst[6]);
/*
    draw_text(dr, scale(525), scale(144), FONT_VARIABLE, INSTRUCTION_TEXT_SIZE, ALIGN_HCENTRE | ALIGN_VNORMAL, COL_LINE, inst[7]);
 */

/*
 * Drawing colours - I will give it a go
 *
 * Start from the back and go in....
 */


/*
 * Some totals
 */

    draw_line(dr, scale(450), scale(135), scale(600), scale(135), COL_LINE);

    sprintf(buf, "%d", (int)view.moves);
    draw_text(dr, scale(460), scale(165), FONT_VARIABLE, 15, ALIGN_HLEFT | ALIGN_VNORMAL, COL_LINE, "Moves:");
    word_rect(dr, sz, 515, 145, 75, 20, COL_WHITE, buf);

    sprintf(buf, "%d", (int)view.rotations);
    draw_text(dr, scale(460), scale(195), FONT_VARIABLE, 15, ALIGN_HLEFT | ALIGN_VNORMAL, COL_LINE, "Turns:");
    word_rect(dr, sz, 515, 175, 75, 20, COL_WHITE, buf);

    sprintf(buf, "%d", (int)view.marks);
    draw_text(dr, scale(460), scale(225), FONT_VARIABLE, 15, ALIGN_HLEFT | ALIGN_VNORMAL, COL_LINE, "Marks:");
    word_rect(dr, sz, 515, 205, 75, 20, COL_WHITE, buf);

    draw_line(dr, scale(450), scale(235), scale(600), scale(235), COL_LINE);

    snprintf(buf, 30, "Size - %d x %d x %d", view.size_x, view.size_y, view.size_z);
    draw_text(dr, scale(525), scale(260), FONT_VARIABLE, 15, ALIGN_HCENTRE | ALIGN_VNORMAL, COL_LINE, buf);
/*
 * A direction pointer or coordinates
 */


    switch(view.mode & 3) {
    case DISPLAY_DIRDIST:
   
    	sprintf(buf, "%d", (int)view.howfar);
    	draw_text(dr, scale(460), scale(290), FONT_VARIABLE, 15, ALIGN_HLEFT | ALIGN_VNORMAL, COL_LINE, "Distance:");
    	word_rect(dr, sz, 535, 270, 55, 20, COL_WHITE, buf);
	

    	draw_text(dr, scale(460), scale(320), FONT_VARIABLE, 15, ALIGN_HLEFT | ALIGN_VNORMAL, COL_LINE, "Direction:");
    	word_rect(dr, sz, 535, 300, 55, 20, COL_WHITE, view.direction);

	break;
    case DISPLAY_COORD:
    	sprintf(buf, "%d-%d-%d", view.x, view.y, view.z);
    	draw_text(dr, scale(525), scale(290), FONT_VARIABLE, 15, ALIGN_HCENTRE | ALIGN_VNORMAL, COL_LINE, "Current location");
    	word_rect(dr, sz, 460, 295, 130, 20, COL_WHITE, buf);
	break;
    }
	


    draw_line(dr, scale(450), scale(330), scale(600), scale(330), COL_LINE);

   /*
    * A compass
    */

    if((view.mode & 4) && (!view.cell[0].target))
    {
    	thread = view.cell[0].d_thread;
    }

    draw_line(dr, scale(525), scale(360), scale(525), scale(420), COL_LINE);
    draw_line(dr, scale(495), scale(390), scale(555), scale(390), COL_LINE);
    draw_line(dr, scale(504), scale(369), scale(546), scale(411), COL_LINE);

    word_rect(dr, sz, (515), (340), (20), (20), thread == MAZE_D_U ? COL_THREAD : COL_WHITE, letter_compass(view.compass[MAZE_D_U]));
    word_rect(dr, sz, (515), (420), (20), (20), thread == MAZE_D_D ? COL_THREAD : COL_WHITE, letter_compass(view.compass[MAZE_D_D]));
    word_rect(dr, sz, (475), (380), (20), (20), thread == MAZE_D_L ? COL_THREAD : COL_WHITE, letter_compass(view.compass[MAZE_D_L]));
    word_rect(dr, sz, (555), (380), (20), (20), thread == MAZE_D_R ? COL_THREAD : COL_WHITE, letter_compass(view.compass[MAZE_D_R]));
    word_rect(dr, sz, (484), (349), (20), (20), thread == MAZE_D_F ? COL_THREAD : COL_WHITE, letter_compass(view.compass[MAZE_D_F]));
    word_rect(dr, sz, (546), (411), (20), (20), thread == MAZE_D_B ? COL_THREAD : COL_WHITE, letter_compass(view.compass[MAZE_D_B]));


   

    mazedraw_square(dr, sz, 0, col_wall(MAZE_D_F, &view));
   
    if(view.distance)
    {
    	if(view.distance > 8)
		a = 16;
    	else
		a = (view.distance * 2) - 1;

    	mazedraw_romb(dr, sz, 0, 0, d_l[a], d_l[a], 'H', col_wall(MAZE_D_U, &view));
    	mazedraw_romb(dr, sz, 450, 0, d_h[a], d_l[a], 'V', col_wall(MAZE_D_R, &view));
    	mazedraw_romb(dr, sz, 0, 450, d_l[a], d_h[a], 'H', col_wall(MAZE_D_D, &view));
    	mazedraw_romb(dr, sz, 0, 0, d_l[a], d_l[a], 'V', col_wall(MAZE_D_L, &view));
   }

    
    i = 0;
    a = 0;
    b = 1;

    

    for(;;)
    {
	wu = view.cell[i].wall[MAZE_D_U] ? -1 : 0;
	wd = view.cell[i].wall[MAZE_D_D] ? -1 : 0;
	wl = view.cell[i].wall[MAZE_D_L] ? -1 : 0;
	wr = view.cell[i].wall[MAZE_D_R] ? -1 : 0;
	wf = view.cell[i].wall[MAZE_D_F] ? -1 : 0;

	

        mk = view.cell[i].mark;

        if(mk)
		colline = COL_MARK;
	else
		colline = COL_LINE;

	if(view.cell[i].target)
        {
		targ = (d_l[a] + d_l[b]) / 2;
		ds->targrad = targ;
	}

	/* First the walls of the trajectories */
	if(!wu)
	{
		mazedraw_rect(dr, sz, d_l[b], d_l[a], d_h[b],  d_l[b], col_wall(MAZE_D_F, &view));
		mazedraw_triangle(dr, sz, d_l[a], d_l[a], d_l[b], d_l[b], d_l[b], d_l[a], col_wall(MAZE_D_L, &view));
		mazedraw_triangle(dr, sz, d_h[a], d_l[a], d_h[b], d_l[b], d_h[b], d_l[a], col_wall(MAZE_D_R, &view));
		draw_line(dr, scale(d_l[b]), scale(d_l[a]), scale(d_l[b]), scale(d_l[b]), COL_LINE);
		draw_line(dr, scale(d_h[b]), scale(d_l[a]), scale(d_h[b]), scale(d_l[b]), COL_LINE);
	}
	if(!wr)
	{
		mazedraw_rect(dr, sz, d_h[b], d_l[b], d_h[a],  d_h[b], col_wall(MAZE_D_F, &view));
		mazedraw_triangle(dr, sz, d_h[a], d_l[a], d_h[b], d_l[b], d_h[a], d_l[b], col_wall(MAZE_D_U, &view));
		mazedraw_triangle(dr, sz, d_h[a], d_h[a], d_h[b], d_h[b], d_h[a], d_h[b], col_wall(MAZE_D_D, &view));
		draw_line(dr, scale(d_h[a]), scale(d_l[b]), scale(d_h[b]), scale(d_l[b]), COL_LINE);
		draw_line(dr, scale(d_h[a]), scale(d_h[b]), scale(d_h[b]), scale(d_h[b]), COL_LINE);
	}
	if(!wd)
	{
		mazedraw_rect(dr, sz, d_l[b], d_h[b], d_h[b],  d_h[a], col_wall(MAZE_D_F, &view));
		mazedraw_triangle(dr, sz, d_h[a], d_h[a], d_h[b], d_h[b], d_h[b], d_h[a], col_wall(MAZE_D_R, &view));
		mazedraw_triangle(dr, sz, d_l[a], d_h[a], d_l[b], d_h[b], d_l[b], d_h[a], col_wall(MAZE_D_L, &view));
		draw_line(dr, scale(d_h[b]), scale(d_h[a]), scale(d_h[b]), scale(d_h[b]), COL_LINE);
		draw_line(dr, scale(d_l[b]), scale(d_h[a]), scale(d_l[b]), scale(d_h[b]), COL_LINE);
	}
	if(!wl)
	{
		mazedraw_rect(dr, sz, d_l[a], d_l[b], d_l[b],  d_h[b], col_wall(MAZE_D_F, &view));
		mazedraw_triangle(dr, sz, d_l[a], d_h[a], d_l[b], d_h[b], d_l[a], d_h[b], col_wall(MAZE_D_D, &view));
		mazedraw_triangle(dr, sz, d_l[a], d_l[a], d_l[b], d_l[b], d_l[a], d_l[b], col_wall(MAZE_D_U, &view));
		draw_line(dr, scale(d_l[a]), scale(d_h[b]), scale(d_l[b]), scale(d_h[b]), COL_LINE);
		draw_line(dr, scale(d_l[a]), scale(d_l[b]), scale(d_l[b]), scale(d_l[b]), COL_LINE);
	}

	/* If NOT the first draw front of the square */
	if(i)
	{
		/*
		 * This is technically if back = wall, but as back is always off (other we
		 * cannot see it) just check for wall
		 */
		if(!wu || mk) draw_line(dr, scale(d_l[a]), scale(d_l[a]), scale(d_h[a]), scale(d_l[a]), colline);
		if(!wr || mk) draw_line(dr, scale(d_h[a]), scale(d_l[a]), scale(d_h[a]), scale(d_h[a]), colline);
		if(!wd || mk) draw_line(dr, scale(d_h[a]), scale(d_h[a]), scale(d_l[a]), scale(d_h[a]), colline);
		if(!wl || mk) draw_line(dr, scale(d_l[a]), scale(d_h[a]), scale(d_l[a]), scale(d_l[a]), colline);
	}

	/* Now the corners */
	if(wl == wu || mk) draw_line(dr, scale(d_l[a]), scale(d_l[a]), scale(d_l[b]), scale(d_l[b]), colline);
	if(wu == wr || mk) draw_line(dr, scale(d_h[a]), scale(d_l[a]), scale(d_h[b]), scale(d_l[b]), colline);
	if(wr == wd || mk) draw_line(dr, scale(d_h[a]), scale(d_h[a]), scale(d_h[b]), scale(d_h[b]), colline);
	if(wd == wl || mk) draw_line(dr, scale(d_l[a]), scale(d_h[a]), scale(d_l[b]), scale(d_h[b]), colline);


	/* The edgets of the tradectories */
	if(wf == wu || mk) draw_line(dr, scale(d_l[b]), scale(d_l[b]), scale(d_h[b]), scale(d_l[b]), colline);
	if(wf == wr || mk) draw_line(dr, scale(d_h[b]), scale(d_l[b]), scale(d_h[b]), scale(d_h[b]), colline);
	if(wf == wd || mk) draw_line(dr, scale(d_h[b]), scale(d_h[b]), scale(d_l[b]), scale(d_h[b]), colline);
	if(wf == wl || mk) draw_line(dr, scale(d_l[b]), scale(d_h[b]), scale(d_l[b]), scale(d_l[b]), colline);

	if(mk)
	{
		if(!i)
		{
			mkh = 225;
			mkl = 225;
		}
		else
		{
			mkh = d_h[a];
			mkl = d_l[a];
		}
		if(wf)
		{
			draw_line(dr, scale(d_l[b]), scale(d_l[b]), scale(d_h[b]), scale(d_h[b]), colline);
			draw_line(dr, scale(d_l[b]), scale(d_h[b]), scale(d_h[b]), scale(d_l[b]), colline);
		}
		if(wu)
		{
			draw_line(dr, scale(mkl   ), scale(d_l[a]), scale(d_h[b]), scale(d_l[b]), colline);
			draw_line(dr, scale(d_l[b]), scale(d_l[b]), scale(mkh  ), scale(d_l[a]), colline);
		}
		if(wr)
		{
			draw_line(dr, scale(d_h[a]), scale(mkl   ), scale(d_h[b]), scale(d_h[b]), colline);
			draw_line(dr, scale(d_h[b]), scale(d_l[b]), scale(d_h[a]), scale(mkh   ), colline);
		}
		if(wd)
		{
			draw_line(dr, scale(mkl   ), scale(d_h[a]), scale(d_h[b]), scale(d_h[b]), colline);
			draw_line(dr, scale(d_l[b]), scale(d_h[b]), scale(mkh   ), scale(d_h[a]), colline);
		}
		if(wl)
		{
			draw_line(dr, scale(d_l[a]), scale(mkh   ), scale(d_l[b]), scale(d_l[b]), colline);
			draw_line(dr, scale(d_l[b]), scale(d_h[b]), scale(d_l[a]), scale(mkl   ), colline);
		}
	}


	if(!wf)
	{

		a++;
		b++;

		if(i == 7 && (!wf))
			mazedraw_square(dr, sz, d_l[a], COL_FOG);
		
		
		draw_line(dr, scale(d_l[a]), scale(d_l[a]), scale(d_l[b]), scale(d_l[b]), COL_LINE);
		draw_line(dr, scale(d_h[a]), scale(d_l[a]), scale(d_h[b]), scale(d_l[b]), COL_LINE);
		draw_line(dr, scale(d_h[a]), scale(d_h[a]), scale(d_h[b]), scale(d_h[b]), COL_LINE);
		draw_line(dr, scale(d_l[a]), scale(d_h[a]), scale(d_l[b]), scale(d_h[b]), COL_LINE);

	}


	if(wf) break;

	i++;
	if(i >= SEE_DIST) break;

	a++;
	b++;
    }

    if(targ)
	draw_circle(dr, scale(225), scale(225), scale(225 - targ), COL_TARGET, COL_LINE);

    if(thread > -1)
	mazedraw_thread(dr, sz, &view, targ, d_l, thread);
		
    draw_line(dr, scale(450), 0, scale(450), scale(450), COL_LINE);
    draw_update(dr, 0, 0, scale(600), scale(450));

}

void mazedraw_thread(drawing *dr, double sz, MAZEVIEW *view, int targ, int *d_l, int thread)
{
	/*
	 * Simple ideas can get REALLY complex
	 *
	 * If there are doubles it is possible to see threads behind the current one.
	 * This routine is complex because of it.
	 *
	 * It is not perfect as the thread can still be seen even if it has already
	 * hit you, but if this is a problem then I need to use the "simple" method
	 * of only showing the first thread seen, whidh is OM, however as I am writing this
  	 * to be a smart arse I will show all threads.
	 */
	int i, j;
	int top[view->distance];
	int left[view->distance];
	int width[view->distance];
	int height[view->distance];
	int across = 0;

    	int w_t[] = {20, 16, 13, 11, 10, 9, 8, 7, 6 };

	if(!targ)
	{
		for(i=0;i<view->distance;i++) top[i] = -1;
		j = 0;
   		for(i=0;i < view->distance;i++)
		{
			if(i > 7) break;
			switch(view->cell[i].d_thread) {
			case MAZE_D_B:
				across = -1;
				break;
			case MAZE_D_F:
				break;
			case MAZE_D_U:
				if(view->cell[i].wall[MAZE_D_D]) across = 0;
				left[j] = 225 - (w_t[i]/2);
				top[j] = d_l[i * 2];
				width[j] =  w_t[i];
				if(across)
					height[j] = (225 - d_l[i * 2]) *2;
				else
					height[j] = 225 - d_l[i * 2];
				across = -1;
				j++;
				break;
			case MAZE_D_R:
				if(view->cell[i].wall[MAZE_D_L]) across = 0;
				top[j] = 225 - (w_t[i]/2);
				height[j] = w_t[i];
				if(across)
				{
					left[j] = d_l[i * 2];
					width[j] = (225 - d_l[i * 2]) * 2;
				}
				else
				{
					left[j] = 225;
					width[j] = 225 - d_l[i * 2];
				}
				j++;
				across = -1;
				break;
			case MAZE_D_D:
				if(view->cell[i].wall[MAZE_D_U]) across = 0;
				left[j] = 225 - (w_t[i]/2);
				width[j] = w_t[i];
				if(across)
				{
					top [j] = d_l[i * 2];
					height[j] = (225 - d_l[i * 2]) *2;
				}
				else
				{
					top[j] = 225;
					height[j] =  225 - d_l[i * 2];
				}
				across = -1;
				j++;
				break;
			case MAZE_D_L:
				if(view->cell[i].wall[MAZE_D_R]) across = 0;
				left[j] = d_l[i * 2];
				top [j] = 225 - (w_t[i]/2);
				height[j] = w_t[i];
				if (across)
					width[j] = (225 - d_l[i * 2]) * 2;
				else
					width[j] = 225 - d_l[i * 2];
				across = -1;
				j++;
				break;
			}
		}
		while(j > 0)
		{
			j--;
			if(top[j] >= 0) word_rect(dr, sz, left[j], top[j], width[j], height[j], COL_THREAD, "");
		}
	}
    	if(thread == MAZE_D_F)
		draw_circle(dr, scale(225), scale(225), scale(w_t[0]/2), COL_THREAD, COL_LINE);
    }

int col_wall(int dir, MAZEVIEW *view)
{
	switch(view->compass[dir]) {
	case MAZE_C_N: return COL_N; break;
	case MAZE_C_S: return COL_S; break;
	case MAZE_C_W: return COL_W; break;
	case MAZE_C_E: return COL_E; break;
	case MAZE_C_U: return COL_U; break;
	case MAZE_C_D: return COL_D; break;
	default: return COL_WHITE; break;
	}
	return COL_WHITE;
}
	


void mazedraw_rect(drawing *dr, double sz, int ax, int ay, int bx, int by, int col)
{
	ax *= sz;
	ay *= sz;
	bx *= sz;
	by *= sz;

	draw_rect(dr, ax, ay, bx - ax + 1, by - ay + 1, col);
}
void mazedraw_square(drawing *dr, double sz, int topleft,int col)
{
	int botright;

	botright = 450 - topleft;

	topleft *= sz;
	botright *= sz;
	
	/*
  	 * botright also width/height...
 	 */
	botright -= topleft;
	botright++;

	draw_rect(dr, topleft, topleft, botright, botright, col);
}


void mazedraw_romb(drawing *dr, double sz, int p1x, int p1y, int p2x, int p2y, char orient, int col)
{
	int coords[8];
	int i;

	coords[0] = p1x;
	coords[1] = p1y;
	coords[2] = p2x;
	coords[3] = p2y;
	if(orient == 'H')
	{
		p1x = 450 - p1x;
		p2x = 450 - p2x;
	}
	else
	{
		p1y = 450 - p1y;
		p2y = 450 - p2y;
	}
	coords[6] = p1x;
	coords[7] = p1y;
	coords[4] = p2x;
	coords[5] = p2y;

	for(i=0;i<8;i++)
		coords[i] *= sz;

	draw_polygon(dr, coords, 4, col, col);
}

void mazedraw_triangle(drawing *dr, double sz, int p1x, int p1y, int p2x, int p2y, int p3x,int p3y, int col)
{
	int coords[6];

	coords[0] = p1x;
	coords[1] = p1y;
	coords[2] = p2x;
	coords[3] = p2y;
	coords[4] = p3x;
	coords[5] = p3y;

	draw_polygon(dr, coords, 3, col, col);
}


char *letter_compass(int p)
{
	switch(p) {
	case MAZE_C_N: return "N"; break;
	case MAZE_C_S: return "S"; break;
	case MAZE_C_W: return "W"; break;
	case MAZE_C_E: return "E"; break;
	case MAZE_C_U: return "U"; break;
	case MAZE_C_D: return "D"; break;
	default: return "?"; break;
	}
}

	
void word_rect(drawing *dr, double sz, int left, int top, int width, int height, int col, char *word)
{
	left = scale(left);
	top = scale(top);
	width = scale(width);
	height = scale(height);

	draw_rect(dr, left, top, width, height, col);
	
	draw_line(dr, left, top, left+width, top, COL_LINE);
	draw_line(dr, left+width, top, left+width, top+height, COL_LINE);
	draw_line(dr, left+width, top+height, left, top+height, COL_LINE);
	draw_line(dr, left, top+height, left, top, COL_LINE);

	if(*word) draw_text(dr, left + (width/2), top + (height/2), FONT_FIXED, height-2, ALIGN_VCENTRE | ALIGN_HCENTRE, COL_LINE, word);
}

	
	

static float game_anim_length(game_state *oldstate, game_state *newstate,
			      int dir, game_ui *ui)
{
    if(newstate->target)
	return 1.0F;
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
#define thegame maze3d
#endif

const struct game thegame = {
    "Maze 3D", "games.maze3d", "maze3d",
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
    PREFFERED_TILE_SIZE, game_compute_size, game_set_size,
    game_colours,
    game_new_drawstate,
    game_free_drawstate,
    game_redraw,
    game_anim_length,
    game_flash_length,
    FALSE, FALSE, NULL, NULL,
    FALSE,			       /* wants_statusbar */
    FALSE, game_timing_state,
    REQUIRE_LARGE_SCREEN,	       /* flags */
};

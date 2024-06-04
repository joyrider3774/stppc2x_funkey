#ifndef __MAZE3DC_H
#define __MAZE3DC_H


/*
 * ******************** Some initial deffinitions... *************************
 */

typedef struct MAZE MAZE;
typedef struct MAZEVIEW MAZEVIEW;


/*
 * The maximum size of a maze - should not be too big
 * It uses 2 bytes per cell, so a 100 x 100 x 100 maze
 * would use 2 mega-bytes of RAM (and the rest)
 * It would also take a long time to generatem and
 * probably YEARS to solve
 *
 * Even 20 x 20 x 20  takes a long time, but that is the max 
 * for now
 */
#define MAX_DIM 20


/*
 * ******************* Now for the methods.... ****************************
 */
	

/*
 * The constructor - that creates the maze
 *
 * xx, yy, zz are the dimensions from 1 to 20,
 *     the smallest maze hase dims 3,2,1
 * target is to decode if you want the target to be in the corner(default),
 *       edge, face or anywhere, use...
 *       MAZE_CORNER, MAXE_EDGE, MAZE_FACE or MAZE_ANYWHERE
 *       Which ALWAYS index to 0, 1, 2, 3 respectively
 * isdouble means that there will be double routes and circles
 *    (walls get knocked down after initialization to make it harder)
 * seed is the seed for the number random generator.  The same seeds
 *     will generate the same maze.  0 a random seed is generated
 *     for you
 *
 * Returns a new ponter to the maze if successful, NULL if not
 */

MAZE *maze_new(int xx, int yy, int zz, int target, int isdouble, int seed);


#define MAZE_CORNER 0
#define MAZE_EDGE 1
#define MAZE_FACE 2
#define MAZE_ANYWHERE 3
/*
 * This does not copy, but increments a referrence count
 * It returns a reference to the same maze object
 */
MAZE *maze_copy(MAZE *maze);
 
/*
 * This unrefs the reference count, and frees on the last one
 */
void maze_free(MAZE *maze);


/* This resets the maze to the beginning */
void maze_reset(MAZE *maze);


/*
 * This transports the user to the appropriate cell
 * The x, y and z are co-ordinates
 * d is the direction, bet-ored between the following two:
 *   one of the MAZE_T MAZE_F_* - Which way you are facing
 *   one of the MAZE_T MAZE_T_* - Which way the top (of the screen) is faceing
 *   Where (*) is N, S, W, E, U, D == North, South, West, East, Up, Down
 *   Care should be taken because not all combos are valid  however, an
 *          invalid one (like -1) will keep the current direction
 * m is set to 1 if a mark is to be placed, o if it is to be removed and
 *    a value over 100 to be ignored
 * moves and rotations ignored if set to -1
 *
 * returns 0 on success, -1 on cannot be placed (i.e - outside maze)
 *
 */
int maze_setpos(MAZE *maze, int x, int y, int z, int d, int m, long moves, long rotations);

/*
 * The oposite of setpos is... (Quicker than view)
 *
 * t is the target, it is 1 if you are on it, 2 if one away iand faceing it 
 * and 0 if cannot see
 * (Those are the only possibilities
 */
void maze_getpos(MAZE *maze, int *x, int *y, int *z, int *d, int *m, int *t, long *moves, long *rotations);
/*
 * the following moves you
 *
 * MAZE_FORWARD MAZE_BACK moves you 1 cell forward or backward (if it can - no walls)
 *	these are the only ones that actually moves you
 *
 * MAZE_LEFT, MAZE_RIGHT, MAZE_UP, MAZE_DOWN will rotate you in that direction
 *           as in "yaw" and "pitch"
 * MAZE_TURN will turn you around
 *
 * MAZE_CLOCK and MAZE_ANTICLOCK will orient you clockwise or anti-clockwise
 *     as in "roll"
 *
 * MAZE_MARK will place a mark at you current position if there is none, or
 *           removes one if there is
 *
 * Moves and rotations are the number of each
 */


int maze_move(MAZE *maze, char move);


/*
 *Mmaze_view populates a structure the front end uses to draw the maze
 * the MAZEVIEW structure is defined (with some comemnts) below.
 * It should ALLWAYS return 0, however it will return -1 if it comes across
 * an error, but that should not happen and it is really abort time if it does
 *
 */
int maze_view(MAZE *maze, MAZEVIEW *view);


/*
 * FILE operations - fp needs to be open in propper mode
 */

/*
 * prints an ASCII mal of the maze, originally written for debugging */
void maze_ascii(MAZE *maze, FILE *fp);

/* Saves the maze to a file */
void maze_save(MAZE *maze, FILE *fp);

/* Loads it up again */
MAZE *maze_load(FILE *fp);


/*
 * The maze structure has a miscellaneous mode field which
 * it maze3dc does not use itself, but is there for front enders.
 * You can set it and get it with setmode and getmode, it also
 * is copied into the mode field  of the "MAZEVIEW" structure
 * in maze_view()
 */
long maze_getmode(MAZE *maze);
void maze_setmode(MAZE *maze, long mode);

/*
 * **************************** The maze view structure **********************
 *
 * It uses the Compass Directions and Relative Directions defined below it
 */
struct MAZEVIEW {
	int size_x, size_y, size_z; /* The size of the maze */
	char distance;	/* As far as you can see, this is (for now) max of 10 */
	char compass[6]; /* The compass positions (index being relative positions) */
	double howfar; /* The direct distance from the target as crow flies */
	char direction[8]; /* The direct direction hint as a string */
	struct {
		char wall[6];	/* The walls , set for wall, 0 for not. Relative positions */
		char c_thread;	/* Compass direction of thread */
		char d_thread;	/* relative direction of thread */
		char mark;	/* Set for marked */
		char target;	/* Set for target */
	} cell[MAX_DIM];
	char x, y, z;	/* The position in the maze */
	long moves;	/* The total number of moves to date */
	long marks;	/* The total number of markers to date */
	long rotations;	/* The total number of rotations to date */
	long mode;	/* A miscellaneous user defined mode */
};


/*
 * Compass directions used above
 */
#define MAZE_C_N 0	/* North */
#define MAZE_C_S 1	/* South */
#define MAZE_C_W 2	/* West */
#define MAZE_C_E 3	/* East */
#define MAZE_C_U 4	/* Up */
#define MAZE_C_D 5	/* Down */

/*
 * Relative Directions
 */
#define MAZE_D_F 0	/* Forward */
#define MAZE_D_B 1	/* Back */
#define MAZE_D_L 2	/* Left */
#define MAZE_D_R 3	/* Right */
#define MAZE_D_U 4	/* Up */
#define MAZE_D_D 5	/* Down */


/*
 * ************************** Other Deffinitions ***************************
 *
 * The "maze_setpos" directions, a "Face" integer (MAZE_F_*) is bit-ored with a
 * "Top" (MAZE_T_*) integer to get the direction
 *
 * Where (*) is N, S, W, E, U, D == North, South, West, East, Up, Down
 */
/*
 * These four bits deal with where we are faceing
 */
#define MAZE_F_N 0x00
#define MAZE_F_S 0x10
#define MAZE_F_W 0x20
#define MAZE_F_E 0x30
#define MAZE_F_U 0x40
#define MAZE_F_D 0x50

/*
 * These with where the top is
 */
#define MAZE_T_N 0x00
#define MAZE_T_S 0x01
#define MAZE_T_W 0x02
#define MAZE_T_E 0x03
#define MAZE_T_U 0x04
#define MAZE_T_D 0x05

/*
 * The movement macros defined here..
 */
#define MAZE_FORWARD 0
#define MAZE_TURN 1
#define MAZE_LEFT 2
#define MAZE_RIGHT 3
#define MAZE_UP 4
#define MAZE_DOWN 5
#define MAZE_CLOCK 6
#define MAZE_ANTICLOCK 7
#define MAZE_BACK 8
#define MAZE_MARK 9


#endif

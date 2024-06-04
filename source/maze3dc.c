/*

MAZE 3d CLASS PROGRAM (maze3dc.c)

VERSION 2 - 15-jan-2006 - Placed randomness in make_getlost
                          where furthest point equals current point

There is a good chance you do not want to be looking here.  For a description
of the interface and how to use see the "maze3dc.h" file.  Unless you are
interested in the hacking elements of this you will get bogged down


In this, the following number represents each digit:

0 - North
1 - South
2 - West
3 - East
4 - Up
6 - Down

I have some "defines" defining that of course, but it
cannot be so everywhere.  So I am documenting that first.
 */



/*
 * I am putting theinternal header file as part of the maoin one
 * to keep the maze object as one file.   I may split it out later
 * I do not know
 */
#ifndef __MAZE3DCINT_H
#define __MAZE3DCINT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "maze3dc.h"
#include "puzzles.h"

/*
 * the structure - where everything happens!
 */

typedef unsigned int MAZECELL;

struct MAZE {
	MAZECELL *cell;		/* A (sort of 3d)  array of the cells of the maze */
	int xx, yy, zz;		/* The dimension sizes  of the mize */
	int target;		/* 0 is end target not neccessarily in corner */
	int t_xx, t_yy, t_zz;	/* The dimensions-1, used a lot so optimized here */
	long seed;		/* The random seed */
	int tar_x, tar_y, tar_z;	/* Co-ordinates of target cell */
	int sec_x, sec_y, sec_z;	/* Co-ordinates of second cell (next to target) */
	int c_x, c_y, c_z, c_d;		/* Working current co-ordinates used in generation */
	int lost_x, lost_y, lost_z;	/* Starting position */
	int lost_dir;			/* Starting direction */
	int tracklen;			/* Used in generation - length of "tracks" or "spurs" */
	int cntref;			/* Reference count used for garbage collection */
	long moves;			/* The number of moves taken */
	long marks;			/* The number of marks out there */
	long rotations;			/* The number of rtations taken */
	long mode;			/* A miscellaneous mode */
};

void make_track(MAZE *maze, int t_xx, int t_yy, int t_zz, int p_d);
void make_double(MAZE *maze);
void make_getlost(MAZE *maze);


/*
 * Bit 1 ( 1) = NORTH WALL EXISTS
 * Bit 2 ( 2) = SOUTH WALL EXISTS
 * Bit 3 ( 4) = WEST WALL EXISTS
 * Bit 4 ( 8) = EAST WALL EXISTS
 * Bit 5 (16) = UP WALL EXISTS
 * Bit 6 (32) = DOWN WALL EXISTS
 */

/*
 * Redeffinition of compass directions 
 */
#define N_WALL 0
#define S_WALL 1
#define W_WALL 2
#define E_WALL 3
#define U_WALL 4
#define D_WALL 5

/*
 * Compass directions as in bit walls
 */
#define N_BIT 1
#define S_BIT 2
#define W_BIT 4
#define E_BIT 8
#define U_BIT 0x10
#define D_BIT 0x20

#define SOL_BIT 0x3f	/* Solitary confinement - all walls! */

/*
 * The rest of the cell bits
 */

#define INIT_BIT 0x40
#define TARG_BIT 0x80
#define ADJ_BIT 0x0100	/* This is adjacent to a made bit, but not made yet */
#define PEBBLE_BIT 0x0200

/*
 * These use next three bits as number (It can always be ored with 0x0400 then >> 11
 *
 * These numbers represents the thread direction for solving
*/
#define TN_BIT  0x0400
#define TS_BIT  0x0c00
#define TW_BIT  0x1400
#define TE_BIT  0x1c00
#define TU_BIT  0x2400
#define TD_BIT  0x2c00

#define THREAD_BIT 0x3c00



#define ALL_BIT 0x7fff;	/* All bits (I care about) set */
/*
 * The directons combined
 */
#define MV_NN 0x00
#define MV_NS 0x01
#define MV_NW 0x02
#define MV_NE 0x03
#define MV_NU 0x04
#define MV_ND 0x05
#define MV_SN 0x10
#define MV_SS 0x11
#define MV_SW 0x12
#define MV_SE 0x13
#define MV_SU 0x14
#define MV_SD 0x15
#define MV_WN 0x20
#define MV_WS 0x21
#define MV_WW 0x22
#define MV_WE 0x23
#define MV_WU 0x24
#define MV_WD 0x25
#define MV_EN 0x30
#define MV_ES 0x31
#define MV_EW 0x32
#define MV_EE 0x33
#define MV_EU 0x34
#define MV_ED 0x35
#define MV_UN 0x40
#define MV_US 0x41
#define MV_UW 0x42
#define MV_UE 0x43
#define MV_UU 0x44
#define MV_UD 0x45
#define MV_DN 0x50
#define MV_DS 0x51
#define MV_DW 0x52
#define MV_DE 0x53
#define MV_DU 0x54
#define MV_DD 0x55


/*
 * get an element from mve based on
 * direction value above and 
 * where we want to go
 */
#define MV_ELEMENT(c, m) (((((c) & 0xf0) >> 4) * 48) + (((c) & 0x0f) * 8) + m)

/*
 * Max number of MVE(+1), 288
 */
#define MV_MAX 288



/*
 * Find the particular cell in the maze matrix
 *
 * defined as a macro for optimization
 */
#define mazecell(mz, x, y, z) (mz->cell + ((z) + ((y) * mz->zz) + ((x) * mz->zz * mz->yy)))
/*
 *
 * The above macro as a function that error reports
 * for debugging purposes
 *
MAZECELL *dbg_mazecell(MAZE *maze, int x, int y, int z);
#define mazecell(mz, x, y, z) dbg_mazecell(mz, x, y, z)
 */


#endif

/*
 * Some functions only used locally
 */
void make_track(MAZE *maze, int t_xx, int t_yy, int t_zz, int p_d);
void make_double(MAZE *maze);
void make_getlost(MAZE *maze);
int make_spur(MAZE *maze, int tx, int ty, int tz);
int random_do(MAZE *maze, int maxnum);
int maze_load_do(FILE *fp, MAZE *maze);

/*
 * The possible directions you can fase
 * and some impossible ones too)
 */
/*
 * The movement array - where are we pointing after rotating?
 *
 * Rathert than work it out each timne will store all possible combinations
 * in an array.  The system records which way you are facing, and which way
 * the top of screen is.  Not all combinations are valid, -1 signifies error
 */

char mve[] = {

    /*                    New direction pointed to when rotated        */
    /* Pos    Cur    Bac  Left    Right  Up     Down   Clock  AntiClock */

    /* NN */  -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    /* NS */  -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    /* NW */  MV_NN, MV_SW, MV_DW, MV_UW, MV_WS, MV_EN, MV_NU, MV_ND,
    /* NE */  MV_NE, MV_SE, MV_UE, MV_DE, MV_ES, MV_WN, MV_ND, MV_NU,
    /* NU */  MV_NU, MV_SU, MV_WU, MV_EU, MV_US, MV_DN, MV_NE, MV_NW,
    /* ND */  MV_ND, MV_SD, MV_ED, MV_WD, MV_DS, MV_UN, MV_NW, MV_NE,

    /* SN */  -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    /* SS */  -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    /* SW */  MV_SW, MV_NW, MV_UW, MV_DW, MV_WN, MV_ES, MV_SD, MV_SU,
    /* SE */  MV_SE, MV_NE, MV_DE, MV_UE, MV_EN, MV_WS, MV_SU, MV_SD,
    /* SU */  MV_SU, MV_NU, MV_EU, MV_WU, MV_UN, MV_DS, MV_SW, MV_SE,
    /* SD */  MV_SD, MV_ND, MV_WD, MV_ED, MV_DN, MV_US, MV_SE, MV_SW,

    /* WN */  MV_WN, MV_EN, MV_UN, MV_DN, MV_NE, MV_SW, MV_WD, MV_WU,
    /* WS */  MV_WS, MV_ES, MV_DS, MV_US, MV_SE, MV_NW, MV_WU, MV_WD,
    /* WW */  -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    /* WE */  -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    /* WU */  MV_WU, MV_EU, MV_SU, MV_NU, MV_UE, MV_DW, MV_WN, MV_WS,
    /* WD */  MV_WD, MV_ED, MV_ND, MV_SD, MV_DE, MV_UW, MV_WS, MV_WN,

    /* EN */  MV_EN, MV_WN, MV_DN, MV_UN, MV_NW, MV_SE, MV_EU, MV_ED,
    /* ES */  MV_ES, MV_WS, MV_US, MV_DS, MV_SW, MV_NE, MV_ED, MV_EU,
    /* EW */  -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    /* WE */  -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    /* EU */  MV_EU, MV_WU, MV_NU, MV_SU, MV_UW, MV_DE, MV_ES, MV_EN,
    /* ED */  MV_ED, MV_WD, MV_SD, MV_ND, MV_DW, MV_UE, MV_EN, MV_ES,

    /* UN */  MV_UN, MV_DN, MV_EN, MV_WN, MV_ND, MV_SU, MV_UW, MV_UE,
    /* US */  MV_US, MV_DS, MV_WS, MV_ES, MV_SD, MV_NU, MV_UE, MV_UW,
    /* UW */  MV_UW, MV_DW, MV_NW, MV_SW, MV_WD, MV_EU, MV_US, MV_UN,
    /* UE */  MV_UE, MV_DE, MV_SE, MV_NE, MV_ED, MV_WU, MV_UN, MV_US,
    /* UU */  -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    /* UD */  -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,

    /* DN */  MV_DN, MV_UN, MV_WN, MV_EN, MV_NU, MV_SD, MV_DE, MV_DW,
    /* DS */  MV_DS, MV_US, MV_ES, MV_WS, MV_SU, MV_ND, MV_DW, MV_DE,
    /* DW */  MV_DW, MV_UW, MV_SW, MV_NW, MV_WU, MV_ED, MV_DN, MV_DS,
    /* DE */  MV_DE, MV_UE, MV_NE, MV_SE, MV_EU, MV_WD, MV_DS, MV_DN,
    /* DU */  -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    /* DD */  -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1  
};


/*
 
MAZE GENERATION

The following steps are done

1 - An array of cells (of unsigned shorts) is created representing the"building" the maze
    is to be.

    Each cell represents a "room" of the maze.  6 of the bits represent the "walls".  When the
    bit is set a wll is there, otherwise not.  Care ion generation is needed when one wall
    is created the same wall is created on the adjoining cell.

2 - Walls are built around the edge of the maze to stop people "falling off".

3 - A cell is defined as the "target", the destination. and walls built around it.  An adjoining cell
    is defined (the second cell), walls are built aroudn that and the wall connecting the two is "smashed".  

4 - From the seond cell a line of cells is created (not straight, but randomly squiggly), though care is
    taken that the wall oposite the target cell is not broken - ensuring you can only see the target
    when you are next to it.  The length of this line depends on the size of the maze.

5 - From this line a whole lot of "spurs" are created (of a length dependant on the size of the maze and randomly
    squiggly) at random places chosen from places on the initial line and any spurs that have already been created.
    Care is taken not to brach further walsl of the target and second cell.

    The structure of the maze is now complete

6 - From the target a process wals the maze doing the 3d equivalent of "always touching the (right) wall"
    as in a 2d one (the algorithm to do this is more complex and described in the make_getlost() below).
    As it walks it drops a pebble (if none is there) and picks it up if it is.  It knows how far away from
    the target it is by the number of pebbles it has dropped (less the ones it has picked up).  It remebers
    the point furthest away to start the player from.  It eventually ends up where it started from (the
    target cell).

    As it picks up pebbles it stores the direction it is going on the cell so the "solve" will know
    which direction to point players.

7 - If "Double Routes/Circles" have been selected then it will smash a few walls (but not target or second)
    to create them


*/

MAZE *maze_new(int xx, int yy, int zz, int target, int isdouble, int seed)
{

/*

	'xx = 1=s x=n
	'yy = 1=e,y=w
	'zz = 1=d,z=u
	'N=1,1 S=2,2 W=3,4 E=4,8 u=5,16 d=6,32
 */

	int i, j, k;
	int ii, jj, kk;
	long l;
	int diff;
	
	int t_xx, t_yy, t_zz;

	MAZECELL *cell;
	MAZE *maze;

	/*
	 * First - sanity checks to stop crashes
	 */
	if(xx > MAX_DIM || xx < 1) return NULL;
	if(yy > MAX_DIM || yy < 1) return NULL;
	if(zz > MAX_DIM || zz < 1) return NULL;

	/*
	 * The following should check for min sizes
 	 * two cannot be 1 and one must be more than 2
	 */
	if((xx == 1 && yy == 1) ||
	   (xx == 1 && zz == 1) ||
	   (yy == 1 && zz == 1)) return NULL;
	if(xx <= 2 && yy <= 2 && zz <= 2) return NULL;

	t_xx = xx - 1;
	t_yy = yy - 1;
	t_zz = zz - 1;


	maze = (MAZE *)smalloc(sizeof(MAZE));

	/*
	 * The seed is 0, find any number (Location of maze im memory maybe?
	 * 
	 */

	if(!seed) memcpy(&seed, &maze, sizeof(int));
		

	memset(maze, 0, sizeof(MAZE));	/* Should be uneccessary - but I like to do it anyway */

	maze->xx = xx;	/* The dimensions */
	maze->yy = yy;
	maze->zz = zz;
	maze->target = target;
	maze->t_xx = xx - 1;	/* The dimensions -1, set here for optimization reasons */
	maze->t_yy = yy - 1;
	maze->t_zz = zz - 1;
	maze->seed = seed >= 0 ? seed : 0 - seed;
	maze->sec_x = 0;
	maze->sec_y = 0;
	maze->sec_z = 0;

	maze->lost_x = 0;
	maze->lost_y = 0;
	maze->lost_z = 0;
	maze->lost_dir = -1;
	maze->cntref = 1;
	maze->moves = 0;
	maze->marks = 0;
	maze->rotations = 0;
	maze->mode = 0;

	maze->tracklen = (int)((xx + yy + zz) / 12);	/* The length of a track */
	cell = (MAZECELL *)smalloc(xx * yy * zz * sizeof(MAZECELL));	/* The maze cells */
	maze->cell = cell;

	
	for(i=0;i<xx;i++)
		for(j=0;j<yy;j++)
			for(k=0;k<zz;k++)
				*mazecell(maze, i, j, k) = INIT_BIT;

		
	/*
	 * To place walls around the edges...
	 */

	/*
    	 * xx = 0=s x=n
    	 * yy = 0=e,y=w
   	 * zz = 0=d,z=u
    	 * N=1,1 S=2,2 E=3,4 W=4,8 u=5,16 d=6,32
	 */

	for(j=0;j<yy;j++)
	{
		for(k=0;k<zz;k++)
		{
			*mazecell(maze, 0, j, k) |= S_BIT;
			*mazecell(maze, t_xx, j, k) |= N_BIT;
		}
	}

	for(i=0;i<xx;i++)
	{
		for(k=0;k<zz;k++)
		{
			*mazecell(maze, i, 0, k) |= E_BIT;
			*mazecell(maze, i, t_yy, k) |= W_BIT;
		}
	}
	for(i=0;i<xx;i++)
	{
		for(j=0;j<yy;j++)
		{
			*mazecell(maze, i, j, 0) |= D_BIT;
			*mazecell(maze, i, j, t_zz) |= U_BIT;
		}
	}

	/*
	 * First to create a track, or line of cells wandering through the maze
	 * It is from this the rest is spawned
	 */

	make_track(maze, -1, -1, -1, -1);	

	/*
	 * Basically - generate lots of spurs if we can randomly
	 */

	l = (xx + 1) * (yy + 1) * (zz + 1);
	for(;l;l--)
		make_spur(maze, -1, -1, -1);



	/*
	 * Now we have done that, go through
	 * each cell in turn to see if we can generate spurs
	 *
	 * To get better "randomness" the increment should
	 * really be a random generated number that is not a divisor of
	 * the size, but I could not really be bothered!
	 */

	maze->tracklen *= 2;


	ii = maze->tar_x;
	jj = maze->tar_y;
	kk = maze->tar_z;

	diff = 1;

	for(i=0;i<xx;i++, ii++)
	{
		for(j=0;j<yy;j++, jj++)
		{
			for(k=0;k<zz;k++, kk++)
			{
				if (ii >= xx) ii -= xx;
				if (jj >= yy) jj -= yy;
				if (kk >= zz) kk -= zz;
				make_spur(maze, ii, jj, kk);
			}
		}
	}

	/*
	 * Place double routes here
	 */


	/*
	 * Take the target bit off the second cell
	 */

	*mazecell(maze, maze->sec_x, maze->sec_y, maze->sec_z) &=  ~TARG_BIT;

	make_getlost(maze);

	/*
	 * Double routes/circles - the last thing
	 */
	if(isdouble)
	{
		/*
		 * Make sure we do not use target, second or styarting pos
		 * A hack, set these all to targets
		 */
		*mazecell(maze, maze->sec_x, maze->sec_y, maze->sec_z) |=  TARG_BIT;
		*mazecell(maze, maze->lost_x, maze->lost_y, maze->lost_z) |=  TARG_BIT;

		make_double(maze);

		/*
		 * Reset second and start cells
		 */
		*mazecell(maze, maze->sec_x, maze->sec_y, maze->sec_z) &=  ~TARG_BIT;
		*mazecell(maze, maze->lost_x, maze->lost_y, maze->lost_z) &=  ~TARG_BIT;
	}

	maze->c_x = maze->lost_x;
	maze->c_y = maze->lost_y;
	maze->c_z = maze->lost_z;
	maze->c_d = maze->lost_dir;

	return maze;
}

/*
 * Thi UNREF's and frees on zero
 */
void maze_free(MAZE *maze)
{
	maze->cntref--;
	if(!maze->cntref)
	{
		sfree(maze->cell);
		sfree(maze);
	}
}

/*
 * This increases the copy
 */
MAZE *maze_copy(MAZE *maze)
{
	maze->cntref++;
	return maze;
}

void makeawall(MAZECELL *ordw, int wall)
{
	/*
	 * Making adjoining walls
	 *
	 * If the cell has not been initialized then
	 * I mark it as "adjoining".  This means that
	 * I just need to look at this to know if
	 * it can be used as a start of a "spur".
	 */
	if (*ordw & INIT_BIT)
	{
		*ordw |=  ADJ_BIT;
	}
		*ordw |= wall;
}


void make_owall(MAZE *maze, MAZECELL ord, int t_xx, int t_yy, int t_zz)
{

	/*
	 * This will examine a cell and make sure adjoining cell's walls
	 * are built
	 */
	/*
	 * The position in the matrix (Compass points)
	 *
	 * xx = 0=s x=n
	 * yy = 0=e,y=w
	 * zz = 0=d,z=u
	 */

	if(ord & N_BIT) if(t_xx < maze->t_xx) makeawall(mazecell(maze, t_xx + 1, t_yy, t_zz), S_BIT);
	if(ord & S_BIT) if(t_xx > 0         ) makeawall(mazecell(maze, t_xx - 1, t_yy, t_zz), N_BIT);

	if(ord & W_BIT) if(t_yy < maze->t_yy) makeawall(mazecell(maze, t_xx, t_yy + 1, t_zz), E_BIT);
	if(ord & E_BIT) if(t_yy > 0         ) makeawall(mazecell(maze, t_xx, t_yy - 1, t_zz), W_BIT);

	if(ord & U_BIT) if(t_zz < maze->t_zz) makeawall(mazecell(maze, t_xx, t_yy, t_zz + 1), D_BIT);
	if(ord & D_BIT) if(t_zz > 0         ) makeawall(mazecell(maze, t_xx, t_yy, t_zz - 1), U_BIT);
}


int make_spur(MAZE *maze, int tx, int ty, int tz)
{
	int dd;
	MAZECELL *ord, *ordw;
	int pnumb;
	int notries, found;

	int wx, wy, wz, wdd = 0;
	int c_dd;

	if(tx == -1)	/* Don't know where I am - First go.  Lets simply decide :-) */
	{
		tx = random_do(maze, maze->xx);
		ty = random_do(maze, maze->yy);
		tz = random_do(maze, maze->zz);
	}


	ord = mazecell(maze, tx, ty, tz);

	if (!(*ord & ADJ_BIT)) return 0;	/* if not been "made" don't want to know */

	/*
	 * I need to see if I am adjacent to a made cell that is not a target
	 */

	dd = random_do(maze, 6);
	pnumb = (random_do(maze, 2) * 2) - 1;	/* pnumb is 1 or -1 */
	notries = 0;
	found = 0;
	for(;;)
	{
		wx = tx;
		wy = ty;
		wz = tz;
		switch(dd) {
		case N_WALL: wx = tx + 1; wdd = S_BIT; break;
		case S_WALL: wx = tx - 1; wdd = N_BIT; break;
		case W_WALL: wy = ty + 1; wdd = E_BIT; break;
		case E_WALL: wy = ty - 1; wdd = W_BIT; break;
		case U_WALL: wz = tz + 1; wdd = D_BIT; break;
		case D_WALL: wz = tz - 1; wdd = U_BIT; break;
		}

		if(wx >= 0 && wx < maze->xx && wy >= 0 && wy < maze->yy && wz >= 0 && wz < maze->zz) /* if I am on the wall */
		{
			ordw = mazecell(maze, wx, wy, wz);	/* See adjacent */
			if ((!(*ordw & TARG_BIT)) && (!(*ordw & INIT_BIT)))	/* if not target and been init'ed */
			{
				found = -1;
				c_dd =  1 << dd;
				*ordw &= ~wdd;	/* Smash adjacent wall */
				*ord &= ~c_dd;	/* Smash current wall */
				make_track(maze, tx, ty, tz, dd);
				break;	/* Out of lop */
			}
		}

		if (!found)
		{
			dd += pnumb;
			if (dd > 5)  dd -= 6;
			if (dd < 0)  dd += 6;
		}
		notries++;
		if (notries > 6) break;	
	}
	return found;
	
}

void make_track(MAZE *maze, int t_xx, int t_yy, int t_zz, int p_d)
{

	int firstgo, secgo;
	int notries, flg;
	MAZECELL *pord, ord;
	int found;
	int mlen, clen;
	int pnumb;
	int dd, c_dd, wdd = 0, odd;

	if (t_xx == -1)
	{
		firstgo = -1;
		switch(maze->target) {
		case MAZE_EDGE:
			dd = random_do(maze, 3); /* Which dim is not 0 */
			switch(dd) {
			case 0:
				t_xx = random_do(maze, 2) * maze->t_xx;
				t_yy = random_do(maze, 2) * maze->t_yy;
				t_zz = random_do(maze, maze->zz);
				break;
			case 1:
				t_xx = random_do(maze, 2) * maze->t_xx;
				t_yy = random_do(maze, maze->yy);
				t_zz = random_do(maze, 2) * maze->t_zz;
				break;
			default:
				t_xx = random_do(maze, maze->xx);
				t_yy = random_do(maze, 2) * maze->t_yy;
				t_zz = random_do(maze, 2) * maze->t_zz;
				break;
			}
			break;
		case MAZE_FACE:
			dd = random_do(maze, 3); /* Which dim is 0 */
			switch(dd) {
			case 0:
				t_xx = random_do(maze, 2) * maze->t_xx;
				t_yy = random_do(maze, maze->yy);
				t_zz = random_do(maze, maze->zz);
				break;
			case 1:
				t_xx = random_do(maze, maze->xx);
				t_yy = random_do(maze, 2) * maze->t_yy;
				t_zz = random_do(maze, maze->zz);
				break;
			default:
				t_xx = random_do(maze, maze->xx);
				t_yy = random_do(maze, maze->yy);
				t_zz = random_do(maze, 2) * maze->t_zz;
				break;
			}
			break;
		case MAZE_ANYWHERE:
			t_xx = random_do(maze, maze->xx);
			t_yy = random_do(maze, maze->yy);
			t_zz = random_do(maze, maze->zz);
			break;
		default:	/* Corner */
			t_xx = random_do(maze, 2) * maze->t_xx;
			t_yy = random_do(maze, 2) * maze->t_yy;
			t_zz = random_do(maze, 2) * maze->t_zz;
			break;
		}
			
	}
	else
	{
		firstgo = 0;
	}



	if (firstgo)
	{
		secgo = -1;
		mlen = (int)((maze->xx * maze->yy * maze->zz) / 16) + 1;	/* Length of the initial track */
		if (mlen < 3) mlen = 3;
		maze->tar_x = t_xx;
		maze->tar_y = t_yy;
		maze->tar_z = t_zz;
		*mazecell(maze, t_xx, t_yy, t_zz) |= TARG_BIT;
	}
	else
	{
		mlen = maze->tracklen;
		if (!mlen) mlen = 1;
		secgo = 0;
	}

	clen = 0;


	pnumb = (random_do(maze, 2) * 2) - 1;	/* pnumb is -1 or 1 */
	dd = random_do(maze, 6);			/* Face a certain direction */
	notries = 0;

	pord = mazecell(maze, t_xx, t_yy, t_zz);

	found = 0;
	for(;;)
	{
		c_dd = 1 << dd;			/* Convert the direction to enable bit oring */
		if ((*pord & c_dd) || (dd == p_d)) 	/* Is there a wall? or going from whence I came? */
		{
			notries++;			/* Try another direction */
			if (notries > 6)
			{
				break;	/* Exit loop - not found */
			}
			else
			{
				dd = dd + pnumb;
				if (dd > 5) dd -= 6;
				if (dd < 0) dd += 6;
			}
		}
		else
		{
			found = -1;
			break;
		}
	}


	if (!found)				/* if not found a direction */
	{
		*pord = SOL_BIT & ~(INIT_BIT | ADJ_BIT);	/* All walls, been through init */

		if (p_d >= 0)		/* if coming FROM a direction */
			*pord &= ~( 1 << p_d);	/* Take away the wall from whence I came */

		make_owall(maze, *pord, t_xx, t_yy, t_zz);
		return;
	}

	/*
	 * To initialize the wall - Create it with all walls EXCEPT for direction going 
	 * I probably do not need to retain pord's values but it is safer to do so
	 * for when I may add bits...
	 */


	*pord |= (SOL_BIT & ~(1 << dd));
	*pord &= ~(INIT_BIT | ADJ_BIT);

	/*
	 * if we are coming from somewhere break a wall thataway
	 */
	if (p_d >= 0)
		*pord &= ~(1 << p_d);



	/*
	 * Set up the current position to target
	 */

	if (firstgo)
	{
	
		maze->c_x = t_xx;
		maze->c_y = t_yy;
		maze->c_z = t_zz;
		maze->c_d = 0;
	}

	make_owall(maze, *pord, t_xx, t_yy, t_zz);

	/*
	 * The loop to create the track
	 *
	 * dd is the direction I need to go
	 * t_xx, t_yy and t_zz is the cell I am at
	 */
	 

	flg = -1;
	while(flg)
	{
		c_dd = 1 << dd;	/* Convert to bit */

		if (*mazecell(maze, t_xx, t_yy, t_zz) & c_dd)	/* if a wall in this dir */
		{
			flg = 0;			/* End of track */
		}
		else
		{
			clen += 1;	/* Increase the length */

			/*
			 * Now move to the NEW cell 
			 */
			switch(dd) {
			case N_WALL: t_xx++; wdd = S_WALL;break;
			case S_WALL: t_xx--; wdd = N_WALL;break;
			case W_WALL: t_yy++; wdd = E_WALL;break;
			case E_WALL: t_yy--; wdd = W_WALL;break;
			case U_WALL: t_zz++; wdd = D_WALL;break;
			case D_WALL: t_zz--; wdd = U_WALL;break;
			}

			/*
			 * Now to find a NEW dir for NEW cell
			 */

			odd = dd;	/* Remember original direction */
			notries = 0;
			pnumb = (random_do(maze, 2) * 2) - 1;	/* -1 0r +1 */

			if (clen < mlen) /* Have we finished the track yet?, if not... */
			{
				/*
				 * Find a new valid dir
				 */
				dd = random_do(maze, 6);
				found = 0;
				for(;;)
				{
					c_dd = 1 << dd;

					/*
					 * Invalid dirs are...
					 * if there is a wall in this dir,      or we are coming whence we came
                                         *                                                   or second go and in same dir
					 */
					if ((*mazecell(maze, t_xx, t_yy, t_zz) & c_dd) || (dd == wdd || ((secgo) && dd == odd)))
					{
						notries++;
						if (notries > 5)
						{
							break;
						}
						else
						{
							dd = dd + pnumb;
							if (dd > 5) dd -= 6;
							if (dd < 0) dd += 6;
						}
					}
					else
					{
						found = -1;
						break;
					}
				}
				if (found)	/* We have found a valid dir */
				{
					ord = SOL_BIT & ~c_dd;	/* Build walls except for dir  */
				}
				else
				{
					ord =SOL_BIT;	/* Not found - build all walls */
					flg = 0;	/* Stop building track */
				}
			}
			else	/* End of the track */
			{
				ord = SOL_BIT;	/* Build all walls */
				flg = 0;	/* Stop building track */
			}

			ord &= ~(1 << wdd);	/* Do not build from wence we come */

			pord = mazecell(maze, t_xx, t_yy, t_zz);
			*pord = (*pord | ord) &  ~(INIT_BIT | ADJ_BIT); /* Un-initailaize it too */

			if(secgo)
			{
				maze->sec_x = t_xx;
				maze->sec_y = t_yy;
				maze->sec_z = t_zz;
				*pord |= TARG_BIT;
				secgo = 0;
			}

			make_owall(maze, *pord, t_xx, t_yy, t_zz);

		}
	}
}


/*
 * When double routes are required
 *
 * At the end of it all smash some walls (not the two targets though)
 */

void make_double(MAZE *maze)
{
	long dbl;
	int i;

	int t_xx, t_yy, t_zz, dd, c_dd, b_dd = 0;
	int n_xx, n_yy, n_zz;
	int  isval;
	int pnumb;
	MAZECELL *ord, *ordw;
	int notries;

	dbl = maze->xx * maze->yy * maze->zz;
	dbl = (long)(dbl / 30);

	while(dbl)
	{

		t_xx = random_do(maze, maze->xx);
		t_yy = random_do(maze, maze->yy);
		t_zz = random_do(maze, maze->zz);

		ord = mazecell(maze, t_xx, t_yy, t_zz);

		if((*ord) & INIT_BIT) continue;
		if((*ord) & TARG_BIT) continue;
		if((*ord) & ADJ_BIT) continue;

		i = 0;
		pnumb = (random_do(maze, 2) * 2) - 1;
		dd = random_do(maze, 6);


		notries = 0;
		for(;;)
		{
			c_dd = 1 << dd;
			if(!(*ord & c_dd)) isval = 0;
			isval = -1;

			if(isval)
			{
				n_xx = t_xx;
				n_yy = t_yy;
				n_zz = t_zz;

				switch(dd) {
				case N_WALL:
					if(t_xx == maze->t_xx)
						isval = 0;
					else
					{
						b_dd = S_BIT;
						t_xx++;
					}
					break;
				case S_WALL:
					if(t_xx == 0)
						isval = 0;
					else
					{
						b_dd = N_BIT;
						t_xx--;
					}
					break;
				case W_WALL:
					if(t_yy == maze->t_yy)
						isval = 0;
					else
					{
						b_dd = E_BIT;
						t_yy++;
					}
					break;
				case E_WALL:
					if(t_yy == 0)
						isval = 0;
					else
					{
						b_dd = W_BIT;
						t_yy--;
					}
					break;
				case U_WALL:
					if(t_zz == maze->t_zz)
						isval = 0;
					else
					{
						b_dd = D_BIT;
						t_zz++;
					}
					break;
				case D_WALL:
					if(t_zz == 0)
						isval = 0;
					else
					{
						b_dd = U_BIT;
						t_zz--;
					}
					break;
				}


				if(isval)
				{
					ordw = mazecell(maze, t_xx, t_yy, t_zz);
					if(*ordw & TARG_BIT) isval = 0;
					if(*ordw & INIT_BIT) isval = 0;
					if(*ordw & ADJ_BIT) isval = 0;
				}

				if(isval)
				{
					*ord &= ~c_dd;
					*ordw &= ~b_dd;
					dbl--;
					break;
				}

				notries++;
				if(notries > 6) break;
				dd += pnumb;
				if(dd < 0) dd += 6;
				if(dd > 5) dd -= 6;

			}
		}
	}
}
					

				

			


/*
 * Now to get lost - start from the target and find as far away a point as possible
 */

void make_getlost(MAZE *maze)
{

	/*
	 * Which dir matrix - when walking a two dim maze (with no double routes) you
	 * can cover every single square by turning right at the junctions (or left), i.e, you
	 * simply keep touching the right wall.
	 *
	 * You can do the same sort of thing with 3d, except it is a little more difficult.
	 * Each "string" below represents the preference order to go when traveling in that direction
	 *
	 * This is quite complex, basically, at a junction, you turn in the priorities listed below
	 * for that direction (i.e, if you are going north and your path can go east, west and north you go east)
	 *
	 *
	 * As we go we place a pebble in the cell if one is not there, and pick one up if one is there
	 *
	 * The total number of pebbles put down (less those picked up) is the distance from
	 * the original point (which is the target of the mazer in fact)
	 *
	 * Thus we can find the furthest point away
	 *
	 * As I am picking pebbles up I am always going to target (original point) so
	 * I set the T*_BIT directional bits to point that way
	 *
	 * To maze walk every square
 	 * In the below, 0=North, 1=South, 2=West, 3=East, 4=Up, 5=Down
	 * The element of the array is the direction of travel
	 * The strings list the direction to go in priority order
	 *
	 * R U S L D B
	 */
	 
	char *wdir[] = { "340251", "251340", "402513", "513402", "134025", "025134" };


	int lost_x = maze->tar_x;	/* Co-ordinates of furthest away point */
	int lost_y = maze->tar_y;
	int lost_z = maze->tar_z;
	long lost_d = 0;	/* Distance from origin (as the bug crawles) */
	long lost_c = 0; /* Furthest distance away to date */

	int c_x = lost_x;
	int c_y = lost_y;
	int c_z = lost_z;

	int s_x = lost_x;
	int s_y = lost_y;
	int s_z = lost_z;

	int o_x, o_y, o_z;

	MAZECELL ord, *ptrord;
	int dd;
	long ilong, iii;
	int rflg = 0, j, pe, ge = 0;
	char *ps, pn;


	ord = *mazecell(maze, c_x, c_y, c_z);

	dd = -1;

	/*
	 * The first cell should only have one exit
	 */

	if (!(ord & N_BIT)) dd = N_WALL;
	if (!(ord & S_BIT)) dd = S_WALL;
	if (!(ord & W_BIT)) dd = W_WALL;
	if (!(ord & E_BIT)) dd = E_WALL;
	if (!(ord & U_BIT)) dd = U_WALL;
	if (!(ord & D_BIT)) dd = D_WALL;

	if (dd < 0) return;

	ilong = maze->xx * maze->yy * maze->zz;
	ilong *= 6;

	rflg = 0;	/* This is marked to true if all walls.. */

	
	for(iii=0;iii<ilong;iii++)
	{

		/*
		 * Save current position
		 */
		o_x = c_x;
		o_y = c_y;
		o_z = c_z;

		/*
		 * Go in the appropriate direction
		 */
		switch(dd)
		{
		case N_WALL: c_x++; break;
		case S_WALL: c_x--; break;
		case W_WALL: c_y++; break;
		case E_WALL: c_y--; break;
		case U_WALL: c_z++; break;
		case D_WALL: c_z--; break;
		}

		ord = *mazecell(maze, c_x, c_y, c_z);


		if (!(ord & PEBBLE_BIT))	/* If one going TO has no pebble */
		{
			*mazecell(maze, o_x, o_y, o_z) |= PEBBLE_BIT;	/* Place a pebble in OLD cell */
			lost_d++;
			if (lost_d > lost_c)	/* Are we furthest away from target? */
				ge = -1;
			else if(lost_d == lost_c)	/* if equal - throw the dice */
			{
				if(random_do(maze, 2)) ge = -1;
			}
			else
				ge = 0;
			if(ge)
			{
				lost_x = c_x;
				lost_y = c_y;
				lost_z = c_z;
				lost_c = lost_d;
				switch(dd) {
				case N_WALL: maze->lost_dir = MV_SU;break;
				case S_WALL: maze->lost_dir = MV_NU;break;
				case W_WALL: maze->lost_dir = MV_EU;break;
				case E_WALL: maze->lost_dir = MV_WU;break;
	
				case U_WALL: maze->lost_dir = MV_DN;break;
				case D_WALL: maze->lost_dir = MV_UN;break;
				}
			}
		}
		else	/* otherwise if one going TO has pebble */
		{
			ptrord = mazecell(maze, o_x, o_y, o_z);	/* Pick up pebble going FROM  and mark "thread" dir */
			*ptrord &= (~PEBBLE_BIT) & 0x3ff;
			*ptrord |= 0x400 | (dd << 11);
		
			lost_d--;
		}

		if (c_x == s_x && c_y == s_y && c_z == s_z) /* Out of loop when baxk to start */
		{
			*mazecell(maze, s_x, s_y, s_z) &= ~PEBBLE_BIT;
			break;
		}

		ps = wdir[dd];

		/*
		 * Find out which way we can go, looking in appropriate order
		 */
		j = 0;
		for(;;)
		{
			pn = ps[j] & 0x07;
			pe = 1 << pn;
			if (!(ord & pe))
			{
				break;
			}
			j++;
			if (j >= 6)
			{
				rflg = -1;	/* We are trapped!!! */
				break;
			}
		}

		dd = pn;

		if(rflg) break;	/* Error */
	}

	maze->lost_x = lost_x;
	maze->lost_y = lost_y;
	maze->lost_z = lost_z;
	
}

void maze_reset(MAZE *maze)
{
	int i, j, k;

	for(i=0;i<maze->xx;i++)
		for(j=0;i<maze->yy;j++)
			for(k=0;i<maze->zz;k++)
				*mazecell(maze, i, j, k) &= ~PEBBLE_BIT;

	maze->c_x = maze->lost_x;
	maze->c_y = maze->lost_y;
	maze->c_z = maze->lost_z;
	maze->c_d = maze->lost_dir;
}


/*
 * This goes to a position and sets the mark accordingly
 * It ignores the mark value if m is >= 100 (or <= 100)
 */
void maze_getpos(MAZE *maze, int *x, int *y, int *z, int *d, int *m, int *t, long *moves, long *rotations)
{
	MAZECELL ord;
	*x = maze->c_x;
	*y = maze->c_y;
	*z = maze->c_z;
	*d = maze->c_d;
	ord = *mazecell(maze, maze->c_x, maze->c_y, maze->c_z);
	*m = ord & PEBBLE_BIT ? -1 : 0;
	if (maze->c_x == maze->tar_x && maze->c_y == maze->tar_y && maze->c_z == maze->tar_z)
		*t = 1;
	else if (maze->c_x == maze->sec_x && maze->c_y == maze->sec_y && maze->c_z == maze->sec_z
			&& ((maze->c_d & 0xf0) >> 4) == ((ord & THREAD_BIT) >> 11))
		*t = 2;
	else
		*t = 0;
	*moves = maze->moves;
	*rotations = maze->rotations;
}


int maze_setpos(MAZE *maze, int x, int y, int z, int d, int m, long moves, long rotations)
{
	int a;
	int oldmark;

	MAZECELL *ord = NULL;

	oldmark = m;

	/*
	 * First see if we need to do anyhting....
	 */
	if(x == maze->c_x && y == maze->c_y && z == maze->c_z && d == maze->c_d)
	{
		ord = mazecell(maze, x, y, z);
		if(m >= 100 || m <= -100) return 0;
		if(*ord & PEBBLE_BIT)
		{
			if(m) return 0;
		}
		else
		{
			if(!m) return 0;
		}
	}
		
		
	if(ord == NULL)
	{
		if(x < 0 || x >= maze->xx) return -1;
		if(y < 0 || y >= maze->yy) return -1;
		if(z < 0 || z >= maze->zz) return -1;
		ord = mazecell(maze, x, y, z);
	}

	if(*ord & INIT_BIT) return -1;	/* This bit of the maze has not been built */
	oldmark = (*ord & PEBBLE_BIT) ? 1 : 0;
	m = m ? 1 : 0;

	if(moves > -1) maze->moves = moves;
	if(rotations > -1); maze->rotations = rotations;


	if(x != maze->c_x || y != maze->c_y || z != maze->c_z)
	{
		maze->c_x = x;
		maze->c_y = y;
		maze->c_z = z;
	}


	/*
	 * Direction - ignore if invalid 
	 */

	if((d >= 0 && d < MV_MAX) && d != maze->c_d)
	{
		a = (((d & 0xf0) >> 4) * 6) + (d & 0x0f);
		if(a < 36)
		{
			if (mve[a * 8] != -1)
			{
				maze->c_d = d;
			}
		}
	}

	
	if(m < 100 && m > -100 && oldmark != m)
	{
		if(m)
		{
			maze->marks++;
			*ord |= PEBBLE_BIT;
		}
		else
		{
			*ord &= ~PEBBLE_BIT;
			maze->marks--;
		}
	}

	return 0;
}
int maze_move(MAZE *maze, char move)
{

	int face;
	MAZECELL *ord;
	int c_dd;
	
	switch(move) {
	case MAZE_LEFT:
	case MAZE_RIGHT:
	case MAZE_UP:
	case MAZE_DOWN:
	case MAZE_TURN:
		maze->rotations++;
		/* no break - fall through */
	case MAZE_ANTICLOCK:
	case MAZE_CLOCK:
		maze->c_d = mve[MV_ELEMENT(maze->c_d, move)];
		return 0;
		break;

	case MAZE_FORWARD:
	case MAZE_BACK:

		face = (maze->c_d & 0xf0) >> 4;

		if(move == MAZE_BACK)
		{
			switch(face) {
			case N_WALL: face = S_WALL; break;
			case S_WALL: face = N_WALL; break;
			case W_WALL: face = E_WALL; break;
			case E_WALL: face = W_WALL; break;
			case U_WALL: face = D_WALL; break;
			case D_WALL: face = U_WALL; break;
			}
		}

		c_dd = (1 << face);
		ord = mazecell(maze, maze->c_x, maze->c_y, maze->c_z);
		if(*ord & c_dd) return -1;	/* Fail */

		maze->moves++;

		switch(face) {
		case N_WALL: maze->c_x++; break;
		case S_WALL: maze->c_x--; break;
		case W_WALL: maze->c_y++; break;
		case E_WALL: maze->c_y--; break;
		case U_WALL: maze->c_z++; break;
		case D_WALL: maze->c_z--; break;
		}
		return 0;
		break;
	
	case MAZE_MARK:	/* TODO - Have a max number of markers? */
		ord = mazecell(maze, maze->c_x, maze->c_y, maze->c_z);
		if(*ord & PEBBLE_BIT)
		{
			maze->marks--;
			*ord &= ~PEBBLE_BIT;
		}
		else
		{
			maze->marks++;
			*ord |= PEBBLE_BIT;
		}
		return 0;
		break;
	default:
		return -1;
		break;
	}

	return 0;
}




/*
 * This loads a structure view up for drawing purposes
 */

/* A macro used here to simplify things */
#define m_maze_howfar(x, y, z) sqrt(((x) * (x)) + ((y) * (y)) + ((z) * (z)))
int maze_view(MAZE *maze, MAZEVIEW *view)
{
	int i, j;
	int c_dd;
	int x = maze->c_x;
	int y = maze->c_y;
	int z = maze->c_z;
	int d = ((maze->c_d & 0xf0) >> 4);
	char *t = view->direction;

	int end = 0;

	int wi;


	MAZECELL ord;

	/* Set to zero the quick way :-) */
	memset(view, 0, sizeof(MAZEVIEW));

	/*
	 * the current position
	 * (Direction got by compass[0]
	 *
	 */
	view->x = x;
	view->y = y;
	view->z = z;

	/*
	 * The size
	 */

	view->size_x = maze->xx;
	view->size_y = maze->yy;
	view->size_z = maze->zz;

	/*
	 * To show the howfar (as crow flies) from the target
	 */
	view->howfar = m_maze_howfar(x - maze->tar_x, y - maze->tar_y, z - maze->tar_z);

	wi = ((int)view->howfar)/ 2;
	
	/*
	 * A direct compass hint, the compass letter fires up if outside 30% (asin 0.5)
	 */
	if(x - wi > maze->tar_x) *t++ = 'S';
	if(x + wi < maze->tar_x) *t++ = 'N';
	if(y - wi > maze->tar_y) *t++ = 'E';
	if(y + wi < maze->tar_y) *t++ = 'W';
	if(z - wi > maze->tar_z) *t++ = 'D';
	if(z + wi < maze->tar_z) *t++ = 'U';
	*t = 0;

	for(i=0;i<6;i++)
	{
		j = MV_ELEMENT(maze->c_d, i);
		j = mve[j];
		if(j < 0 || j >= MV_MAX) return -1; /* size of mve aarray */
		j = ((j & 0xf0) >> 4);

		if(j >= 0 && j <= 5)
			view->compass[i] = j; /* record the compass position with regards relative one */

	}
	view->moves = maze->moves;
	view->marks = maze->marks;
	view->rotations = maze->rotations;
	view->mode = maze->mode;

	for(view->distance = 0;!end;view->distance++)
	{
		/*
		 * Check for sanity
		 */
		if(x < 0 || x >= maze->xx) return -1;
		if(y < 0 || y >= maze->yy) return -1;
		if(z < 0 || z >= maze->zz) return -1;

		if(view->distance > MAX_DIM)
		{
			break;
		}

		ord = *mazecell(maze, x, y, z);	/* get attributes of this cell */

		/* Set up aome attributes */
		if(ord & PEBBLE_BIT) view->cell[(int)view->distance].mark = -1;
		if(ord & TARG_BIT) view->cell[(int)view->distance].target = -1;

		/*
		 * look at the walls here
		 */
		for(i=0;i<6; i++)	/* look at all directions */
		{
			/*
			 * Convert compass direction to relative one using mve array
			 */
			j = MV_ELEMENT(maze->c_d, i);
			j = mve[j];
			if(j < 0 || j >= MV_MAX) return -1; /* size of mve aarray */
			j = ((j & 0xf0) >> 4);
			c_dd = 1 << j;

			if(j < 0 || j >= 6) return -1;


			if(ord & c_dd)	/* There is a wall here */
			{
				view->cell[(int)view->distance].wall[i] = 1;	
				if(j == d) end = -1;	/* There is a wall in front of this  so stop */
			}
			if(j == ((ord & THREAD_BIT) >> 11))	/* Is this the direction of the thread */
			{
				view->cell[(int)view->distance].d_thread = i;
				view->cell[(int)view->distance].c_thread = j;
			}

		}

		/*
		 * Now go to  next cell
		 */
		switch(d) {
		case MAZE_C_N: x++; break;
		case MAZE_C_S: x--; break;
		case MAZE_C_W: y++; break;
		case MAZE_C_E: y--; break;
		case MAZE_C_U: z++; break;
		case MAZE_C_D: z--; break;
		}
	}
	return 0;
}


/* Getmode and setmode do just that */

long maze_getmode(MAZE *maze)
{
	return maze->mode;
}
void maze_setmode(MAZE *maze, long mode)
{
	 maze->mode = mode;
}

/*
 * Portable random number generator implementing the recursion:
 *     IX = 16807 * IX MOD (2**(31) - 1)
 * Using only 32 bits, including sign.
 *
 * Taken from "A Guide to Simulation" by Bratley, Fox and Schrage.
 *
 * A bit ancient but it should work and it is portable
 */

/*
 * This creates a (31) bit integer random number from a seed
 */
int random_do(MAZE *maze, int maxnum)
{
	int wi;
	int seed = maze->seed;


	wi = seed / 127773;
	seed = 16807 * (seed - wi * 127773) - wi * 2836;
	if (seed < 0) seed += 2147483647;
	maze->seed = seed;

	/*
	 * The next bit is not perfect, but maxnum is usually less than 10 here,
	 * certainly not greater than 20
	 *
	 * therefore the weighting to low numbers is REALLY minimal, and
	 * neglegable for my purposes
	 */

	return seed % maxnum;
}


/*
 * The files it is up to the caller to open and close them
 * in the correct mode
 */


void maze_save(MAZE *maze, FILE *fp)
{

	int i, j, k;
	fprintf(fp, "<<-MAZE 3D Version 1 Save File->>\n");
	fprintf(fp, "%d %d %d %d\n", maze->c_x, maze->c_y, maze->c_z, maze->c_d);
	fprintf(fp, "%d %d %d\n", maze->tar_x, maze->tar_y, maze->tar_z);
	fprintf(fp, "%d %d %d\n", maze->sec_x, maze->sec_y, maze->sec_z);
	fprintf(fp, "%d %d %d\n", maze->sec_x, maze->sec_y, maze->sec_z);
	fprintf(fp, "%d %d %d %d\n", maze->lost_x, maze->lost_y, maze->lost_z, maze->lost_dir);
	fprintf(fp, "%ld %ld %ld %ld %ld\n", maze->marks, maze->moves, maze->rotations, maze->seed, maze->mode);
	fprintf(fp, "%d %d %d\n", maze->xx, maze->yy, maze->zz);
	for(i=0;i<maze->xx;i++)
		for(j=0;j<maze->yy;j++)
			for(k=0;k<maze->zz;k++)
				fprintf(fp, "%d\n", *mazecell(maze, i, j, k));
	fprintf(fp, "<<-MAZE 3D  EOF->>\n");
	fflush(fp);
}


#define ml_if(a, f, t) ((a) < (f) || a > (t))

int ml_ifdir(d)
{
	if(d < 0 || d > 128) return -1;
	if(mve[d] < 0) return -1;
	return 0;
}

MAZE *maze_load(FILE *fp)
{
	MAZE *maze;

	maze = (MAZE *)smalloc(sizeof(MAZE));

	/*
	 * The seed is 0, find any number (Location of maze im memory maybe?
	 * 
	 */

	if(!maze->seed) memcpy(&(maze->seed), &maze, sizeof(int));
		

	memset(maze, 0, sizeof(MAZE));	/* Should be uneccessary - but I like to do it anyway */

	maze->xx = -1;	/* The dimensions */
	maze->yy = -1;
	maze->zz = -1;
	maze->target = 0;	/* (Irrelevant after generation) */
	maze->seed = 0;
	maze->sec_x = -1;
	maze->sec_y = -1;
	maze->sec_z = -1;

	maze->lost_x = -1;
	maze->lost_y = -1;
	maze->lost_z = -1;
	maze->lost_dir = -1;
	maze->cntref = 1;
	maze->moves = -1;
	maze->marks = -1;

	maze->tracklen = 0;	/* irrelevant after generation */
	maze->cell = NULL;


	if(maze_load_do(fp, maze))
	{
		if(maze->cell != NULL) sfree(maze->cell);
		sfree(maze);
		return NULL;
	}
	return maze;
}

	

int maze_load_do(FILE *fp, MAZE *maze)
{

	char s[100];
	int i, j, k, t;

	if(fgets(s, 90, fp) == NULL) return -1;
	if(strncmp(s, "<<-MAZE 3D Version 1 Save File->>", 33)) return -1;

	
	if(fgets(s, 90, fp) == NULL) return -1;
	sscanf(s, "%d %d %d %d\n", &(maze->c_x), &(maze->c_y), &(maze->c_z), &(maze->c_d));
	if(fgets(s, 90, fp) == NULL) return -1;
	sscanf(s, "%d %d %d\n", &(maze->tar_x), &(maze->tar_y), &(maze->tar_z));
	if(fgets(s, 90, fp) == NULL) return -1;
	sscanf(s, "%d %d %d\n", &(maze->sec_x), &(maze->sec_y), &(maze->sec_z));
	if(fgets(s, 90, fp) == NULL) return -1;
	sscanf(s, "%d %d %d\n", &(maze->sec_x), &(maze->sec_y), &(maze->sec_z));
	if(fgets(s, 90, fp) == NULL) return -1;
	sscanf(s, "%d %d %d %d\n", &(maze->lost_x), &(maze->lost_y), &(maze->lost_z), &(maze->lost_dir));
	if(fgets(s, 90, fp) == NULL) return -1;
	sscanf(s, "%ld %ld %ld %ld %ld\n", &(maze->marks), &(maze->moves), &(maze->rotations), &(maze->seed), &(maze->mode));
	if(fgets(s, 90, fp) == NULL) return -1;
	sscanf(s, "%d %d %d\n", &(maze->xx), &(maze->yy), &(maze->zz));

	if(ml_if(maze->xx, 1, MAX_DIM)) return -1;
	if(ml_if(maze->yy, 1, MAX_DIM)) return -1;
	if(ml_if(maze->zz, 1, MAX_DIM)) return -1;

	if((maze->xx == 1 && maze->yy == 1) ||
	   (maze->xx == 1 && maze->zz == 1) ||
	   (maze->yy == 1 && maze->zz == 1)) return -1;
	if(maze->xx <= 2 && maze->yy <= 2 && maze->zz <= 2) return -1;

	maze->t_xx = maze->xx - 1;
	maze->t_yy = maze->yy - 1;
	maze->t_zz = maze->zz - 1;

	if(ml_if(maze->c_x, 0, maze->t_xx)) return -1;
	if(ml_if(maze->c_y, 0, maze->t_yy)) return -1;
	if(ml_if(maze->c_z, 0, maze->t_zz)) return -1;
	if(ml_ifdir(maze->c_d)) return -1;
	
	if(ml_if(maze->lost_x, 0, maze->t_xx)) return -1;
	if(ml_if(maze->lost_y, 0, maze->t_yy)) return -1;
	if(ml_if(maze->lost_z, 0, maze->t_zz)) return -1;
	if(ml_ifdir(maze->lost_dir)) return -1;
	
	if(ml_if(maze->tar_x, 0, maze->t_xx)) return -1;
	if(ml_if(maze->tar_y, 0, maze->t_yy)) return -1;
	if(ml_if(maze->tar_z, 0, maze->t_zz)) return -1;
	
	if(ml_if(maze->sec_x, 0, maze->t_xx)) return -1;
	if(ml_if(maze->sec_y, 0, maze->t_yy)) return -1;
	if(ml_if(maze->sec_z, 0, maze->t_zz)) return -1;

	if(maze->marks < 0) return -1;
	if(maze->moves < 0) return -1;
	if(maze->rotations < 0) return -1;
	if(maze->seed < 0) return -1;
		
	maze->cell = (MAZECELL *)smalloc(maze->xx * maze->yy * maze->zz * sizeof(MAZECELL));	/* The maze cells */

	for(i=0;i<maze->xx;i++)
	{
		for(j=0;j<maze->yy;j++)
		{
			for(k=0;k<maze->zz;k++)
			{
				if(fgets(s, 90, fp) == NULL) return -1;
				sscanf(s, "%d", &t);
				if(ml_if(t, 0, 0x7fff)) return -1;

				/*
				 * just one check to make sure I do not fall off
				 * I should do more but it can get REALLY complex
				 * so I will just stop segment faults etc
				 */
			
				if(i == 0         ) if(!(t & S_BIT)) return -1;
				if(i == maze->t_xx) if(!(t & N_BIT)) return -1;
				if(j == 0         ) if(!(t & E_BIT)) return -1;
				if(j == maze->t_yy) if(!(t & W_BIT)) return -1;
				if(k == 0         ) if(!(t & D_BIT)) return -1;
				if(k == maze->t_zz) if(!(t & U_BIT)) return -1;

				*mazecell(maze, i, j, k) = (MAZECELL)t;
			}
		}
	}
	if(fgets(s, 90, fp) == NULL) return -1;
		
	
	if(strncmp(s, "<<-MAZE 3D  EOF->>", 18)) return -1;
	return 0;
}

	



/*
 * This prints the maze in ASCII format, written primarily for
 * debugging
 */

void maze_ascii(MAZE *maze, FILE *fp)
{
	int i, j, k;
	
	MAZECELL ord;

	fprintf(fp, "Dims: %d, %d, %d\n\n", maze->xx, maze->yy, maze->zz);
	fprintf(fp, "Target: %d, %d, %d\n\n", maze->tar_x, maze->tar_y, maze->tar_z);
	fprintf(fp, "Start: %d, %d, %d\n\n", maze->lost_x, maze->lost_y, maze->lost_z);

	for(k = maze->t_zz; k >= 0 ; k--)
	{
		fprintf(fp, "Level: %d\n\n    ", k);
		for(j=maze->t_yy;j>= 0; j--)
		{
			fprintf(fp, " %3d  ", j);
		}
		fprintf(fp, "\n    ");
		for(i=maze->t_xx;i>= 0; i--)
		{

			for(j=maze->t_yy;j>= 0; j--)
			{
				fprintf(fp, "+");
				if(*mazecell(maze, i, j, k) & N_BIT)
					fprintf(fp, "----");
				else
					fprintf(fp, "    ");
				fprintf(fp, "+");
			}
			fprintf(fp, "\n%3d ", i);

			for(j=maze->t_yy;j>= 0; j--)
			{
				
				ord = *mazecell(maze, i, j, k);

				if(ord & W_BIT)
					fprintf(fp, "|");
				else
					fprintf(fp, " ");
				if(ord & INIT_BIT)
					fprintf(fp, "XXXX");
				else if(ord &  TARG_BIT)
					fprintf(fp, "//\\\\");
				else
				{
					switch(ord & THREAD_BIT) {
					case TN_BIT: fprintf(fp, "^"); break;
					case TS_BIT: fprintf(fp, "v"); break;
					case TW_BIT: fprintf(fp, "<"); break;
					case TE_BIT: fprintf(fp, ">"); break;
					case TU_BIT: fprintf(fp, "/"); break;
					case TD_BIT: fprintf(fp, "\\"); break;
					default: fprintf(fp, " "); break;
					}

					if(!(ord & D_BIT))
						fprintf(fp, " \\");
					else
						fprintf(fp, "  ");
					if(!(ord & U_BIT))
						fprintf(fp, "/");
					else
						fprintf(fp, " ");
				}
				if(ord & E_BIT)
					fprintf(fp, "|");
				else
					fprintf(fp, " ");
			}
			fprintf(fp, "\n    ");
			for(j=maze->t_yy;j>= 0; j--)
			{

				ord = *mazecell(maze, i, j, k);

				if(ord & W_BIT)
					fprintf(fp, "|");
				else
					fprintf(fp, " ");
				if(ord & INIT_BIT)
					fprintf(fp, "XXXX");
				else if(ord &  TARG_BIT)
					fprintf(fp, "\\\\//");
				else
				{
					if(ord & PEBBLE_BIT)
						fprintf(fp, "*");
					else
						fprintf(fp, " ");
					if(!(ord & U_BIT))
						fprintf(fp, " /");
					else
						fprintf(fp, "  ");
					if(!(ord & D_BIT))
						fprintf(fp, "\\");
					else
						fprintf(fp, " ");
				}
				if(ord & E_BIT)
					fprintf(fp, "|");
				else
					fprintf(fp, " ");
			}
			fprintf(fp, "\n    ");
			for(j=maze->t_yy;j>= 0; j--)
			{
				fprintf(fp, "+");
				if(*mazecell(maze, i, j, k) & S_BIT)
					fprintf(fp, "----");
				else
					fprintf(fp, "    ");
				fprintf(fp, "+");
			}
			fprintf(fp, "\n    ");
		}

		fprintf(fp, "\n\n\n\n");
	}
	fflush(fp);
}

	



/*
 * A debug for when things do not work
 *
MAZECELL *dbg_mazecell(MAZE *mz, int x, int y, int z)
{
	int l =  (z) + ((y) * mz->zz) + ((x) * mz->zz * mz->yy);

	if(l < 0 || l > (mz->xx * mz->yy * mz->zz))
		printf("ERROR - (%d, %d, %d) %d\n", x, y, z, l);fflush(fp);

	return mz->cell + l;
}
 */





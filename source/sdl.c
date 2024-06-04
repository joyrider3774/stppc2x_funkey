/*
TODO:
  Try hardware SDL again - doublebuffering & simple HW both have problems due
			 to the midend updates. (e.g. try repeated solve buttons)
  Add hardware window zoom (Paeryn's Library)

  Blackbox - OK
  Bridges - OK
  Cube - OK
  Dominosa - OK
  Fifteen - OK
  Filling - OK
  Flip - OK
  Galaxies - OK
  Guess - OK
  Inertia - OK
  Light Up - OK
  Loopy - OK.
  Map - Odd stray lines.
  Mines - GP2X - Weird crash on line 599 with assert, shows mines at start.  Endian/signedness on ARM?
        - PC - No crash, no mines, doesn't terminate properly on lose/win?
  Net - OK
  Netslide - OK
  Pattern - OK.  Font a little too small, because of default size of game.
  Pegs - OK
  Rectangles - OK
  Same Game - OK
  Sixteen - OK
  Slant - OK
  Solo - OK
  Tents - OK
  Twiddle - OK
  Unequal - OK - ideally needs different number range (1-4?) but playable without.
  Untangle - OK
*/

/*
 * sdl.c: SDL front end for the puzzle collection.
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_ttf.h>
#include "puzzles.h"

// GP2X Joystick button mappings
#define GP2X_BUTTON_UP 	        (0)
#define GP2X_BUTTON_DOWN        (4)
#define GP2X_BUTTON_LEFT        (2)
#define GP2X_BUTTON_RIGHT       (6)
#define GP2X_BUTTON_UPLEFT      (1)
#define GP2X_BUTTON_UPRIGHT     (7)
#define GP2X_BUTTON_DOWNLEFT    (3)
#define GP2X_BUTTON_DOWNRIGHT   (5)
#define GP2X_BUTTON_CLICK       (18)
#define GP2X_BUTTON_A           (12)
#define GP2X_BUTTON_B           (13)
#define GP2X_BUTTON_X           (14)
#define GP2X_BUTTON_Y           (15)
#define GP2X_BUTTON_L           (10)
#define GP2X_BUTTON_R           (11)
#define GP2X_BUTTON_START       (8)
#define GP2X_BUTTON_SELECT      (9)
#define GP2X_BUTTON_VOLUP       (16)
#define GP2X_BUTTON_VOLDOWN	(17)

// Constants used in SDL user-defined events
#define RUN_TIMER_LOOP		(1)  // Game timer for midend
#define RUN_MOUSE_TIMER_LOOP	(2)  // "Mouse" timer for joystick control

// Actual screen size and colour depth.
#define SCREEN_WIDTH		(320)
#define SCREEN_HEIGHT		(240)
#define SCREEN_DEPTH		(16)

// Font size (in pixels) of the statusbar font. 
// (Actually in "points" but at 72dpi it makes no difference)
#define STATUSBAR_FONT_SIZE	(8)

// Filename of a Truetype, monospaced font
#define FIXED_FONT_FILENAME	"DejaVuSansMono-Bold.ttf"

// Filename of a Truetype, variable-spaced font
#define VARIABLE_FONT_FILENAME	"DejaVuSansCondensed-Bold.ttf"


#ifdef DEBUGGING
static FILE *debug_fp = NULL;

void dputs(char *buf)
{
    if (!debug_fp) {
        debug_fp = fopen("debug.log", "w");
    }

    fputs(buf, stderr);

    if (debug_fp) {
        fputs(buf, debug_fp);
        fflush(debug_fp);
    }
}

void debug_printf(char *fmt, ...)
{
    char buf[4096];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    dputs(buf);
    va_end(ap);
}
#endif

// Font structure that holds details on each loaded font.
struct font {
    TTF_Font *font;	// Handle to an opened font at a particular fontsize.
    int type;		// FONT_FIXED for monospaced font, FONT_VARIABLE for variable-spaced font.
    int size;		// size (in pixels/points@72dpi) of the font.
};

struct blitter {
    SDL_Surface *pixmap;
    int w, h, x, y;
};

// Structure to keep track of all GP2X buttons.
typedef struct
{
    int mouse_left;
    int mouse_middle;
    int mouse_right;
    int joy_up;
    int joy_down;
    int joy_left;
    int joy_right;
    int joy_upright;
    int joy_downright;
    int joy_upleft;
    int joy_downleft;
    int joy_l;
    int joy_r;
    int joy_a;
    int joy_b;
    int joy_x;
    int joy_y;
    int joy_start;
    int joy_select;
    int joy_volup;
    int joy_voldown;
    int joy_stick;
} button_status;

// Because some of the functions (and the midend) were based off GTK colour functions,
// they rely on converting GTK colours to SDL ones.
typedef struct {
  Uint32 pixel;
  Uint16 red;
  Uint16 green;
  Uint16 blue;
} GdkColor;

// The main frontend struct.  Some of the elements are not used anymore (GTK legacy).
struct frontend
{
    SDL_Surface *pixmap;		// Pixmap used for the game
    GdkColor *colours;			// Array of colours used.
    int ncolours;			// Number of colours used.
    int w, h;				// Width and height of 
    midend *me;				// Pointer to a game midend
    int last_status_bar_w;		// Used to blank out the statusbar before redrawing
    int last_status_bar_h;		// Used to blank out the statusbar before redrawing
    int bbox_l, bbox_r, bbox_u, bbox_d; // Bounding box used by the midend
    int timer_active;			// Whether the timer is enabled or not.
    struct timeval last_time;		// Last time the midend game timer was run.
    config_item *cfg;
    int cfg_which, cfgret;
    void *paste_data;
    int paste_data_len;
    int pw, ph;				// pixmap size (w, h are area size)
    int ox, oy;				// offset of pixmap in drawing area
    char *filesel_name;	
    SDL_Surface *screen;		// Main screen
    SDL_Joystick *joy;			// Joystick
    SDL_TimerID sdl_mouse_timer_id;	// "Mouse" timer
    SDL_TimerID sdl_timer_id;		// Midend game timer
    int solveable;			// TRUE if the game is solveable
    struct font *fonts;			// A cache of loaded fonts at particular fontsizes
    int nfonts;				// Number of cached fonts
};

// Required function for midend operation.
void fatal(char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "fatal error: ");

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fprintf(stderr, "\n");
    exit(255);
}

void Lock_SDL_Surface(SDL_Surface *screen_to_lock)
{
    if(SDL_MUSTLOCK(screen_to_lock))
    {
    	if(SDL_LockSurface(screen_to_lock))
    	{
       		printf("Error locking SDL Surface.\n");
		exit(255);
    	}
    }
}

void Unlock_SDL_Surface(SDL_Surface *screen_to_unlock)
{
    if(SDL_MUSTLOCK(screen_to_unlock))
    {
        SDL_UnlockSurface(screen_to_unlock);
    }
}

// Required function for midend operation. 
void get_random_seed(void **randseed, int *randseedsize)
{
    struct timeval *tvp = snew(struct timeval);
    gettimeofday(tvp, NULL);
    *randseed = (void *)tvp;
    *randseedsize = sizeof(struct timeval);
}

// Required function for midend operation.
// Used for querying the default background colour that should be used.
void frontend_default_colour(frontend *fe, float *output)
{

    // Fudge the values to match GTK's default background, a light grey.
    output[0]=0.862745;
    output[1]=0.854902;
    output[2]=0.835294;
}

// This function is called at the start of "drawing" (i.e a frame).
void sdl_start_draw(void *handle)
{
    frontend *fe = (frontend *)handle;
    fe->bbox_l = fe->w;
    fe->bbox_r = 0;
    fe->bbox_u = fe->h;
    fe->bbox_d = 0;
}

// Establishes a clipping rectangle in the puzzle window.
void sdl_clip(void *handle, int x, int y, int w, int h)
{
    frontend *fe = (frontend *)handle;
    SDL_Rect clipping_rectangle;

    clipping_rectangle.x = x;
    clipping_rectangle.y = y;
    clipping_rectangle.w = w;
    clipping_rectangle.h = h;

    SDL_SetClipRect(fe->screen, &clipping_rectangle);
}

// Reverts the effect of a previous call to clip(). 
void sdl_unclip(void *handle)
{
    frontend *fe = (frontend *)handle;
    SDL_Rect clipping_rectangle;

    // Force the clipping to the edges of the *visible game window*, rather than the screen
    // so that we don't get blit garbage at the edges of the game.

    clipping_rectangle.x = 0;
    clipping_rectangle.y = 0;
    clipping_rectangle.w = fe->pw;
    clipping_rectangle.h = fe->ph;
    SDL_SetClipRect(fe->screen, &clipping_rectangle);
}

// Draws coloured text - The games as of SVN 7703 only ever use fonttype=FONT_VARIABLE but this
// supports all possible combinations.
void sdl_draw_text(void *handle, int x, int y, int fonttype, int fontsize, int align, int colour, char *text)
{
    frontend *fe = (frontend *)handle;
    SDL_Color fontcolour;
    SDL_Surface *text_surface;
    SDL_Rect blit_rectangle;
    int font_w,font_h;
    int i;

    // Because of the way fonts work, with font-hinting etc., it's best to load up a font
    // in the exact size that you intend to use it.  Thus, we load up the same fonts multiple
    // times in different sizes to get the best display, rather than try to "shrink" a large
    // font or "enlarge" a small font of the same typeface.

    // To do this, we keep a cache of loaded fonts which can hold mulitple fonts of the
    // same typeface in different sizes.  We also cache by "fonttype" (i.e. typeface) so that
    // if the game asks for 8pt MONO, 8pt VARIABLE, 10pt MONO and 10pt VARIABLE, we end up
    // having all four fonts in memory.  Most games only use a single font at a particular 
    // fontsize anyway, but if we ever "resize", we will need this to work as intended.

    // RAM usage is obviously increased but not by as much as you think and it's a lot faster
    // tha dynamically loading fonts each time in the required sizes.  Even on the GP2X, we
    // have more than enough RAM to run all the games and several fonts without even trying 
    // hard.

    // Loop until we find an existing font of the right type and size.
    for (i = 0; i < fe->nfonts; i++)
        if((fe->fonts[i].size == fontsize) && (fe->fonts[i].type == fonttype))
            break;

    // If we hit the end without finding a suitable, already-loaded font, load one into memory.
    if(i == fe->nfonts)
    {
	// Dynamically increase the fonts array by 1.
        fe->nfonts++;
        fe->fonts = sresize(fe->fonts, fe->nfonts, struct font);

	// Plugin the size and type into the cache
        fe->fonts[i].size = fontsize;
        fe->fonts[i].type = fonttype;

	// Load up the new font in the desired font type and size.
	if(fonttype == FONT_FIXED)
	        fe->fonts[i].font=TTF_OpenFont(FIXED_FONT_FILENAME, fontsize);
	if(fonttype == FONT_VARIABLE)
	        fe->fonts[i].font=TTF_OpenFont(VARIABLE_FONT_FILENAME, fontsize);

        if(!fe->fonts[i].font)
        {
            printf("Error loading TTF font: %s\n", TTF_GetError());
            printf("Make sure ");
	    printf(FIXED_FONT_FILENAME);
	    printf(" and ");
	    printf(VARIABLE_FONT_FILENAME);
	    printf("\nare in the same folder as the program.\n");
            exit(255);
        };
    };

    // Retrieve the actual size of the rendered text for the particular font
    if(TTF_SizeText(fe->fonts[i].font,text,&font_w,&font_h))
    {
        printf("Error getting size of font text: %s\n", TTF_GetError());
	exit(255);
    };

    // Align the text surface based on the requested alignment.
    if(align & ALIGN_VCENTRE)
	blit_rectangle.y = (Sint16) (y - (font_h / 2));
    else
	blit_rectangle.y = (Sint16) (y - font_h);

    if(align & ALIGN_HCENTRE)
	blit_rectangle.x = (Sint16) (x - (font_w / 2));
    else if(align & ALIGN_HRIGHT)
	blit_rectangle.x = (Sint16) (x - font_w);
    else
	blit_rectangle.x = (Sint16) x;

    // SDL can automatically handle the width/height for us as we are 
    // blitting the "entire" text surface.
    blit_rectangle.w = (Uint16) 0;
    blit_rectangle.h = (Uint16) 0;

    // Convert GTK colours to SDL 3 x 8-bit RGB.
    fontcolour.r=(Uint8)(fe->colours[colour].red / 256);
    fontcolour.g=(Uint8)(fe->colours[colour].green / 256);
    fontcolour.b=(Uint8)(fe->colours[colour].blue / 256);
    
    // Draw the text to a temporary SDL surface.
    if(!(text_surface=TTF_RenderText_Solid(fe->fonts[i].font,text,fontcolour)))
    {
        printf("Error rendering font text: %s\n", TTF_GetError());
	exit(255);
    }
    else
    {
	// Blit the text surface onto the screen at the right position.
        SDL_UnlockSurface(fe->screen);
        SDL_BlitSurface(text_surface,NULL,fe->screen,&blit_rectangle);
        SDL_LockSurface(fe->screen);
	
	// Remove the temporary text surface from memory.
        SDL_FreeSurface(text_surface);
    }
}

// Could be buggy due to difference in GTK/SDL_gfx routines - this is the cause of map's 
// stray lines??
void sdl_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
    frontend *fe = (frontend *)handle;
    boxRGBA(fe->screen, (Sint16) x, (Sint16) y, (Sint16) (x+w), (Sint16) (y+h), (Uint8)(fe->colours[colour].red / 256), (Uint8)(fe->colours[colour].green /256), (Uint8)(fe->colours[colour].blue / 256), (Uint8) 255);
}

void sdl_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour)
{
    frontend *fe = (frontend *)handle;
    lineRGBA(fe->screen, (Sint16)x1, (Sint16)y1, (Sint16)x2, (Sint16)y2, (Uint8)(fe->colours[colour].red / 256), (Uint8)(fe->colours[colour].green / 256), (Uint8)(fe->colours[colour].blue / 256), 255);
}

void sdl_draw_poly(void *handle, int *coords, int npoints, int fillcolour, int outlinecolour)
{
    frontend *fe = (frontend *)handle;
    Sint16 *xpoints = snewn(npoints,Sint16);
    Sint16 *ypoints = snewn(npoints,Sint16);
    int i;

    // Convert points list from game format (x1,y1,x2,y2,...,xn,yn) to SDL_gfx format 
    // (x1,x2,x3,...,xn,y1,y2,y3,...,yn)
    for (i = 0; i < npoints; i++)
    {
        xpoints[i]=(Sint16) coords[i*2];
        ypoints[i]=(Sint16) coords[i*2+1];
    }

    // Draw a filled polygon (without outline).
    if (fillcolour >= 0)
    {
        filledPolygonRGBA(fe->screen, xpoints, ypoints, npoints, (Uint8) (fe->colours[fillcolour].red / 256), (Uint8) (fe->colours[fillcolour].green / 256), (Uint8) (fe->colours[fillcolour].blue / 256), 255);
    }

    assert(outlinecolour >= 0);

    // This draws the outline of the polygon using calls to the other existing frontend functions.
    for (i = 0; i < npoints; i++)
    {
	sdl_draw_line(fe, xpoints[i], ypoints[i], xpoints[(i+1)%npoints], ypoints[(i+1)%npoints],outlinecolour);
    }
}

void sdl_draw_circle(void *handle, int cx, int cy, int radius, int fillcolour, int outlinecolour)
{
    frontend *fe = (frontend *)handle;
    assert(outlinecolour >= 0);

    // Draw a filled circle with no outline
    filledCircleRGBA(fe->screen, (Sint16)cx, (Sint16)cy, (Sint16)radius, (Uint8) (fe->colours[fillcolour].red / 256), (Uint8) (fe->colours[fillcolour].green / 256), (Uint8) (fe->colours[fillcolour].blue / 256), 255);
  
    // Draw just an outline circle in the same place
    circleRGBA(fe->screen, (Sint16)cx, (Sint16)cy, (Sint16)radius, (Uint8) (fe->colours[outlinecolour].red / 256), (Uint8) (fe->colours[outlinecolour].green / 256), (Uint8) (fe->colours[outlinecolour].blue / 256), 255);
}

// Routine to handle showing "status bar" messages to the user.
// These are used by both the midend to signal game information and the SDL interface to 
// signal quitting, restarting, entering text etc.
void sdl_status_bar(void *handle, char *text)
{
    frontend *fe = (frontend *)handle;
    SDL_Color fontcolour;
    SDL_Surface *text_surface;
    SDL_Rect blit_rectangle;
    int i;

    // If we haven't got anything to display, don't do anything.
    // Sometimes the midend does this to us.
    if((text!=NULL) && strlen(text))
    {
	// Print messages in the top-left corner of the screen.
        blit_rectangle.x = (Sint16) 0;
        blit_rectangle.y = (Sint16) 0;

        // SDL can automatically handle the width/height for us as we are 
        // blitting the "entire" text surface.
        blit_rectangle.w = (Uint16) 0;
        blit_rectangle.h = (Uint16) 0;

	// Print black text
        fontcolour.r=(Uint8)0;
        fontcolour.g=(Uint8)0;
        fontcolour.b=(Uint8)0;

	// Loop over all the loaded fonts until we find the one for the statusbar
        for (i = 0; i < fe->nfonts; i++)
            if((fe->fonts[i].size == STATUSBAR_FONT_SIZE) && (fe->fonts[i].type == FONT_VARIABLE))
                break;

	// This should ALWAYS be fonts[0] because of the way we initialise and should
	// ALWAYS be loaded.  But it doesn't hurt to check.
	assert(i==0);

	// Render the text to a temporary text surface.
        if(!(text_surface=TTF_RenderText_Solid(fe->fonts[i].font,text,fontcolour)))
	{
            printf("Error rendering font text: %s\n", TTF_GetError());
            exit(255);
        }
        else
        {
	    if(fe->last_status_bar_w || fe->last_status_bar_h)
	    {
	    	// Blank over the last status bar message using the frontend "rect" 
		// routine with colour "0" (i.e. background) and the appropriate size.
	  	sdl_draw_rect(fe, 0, 0, fe->last_status_bar_w, fe->last_status_bar_h, 0);
	    };

	    SDL_UnlockSurface(fe->screen);
	    // Blit the new text into place.
            if(SDL_BlitSurface(text_surface,NULL,fe->screen,&blit_rectangle))
	    {
		printf("Error blitting text surface to screen: %s\n", TTF_GetError());
		exit(255);
	    };
	    SDL_LockSurface(fe->screen);

	    // Update variables so that we know how much to "blank" next time round.
	    fe->last_status_bar_w=text_surface->w;
	    fe->last_status_bar_h=text_surface->h;

	    // Cause a screen update over the relevant area.
            SDL_UpdateRect(fe->screen, 0, 0, (Sint32) text_surface->w, (Sint32) text_surface->h);

	    // Remove the temporary text surface
            SDL_FreeSurface(text_surface);
        };
    };
}

// Create a new "blitter" (temporary surface) of width w, height h.
// These are used by the midend to do things like save what's under the mouse cursor so it can 
// be redrawn later.
blitter *sdl_blitter_new(void *handle, int w, int h)
{
    /*
     * We can't create the pixmap right now, because fe->window
     * might not yet exist. So we just cache w and h and create it
     * during the first call to blitter_save.
     */
    blitter *bl = snew(blitter);
    bl->pixmap = NULL;
    bl->w = w;
    bl->h = h;
    return bl;
}

// Release the blitter specified
void sdl_blitter_free(void *handle, blitter *bl)
{
    if (bl->pixmap)
    {
        SDL_FreeSurface(bl->pixmap);
    };
    sfree(bl);
}

// Save the contents of the screen starting at X, Y into a "blitter" with size bl->w, bl->h
void sdl_blitter_save(void *handle, blitter *bl, int x, int y)
{
    frontend *fe = (frontend *)handle;
    SDL_Rect srcrect, destrect;

    // Create the actual blitter surface now that we know all the details.
    if (!bl->pixmap)
        bl->pixmap=SDL_CreateRGBSurface(SDL_SWSURFACE, bl->w, bl->h, SCREEN_DEPTH, 0, 0, 0, 0);
    bl->x = x;
    bl->y = y;

    // Blit the relevant area of the screen into the blitter.
    srcrect.x = x;
    srcrect.y = y;
    srcrect.w = bl->w;
    srcrect.h = bl->h;
    destrect.x = 0;
    destrect.y = 0;
    destrect.w = bl->w;
    destrect.h = bl->h;
    SDL_UnlockSurface(fe->screen);
    SDL_BlitSurface(fe->screen, &srcrect, bl->pixmap, &destrect);
    SDL_LockSurface(fe->screen);
}

// Load the contents of the screen starting at X, Y from a "blitter" with size bl->w, bl->h
void sdl_blitter_load(void *handle, blitter *bl, int x, int y)
{
    frontend *fe = (frontend *)handle;
    SDL_Rect srcrect, destrect;

    assert(bl->pixmap);

    // If we've been asked to just "use the blitter size", do that.
    if (x == BLITTER_FROMSAVED && y == BLITTER_FROMSAVED)
    {
        x = bl->x;
        y = bl->y;
    }

    // Blit the contents of the blitter into the relevant area of the screen.
    srcrect.x = 0;
    srcrect.y = 0;
    srcrect.w = bl->w;
    srcrect.h = bl->h;
    destrect.x = x;
    destrect.y = y;
    destrect.w = bl->w;
    destrect.h = bl->h;
    SDL_UnlockSurface(fe->screen);
    SDL_BlitSurface(bl->pixmap, &srcrect, fe->screen, &destrect);
    SDL_LockSurface(fe->screen);
}

// Informs the front end that a rectangular portion of the puzzle window 
// has been drawn on and needs to be updated.
void sdl_draw_update(void *handle, int x, int y, int w, int h)
{
    frontend *fe = (frontend *)handle;
    if (fe->bbox_l > x) fe->bbox_l = x;
    if (fe->bbox_r < x+w) fe->bbox_r = x+w;
    if (fe->bbox_u > y) fe->bbox_u = y;
    if (fe->bbox_d < y+h) fe->bbox_d = y+h;

    // Request a partial screen update of the relevant rectangle.
    SDL_UpdateRect(fe->screen, (Sint32) x, (Sint32) y, (Sint32) w, (Sint32) h);
}

// This function is called at the end of drawing (i.e. a frame).
void sdl_end_draw(void *handle)
{
    frontend *fe = (frontend *)handle;

    // In SDL software surfaces, SDL_Flip() is just SDL_UpdateRect(everything).
    // In double-buffered SDL hardware surfaces, this does the flip between the two screen 
    // buffers.  

    SDL_UnlockSurface(fe->screen);
    SDL_Flip(fe->screen);
    SDL_LockSurface(fe->screen);
}

// Provide a list of functions for the midend to call when it needs to draw etc.
const struct drawing_api sdl_drawing = {
    sdl_draw_text,
    sdl_draw_rect,
    sdl_draw_line,
    sdl_draw_poly,
    sdl_draw_circle,
    sdl_draw_update,
    sdl_clip,
    sdl_unclip,
    sdl_start_draw,
    sdl_end_draw,
    sdl_status_bar,
    sdl_blitter_new,
    sdl_blitter_free,
    sdl_blitter_save,
    sdl_blitter_load,

//  The next functions are only used when printing a puzzle, hence they remain
//  unwritten and NULL.
    NULL, NULL, NULL, NULL, NULL, NULL, /* {begin,end}_{doc,page,puzzle} */
    NULL,			       /* line_width */
};

// Happens on resize of window and at startup
static int configure_area(int x, int y, void *data)
{
    frontend *fe = (frontend *)data;

    // If we're re-sizing an existing surface, free it first so we can recreate it with 
    // the new dimensions later.
    if (fe->pixmap)
        SDL_FreeSurface(fe->pixmap);

    fe->w=x;
    fe->h=y;

    // Get the new size of the puzzle.
    midend_size(fe->me, &x, &y, TRUE);
    fe->pw = x;
    fe->ph = y;

    // This calculates a new origin for the puzzle screen.
    fe->ox = (fe->w - fe->pw) / 2;
    fe->oy = (fe->h - fe->ph) / 2;

    // Allocate a surface of the right size.
    fe->pixmap=SDL_CreateRGBSurface(SDL_SWSURFACE, fe->pw, fe->ph, SCREEN_DEPTH, 0, 0, 0, 0);

    // Force a redraw of the game onto the new surface
    midend_force_redraw(fe->me);
    return TRUE;
}

// Called regularly whenever the midend enables a timer.
Uint32 sdl_timer_func(Uint32 interval, void *data)
{
    frontend *fe = (frontend *)data;

    if (fe->timer_active) {
      // Generate a user event and push it to the event queue so that the event does
      // the work and not the timer (you can't run SDL code inside an SDL timer)
      SDL_Event event;
    
      event.type = SDL_USEREVENT;
      event.user.code = RUN_TIMER_LOOP;
      event.user.data1 = 0;
      event.user.data2 = 0;
     
      SDL_PushEvent(&event);

	// Specify to run this event again in interval milliseconds.
	return interval;
    }
    else
    {
        // If the timer should not be active, return 0 to stop the timer from firing again.
	return 0;
    };
}

// Called regularly to update the mouse position.
Uint32 sdl_mouse_timer_func(Uint32 interval, void *data)
{
    SDL_Event event;
  
    // Generate a user event and push it to the event queue so that the event does
    // the work and not the timer (you can't run SDL code inside an SDL timer)
    event.type = SDL_USEREVENT;
    event.user.code = RUN_MOUSE_TIMER_LOOP;
    event.user.data1 = 0;
    event.user.data2 = 0;
     
    SDL_PushEvent(&event);

    // Specify to run this event again in interval milliseconds.
    return interval;
}

// Stop a timer from firing again.
void deactivate_timer(frontend *fe)
{
    if(!fe)
	return;			       /* can happen due to --generate */

    if(fe->timer_active)
    {
        SDL_RemoveTimer(fe->sdl_timer_id);
    };

    // Set a flag so that any remaining timer fires end up removing the timer too.
    fe->timer_active = FALSE;
}

void activate_timer(frontend *fe)
{
    if(!fe)
	return;			       /* can happen due to --generate */

    // If a timer isn't already running
    if(!fe->timer_active) {
	// Set the game timer to fire every 50ms.
        // This is an arbitrary number, chosen to make the animation smooth without
        // generating too many events for the GP2X to process properly.
        fe->sdl_timer_id=SDL_AddTimer(50, sdl_timer_func, fe);
	if(!fe->sdl_timer_id)
	{
		printf("Error - Timer Creation failed\n");
		exit(255);
	};

	// Update the last time the event fired so that the midend knows how long it's been.
	gettimeofday(&fe->last_time, NULL);
    }

    fe->timer_active = TRUE;
}

// Returns the "screen size" that the puzzle should be working with.
static void get_size(frontend *fe, int *px, int *py)
{
    int x, y;

    // Set both to INT_MAX to disable scaling, or window size to enable scaling.
    // We set these to twice the GP2X screen size - the midend is "playing" on a 640x480 
    // screen but automatically scales it to fit in SCREEN_HEIGHT x SCREEN_WIDTH.

    x = SCREEN_WIDTH * 2;
    y = SCREEN_HEIGHT * 2;
    midend_size(fe->me, &x, &y, FALSE);
    *px = x;
    *py = y;
}

// Used later when parsing command line arguments.  Trivia that should be removed.
// GTK legacy
static int savefile_read(void *wctx, void *buf, int len)
{
    FILE *fp = (FILE *)wctx;
    int ret;

    ret = fread(buf, 1, len, fp);
    return (ret == len);
}

enum { ARG_EITHER, ARG_SAVE, ARG_ID }; /* for argtype */

// Mostly legacy GTK command-line processing code but does all the important 
// setting up of the games.
static frontend *new_window(char *arg, int argtype, char **error)
{
    frontend *fe;
    int x, y;
    char errbuf[1024];

    fe = snew(frontend);

    fe->timer_active = FALSE;
//    fe->timer_id = -1;

    fe->me = midend_new(fe, &thegame, &sdl_drawing, fe);
    fe->solveable=thegame.can_solve;

    if (arg) {
	char *err;
	FILE *fp;

	errbuf[0] = '\0';

	switch (argtype) {
	  case ARG_ID:
	    err = midend_game_id(fe->me, arg);
	    if (!err)
		midend_new_game(fe->me);
	    else
		sprintf(errbuf, "Invalid game ID: %.800s", err);
	    break;
	  case ARG_SAVE:
	    fp = fopen(arg, "r");
	    if (!fp) {
		sprintf(errbuf, "Error opening file: %.800s", strerror(errno));
	    } else {
		err = midend_deserialise(fe->me, savefile_read, fp);
                if (err)
                    sprintf(errbuf, "Invalid save file: %.800s", err);
                fclose(fp);
	    }
	    break;
	  default /*case ARG_EITHER*/:
	    /*
	     * First try treating the argument as a game ID.
	     */
	    err = midend_game_id(fe->me, arg);
	    if (!err) {
		/*
		 * It's a valid game ID.
		 */
		midend_new_game(fe->me);
	    } else {
		FILE *fp = fopen(arg, "r");
		if (!fp) {
		    sprintf(errbuf, "Supplied argument is neither a game ID (%.400s)"
			    " nor a save file (%.400s)", err, strerror(errno));
		} else {
		    err = midend_deserialise(fe->me, savefile_read, fp);
		    if (err)
			sprintf(errbuf, "%.800s", err);
		    fclose(fp);
		}
	    }
	    break;
	}
	if (*errbuf) {
	    *error = dupstr(errbuf);
	    midend_free(fe->me);
	    sfree(fe);
	    return NULL;
	}

    }
    else
    {
	midend_new_game(fe->me);
    }

    {
        int i, ncolours;
        float *colours;
        int *success;

	// Get the colours that the midend thinks it needs.
	// The midend seems to return values designed in the same way as
        // GTK colours, so use that format and convert it when we need to.

        colours = midend_colours(fe->me, &ncolours);
        fe->ncolours = ncolours;
        fe->colours = snewn(ncolours, GdkColor);
        for (i = 0; i < ncolours; i++) {
            fe->colours[i].red = colours[i*3] * 0xFFFF;
            fe->colours[i].green = colours[i*3+1] * 0xFFFF;
            fe->colours[i].blue = colours[i*3+2] * 0xFFFF;
        }
        success = snewn(ncolours, int);
    }

    get_size(fe, &x, &y);

    fe->pixmap = NULL;

    fe->paste_data = NULL;
    fe->paste_data_len = 0;

    return fe;
}

// Called on exit.
void game_quit()
{
    // Turn the cursor off to stop it lingering after program exit (e.g. in the GP2X menu)
    SDL_ShowCursor(SDL_DISABLE);

    // Although we could do a lot more cleanup, SDL / the compiler tends to handle it for
    // us from here on out.  If we do file saves, etc. though it may be an idea to 
    // sync() here... this would do?
//    #include <unistd.h>
//    execl("sync", NULL);

    // We *could* also run the menu again, but it's preferable for this program to just
    // terminate.  This way, GMenu2x etc. users don't get thrown back into the GP2X menu
    // and GP2X menu users can use a basic wrapper script to run the game and then re-execute 
    // the menu.

//    #include <unistd.h>
//    chdir("/usr/gp2x");
//    execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);
};

// SDL Main Loop
void Main_SDL_Loop(void *handle)
{
    frontend *fe = (frontend *)handle;
    int mouse_x=0;
    int mouse_y=0;
    int mouse_accel_x=0;
    int mouse_accel_y=0;
    int button=0;
    SDL_Event event;
    button_status *bs=snew(button_status);

    int digit_to_input=1;
    char digit_to_input_as_string[21];
    int keyval;
    struct timeval now;
    float elapsed;

    // Initialise the bs structure (there are quicker ways, I know).
    bs->mouse_left=0;
    bs->mouse_middle=0;
    bs->mouse_right=0;
    bs->joy_upleft=0;
    bs->joy_upright=0;
    bs->joy_downleft=0;
    bs->joy_downright=0;
    bs->joy_up=0;
    bs->joy_down=0;
    bs->joy_left=0;
    bs->joy_right=0;
    bs->joy_l=0;
    bs->joy_r=0;
    bs->joy_a=0;
    bs->joy_b=0;
    bs->joy_x=0;
    bs->joy_y=0;
    bs->joy_start=0;
    bs->joy_select=0;
    bs->joy_volup=0;
    bs->joy_voldown=0;
    bs->joy_stick=0;

    // Set the mouse to the center of the puzzle screen and then show the cursor
    SDL_WarpMouse(fe->pw / 2, fe->ph / 2);
    SDL_ShowCursor(SDL_ENABLE);

    // Start off a timer every 25ms to update the mouse co-ordinates.
    // 25ms is an arbitrary number shown to give smooth mouse movement (if we 
    // move the cursor 2 pixels each tick) without flooding the event queue.

    fe->sdl_mouse_timer_id=SDL_AddTimer(25, sdl_mouse_timer_func, fe);
    if(!fe->sdl_mouse_timer_id)
    {
         printf("Error creating mouse timer\n");
	 exit(255);
    };

    while(1)
    {
        while( SDL_PollEvent( &event ))
        {     
          if (fe->pixmap)
	  {
            switch( event.type )
            {
	    case SDL_USEREVENT:
		switch(event.user.code)
		{
			case RUN_MOUSE_TIMER_LOOP:
				// Mouse timer code
/*
   A note on GP2X joystick button handling (F-100 specific):

   There are actually eight buttons, up, upleft, left, etc. but they are switched on
   in combination to provide greater directional accuracy.  E.g. upleft + left means 
   W-NW, not just a leftover press from either W or NW.

   In these games, we effectively remove such accuracy to provide greater control
   by treating W-NW, W and W-SW the same as just W.  This gives us four large areas
   which are seen as up, down, left, right and four smaller areas seen as the diagonals.

   The F-200 doesn't have such fine control and just returns up, left or upleft. (???)
	
   SDL events are unreliable for lots of events - with timers, mouse timers and joystick
   events all happening simultaneously while the midend was trying to draw/solve puzzles,
   the GP2X with it's 200MHz couldn't keep up where a 600MHz laptop showed no problems.

   The queue was getting flooded and I was losing joystick button-up events... so the 
   cursor was flying across the screen because it thought you were still holding the 
   button down!  The SDL event queue is of limited size and it was losing critical events 
   (game timers and mouse timers aren't critical but button-up/down events are), so the 
   mouse timer now updates the status of the most critical buttons directly from the hardware 
   before it does anything with the information.
*/

				// Update the joystick button status from the hardware
				bs->joy_upleft=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_UPLEFT);
				bs->joy_upright=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_UPRIGHT);
				bs->joy_downleft=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_DOWNLEFT);
				bs->joy_downright=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_DOWNRIGHT);
				bs->joy_up=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_UP);
				bs->joy_down=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_DOWN);
				bs->joy_left=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_LEFT);
				bs->joy_right=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_RIGHT);
				bs->mouse_left=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_A);
				bs->mouse_middle=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_Y);
				bs->mouse_right=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_B);

				// Generate an acceleration direction (-1,0,1) based on the 
				// joystick's x and y axes.
				mouse_accel_x=0;
				mouse_accel_y=0;
				if(bs->joy_upleft || bs->joy_up || bs->joy_upright)
					mouse_accel_y=-1;
				if(bs->joy_downleft || bs->joy_down || bs->joy_downright)
					mouse_accel_y=1;
				if(bs->joy_upleft || bs->joy_left || bs->joy_downleft)
					mouse_accel_x=-1;
				if(bs->joy_upright || bs->joy_right || bs->joy_downright)
					mouse_accel_x=1;

				// Move mouse according to motion recorded on GP2X joystick
				// We move 2 pixels per tick to increase cursor speed and
				// still allow good accuracy.
				mouse_x += (mouse_accel_x * 2);
				mouse_y += (mouse_accel_y * 2);

				// Warp the actual SDL mouse cursor to the new location
				// only if it changed.
				if((mouse_accel_x) || (mouse_accel_y))
					SDL_WarpMouse((Uint16)mouse_x, (Uint16) mouse_y);
				break;

			case RUN_TIMER_LOOP:
				// Game midend timer code

				// Update the time elapsed since the last timer, so that the 
				// midend can take account of this.
	  			gettimeofday(&now, NULL);
				elapsed = ((now.tv_usec - fe->last_time.tv_usec) * 0.000001F + (now.tv_sec - fe->last_time.tv_sec));

				// Run the midend timer routine
			        midend_timer(fe->me, elapsed);	/* may clear timer_active */

				// Update the time the timer was last fired.
				fe->last_time = now;
				break;			
		};
                break; // switch( event.type ) case SDL_USEREVENT

            case SDL_JOYBUTTONUP:
		switch(event.button.button)
		{
			case GP2X_BUTTON_A:
				// Simulate a left-mouse button release
				bs->mouse_left=0;
				button=LEFT_BUTTON;
       				button += LEFT_RELEASE - LEFT_BUTTON;
	    			midend_process_key(fe->me, mouse_x, mouse_y, button);
				break;
			case GP2X_BUTTON_Y:
				// Simulate a middle-mouse button release
				bs->mouse_middle=0;
				button=MIDDLE_BUTTON;
	       			button += LEFT_RELEASE - LEFT_BUTTON;
		    		midend_process_key(fe->me, mouse_x, mouse_y, button);
				break;
			case GP2X_BUTTON_B:
				// Simulate a right-mouse button release
				bs->mouse_right=0;
				button=RIGHT_BUTTON;
	       			button += LEFT_RELEASE - LEFT_BUTTON;
		    		midend_process_key(fe->me, mouse_x, mouse_y, button);
	                	break;
			case GP2X_BUTTON_L:
				bs->joy_l=0;
				break;
			case GP2X_BUTTON_R:
				bs->joy_r=0;
				break;
			case GP2X_BUTTON_VOLDOWN:
				bs->joy_voldown=0;
				break;
			case GP2X_BUTTON_VOLUP:
				bs->joy_volup=0;
				break;
		};
		break; // switch( event.type ) case SDL_JOYBUTTONUP

            case SDL_JOYBUTTONDOWN:
		switch(event.button.button)
		{		
			case GP2X_BUTTON_A:
				// Simulate a left-mouse click
				bs->mouse_left=1;
				button=LEFT_BUTTON;
		    		midend_process_key(fe->me, mouse_x, mouse_y, button);
				break;
			case GP2X_BUTTON_Y:
				// Simulate a middle-mouse click
				bs->mouse_middle=1;
				button=MIDDLE_BUTTON;
				midend_process_key(fe->me, mouse_x, mouse_y, button);
				break;
			case GP2X_BUTTON_B:
				// Simulate a right-mouse click
				bs->mouse_right=1;
				button=RIGHT_BUTTON;
				midend_process_key(fe->me, mouse_x, mouse_y, button);
				break;
			case GP2X_BUTTON_X:
				// Convert the current digit to ASCII, then make it look like it 
				// was typed in at the keyboard, to allow entering numbers into
				// solo, unequal, etc.  (48=ASCII value of the 0 key)
				keyval = MOD_NUM_KEYPAD | (digit_to_input + 48);
				midend_process_key(fe->me, 0, 0, keyval);
				break;
			case GP2X_BUTTON_L:
				// If we're holding the other shoulder button too...
				if(bs->joy_r)
				{
					sdl_status_bar(fe,"Quitting...");
					exit(0);
				}
				else
				{
					bs->joy_l=1;
					// Cycle the number we would input
					digit_to_input -= 1;
					if(digit_to_input < 1)
						digit_to_input = 9;
					sprintf(digit_to_input_as_string, "Press X to enter a %d", digit_to_input);
					sdl_status_bar(fe,digit_to_input_as_string);
				};
				break;
			case GP2X_BUTTON_R:
				// If we're holding the other shoulder button too...
				if(bs->joy_l)
				{
					sdl_status_bar(fe,"Quitting...");
					exit(0);
				}
				else
				{
					bs->joy_r=1;
					// Cycle the number we would input
					digit_to_input += 1;
					if(digit_to_input > 9)
						digit_to_input = 1;
					sprintf(digit_to_input_as_string, "Press X to enter a %d", digit_to_input);
					sdl_status_bar(fe,digit_to_input_as_string);
				};
				break;
			case GP2X_BUTTON_VOLDOWN:
				bs->joy_voldown=1;
				// If we're holding the other volume button too...
				if(bs->joy_volup)
				{
					// and the puzzle is solveable...
					if(fe->solveable)
					{
						// Solve the puzzle
						sdl_status_bar(fe,"Solving...");
						midend_solve(fe->me);
					};
				};
				break;
			case GP2X_BUTTON_VOLUP:
				bs->joy_volup=1;
				// If we're holding the other volume button too...
				if(bs->joy_voldown)
				{
					// and the puzzle is solveable...
					if(fe->solveable)
					{
						// Solve the puzzle
						sdl_status_bar(fe,"Solving...");
						midend_solve(fe->me);
					};
				};
				break;
			case GP2X_BUTTON_START:
				// Start a new game
				sdl_status_bar(fe,"Generating a new game...");
				midend_new_game(fe->me);
				midend_force_redraw(fe->me);
				break;
			case GP2X_BUTTON_SELECT:
				// Restart the current game
				sdl_status_bar(fe,"Restarting game...");
				midend_restart_game(fe->me);
				midend_force_redraw(fe->me);
				break;
		}; // switch(event.button.button)
                break; // switch( event.type ) case SDL_JOYBUTTONDOWN

#ifndef TARGET_GP2X
// Used for debugging so that we can quit the game when using a Linux machine
// running on the same code!
	    case SDL_KEYDOWN:
		switch(event.key.keysym.sym)
		{
			case SDLK_q:
				sdl_status_bar(fe,"Quitting...");
				exit(0);
				break;
			case SDLK_s:
				if(fe->solveable)
				{
					sdl_status_bar(fe,"Solving...");
					midend_solve(fe->me);
				};
				break;
			case SDLK_l:
				// Cycle the number we would input
				digit_to_input -= 2;
			case SDLK_r:
				digit_to_input += 1;
				if(digit_to_input < 1)
					digit_to_input = 9;
				if(digit_to_input > 9)
					digit_to_input = 1;
				sprintf(digit_to_input_as_string, "Press X to enter a %d", digit_to_input);
				sdl_status_bar(fe,digit_to_input_as_string);
				break;
			default:
				break;
		}; // switch(event.key.keysym.sym)
		break; // switch( event.type ) case SDL_KEYDOWN
#endif

            case SDL_MOUSEBUTTONUP:
		if (event.button.button == 2)
			button = MIDDLE_BUTTON;
		else if (event.button.button == 3)
			button = RIGHT_BUTTON;
		else if (event.button.button == 1)
			button = LEFT_BUTTON;
       		button += LEFT_RELEASE - LEFT_BUTTON;
    		midend_process_key(fe->me, event.button.x, event.button.y, button);
                break; // switch( event.type ) case SDL_MOUSEBUTTONUP

            case SDL_MOUSEBUTTONDOWN:
		if (event.button.button == 2)
			button = MIDDLE_BUTTON;
		else if (event.button.button == 3)
			button = RIGHT_BUTTON;
		else if (event.button.button == 1)
			button = LEFT_BUTTON;
    		midend_process_key(fe->me, event.button.x, event.button.y, button);
                break; // switch( event.type ) case SDL_MOUSEBUTTONDOWN

	    case SDL_MOUSEMOTION:
		// Both joystick "warps" and real mouse movement (touchscreen etc.) end up here.

		// Use the co-ords from the actual event, because the mouse/touchscreen wouldn't
		// update our internal co-ords variables itself otherwise.
		mouse_x=event.motion.x;
		mouse_y=event.motion.y;

		// Contain the mouse cursor inside the edges of the puzzle.
		if(mouse_x < 0)
			mouse_x = 0;
		if(mouse_y < 0)
			mouse_y = 0;
		if(mouse_x > (fe->pw - 1))
			mouse_x = fe->pw - 1;
		if(mouse_y > (fe->ph - 1))
			mouse_y = fe->ph - 1;

		// If we had to move the mouse with those checks...
		if((mouse_x != event.motion.x) || (mouse_y != event.motion.y))
		{
			// Move the mouse cursor to where it needs to be now
			// (this will automatically generate a new MOUSEMOTION event)
			SDL_WarpMouse((Uint16)mouse_x, (Uint16) mouse_y);
		}
		else
		{
			// Send a "drag" event to the midend if necessary, 
			// or just a motion event.
			if( (event.motion.state == 2) || (bs->mouse_middle) )
				button = MIDDLE_DRAG;
			else if( (event.motion.state == 3) || (bs->mouse_right) )
				button = RIGHT_DRAG;
			else if( (event.motion.state == 1) || (bs->mouse_left) )
				button = LEFT_DRAG;
	    		midend_process_key(fe->me, event.motion.x, event.motion.y, button);
		};			
                break; // switch( event.type ) case SDL_MOUSEMOTION

            case SDL_QUIT:
		sdl_status_bar(fe,"Quitting...");
                exit(0);
                break;

            } // switch(event.type)
          } // if(fe->pixmap)
        } // while( SDL_PollEvent( &event ))
    } // while(1)
}

int main(int argc, char **argv)
{
    char *error;
    char *arg = NULL;
    int argtype = ARG_EITHER;
    frontend *fe;

    // Preprocessor magic so that we don't include unnecessary code in the binaries for GP2X
    // but can benefit from debugging code when running on other platforms.
#ifdef TARGET_GP2X
    printf("I appear to be running on a real GP2X.\n");
#else
    printf("I am not running on a real GP2X.\n");
#endif

    fe = new_window(arg, argtype, &error);

    if (!fe)
    {
        printf("Error initialising frontend: %s\n", error);
	exit(255);
    }
    else
    {
        printf("Initialised frontend for '%s'.\n", thegame.name);
    };

    fe->last_status_bar_w=0;
    fe->last_status_bar_h=0;

    // Make sure that SDL etc. is cleaned up should anything fail.
    atexit(SDL_Quit);
    atexit(TTF_Quit);
    atexit(game_quit);

    // Initialise the various components of SDL that we intend to use.
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK|SDL_INIT_TIMER))
    {
        printf("Error initialising SDL: %s\n", SDL_GetError());
	exit(255);
    }
    else
    {
        printf("Initialised SDL.\n");
    };

    // Initialise SDL_TTF.
    if(TTF_Init()) 
    {
        printf("Error intialising SDL_ttf: %s\n", TTF_GetError());
        exit(255);
    }
    else
    {
        printf("Initialised SDL TTF library.\n");
    };

    // Initialise the font structure so that we can load our first font.
    fe->fonts=snewn(1,struct font);

    // Double-check that a font file is actually present by loading the font for the 
    // status bar in.  The fonts structure will make sure that we don't load it up
    // again unnecessarily, by caching fonts used at the particular sizes used and growing
    // accordingly.

    // This particular font will stay loaded for the length of the program's execution.

    fe->fonts[0].size=STATUSBAR_FONT_SIZE;
    fe->fonts[0].type=FONT_VARIABLE;
    fe->fonts[0].font=TTF_OpenFont(VARIABLE_FONT_FILENAME, STATUSBAR_FONT_SIZE);
    fe->nfonts=1;

    if(fe->fonts[0].font)
    {
	printf("Loaded TTF font.\n");
    }
    else
    {
        printf("Error loading TTF font: %s\n", TTF_GetError());
        printf("Make sure DejaVuSansCondensed-Bold.ttf and DejaVuSansMono-Bold.ttf\nare in the same folder as the program.\n");
        exit(255);
    };

    // Check for a joystick
    if(SDL_NumJoysticks()>0)
    {
        // Open joystick
        fe->joy=SDL_JoystickOpen(0);
  
        if(fe->joy)
        {
            printf("Initialised Joystick 0.\n");
        }
        else
        {
            printf("Error initialising Joystick 0.\n");
	    exit(255);
        };
    }
    else
    {
	printf("No joysticks were found on this system.  This is not a real GP2X.\n");
    };

    // Initialise video 

    // Because of the way the midend does animation, double-buffering is a pain that isn't 
    // worth the effort (e.g. pressing "solve" repeatedly flips between old and new buffers
    // etc.).  And HW Surfaces flicker because of Vsync etc.

//    fe->screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, SDL_HWSURFACE);
//    fe->screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, SDL_HWSURFACE | SDL_DOUBLEBUF);
    fe->screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, SDL_SWSURFACE);

    if(fe->screen)
    {
	printf("Initialised %u x %u @ %u video mode.\n", SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH);
    }
    else
    {
	printf("Error initialising %u x %u @ %u video mode: %s\n", SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, SDL_GetError());
      	exit(255);
    };

    // Blank over the whole screen using the frontend "rect" routine with colour "0" 
    // (i.e. background)
    sdl_draw_rect(fe, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    // Turn off the cursor as soon as we can.
    SDL_ShowCursor(SDL_DISABLE);

    // Update the user in case the next bit takes a long time.
    sdl_status_bar(fe,"Loading...");

    configure_area(SCREEN_WIDTH,SCREEN_HEIGHT,fe);

    // Set the clipping region for SDL so that we start with a clipping region
    // that prevents blit garbage at the edges of the game window.
    sdl_unclip(fe);

    printf("Starting Event Loop...\n");

    // Start the infinite loop that handles all events.
    Main_SDL_Loop(fe);

    // This line is never reached but keeps the compiler happy.
    return 0;
}

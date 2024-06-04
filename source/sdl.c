// Define this to get lots of debugging info at runtime.
// #define DEBUGGING

// Define this to implement kludges for certain game-specific options
// (Usually the ones that require floating point numbers)
#define OPTION_KLUDGES

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
#include <ctype.h>
#include <sys/time.h>

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_ttf.h>
#include "puzzles.h"

// GP2X Joystick button mappings
#define GP2X_BUTTON_UP 	        (0)
#define GP2X_BUTTON_UPLEFT      (1)
#define GP2X_BUTTON_LEFT        (2)
#define GP2X_BUTTON_DOWNLEFT    (3)
#define GP2X_BUTTON_DOWN        (4)
#define GP2X_BUTTON_DOWNRIGHT   (5)
#define GP2X_BUTTON_RIGHT       (6)
#define GP2X_BUTTON_UPRIGHT     (7)
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

// Font size (in pixels) of the statusbar font
// (Actually in "points" but at 72dpi it makes no difference)
#define STATUSBAR_FONT_SIZE	(8)

// Font size (in pixels) of the menu font
#define MENU_FONT_SIZE		(13)

// Font size (in pixels) of the help text
#define HELP_FONT_SIZE		(10)

// Filename of a Truetype, Unicode-capable, monospaced font
#define FIXED_FONT_FILENAME	"DejaVuSansMono-Bold.ttf"

// Filename of a Truetype, Unicode-capable, variable-spaced font
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
struct font
{
    TTF_Font *font;	// Handle to an opened font at a particular fontsize.
    int type;		// FONT_FIXED for monospaced font, FONT_VARIABLE for variable-spaced font.
    int size;		// size (in pixels, points@72dpi) of the font.
};

// Used as a temporary area by the games to save/load portions of the screen.
struct blitter
{
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

// Structure to hold one of the game's configuration options
struct config_window_option
{
    int y;			// Vertical position on screen
    int config_index;		// Which configuration option
    int config_option_index;	// Which configuration sub-option (for "dropdown" options)
};

// The main frontend struct.  Some of the elements are not used anymore (GTK legacy).
struct frontend
{
    SDL_Surface *pixmap;		// Pixmap used for the game
    SDL_Color *sdlcolours;		// Array of colours used.
    int ncolours;			// Number of colours used.
    int white_colour;			// Index of white
    int background_colour;		// Index of background colour
    int black_colour;			// Index of black colour
    int w, h;				// Width and height of 
    midend *me;				// Pointer to a game midend
    int last_status_bar_w;		// Used to blank out the statusbar before redrawing
    int last_status_bar_h;		// Used to blank out the statusbar before redrawing
    int bbox_l, bbox_r, bbox_u, bbox_d; // Bounding box used by the midend
    int timer_active;			// Whether the timer is enabled or not.
    int timer_was_active;		// Whether the timer was enabled before pause or not.
    struct timeval last_time;		// Last time the midend game timer was run.
    config_item *cfg;
    int ncfg;
    int pw, ph;				// pixmap size (w, h are area size)
    int ox, oy;				// offset of pixmap in drawing area
    SDL_Surface *screen;		// Main screen
    SDL_Joystick *joy;			// Joystick
    SDL_TimerID sdl_mouse_timer_id;	// "Mouse" timer
    SDL_TimerID sdl_timer_id;		// Midend game timer
    int solveable;			// TRUE if the game is solveable
    struct font *fonts;			// A cache of loaded fonts at particular fontsizes
    int nfonts;				// Number of cached fonts
    int paused;				// True if paused (menu showing)
    int helpfile_displayed;		// True if helpfile showing
    SDL_Rect clipping_rectangle;	// Clipping rectangle in use by the game.
    char *configure_window_title;	// The window title for the configure window
    struct config_window_option *config_window_options;		// The games configuration options
};

// "Mouse" (virtual or real) co-ordinates.
int mouse_x=0;
int mouse_y=0;

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

// Locks a surface
void Lock_SDL_Surface(SDL_Surface *screen_to_lock)
{
#ifdef DEBUGGING
    printf("Locking SDL Surface.\n");
#endif
    if(SDL_MUSTLOCK(screen_to_lock))
    {
        if(SDL_LockSurface(screen_to_lock))
        {
       	    printf("Error locking SDL Surface.\n");
	    exit(255);
        }
    }
}

// Unlocks a surface
void Unlock_SDL_Surface(SDL_Surface *screen_to_unlock)
{
#ifdef DEBUGGING
    printf("Unlocking SDL Surface.\n");
#endif
    if(SDL_MUSTLOCK(screen_to_unlock))
        SDL_UnlockSurface(screen_to_unlock);
}

// Required function for midend operation. 
// Returns a random seed (time of day to high-precision, because the GP2X lacks a RT clock).
void get_random_seed(void **randseed, int *randseedsize)
{
#ifdef DEBUGGING
    printf("Random seed requested.\n");
#endif
    struct timeval *tvp = snew(struct timeval);
    gettimeofday(tvp, NULL);
    *randseed = (void *)tvp;
    *randseedsize = sizeof(struct timeval);
}

// Required function for midend operation.
// Used for querying the default background colour that should be used.
void frontend_default_colour(frontend *fe, float *output)
{
#ifdef DEBUGGING
    printf("Default frontend colour requested.\n");
#endif

    // Light grey
    output[0]=0.85;
    output[1]=0.85;
    output[2]=0.85;
}

// This function is called at the start of "drawing" (i.e a frame).
void sdl_start_draw(void *handle)
{
#ifdef DEBUGGING
    printf("Start of a frame.\n");
#endif

    frontend *fe = (frontend *)handle;
    fe->bbox_l = fe->w;
    fe->bbox_r = 0;
    fe->bbox_u = fe->h;
    fe->bbox_d = 0;
}

// Removes ALL clipping from the entire physical screen without affecting the "logical" puzzle 
// clipping rectangle.
void sdl_no_clip(void *handle)
{
    frontend *fe = (frontend *)handle;
    SDL_Rect fullscreen_clipping_rectangle;

    fullscreen_clipping_rectangle.x = 0;
    fullscreen_clipping_rectangle.y = 0;
    fullscreen_clipping_rectangle.w = SCREEN_WIDTH;
    fullscreen_clipping_rectangle.h = SCREEN_HEIGHT;

    SDL_SetClipRect(fe->screen, &fullscreen_clipping_rectangle);

#ifdef DEBUGGING
    printf("Clipping area now: %u,%u - %u,%u\n", 0,0,SCREEN_WIDTH, SCREEN_HEIGHT);
#endif
}

// Establishes a clipping rectangle in the puzzle window.
void sdl_clip(void *handle, int x, int y, int w, int h)
{
#ifdef DEBUGGING
    printf("Clipping area now: %u,%u - %u,%u\n", x, y, x+w, y+h);
#endif

    frontend *fe = (frontend *)handle;

    fe->clipping_rectangle.x = x;
    fe->clipping_rectangle.y = y;
    fe->clipping_rectangle.w = w;
    fe->clipping_rectangle.h = h;

    SDL_SetClipRect(fe->screen, &fe->clipping_rectangle);
}

// Reverts the effect of a previous call to clip(). 
void sdl_unclip(void *handle)
{
    frontend *fe = (frontend *)handle;

    // Force the clipping to the edges of the *visible game window*, rather than the screen
    // so that we don't get blit garbage at the edges of the game.

    fe->clipping_rectangle.x = 0;
    fe->clipping_rectangle.y = 0;
    fe->clipping_rectangle.w = fe->pw;
    fe->clipping_rectangle.h = fe->ph;
    SDL_SetClipRect(fe->screen, &fe->clipping_rectangle);

#ifdef DEBUGGING
    printf("Clipping area now: %u,%u - %u,%u\n", 0, 0, fe->pw, fe->ph);
#endif
}

// Draws coloured text - The games as of SVN 7703 only ever use fonttype=FONT_VARIABLE but this
// supports all possible combinations.  This is also used to draw text for various internal
// functions like the help file, configuration options etc.
void sdl_draw_text(void *handle, int x, int y, int fonttype, int fontsize, int align, int colour, char *text)
{
    frontend *fe = (frontend *)handle;
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
    // having all four fonts in memory.  

    // RAM usage is obviously increased but not by as much as you think and it's a lot faster
    // than dynamically loading fonts each time in the required sizes.  Even on the GP2X, we
    // have more than enough RAM to run all the games and several fonts without even trying.

    // Only if we're being asked to actually draw some text...
    // Sometimes the midend does this to us.
    if((text!=NULL) && strlen(text))
    {
        // Loop until we find an existing font of the right type and size.
        for (i = 0; i < fe->nfonts; i++)
            if((fe->fonts[i].size == fontsize) && (fe->fonts[i].type == fonttype))
	        break;

        // If we hit the end without finding a suitable, already-loaded font, load one into memory.
        if(i == fe->nfonts)
        {
            // Dynamically increase the fonts array by 1
            fe->nfonts++;
            fe->fonts = sresize(fe->fonts, fe->nfonts, struct font);

            // Plugin the size and type into the cache
            fe->fonts[i].size = fontsize;
            fe->fonts[i].type = fonttype;

            // Load up the new font in the desired font type and size.
            if(fonttype == FONT_FIXED)
            {
                fe->fonts[i].font=TTF_OpenFont(FIXED_FONT_FILENAME, fontsize);
            }
            else if(fonttype == FONT_VARIABLE)
            {
                fe->fonts[i].font=TTF_OpenFont(VARIABLE_FONT_FILENAME, fontsize);
	    };

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
        blit_rectangle.w = 0;
        blit_rectangle.h = 0;

        // Draw the text to a temporary SDL surface.
        if(!(text_surface=TTF_RenderUTF8_Blended(fe->fonts[i].font,text,fe->sdlcolours[colour])))
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
        };
    };
}

// Could be buggy due to difference in GTK/SDL_gfx routines - this is the cause of map's 
// stray lines??
void sdl_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
#ifdef DEBUGGING
    printf("Rectangle: %u,%u - %u,%u Colour %u\n", x, y, x+w, y+h, colour);
#endif

    frontend *fe = (frontend *)handle;
    boxRGBA(fe->screen, (Sint16) x, (Sint16) y, (Sint16) (x+w), (Sint16) (y+h), fe->sdlcolours[colour].r, fe->sdlcolours[colour].g, fe->sdlcolours[colour].b, (Uint8) 255);
}

void sdl_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour)
{
#ifdef DEBUGGING
    printf("Line: %u,%u - %u,%u Colour %u\n", x1, y1, x2, y2, colour);
#endif

    frontend *fe = (frontend *)handle;

    // Draw an anti-aliased line.
    aalineRGBA(fe->screen, (Sint16)x1, (Sint16)y1, (Sint16)x2, (Sint16)y2, fe->sdlcolours[colour].r, fe->sdlcolours[colour].g, fe->sdlcolours[colour].b, 255);
}

void sdl_draw_poly(void *handle, int *coords, int npoints, int fillcolour, int outlinecolour)
{
    frontend *fe = (frontend *)handle;
    Sint16 *xpoints = snewn(npoints,Sint16);
    Sint16 *ypoints = snewn(npoints,Sint16);
    int i;
#ifdef DEBUGGING
    printf("Polygon: ");
#endif

    // Convert points list from game format (x1,y1,x2,y2,...,xn,yn) to SDL_gfx format 
    // (x1,x2,x3,...,xn,y1,y2,y3,...,yn)
    for (i = 0; i < npoints; i++)
    {
        xpoints[i]=(Sint16) coords[i*2];
        ypoints[i]=(Sint16) coords[i*2+1];

#ifdef DEBUGGING
    printf("%u,%u ",xpoints[i],ypoints[i]);
#endif

    };

#ifdef DEBUGGING
    printf(" Colour: %u\n", fillcolour);
#endif

    // Draw a filled polygon (without outline).
    if (fillcolour >= 0)
        filledPolygonRGBA(fe->screen, xpoints, ypoints, npoints, fe->sdlcolours[fillcolour].r, fe->sdlcolours[fillcolour].g, fe->sdlcolours[fillcolour].b, 255);

    assert(outlinecolour >= 0);

    // This draws the outline of the polygon using calls to SDL_gfx anti-aliased routines.
    aapolygonRGBA(fe->screen, xpoints, ypoints, npoints,fe->sdlcolours[outlinecolour].r, fe->sdlcolours[outlinecolour].g, fe->sdlcolours[outlinecolour].b, 255);

/*
UNUSED
    // This draws the outline of the polygon using calls to the other existing frontend functions.
    for (i = 0; i < npoints; i++)
    {
	sdl_draw_line(fe, xpoints[i], ypoints[i], xpoints[(i+1)%npoints], ypoints[(i+1)%npoints],outlinecolour);
    }
*/
}

void sdl_draw_circle(void *handle, int cx, int cy, int radius, int fillcolour, int outlinecolour)
{
#ifdef DEBUGGING
    printf("Circle: %u,%u radius %u Colour %u\n", cx, cy, radius, fillcolour);
#endif

    frontend *fe = (frontend *)handle;

    // Draw a filled circle with no outline
    // We don't anti-alias because it looks ugly when things try to draw circles over circles
    filledCircleRGBA(fe->screen, (Sint16)cx, (Sint16)cy, (Sint16)radius, fe->sdlcolours[fillcolour].r, fe->sdlcolours[fillcolour].g, fe->sdlcolours[fillcolour].b, 255);

    assert(outlinecolour >= 0);
  
    // Draw just an outline circle in the same place
    // We don't anti-alias because it looks ugly when things try to draw circles over circles
    circleRGBA(fe->screen, (Sint16)cx, (Sint16)cy, (Sint16)radius, fe->sdlcolours[outlinecolour].r, fe->sdlcolours[outlinecolour].g, fe->sdlcolours[outlinecolour].b, 255);
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

    // Keep the (overly-fussy) compiler happy.
    fontcolour.unused=(Uint8)0;

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

        if(fe->paused)
        {
            // Print white text
            fontcolour=fe->sdlcolours[fe->white_colour];
        }
        else
        {
            // Print black text
            fontcolour=fe->sdlcolours[fe->black_colour];
        };

        // Loop over all the loaded fonts until we find the one for the statusbar
        for (i = 0; i < fe->nfonts; i++)
            if((fe->fonts[i].size == STATUSBAR_FONT_SIZE) && (fe->fonts[i].type == FONT_VARIABLE))
                break;

        // This should ALWAYS be fonts[0] because of the way we initialise and should
        // ALWAYS be loaded.  But it doesn't hurt to check.
        assert(i==0);

        // Render the text to a temporary text surface.
        if(!(text_surface=TTF_RenderUTF8_Blended(fe->fonts[i].font,text,fontcolour)))
        {
            printf("Error rendering font text: %s\n", TTF_GetError());
            exit(255);
        }
        else
        {
            // If we've drawn text on the status bar before
            if(fe->last_status_bar_w || fe->last_status_bar_h)
            {
                if(fe->paused)
                {
                    // Blank over the last status bar messages using a black rectangle.
                    sdl_draw_rect(fe, 0, 0, fe->last_status_bar_w, fe->last_status_bar_h, fe->black_colour);
                }
                else
                {
                    // Blank over the last status bar messages using a background-coloured
                    // rectangle.
                    sdl_draw_rect(fe, 0, 0, fe->last_status_bar_w, fe->last_status_bar_h, fe->background_colour);
                };
            };

            // Blit the new text into place.
            SDL_UnlockSurface(fe->screen);
            if(SDL_BlitSurface(text_surface,NULL,fe->screen,&blit_rectangle))
            {
                printf("Error blitting text surface to screen: %s\n", TTF_GetError());
                exit(255);
            };
            SDL_LockSurface(fe->screen);

            // Update variables so that we know how much screen to "blank" next time round.
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
        SDL_FreeSurface(bl->pixmap);
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
    if ((x == BLITTER_FROMSAVED) && (y == BLITTER_FROMSAVED))
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

    // Update the limits of the puzzle if they have changed
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
#ifdef DEBUGGING
    printf("End of frame.\n");
#endif

    // In SDL software surfaces, SDL_Flip() is just SDL_UpdateRect(everything).
    // In double-buffered SDL hardware surfaces, this does the flip between the two screen 
    // buffers.  We don't do double-buffering properly but if we ever did, we would have to 
    // move this.

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
static void configure_area(int x, int y, void *data)
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

    // This calculates a new offset for the puzzle screen.
    // This would be used for "centering" the puzzle on the screen.
    fe->ox = (fe->w - fe->pw) / 2;
    fe->oy = (fe->h - fe->ph) / 2;

#ifdef DEBUGGING
    printf("New puzzle size is %u x %u\n", fe->pw, fe->ph);
    printf("New puzzle offset is %u x %u\n", fe->ox, fe->oy);
#endif

    // Allocate a surface of the right size.
    fe->pixmap=SDL_CreateRGBSurface(SDL_SWSURFACE, fe->pw, fe->ph, SCREEN_DEPTH, 0, 0, 0, 0);

    // Force a redraw of the game onto the new surface
    midend_force_redraw(fe->me);
}

// Called regularly whenever the midend enables a timer.
Uint32 sdl_timer_func(Uint32 interval, void *data)
{
    frontend *fe = (frontend *)data;
    if(fe->paused)
    {
        // If we are paused, don't do anything, but just let the timer re-fire again
        // after another X ms.
        return interval;
    }
    else
    {
        if (fe->timer_active)
        {
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
        // Set a flag so that any remaining timer fires end up removing the timer too.
        fe->timer_active = FALSE;

	// Remove the timer.
        SDL_RemoveTimer(fe->sdl_timer_id);
#ifdef DEBUGGING
        printf("Timer deactivated.\n");
    }
    else
    {
        printf("Timer deactivated but wasn't active.\n");
#endif
    }
}

void activate_timer(frontend *fe)
{
    if(!fe)
        return;			       // can happen due to --generate

    // If a timer isn't already running
    if(!fe->timer_active)
    {
#ifdef DEBUGGING
        printf("Timer activated (wasn't already running).\n");
#endif

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

        fe->timer_active = TRUE;
#ifdef DEBUGGING
    }
    else
    {
        printf("Timer activated (was already running).\n");
#endif

    }
}

// Returns the "screen size" that the puzzle should be working with.
static void get_size(frontend *fe, int *px, int *py)
{
    int x, y;

    // Set both to INT_MAX to disable scaling, or window size to enable scaling.
    // We set these to the GP2X screen size, larger multiples automatically 
    // scales it to fit in SCREEN_HEIGHT x SCREEN_WIDTH.

    x = SCREEN_WIDTH;
    y = SCREEN_HEIGHT;

    midend_size(fe->me, &x, &y, FALSE);
    *px = x;
    *py = y;
}

// Loads and shows the text file of the game's name for instructions.
// This should only ever be called while the game is already paused!
int read_game_helpfile(void *handle)
{
    const int buffer_length=60;
    FILE *helpfile;
    char str_buf[buffer_length];
    int line=1;
    char game_name[255];
    int i;
    char *filename;

    frontend *fe = (frontend *)handle;

    // Put the game name into a variable we can change
    sprintf(game_name, "%.200s", thegame.name);

    // Sanitise the game name - if it contains spaces, replace them with underscores in 
    // the filename and convert the filename to lowercase (doesn't matter on the GP2X
    // because it uses case-insensitive FAT32, but you never know what might happen and
    // this is better for development).
    for(i=0;i<255;i++)
    {
        if(isspace(game_name[i]))
            game_name[i]='_';
        game_name[i]=tolower(game_name[i]);
    }

    // Add .txt to the end of the game name to get the filename
    filename=strcat(game_name,".txt");
	
    // Open the help text file.
    helpfile = fopen(filename, "r");
    if(helpfile == NULL)
    {
        printf("Cannot open helpfile for game %s\n",game_name);
        return(FALSE);
    }
    else
    {
        // Blank the entire screen
        sdl_draw_rect(fe, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, fe->black_colour);

        // Read one line at a time from the file into a buffer
        while(fgets(str_buf, buffer_length, helpfile) != NULL)
        {
            // Strip all line returns (char 10) from the buffer because they show up as
            // squares.  Probably have to do this for carriage returns (char 13) too if
            // someone edits the text files in DOS/Windows
            for(i=0; i<buffer_length; i++)
            {
                if(str_buf[i]==10)
                    str_buf[i]=0;
            };
		
            // Draw the buffer to the screen.
            sdl_draw_text(fe, 0, line * (HELP_FONT_SIZE+2), FONT_VARIABLE, HELP_FONT_SIZE, ALIGN_VNORMAL & ALIGN_HLEFT, fe->white_colour, str_buf);
            line++;
			
            // Set a flag to say that we are showing the helpfile, so the game ignores mouse clicks etc. for a while.
            fe->helpfile_displayed=TRUE;
        };

        // Update screen.
        sdl_end_draw(fe);

        // Cleanup
        fclose(helpfile);
        return(TRUE);
    };
}

// Used later when parsing command line arguments.  Trivia that should be removed.
// GTK legacy
/*
static int savefile_read(void *wctx, void *buf, int len)
{
    FILE *fp = (FILE *)wctx;
    int ret;

    ret = fread(buf, 1, len, fp);
    return (ret == len);
}
*/

enum { ARG_EITHER, ARG_SAVE, ARG_ID }; /* for argtype */

// Mostly legacy GTK command-line processing code but does all the important 
// setting up of the games.
static frontend *new_window(char *arg, int argtype, char **error)
{
    frontend *fe;
    int x, y;

    fe = snew(frontend);

    fe->black_colour=-1;
    fe->white_colour=-1;

    fe->timer_active = FALSE;
//    fe->timer_id = -1;

    fe->me = midend_new(fe, &thegame, &sdl_drawing, fe);
    fe->solveable=thegame.can_solve;
    midend_new_game(fe->me);

    {
        int i, ncolours;
        float *colours;
        int *success;
	int black_found=FALSE, white_found=FALSE;

        // Get the colours that the midend thinks it needs.
        colours = midend_colours(fe->me, &ncolours);

        fe->ncolours = ncolours;
        fe->sdlcolours = snewn(fe->ncolours, SDL_Color);

        // Plug them into an array of SDL colours, so that we can use either
        // interchangably when given a colour index.
        for (i = 0; i < ncolours; i++)
        {
            fe->sdlcolours[i].r = (Uint8)(colours[i*3] * 0xFF);
            fe->sdlcolours[i].g = (Uint8)(colours[i*3+1] * 0xFF);
            fe->sdlcolours[i].b = (Uint8)(colours[i*3+2] * 0xFF);

            // If we find a black-like colour, remember it
            if((!black_found) && (fe->sdlcolours[i].r == 0) && (fe->sdlcolours[i].g == 0) && (fe->sdlcolours[i].b==0))
            {
                fe->black_colour=i;
#ifdef DEBUGGING
                printf("Black is colour index %u\n",i);
#endif
                black_found=TRUE;
            };

            // If we find a white-like colour, remember it
            if((!white_found) && (fe->sdlcolours[i].r == 0xFF) && (fe->sdlcolours[i].g == 0xFF) && (fe->sdlcolours[i].b==0xFF))
            {
                fe->white_colour=i;
#ifdef DEBUGGING
                printf("White is colour index %u\n",i);
#endif
                white_found=TRUE;
            };
        };

        success = snewn(ncolours, int);

        // If we didn't find both white and black
        if(!white_found)
        {
       	    printf("Did not find white colour in the palette.\n");
       	    printf("Tacking it to the end of the %u colour palette.\n",ncolours);

            // Create a white colour and add it to the end of the SDL palette.
            fe->ncolours=ncolours+1;
            fe->sdlcolours = sresize(fe->sdlcolours, fe->ncolours, SDL_Color);

            fe->sdlcolours[ncolours].r = 255;
            fe->sdlcolours[ncolours].g = 255;
            fe->sdlcolours[ncolours].b = 255;

            fe->white_colour=ncolours;
        };

        if(!black_found)
        {
       	    printf("Did not find black colour in the palette.\n");
       	    printf("Tacking it to the end of the %u colour palette.\n",ncolours);

            // Create a black colour and add it to the end of the SDL palette.
            fe->ncolours=ncolours+1;
            fe->sdlcolours = sresize(fe->sdlcolours, fe->ncolours, SDL_Color);

            fe->sdlcolours[ncolours+1].r = 0;
            fe->sdlcolours[ncolours+1].g = 0;
            fe->sdlcolours[ncolours+1].b = 0;

            fe->black_colour=ncolours+1;
            printf("Black is now colour index %u\n",ncolours+1);
        };
    }

    get_size(fe, &x, &y);

    fe->pixmap = NULL;

    return fe;
}

// Called on whenever the game is paused or unpaused.  
// Also called from inside "pause" mode to redraw the config options.
void game_pause(frontend *fe)
{
    int j=0;
    int current_y;
    char delimiter;
    SDL_Rect font_clip_rectangle;
#ifdef DEBUGGING
    printf("fe->timer_active: %u\n",fe->timer_active);
#endif

    fe->ncfg=0;
    fe->config_window_options=snew(struct config_window_option);

    // If the game needs to be paused.
    if(fe->paused)
    {
#ifdef DEBUGGING
        printf("Game paused.\n");
#endif

        // Set the clipping region to the whole physical screen
        sdl_no_clip(fe);

        // Black out the whole screen using the drawing routines.
        sdl_draw_rect(fe, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, fe->black_colour);

        // Print up a message for the user
        sdl_status_bar(fe,"Paused.");

        // Print the game's configuration window title
        sdl_draw_text(fe, 100, 3*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL & ALIGN_HLEFT, fe->white_colour, fe->configure_window_title);

        // Draw the static items of the menu.
        sdl_draw_text(fe, 10, 4*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL & ALIGN_HLEFT, fe->white_colour, "New Game");
        sdl_draw_text(fe, 10, 5*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL & ALIGN_HLEFT, fe->white_colour, "Restart Current Game");
        sdl_draw_text(fe, 10, 6*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL & ALIGN_HLEFT, fe->white_colour, "Display Help");

	// Draw a seperating line
        sdl_draw_line(fe, 0, 6.5*(MENU_FONT_SIZE+2),SCREEN_WIDTH, 6.5*(MENU_FONT_SIZE+2),fe->white_colour);

        current_y=8*(MENU_FONT_SIZE+2);
		
        // This part collects all the configuration options for the current game
        // and dynamically adds the options/current values to the "menu".
        while(fe->cfg[j].type!=C_END)
        {
            // Dyncamically increase the size of the config options
            fe->ncfg++;
            fe->config_window_options=sresize(fe->config_window_options, fe->ncfg, struct config_window_option);
            fe->config_window_options[fe->ncfg-1].config_index=j;
            fe->config_window_options[fe->ncfg-1].config_option_index=0;

            font_clip_rectangle.x=0;
            font_clip_rectangle.w=220;

            font_clip_rectangle.y=current_y-(MENU_FONT_SIZE+2);
            font_clip_rectangle.h=MENU_FONT_SIZE+2;

            sdl_clip(fe, font_clip_rectangle.x, font_clip_rectangle.y, font_clip_rectangle.w, font_clip_rectangle.h);

            // Draw the configuration option name.
            sdl_draw_text(fe, 5, current_y, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL & ALIGN_HLEFT, fe->white_colour, fe->cfg[j].name);

            // Setup a clipping rectangle to stop text overlapping.
            font_clip_rectangle.x=225;
            font_clip_rectangle.w=SCREEN_WIDTH-225;

            font_clip_rectangle.y=current_y-(MENU_FONT_SIZE+2);
            font_clip_rectangle.h=MENU_FONT_SIZE+2;

            sdl_clip(fe, font_clip_rectangle.x, font_clip_rectangle.y, font_clip_rectangle.w, font_clip_rectangle.h);

            // Depending on the type of the configuration option (string, boolean, dropdown options)
            switch(fe->cfg[j].type)
            {
                case C_STRING:
#ifdef OPTION_KLUDGES
                    // Kludge - if we find "probablity" in the option name, it's probably expecting
                    // decimal numbers instead of integers, so add the relevant ".0" to the end
                    // of the value.
                    if(!strcmp(fe->cfg[j].name,"Barrier probability"))
                        sprintf(fe->cfg[j].sval, "%.1f", atof(fe->cfg[j].sval));

                    // Kludge - if the option name is "Expansion factor", it's probably expecting
                    // decimal numbers instead of integers, so add the relevant ".0" to the end
                    // of the value.
                    if(!strcmp(fe->cfg[j].name,"Expansion factor"))
                        sprintf(fe->cfg[j].sval, "%.1f", atof(fe->cfg[j].sval));
#endif
                    // Just draw the current setting straight to the menu.
                    sdl_draw_text(fe, 225, current_y, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL & ALIGN_HLEFT, fe->white_colour, fe->cfg[j].sval);

                    // Store y position of this menu item
                    fe->config_window_options[fe->ncfg-1].y=current_y;
#ifdef DEBUGGING
                    printf("Config option: %u - ", j);
                    printf(fe->cfg[j].name);
                    printf(", default value = ");
                    printf(fe->cfg[j].sval);
                    printf("\n");
#endif
                    break;

                case C_BOOLEAN:
                    // Unicode values for special characters
                    // "\342\230\221" = tick in box
                    // "\342\230\222" = cross in box
                    // "\342\234\224" = bold tick
                    // "\342\234\230" = bold "ballot" cross

                    // Print true (unicode tick) or false (unicode cross) depending on the current
                    // value.
                    if(fe->cfg[j].ival)
                    {
                        sdl_draw_text(fe, 225, current_y, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL & ALIGN_HLEFT, fe->white_colour, "\342\234\224");
                    }
                    else
                    {
                        sdl_draw_text(fe, 225, current_y, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL & ALIGN_HLEFT, fe->white_colour, "\342\234\230");
                    };

                    // Store y position of this menu item
                    fe->config_window_options[fe->ncfg-1].y=current_y;
#ifdef DEBUGGING
                    printf("Config option: %u - ", j);
                    printf(fe->cfg[j].name);
                    printf(", default value = %u\n",fe->cfg[j].ival);
#endif
                    break;

                case C_CHOICES:
                    // The options for the drop down box are presented as a delimited list of 
                    // strings, with the first character as the delimiter.

                    // Extract the delimiter from the string.
                    delimiter=fe->cfg[j].sval[0];
                    int k=1, l=0;
                    char current_character;
                    char current_string[255];
                    char number_of_strings=0;

                    // This part extracts them and create menu options for them all.
#ifdef DEBUGGING
                    printf("Config option: %u - ", j);
                    printf(fe->cfg[j].name);
                    printf(", default value = %u\n",fe->cfg[j].ival);
#endif

                    // Start the while loop off
                    current_character=1;
                    while(current_character)
                    {
                        // Get the next character
                        current_character=fe->cfg[j].sval[k];

                        // Build it into a string
                        current_string[l] = current_character;

                        // If it's a delimiter or end-of-line...
                        if((current_character == delimiter) || (current_character == 0))
                        {
                            // Terminate the string
                            current_string[l] = 0;

                            // If it isn't an empty string...
                            if(current_string[0])
                            {
                                l=0;

                                // If it's the currently selected option string from the choices
                                // available
                                if(fe->cfg[j].ival == number_of_strings)
                                    // Draw it on screen.
                                    sdl_draw_text(fe, 225, current_y, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL & ALIGN_HLEFT, fe->white_colour, current_string);

                                // Add it to the array of all config sub-options for this 
                                // config option.
                                number_of_strings++;
                                fe->ncfg++;
                                fe->config_window_options=sresize(fe->config_window_options,fe->ncfg, struct config_window_option);
                                fe->config_window_options[fe->ncfg-1].y=current_y;
                                fe->config_window_options[fe->ncfg-1].config_option_index=number_of_strings;
                                fe->config_window_options[fe->ncfg-1].config_index=j;
#ifdef DEBUGGING
                                printf("Config option: %u.%u - ", j, number_of_strings);
                                printf(current_string);
                                printf("\n");
#endif
                            };
                        }
                        else
                        {
                            // Next character in the config sub-option string
                            l++;
                        };

                        // Next character in the string being parsed.
                        k++;
                    }; // end while
                break;
            }; // end select

            // Increment screen y position of this menu item
            current_y+=MENU_FONT_SIZE + 2;
            j++;
        }; // end while

        // Set the clipping region to the whole physical screen
        sdl_no_clip(fe);

        // Print "Press L+R to quit" to the screen at the bottom
        sdl_draw_text(fe, 100, current_y, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL & ALIGN_HCENTRE, fe->white_colour, "Press L + R to Quit");

        // Update screen
        sdl_end_draw(fe);
    }
    else
    {
#ifdef DEBUGGING
        printf("Game unpaused.\n");
#endif
        // Set the clipping region to the whole physical screen
        sdl_no_clip(fe);

        // Blank over the whole screen using the frontend "rect" routine with background colour
        sdl_draw_rect(fe, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, fe->background_colour);

        // Update screen
        sdl_end_draw(fe);

        // Make the game redraw itself.
        midend_force_redraw(fe->me);

        // Restore clipping rectangle to the puzzle screen
        sdl_unclip(fe);

        sdl_status_bar(fe,"Resumed.");
    };
};

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
    int mouse_accel_x=0;
    int mouse_accel_y=0;
    int button=0;
    SDL_Event event;
    SDL_Event emulated_event_down;
    SDL_Event emulated_event_up;
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
                            // Mouse timer calls this
                            case RUN_MOUSE_TIMER_LOOP:

                                // Update the joystick button status from the hardware
                                bs->joy_upleft=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_UPLEFT);
                                bs->joy_upright=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_UPRIGHT);
                                bs->joy_downleft=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_DOWNLEFT);
                                bs->joy_downright=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_DOWNRIGHT);
                                bs->joy_up=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_UP);
                                bs->joy_down=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_DOWN);
                                bs->joy_left=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_LEFT);
                                bs->joy_right=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_RIGHT);

                                bs->joy_l=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_L);
                                bs->joy_r=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_R);
                                bs->joy_a=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_A);
                                bs->joy_b=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_B);
                                bs->joy_x=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_X);
                                bs->joy_y=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_Y);
                                bs->joy_start=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_START);
                                bs->joy_select=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_SELECT);
                                bs->joy_volup=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_VOLUP);
                                bs->joy_voldown=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_VOLDOWN);
//                                bs->joy_stick=SDL_JoystickGetButton(fe->joy,GP2X_BUTTON_CLICK);

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
                                break; // switch( event.user.code) case MOUSE_TIMER_LOOP

                            // Game midend timer call this
                            case RUN_TIMER_LOOP:
                                if(!fe->paused)
                                {
                                    // Update the time elapsed since the last timer, so that the 
                                    // midend can take account of this.
                                    gettimeofday(&now, NULL);
                                    elapsed = ((now.tv_usec - fe->last_time.tv_usec) * 0.000001F + (now.tv_sec - fe->last_time.tv_sec));
#ifdef DEBUGGING
                                    printf("%f elapsed since last timer. %u, %u, %u\n", elapsed,fe->paused, fe->timer_active, (int) fe->sdl_timer_id);
#endif
                                    // Run the midend timer routine
                                    if(!fe->paused)
                                        midend_timer(fe->me, elapsed);

                                    // Update the time the timer was last fired.
                                    fe->last_time = now;
                                };
	                        break; // switch( event.user.code ) case RUN_TIMER_LOOP
                        }; // switch( event.user.code)
                    break; // switch( event.type ) case SDL_USEREVENT

                case SDL_JOYBUTTONUP:
                    switch(event.jbutton.button)
                    {
                        case GP2X_BUTTON_A:
                            // Simulate releasing left mouse button
                            emulated_event_up.type = SDL_MOUSEBUTTONUP;
                            emulated_event_up.button.type = SDL_MOUSEBUTTONUP;
                            emulated_event_up.button.button = SDL_BUTTON_LEFT;
                            emulated_event_up.button.state = SDL_RELEASED;
                            emulated_event_up.button.x = mouse_x;
                            emulated_event_up.button.y = mouse_y;
                            SDL_PushEvent(&emulated_event_up);
                            break;
                        case GP2X_BUTTON_Y:
                            // Simulate a middle-mouse button release
                            emulated_event_up.type = SDL_MOUSEBUTTONUP;
                            emulated_event_up.button.type = SDL_MOUSEBUTTONUP;
                            emulated_event_up.button.button = SDL_BUTTON_MIDDLE;
                            emulated_event_up.button.state = SDL_RELEASED;
                            emulated_event_up.button.x = mouse_x;
                            emulated_event_up.button.y = mouse_y;
                            SDL_PushEvent(&emulated_event_up);
                            break;
                        case GP2X_BUTTON_B:
                            // Simulate a right-mouse button release
                            emulated_event_up.type = SDL_MOUSEBUTTONUP;
                            emulated_event_up.button.type = SDL_MOUSEBUTTONUP;
                            emulated_event_up.button.button = SDL_BUTTON_RIGHT;
                            emulated_event_up.button.state = SDL_RELEASED;
                            emulated_event_up.button.x = mouse_x;
                            emulated_event_up.button.y = mouse_y;
                            SDL_PushEvent(&emulated_event_up);
                            break;
                        case GP2X_BUTTON_L:
                            bs->joy_l=0;

                            // If we are in the pause menu, let L and R pretend
                            // to be mouse clicks, so they change numbers too
                            if(fe->paused)
                            {
                                emulated_event_down.type = SDL_MOUSEBUTTONUP;
                                emulated_event_down.button.type = SDL_MOUSEBUTTONUP;
                                emulated_event_down.button.button = SDL_BUTTON_LEFT;
                                emulated_event_down.button.state = SDL_RELEASED;
                                emulated_event_down.button.x = mouse_x;
                                emulated_event_down.button.y = mouse_y;
                                SDL_PushEvent(&emulated_event_down);
                            };
                            break;

                        case GP2X_BUTTON_R:
                            bs->joy_r=0;

                            // If we are in the pause menu, let L and R pretend
                            // to be mouse clicks, so they change numbers too
                            if(fe->paused)
                            {
                                emulated_event_down.type = SDL_MOUSEBUTTONUP;
                                emulated_event_down.button.type = SDL_MOUSEBUTTONUP;
                                emulated_event_down.button.button = SDL_BUTTON_RIGHT;
                                emulated_event_down.button.state = SDL_RELEASED;
                                emulated_event_down.button.x = mouse_x;
                                emulated_event_down.button.y = mouse_y;
                                SDL_PushEvent(&emulated_event_down);
                            };
                            break;

                        case GP2X_BUTTON_VOLDOWN:
                            bs->joy_voldown=0;
                            break;

                        case GP2X_BUTTON_VOLUP:
                            bs->joy_volup=0;
                            break;
                    }; // switch(event.jbutton.button)
                    break; // switch( event.type ) case SDL_JOYBUTTONUP
    
                case SDL_JOYBUTTONDOWN:
                    switch(event.jbutton.button)
                    {		
                        case GP2X_BUTTON_A:
                            // Simulate a left-mouse click
                            emulated_event_down.type = SDL_MOUSEBUTTONDOWN;
                            emulated_event_down.button.type = SDL_MOUSEBUTTONDOWN;
                            emulated_event_down.button.button = SDL_BUTTON_LEFT;
                            emulated_event_down.button.state = SDL_PRESSED;
                            emulated_event_down.button.x = mouse_x;
                            emulated_event_down.button.y = mouse_y;
                            SDL_PushEvent(&emulated_event_down);				
                            break;

                    case GP2X_BUTTON_Y:
                        // Simulate a middle-mouse click
                        emulated_event_down.type = SDL_MOUSEBUTTONDOWN;
                        emulated_event_down.button.type = SDL_MOUSEBUTTONDOWN;
                        emulated_event_down.button.button = SDL_BUTTON_MIDDLE;
                        emulated_event_down.button.state = SDL_PRESSED;
                        emulated_event_down.button.x = mouse_x;
                        emulated_event_down.button.y = mouse_y;
                        SDL_PushEvent(&emulated_event_down);
                        break;

                    case GP2X_BUTTON_B:
                        // Simulate a right-mouse click
                        emulated_event_down.type = SDL_MOUSEBUTTONDOWN;
                        emulated_event_down.button.type = SDL_MOUSEBUTTONDOWN;
                        emulated_event_down.button.button = SDL_BUTTON_RIGHT;
                        emulated_event_down.button.state = SDL_PRESSED;
                        emulated_event_down.button.x = mouse_x;
                        emulated_event_down.button.y = mouse_y;
                        SDL_PushEvent(&emulated_event_down);
                        break;

                    case GP2X_BUTTON_X:
                        if(!fe->paused)
                        {
                            // Convert the current digit to ASCII, then make it look like it 
                            // was typed in at the keyboard, to allow entering numbers into
                            // solo, unequal, etc.  (48=ASCII value of the 0 key)
                            keyval = MOD_NUM_KEYPAD | (digit_to_input + 48);
                            midend_process_key(fe->me, 0, 0, keyval);
                        };
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
                            if(fe->paused)
                            {
                                emulated_event_down.type = SDL_MOUSEBUTTONDOWN;
                                emulated_event_down.button.type = SDL_MOUSEBUTTONDOWN;
                                emulated_event_down.button.button = SDL_BUTTON_LEFT;
                                emulated_event_down.button.state = SDL_PRESSED;
                                emulated_event_down.button.x = mouse_x;
                                emulated_event_down.button.y = mouse_y;
                                SDL_PushEvent(&emulated_event_down);
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
                            if(fe->paused)
                            {
                                emulated_event_down.type = SDL_MOUSEBUTTONDOWN;
                                emulated_event_down.button.type = SDL_MOUSEBUTTONDOWN;
                                emulated_event_down.button.button = SDL_BUTTON_RIGHT;
                                emulated_event_down.button.state = SDL_PRESSED;
                                emulated_event_down.button.x = mouse_x;
                                emulated_event_down.button.y = mouse_y;
                                SDL_PushEvent(&emulated_event_down);
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
                        fe->paused=!fe->paused;
			if(fe->helpfile_displayed)
				fe->helpfile_displayed=FALSE;
                        game_pause(fe);
                        break;

                    case GP2X_BUTTON_SELECT:
                        if(!fe->paused)
                        {
                            // Restart the current game
                            sdl_status_bar(fe,"Restarting game...");
                            midend_restart_game(fe->me);
                            midend_force_redraw(fe->me);
                        };
                        break;
                }; // switch(event.jbutton.button)
                break; // switch( event.type ) case SDL_JOYBUTTONDOWN

#ifndef TARGET_GP2X
// Used for debugging so that we can quit the game when using a Linux machine
// running on the same code!
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym)
                {
                    case SDLK_p:
                        // Simulate pressing the Start key
                        emulated_event_down.type = SDL_JOYBUTTONDOWN;
                        emulated_event_down.jbutton.type = SDL_JOYBUTTONDOWN;
                        emulated_event_down.jbutton.which = 0;
                        emulated_event_down.jbutton.button = GP2X_BUTTON_START;
                        emulated_event_down.jbutton.state = SDL_PRESSED;	 
                        SDL_PushEvent(&emulated_event_down);
                        break;

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
                        // Simulate pressing the L key
                        emulated_event_down.type = SDL_JOYBUTTONDOWN;
                        emulated_event_down.jbutton.type = SDL_JOYBUTTONDOWN;
                        emulated_event_down.jbutton.which = 0;
                        emulated_event_down.jbutton.button = GP2X_BUTTON_L;
                        emulated_event_down.jbutton.state = SDL_PRESSED;	 
                        SDL_PushEvent(&emulated_event_down);
                        break;

                    case SDLK_a:
                        // Simulate pressing the A key
                        emulated_event_down.type = SDL_JOYBUTTONDOWN;
                        emulated_event_down.jbutton.type = SDL_JOYBUTTONDOWN;
                        emulated_event_down.jbutton.which = 0;
                        emulated_event_down.jbutton.button = GP2X_BUTTON_A;
                        emulated_event_down.jbutton.state = SDL_PRESSED;	 
                        SDL_PushEvent(&emulated_event_down);
                        break;

                    case SDLK_b:
                        // Simulate pressing the B key
                        emulated_event_down.type = SDL_JOYBUTTONDOWN;
                        emulated_event_down.jbutton.type = SDL_JOYBUTTONDOWN;
                        emulated_event_down.jbutton.which = 0;
                        emulated_event_down.jbutton.button = GP2X_BUTTON_B;
                        emulated_event_down.jbutton.state = SDL_PRESSED;	 
                        SDL_PushEvent(&emulated_event_down);
                        break;

                    case SDLK_x:
                        // Simulate pressing the X key
                        emulated_event_down.type = SDL_JOYBUTTONDOWN;
                        emulated_event_down.jbutton.type = SDL_JOYBUTTONDOWN;
                        emulated_event_down.jbutton.which = 0;
                        emulated_event_down.jbutton.button = GP2X_BUTTON_X;
                        emulated_event_down.jbutton.state = SDL_PRESSED;	 
                        SDL_PushEvent(&emulated_event_down);
                        break;

                    case SDLK_r:
                        // Simulate pressing the R key
                        emulated_event_down.type = SDL_JOYBUTTONDOWN;
                        emulated_event_down.jbutton.type = SDL_JOYBUTTONDOWN;
                        emulated_event_down.jbutton.which = 0;
                        emulated_event_down.jbutton.button = GP2X_BUTTON_R;
                        emulated_event_down.jbutton.state = SDL_PRESSED;
                        SDL_PushEvent(&emulated_event_down);
                        break;

                    default:
                        break;
                }; // switch(event.key.keysym.sym)
                break; // switch( event.type ) case SDL_KEYDOWN

            case SDL_KEYUP:
                switch(event.key.keysym.sym)
                {
                    case SDLK_p:
                        emulated_event_up.type = SDL_JOYBUTTONUP;
                        emulated_event_up.jbutton.type = SDL_JOYBUTTONUP;
                        emulated_event_up.jbutton.which = 0;
                        emulated_event_up.jbutton.button = GP2X_BUTTON_START;
                        emulated_event_up.jbutton.state = SDL_RELEASED;	 
                        SDL_PushEvent(&emulated_event_up);
                        break;

                    case SDLK_l:
                        emulated_event_up.type = SDL_JOYBUTTONUP;
                        emulated_event_up.jbutton.type = SDL_JOYBUTTONUP;
                        emulated_event_up.jbutton.which = 0;
                        emulated_event_up.jbutton.button = GP2X_BUTTON_L;
                        emulated_event_up.jbutton.state = SDL_RELEASED;	 
                        SDL_PushEvent(&emulated_event_up);
                        break;

                    case SDLK_a:
                        emulated_event_up.type = SDL_JOYBUTTONUP;
                        emulated_event_up.jbutton.type = SDL_JOYBUTTONUP;
                        emulated_event_up.jbutton.which = 0;
                        emulated_event_up.jbutton.button = GP2X_BUTTON_A;
                        emulated_event_up.jbutton.state = SDL_RELEASED;	 
                        SDL_PushEvent(&emulated_event_up);
                        break;

                    case SDLK_b:
                        emulated_event_up.type = SDL_JOYBUTTONUP;
                        emulated_event_up.jbutton.type = SDL_JOYBUTTONUP;
                        emulated_event_up.jbutton.which = 0;
                        emulated_event_up.jbutton.button = GP2X_BUTTON_B;
                        emulated_event_up.jbutton.state = SDL_RELEASED;	 
                        SDL_PushEvent(&emulated_event_up);
                        break;

                    case SDLK_r:
                        emulated_event_up.type = SDL_JOYBUTTONUP;
                        emulated_event_up.jbutton.type = SDL_JOYBUTTONUP;
                        emulated_event_up.jbutton.which = 0;
                        emulated_event_up.jbutton.button = GP2X_BUTTON_R;
                        emulated_event_up.jbutton.state = SDL_RELEASED;
                        SDL_PushEvent(&emulated_event_up);
                        break;

                    default:
                        break;
                }; // switch(event.key.keysym.sym)
                break; // switch( event.type ) case SDL_KEYUP
#endif

            case SDL_MOUSEBUTTONUP:
                if(!fe->paused)
                {
                    if(event.button.button == 1)
                    {
                        button = LEFT_BUTTON;
                        bs->mouse_left=0;
                    }
                    else if(event.button.button == 2)
                    {
                        button = MIDDLE_BUTTON;
                        bs->mouse_middle=0;
                    }
                    else if (event.button.button == 3)
                    {
                        button = RIGHT_BUTTON;
                        bs->mouse_right=0;
                    };
                    button += LEFT_RELEASE - LEFT_BUTTON;
                    midend_process_key(fe->me, event.button.x, event.button.y, button);
                };
                break; // switch( event.type ) case SDL_MOUSEBUTTONUP

            case SDL_MOUSEBUTTONDOWN:
                if(!fe->paused)
                {
                    if (event.button.button == 1)
                    {
                        button = LEFT_BUTTON;
                        bs->mouse_left=1;
                    }
                    else if (event.button.button == 2)
                    {
                        button = MIDDLE_BUTTON;
                        bs->mouse_middle=1;
                    }
                    else if (event.button.button == 3)
                    {
                        button = RIGHT_BUTTON;
                        bs->mouse_right=1;
                    };
                    midend_process_key(fe->me, event.button.x, event.button.y, button);
                }
                else
                {
                    // If the helpfile was showing
                    if(fe->helpfile_displayed)
                    {
                        // Just redraw the config options screen.
                        game_pause(fe);
                        fe->helpfile_displayed=FALSE;
                    }
                    else
                    {
                        // Get the line that was clicked (lines are MENU_FONT_SIZE+2 pixels high)
                        int current_line=event.button.y/(MENU_FONT_SIZE+2);
                        char *result;

                        switch(current_line)
                        {
                            case 3:
                                // Start a new game menu item

                                // Plug the currently selected config on the menu screen back into the game
                                result=midend_set_config(fe->me, CFG_SETTINGS, fe->cfg);

                                // If the midend won't accept the new config
                                if(result)
                                {
                                    // Present the user with the midend's error text.
                                    sdl_status_bar(fe,result);
                                }
                                else
                                {
                                    // Unpause the game
                                    fe->paused=FALSE;

                                    // Resize/scale the game screen
                                    configure_area(SCREEN_WIDTH,SCREEN_HEIGHT,fe);	

                                    // Set the clipping region to the whole physical screen
                                    sdl_no_clip(fe);

                                    // Blank over the whole screen using the frontend "rect" routine with background colour
                                    sdl_draw_rect(fe, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, fe->background_colour);

                                    sdl_status_bar(fe,"Generating a new game...");

                                    // Update the screen.
                                    sdl_end_draw(fe);

                                    // Start a new game with the new config
                                    midend_new_game(fe->me);

                                    // Set the clipping region to the whole physical screen
                                    sdl_no_clip(fe);

                                    // Blank over the whole screen using the frontend "rect" routine with background colour
                                    sdl_draw_rect(fe, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, fe->background_colour);

                                    // Make the game redraw itself.	
                                    midend_force_redraw(fe->me);

                                    // Set the clipping region to the game window.
                                    sdl_unclip(fe);
                                };
                                break;

                            case 4:
                                // Restart

                                // Come out of the pause menu and restart the game.
                                fe->paused=FALSE;
                                game_pause(fe);
                                midend_restart_game(fe->me);
                                midend_force_redraw(fe->me);
                                sdl_status_bar(fe,"Restarting game...");
                                break;

                            case 5:
                                // Show helpfile or error if it wasn't found.
                                if(!read_game_helpfile(fe))
                                    sdl_status_bar(fe,"Could not open helpfile.");
                                break;

                            default:
                                // If someone clicking where there might be config options...
                                if(current_line > 6)
                                {
                                    int i,j;
                                    int config_item_found=FALSE;

                                    // Loop over all config options
                                    for(i=0; i < fe->ncfg; i++)
                                    {
                                        // Find the config option that relate to the position clicked
                                        if(((fe->config_window_options[i].y-1) / (MENU_FONT_SIZE+2)) == current_line && !config_item_found)
                                        {
                                            int current_cfg_item_index=fe->config_window_options[i].config_index;
                                            int current_number=0;
                                            int max_config_option_index;
                                            int current_config_option_index;

                                            config_item_found=TRUE;

                                            switch(fe->cfg[current_cfg_item_index].type)
                                            {
                                                case C_STRING:
                                                    // If it's got a decimal point in it
                                                    if(strchr(fe->cfg[current_cfg_item_index].sval,'.'))
                                                    {
                                                        // Add or remove 0.1 from it
                                                        float current_probability=atof(fe->cfg[current_cfg_item_index].sval);
                                                        if (event.button.button == 1)
                                                            current_probability-=0.1;				
                                                        if (event.button.button == 3)
                                                            current_probability+=0.1;
                                                        sprintf(fe->cfg[current_cfg_item_index].sval, "%.1f", current_probability);
                                                    }
                                                    else
                                                    {
                                                        // Add or remove 1 from it
                                                        current_number=atoi(fe->cfg[current_cfg_item_index].sval);
                                                        if (event.button.button == 1)
                                                            current_number--;				
                                                        if (event.button.button == 3)
                                                            current_number++;
                                                        sprintf(fe->cfg[current_cfg_item_index].sval, "%d", current_number);
                                                    };
                                                    break;

                                                case C_BOOLEAN:
                                                    // Toggle the value.
                                                    fe->cfg[current_cfg_item_index].ival=!(fe->cfg[current_cfg_item_index].ival);
                                                    break;

                                                case C_CHOICES:
                                                    max_config_option_index=0;

                                                    // Loop through all the options and find out how
                                                    // many possibilities there are for this config option

                                                    for(j=0; j < fe->ncfg; j++)
                                                    {
                                                 	if(fe->config_window_options[j].config_index==fe->config_window_options[i].config_index)
                                                        {
                                                            if(fe->config_window_options[j].config_option_index > max_config_option_index)
                                                                max_config_option_index=fe->config_window_options[j].config_option_index;
                                                        }
                                                    };
                                                    current_config_option_index=fe->cfg[current_cfg_item_index].ival;
#ifdef DEBUGGING
                                                    printf("Was %u, ",current_config_option_index);
#endif
                                                    // Increment or decrement the index of the selected option
                                                    if (event.button.button == 1)
                                                        current_config_option_index--;
                                                    if (event.button.button == 3)
                                                        current_config_option_index++;

                                                    // Make sure to loop through the options rather than falling off the end
                                                    if(current_config_option_index<0)
                                                        current_config_option_index=max_config_option_index-1;
                                                    if(current_config_option_index>(max_config_option_index-1))
                                                        current_config_option_index=0;

                                                    // Set the new option
                                                    fe->cfg[current_cfg_item_index].ival=current_config_option_index;
#ifdef DEBUGGING
                                                    printf("is %u, out of %u\n",current_config_option_index, max_config_option_index);
#endif
                                                    break;
                                            }; // switch(fe->cfg[current_cfg_item_index].type)
                                        };// if
                                    };// for
                                };// if

                                // Redraw the pause/config menu
                                game_pause(fe);
                                break;
                        }; // switch(current_line)
                    };
                };
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

                if(fe->paused)
                {
                    if(mouse_x > (SCREEN_WIDTH - 1))
                        mouse_x = SCREEN_WIDTH - 1;
                    if(mouse_y > (SCREEN_HEIGHT - 1))
                        mouse_y = SCREEN_HEIGHT - 1;
                }
                else
                {
                    if(mouse_x > (fe->pw - 1))
                        mouse_x = fe->pw - 1;
                    if(mouse_y > (fe->ph - 1))
                        mouse_y = fe->ph - 1;
                };

                // If we had to move the mouse with those checks...
                if((mouse_x != event.motion.x) || (mouse_y != event.motion.y))
                {
                    // Move the mouse cursor to where it needs to be now
                    // (this will automatically generate a new MOUSEMOTION event)
                    SDL_WarpMouse((Uint16) mouse_x, (Uint16) mouse_y);
                }
                else
                {
                    if(!fe->paused)
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
                };			
                break; // switch( event.type ) case SDL_MOUSEMOTION

            case SDL_QUIT:
                sdl_status_bar(fe,"Quitting...");
                exit(0);
                break;
             } // switch(event.type)
            }; // if(fe->pixmap)
        }; // while( SDL_PollEvent( &event ))
    }; // while(1)
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
    fe->paused=FALSE;

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
    // status bar.  The fonts structure will make sure that we don't load it up again 
    // unnecessarily, by caching fonts used at the particular sizes used and growing
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

    fe->screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, SDL_SWSURFACE);

    // If the video mode was initialised successfully
    if(fe->screen)
    {
        printf("Initialised %u x %u @ %u bit video mode.\n", SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH);
    }
    else
    {
        printf("Error initialising %u x %u @ %u bit video mode: %s\n", SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, SDL_GetError());
        exit(255);
    };

    // Index of background colour in colours array.
    fe->background_colour=0;

    // Get configuration options from the midend.
    fe->cfg=midend_get_config(fe->me, CFG_SETTINGS, &fe->configure_window_title);

    // Blank over the whole screen using the frontend "rect" routine with background colour.
    sdl_draw_rect(fe, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, fe->background_colour);

    // Turn off the cursor as soon as we can.
    SDL_ShowCursor(SDL_DISABLE);

    // Update the user in case the next bit takes a long time.
    sdl_status_bar(fe,"Loading...");

    // Size the game for the current screen.
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

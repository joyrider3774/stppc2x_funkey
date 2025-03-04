/*
 * sdl.c: SDL front end for the puzzle collection.
 *        Specifically targetted at the GP2X Linux-based handheld console.
 */

// DEBUGGING
// =========

// Define these to get lots of debugging info at runtime.
// #define DEBUG_DRAWING       // Drawing primitives, frame updates, etc.
// #define DEBUG_PALETTE       // Assignment of game colours, menu colours etc.
// #define DEBUG_FILE_ACCESS   // Saving, loading games, config, helpfiles etc.
// #define DEBUG_MOUSE         // Mouse emulation, movement etc.
// #define DEBUG_TIMER         // Timers, etc.
// #define DEBUG_MISC          // Config options, etc.
// #define DEBUG_FUNCTIONS     // Function calls

//#define SCALELARGESCREEN
#define BACKGROUND_MUSIC

// Define this to implement kludges for certain game-specific options
// (Usually the ones that require floating point numbers but don't tell us)
#define OPTION_KLUDGES

// Define this to use threads for the loading screen.  Valgrind etc. prefer 
// this option to be off, otherwise weird stuff happens.  This option does 
// not affect or imply SDL's EVENTS_IN_SEPERATE_THREADS (see below) which
// doesn't seem to bother Valgrind anyway.
// #define OPTION_USE_THREADS

// Define this to show a tickmark in the main menu for games with the
// REQUIRE_MOUSE_INPUT flag (currently, there are no games that NEED a mouse
// anymore)
// #define OPTION_SHOW_IF_MOUSE_NEEDED

// Define this to check whether we are running on a GP2X and the model we are
// using.  Pretty useless for now, but might come in handy for, e.g. The Wiz.
// #define OPTION_CHECK_HARDWARE


// VIDEO STORAGE AND ACCELERATION
// ==============================

// Software - works perfectly, if a little slowly
#define SDL_SURFACE_FLAGS SDL_SWSURFACE

// Hardware - flickers but works
// #define SDL_SURFACE_FLAGS SDL_HWSURFACE

// Double-buffered Hardware - no flicker, needs changes to locking, off-screen surface etc.
// Not working with this code (but trivial to get working).
// #define SDL_SURFACE_FLAGS SDL_HWSURFACE|SDL_DOUBLEBUF

// EVENT MODEL
// ===========
 
#define FAST_SDL_EVENTS              // Use the SDL Fast Events code
#define WAIT_FOR_EVENTS              // Sleep briefly when no events are pending

#if defined(_WIN32) || defined(__CYGWIN__)
    // Windows-ish thing detected.  Probably doesn't support pthreads.  Disabling event threads.
#else
    #define EVENTS_IN_SEPERATE_THREAD    // Spawn a seperate thread to handle events
#endif

#ifndef uint
typedef unsigned int uint;
#endif

// Standard C includes
// ===================
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>     // For readdir()
#include <dirent.h>        // For readdir()
#include <unistd.h>        // For sync() call.

// Puzzle collection include
// =========================
#include "puzzles.h"
#include "malloc.h"

// SDL library includes
// ====================
#include <SDL/SDL_stdinc.h>
#include <SDL/SDL.h>
//#include <SDL/SDL_gp2x.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_rotozoom.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>
#ifdef BACKGROUND_MUSIC
    #include <SDL/SDL_mixer.h>
#endif

#ifdef OPTION_USE_THREADS
  #include <SDL/SDL_thread.h>
#endif

// INIParser library includes
// ==========================
#include "iniparser.h"

// SDL fast events code
// ====================
#ifdef FAST_SDL_EVENTS
  #include "fastevents.h"
#endif

// Function prototypes for this file itself
// ========================================
#include "sdl.h"

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

// GP2X Clock speed settings
#define FCLK_200                        10
#define FCLK_166                        11
#define FCLK_133                        12
#define FCLK_100                        13
#define FCLK_78                         14
#define FCLK_64                         15

// Constants used in SDL user-defined events
// RUN_GAME_TIMER_LOOP - Game timer for midend
// RUN_MOUSE_TIMER_LOOP	- "Mouse" timer for joystick control
// RUN_SECOND_TIMER_LOOP - Regular 1 per second timer, for non-critical events.
enum { RUN_GAME_TIMER_LOOP, RUN_MOUSE_TIMER_LOOP, RUN_SECOND_TIMER_LOOP};

// Timer Intervals
// Generally, game timer interval has to be larger than mouse timer or the game won't
//  able to draw all the mouse movements properly and in time.
// DO NOT MAKE THESE TWO NUMBERS THE SAME.  The interval is used to distinguish 
// between the events in a consolidated timer function.
#define SDL_GAME_TIMER_INTERVAL  (50)  // Interval in milliseconds for game timer
#define SDL_MOUSE_TIMER_INTERVAL (25)  // Interval in milliseconds for mouse timer

// Number of seconds that a statusbar message should stay on the screen.
#define STATUSBAR_TIMEOUT (3)

#define ANIMATION_DELAY          (200) // Interval in milliseconds for the delay
                                       // between frames in the loading animation.

// Actual screen size and colour depth.
#define SCREEN_WIDTH_SMALL	(240)
#define SCREEN_HEIGHT_SMALL	(240)

#define SCREEN_WIDTH_LARGE	(640)
#define SCREEN_HEIGHT_LARGE	(480)

#define SCREEN_DEPTH	(16)

// Font sizes in pixels (double these sizes are used for large-screen games).
// (Actually in "points" but at 72dpi it makes no difference)

// Font size of the statusbar font
#define DEFAULT_STATUSBAR_FONT_SIZE  (8)

// Font size of the keyboard icon
#define DEFAULT_KEYBOARD_ICON_FONT_SIZE  (17)

// Font size of the menu font
#define DEFAULT_MENU_FONT_SIZE      (12)

// Font size of the help text
#define DEFAULT_HELP_FONT_SIZE      (10)

// Maximum pixels of movement per mouse timer tick movement.
#define MAX_MOUSE_ACCELERATION      (30)

// Filename of a Truetype, Unicode-capable, monospaced font
#define FIXED_FONT_FILENAME	"fonts/DejaVuSansMono-Bold.ttf"

// Filename of a Truetype, Unicode-capable, variable-spaced font
#define VARIABLE_FONT_FILENAME	"fonts/DejaVuSansCondensed-Bold.ttf"

// Filename of a bitmap for the inital loading screen
#define LOADING_SCREEN_FILENAME "images/loading.png"

// Filename of a bitmap for the menu background image
#define MENU_BACKGROUND_IMAGE "images/background.png"

// Filename of a bitmap for the music credits image
#define MENU_MUSIC_CREDITS_IMAGE "images/music.png"

// Filename of a bitmap for the menu "about" dialog
#define MENU_ABOUT_IMAGE "images/about.png"

// Filespec of bitmaps for the game previews
#define MENU_PREVIEW_IMAGES "images/%s.png"

// Filespec of helpfiles
#define MENU_HELPFILES "help/%s.txt"

// Filespec of helpfiles
#define MENU_KEY_HELPFILE "help/keys.txt"

// Filespec of helpfiles
#define MENU_INGAME_KEY_HELPFILE "help/ingamekeys.txt"

// Filename of the menu "game description" data
#define MENU_DATA_FILENAME "stppc2x.data"

// Filename of the global configuration INI file
#define GLOBAL_CONFIG_FILENAME "stppc2x.ini"

// Filename of the credits file.
#define MENU_CREDITS_FILENAME "credits.txt"

// Filename of a saved screenshot
#define SCREENSHOT_FILENAME "screenshots/screenshot%04u.bmp"

// Path for music files
#define MUSIC_PATH "music/"

// The longest internal name of any game to crop all filenames
// etc. to.  Longest name is "Black Box" or "Rectangles" at the
// moment.
#define MAX_GAMENAME_SIZE               10

// Unicode values for special characters
#define UNICODE_TICK_CHAR   "\342\234\224"
//                          = bold tick
#define UNICODE_CROSS_CHAR  "\342\234\230"
//                          = bold "ballot" cross
#define UNICODE_UP_ARROW    "\342\226\262"
//                          = BLACK UP-POINTING TRIANGLE
#define UNICODE_DOWN_ARROW  "\342\226\274"
//                          = BLACK DOWN-POINTING TRIANGLE
#define UNICODE_SKULL_CROSSBONES  "\342\230\240"
//                          = SKULL AND CROSSBONES
#define UNICODE_MUSICAL_NOTE "\342\231\253"
//                          = BEAMED EIGHTH NOTES
#define UNICODE_KEYBOARD     "\342\214\250"
//
#define UNICODE_STYLUS     "\342\234\215"
//
#define UNICODE_NUMERO     "\342\204\226"

// Other Unicode values.
// (not all supported by DejaVu fonts)
// "\342\230\221" = tick in box
// "\342\230\222" = cross in box
// "\342\226\266" = Black right-pointing triangle
// "\342\226\272" = Black right-pointing pointer
// "\342\227\274" = Black medium square
// "\342\226\240" = Black square
// "\342\214\202" = House
// "\342\214\233" = Hourglass
// "\342\231\252" = Eighth Note
// "\342\214\250" = Keyboard
// "\342\234\215" = writing hand

#ifdef DEBUGGING
static FILE *debug_fp = NULL;

void dputs(char *buf)
{
#ifdef DEBUG_FUNCTIONS
    printf("dputs()\n");
#endif
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
#ifdef DEBUG_FUNCTIONS
    printf("debug_printf()\n");
#endif
    char buf[4096];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    dputs(buf);
    va_end(ap);
}
#endif

#ifdef DEBUG_TIMER
void print_time()
{
#ifdef DEBUG_FUNCTIONS
    printf("print_time()\n");
#endif

    float elapsed;
    struct timeval *time_now = snew(struct timeval);
    gettimeofday(time_now, NULL);
    elapsed = time_now->tv_usec * 0.000001F + time_now->tv_sec;
    printf("%f : ", elapsed);
    sfree(time_now);
}
#endif 

// Font structure that holds details on each loaded font.
struct font
{
    TTF_Font *font;	// Handle to the opened font at a particular fontsize.
    uint type;		// FONT_FIXED for monospaced font, FONT_VARIABLE for variable-spaced font.
    uint size;		// size (in pixels, points@72dpi) of the font.
};

// Used as a temporary area by the games to save/load portions of the screen.
struct blitter
{
    SDL_Surface *pixmap;  // Actual surface
    int w, h;             // Size
    int x, y;             // Position to load/save
};

// Structure to keep track of all GP2X buttons.
struct button_status
{
    uint mouse_left;
    uint mouse_middle;
    uint mouse_right;
    uint joy_up;
    uint joy_down;
    uint joy_left;
    uint joy_right;
    uint joy_upright;
    uint joy_downright;
    uint joy_upleft;
    uint joy_downleft;
    uint joy_l;
    uint joy_r;
    uint joy_a;
    uint joy_b;
    uint joy_x;
    uint joy_y;
    uint joy_start;
    uint joy_select;
    uint joy_volup;
    uint joy_voldown;
    uint joy_stick;
};

// Structure to hold one of the game's configuration options
struct config_window_option
{
    uint y;			// Vertical position on menu screen
    uint config_index;		// Which configuration option
    uint config_option_index;	// Which configuration sub-option (for "dropdown" options)
};

enum{ MOUSE_EMULATION, CURSOR_KEYS_EMULATION };

struct global_configuration
{
    uint autosave_on_exit;
    uint always_load_autosave;
    uint play_music;
    uint screenshots_enabled;
    uint screenshots_include_cursor;
    uint screenshots_include_statusbar;
    uint control_system;
    uint tracks_to_play[10];
    uint music_volume;
};

enum{ GAMELISTMENU, INGAME, GAMEMENU, SAVEMENU, CONFIGMENU, PRESETSMENU, HELPMENU, CREDITSMENU, MUSICCREDITSMENU, SETTINGSMENU, MUSICMENU} ;

// The main frontend struct.
struct frontend
{
    SDL_Color *sdlcolours;		// Array of colours used.
    uint ncolours;       		// Number of colours used.
    uint white_colour;			// Index of white colour
    uint background_colour;		// Index of background colour
    uint black_colour;			// Index of black colour
    int w, h;				// Width and height of screen?
    midend *me;				// Pointer to a game midend
    uint last_status_bar_w;		// Used to blank out the statusbar before redrawing
    uint last_status_bar_h;		// Used to blank out the statusbar before redrawing
    uint timer_active;			// Whether the timer is enabled or not.
    uint timer_was_active;		// Whether the timer was enabled before we paused or not.
    struct timeval last_time;		// Last time the midend game timer was run.
    config_item *cfg;                   // The game's configuration options
    uint ncfg;                          // Number of configuration options
    int pw, ph;				// Puzzle size
    int ox, oy;				// Offset of puzzle in drawing area (for centering)
    SDL_Surface *screen;		// Main screen
    SDL_Joystick *joy;			// Joystick
    SDL_TimerID sdl_mouse_timer_id;	// "Mouse" timer
    SDL_TimerID sdl_timer_id;		// Midend game timer
    SDL_TimerID sdl_second_timer_id;	// Once per second utility timer
    struct font *fonts;			// A cache of loaded fonts at particular fontsizes
    uint nfonts;			// Number of cached fonts
    uint paused;			// True if paused (menu showing)
    SDL_Rect clipping_rectangle;	// Clipping rectangle in use by the game.
    char *configure_window_title;	// The window title for the configure window
    struct config_window_option *config_window_options;
					// The games configuration options, with extra information
					// used for determining the position on screen.
    dictionary *ini_dict;		// In-memory INI "dictionary"
    char* sanitised_game_name;          // A copy of the game name suitable for use in filenames
    uint first_preset_showing;          // The preset currently at the top of the preset menu.
    struct timeval last_statusbar_update;		// Last time the status bar was updated.    
};

struct button_status *bs;
int current_game_index=-1;
game this_game;
uint screen_width, screen_height, current_screen;
int savegame_to_delete=-1;
int respawn_gp2x_menu=TRUE;

extern char stppc2x_ver[];

struct global_configuration *global_config;

// Variables to hold the font size (which doubles and halves
// according to the resolution).
uint STATUSBAR_FONT_SIZE = DEFAULT_STATUSBAR_FONT_SIZE;
uint MENU_FONT_SIZE = DEFAULT_MENU_FONT_SIZE;
uint HELP_FONT_SIZE = DEFAULT_HELP_FONT_SIZE;
uint KEYBOARD_ICON_FONT_SIZE = DEFAULT_KEYBOARD_ICON_FONT_SIZE;

// "Mouse" (virtual or real) co-ordinates.
signed int mouse_x=0;
signed int mouse_y=0;

// Temporary screen placeholder - DO NOT USE.
// We need this to keep hold of the screen until the frontend is fully 
// up and running, then we copy it into the frontend structure.
SDL_Surface *screen;

#ifdef SCALELARGESCREEN
SDL_Surface *real_screen;
#endif

#ifdef OPTION_USE_THREADS
  SDL_Thread *splashscreen_animation_thread;
#endif

uint *loading_flag;

SDL_Surface *loading_screen;
SDL_Surface *menu_screen;

// Currently selected save slot.
uint current_save_slot=0;

uint current_music_track=0;
uint music_track_changed=FALSE;

uint helpfile_showing=FALSE;

enum{UNKNOWN, NOT_GP2X, GP2X_F100, GP2X_F200};

#ifdef OPTION_CHECK_HARDWARE
int hardware_type=UNKNOWN;
#endif

uint first_run=TRUE;

#ifdef BACKGROUND_MUSIC
Mix_Music *music = NULL;
#else
    #define MIX_MAX_VOLUME 128
#endif

// Required function for midend operation.
void fatal(char *fmt, ...)
{
#ifdef DEBUG_FUNCTIONS
    printf("fatal()\n");
#endif
    char buf[2048];
    va_list ap;

    memset(buf, 0, 2048 * sizeof(char));

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    printf("Fatal error: %s\n", buf);
    exit(EXIT_FAILURE);
}

void change_clockspeed(frontend *fe, uint new_clock_speed)
{
#ifdef DEBUG_MISC
    printf("Requesting clockspeed change: %u (10 = max, 15 = min speed)\n", new_clock_speed);
#endif
#ifdef TARGET_GP2X
    SDL_SYS_JoystickGp2xSys(fe->joy,new_clock_speed);
#endif
};

void Push_SDL_Event(SDL_Event *event)
{
#ifdef DEBUG_FUNCTIONS
    printf("Push_SDL_Event()\n");
#endif

#ifdef FAST_SDL_EVENTS
    FE_PushEvent(event);
#else
    SDL_PushEvent(event);
#endif
};

// Locks a surface
void Lock_SDL_Surface(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("Lock_SDL_Surface()\n");
#endif

#ifdef DEBUG_DRAWING
    printf("Locking Screen Surface.\n");
#endif
    actual_lock_surface(fe->screen);
};

void actual_lock_surface(SDL_Surface *surface)
{
#ifdef DEBUG_FUNCTIONS
    printf("actual_lock_surface()\n");
#endif

    if(SDL_MUSTLOCK(surface))
    {
        if(SDL_LockSurface(surface))
        {
       	    printf("Error locking SDL Surface.\n");
	    exit(EXIT_FAILURE);
        }
    }
}

// Unlocks a surface
void Unlock_SDL_Surface(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("Unlock_SDL_Surface()\n");
#endif

#ifdef DEBUG_DRAWING
    printf("Unlocking SDL Surface.\n");
#endif
    actual_unlock_surface(fe->screen);
}

void actual_unlock_surface(SDL_Surface *surface)
{
#ifdef DEBUG_FUNCTIONS
    printf("actual_unlock_surface()\n");
#endif

    if(SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
}

void cleanup(frontend *fe)
{
    uint i=0;

#ifdef DEBUG_FUNCTIONS
    printf("cleanup()\n");
#endif
    printf("Cleaning up...\n");

    if(fe->ini_dict != NULL)
    {
        iniparser_freedict(fe->ini_dict);
        fe->ini_dict=NULL;
    };

    if(fe->sanitised_game_name != NULL)
        sfree(fe->sanitised_game_name);

    if(fe->me != NULL)
    {
        midend_free(fe->me);
        fe->me=NULL;
    };

    if(fe->sdlcolours != NULL)
        sfree(fe->sdlcolours);

    if(fe->config_window_options != NULL)
    {
        sfree(fe->config_window_options);
        fe->config_window_options=NULL;
    };

    if(fe->configure_window_title != NULL)
        sfree(fe->configure_window_title);

    for(i=0;i<fe->nfonts;i++)
    {
        TTF_CloseFont(fe->fonts[i].font);
        fe->fonts[i].font=NULL;
    };

    if(fe->fonts != NULL)
        sfree(fe->fonts);

    if(fe->cfg != NULL)
        free_cfg(fe->cfg);

    if(SDL_JoystickOpened(0))
        SDL_JoystickClose(fe->joy);
};

void cleanup_and_exit(frontend *fe, int return_value)
{
#ifdef DEBUG_FUNCTIONS
    printf("cleanup_and_exit()\n");
#endif
    sfree(bs);
    sfree(loading_flag);
    if(loading_screen != NULL)
        SDL_FreeSurface(loading_screen);
    cleanup(fe);
    sfree(fe);
    DestroyMemPool();
#ifdef BACKGROUND_MUSIC
    Mix_CloseAudio();
#endif
    SDL_Quit();
    printf("Exiting...\n");
    exit(return_value);
};

// Required function for midend operation. 
// Returns a random seed (various times to high-precision, because the GP2X lacks a RT clock).
void get_random_seed(void **randseed, int *randseedsize)
{
#ifdef DEBUG_FUNCTIONS
    printf("get_random_seed()\n");
#endif

#ifdef DEBUG_MISC
    printf("Random seed requested.\n");
#endif
/*
    // Initialise the random seed using the number of milliseconds
    // since SDL initialisation.
    Uint32 *milliseconds_since_SDL_initialisation = snew(Uint32);
    *milliseconds_since_SDL_initialisation=SDL_GetTicks();
    *randseed = (void *)milliseconds_since_SDL_initialisation;
    *randseedsize = sizeof(Uint32);
*/

    // Initialise the random seed using the number of microseconds
    // since the GP2X was first turned on.
    struct timeval *tvp = snew(struct timeval);
    gettimeofday(tvp, NULL);
    *randseed = (void *)tvp;
    *randseedsize = sizeof(struct timeval);
}

// Required function for midend operation.
// Used for querying the default background colour that should be used.
void frontend_default_colour(frontend *fe, float *output)
{
#ifdef DEBUG_FUNCTIONS
    printf("frontend_default_colour()\n");
#endif

#ifdef DEBUG_DRAWING
    printf("Default frontend colour requested.\n");
#endif

    // This is the RGB for the background used for the games, expressed
    // as three floats from 0 to 1.

    // The games use this to work out what colours to use for highlights
    // etc.
    output[0]=0.75;
    output[1]=0.75;
    output[2]=0.75;

#ifdef DEBUG_PALETTE
    printf("Background colour selected is: %f, %f, %f (RGB: %u, %u, %u)\n", output[0], output[1], output[2], (Uint8) output[0] * 255, (Uint8) output[1] * 255, (Uint8) output[2] * 255);
#endif
}

// Searches through the font cache for a particular font.
// If found, returns index, if not, loads, caches and returns index
int find_and_cache_font(void *handle, int fonttype, int fontsize)
{
#ifdef DEBUG_FUNCTIONS
    printf("find_and_cache_font()\n");
#endif

    frontend *fe = (frontend *)handle;
    uint i;

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

    // Loop until we find an existing font of the right type and size.
    for (i = 0; i < fe->nfonts; i++)
        if((fe->fonts[i].size == (uint) fontsize) && (fe->fonts[i].type == (uint) fonttype))
           break;

    // If we hit the end without finding a suitable, already-loaded font, load one into memory.
    if(i == fe->nfonts)
    {
        // Dynamically increase the fonts array by 1
        fe->nfonts++;
        fe->fonts = sresize(fe->fonts, fe->nfonts, struct font);

        // Plug the size and type into the cache
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
            printf("Error loading TTF font: %s\nMake sure %s and %s\nare in the same folder as the program.\n",TTF_GetError(), FIXED_FONT_FILENAME, VARIABLE_FONT_FILENAME);
	    cleanup_and_exit(fe, EXIT_FAILURE);
        };
    };
    return(i);
};

// This function is called at the start of "drawing" (i.e a frame).
void sdl_start_draw(void *handle)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_start_draw()\n");
#endif

    // I don't think that there's anything special that needs doing at the start of a game 
    // frame - maybe if we were double-buffering?

#ifdef DEBUG_DRAWING
    printf("Start of a frame.\n");
#endif
}

// Removes ALL clipping from the entire physical screen without affecting the "logical" puzzle 
// clipping rectangle.
void sdl_no_clip(void *handle)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_no_clip()\n");
#endif

    frontend *fe = (frontend *)handle;
    SDL_Rect fullscreen_clipping_rectangle;

    fullscreen_clipping_rectangle.x = 0;
    fullscreen_clipping_rectangle.y = 0;
    fullscreen_clipping_rectangle.w = screen_width;
    fullscreen_clipping_rectangle.h = screen_height;

#ifdef DEBUG_DRAWING
    printf("Clipping area now: %u,%u - %u,%u\n", 0, 0, screen_width-1, screen_height-1);
#endif
    SDL_SetClipRect(fe->screen, &fullscreen_clipping_rectangle);
}

// Establishes a clipping rectangle in the puzzle window by passing through to the 
// do-all clipping function with the puzzle offset.
void sdl_clip(void *handle, int x, int y, int w, int h)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_clip()\n");
#endif

    frontend *fe = (frontend *)handle;
    sdl_actual_clip(handle, x + fe->ox, y + fe->oy, w, h);
};

// Establishes a clipping rectangle on the screen.
void sdl_actual_clip(void *handle, int x, int y, int w, int h)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_actual_clip()\n");
#endif

    frontend *fe = (frontend *)handle;
    fe->clipping_rectangle.x = x;
    fe->clipping_rectangle.y = y;
    fe->clipping_rectangle.w = w;
    fe->clipping_rectangle.h = h;

#ifdef DEBUG_DRAWING
    printf("Clipping area now: %u,%u - %u,%u\n", x, y, x+w, y+h);
#endif
    SDL_SetClipRect(fe->screen, &fe->clipping_rectangle);
}

// Reverts the effect of a previous call to clip(). 
void sdl_unclip(void *handle)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_unclip()\n");
#endif

    frontend *fe = (frontend *)handle;

    // Force the clipping to the edges of the *visible game window*, rather than the screen
    // so that we don't get blit garbage at the edges of the game.

    fe->clipping_rectangle.x = fe->ox;
    fe->clipping_rectangle.y = fe->oy;
    fe->clipping_rectangle.w = fe->pw;
    fe->clipping_rectangle.h = fe->ph;

#ifdef DEBUG_DRAWING
    printf("Clipping area now: %u,%u - %u,%u\n", fe->ox, fe->oy, fe->ox + fe->pw - 1, fe->oy + fe->ph - 1);
#endif
    SDL_SetClipRect(fe->screen, &fe->clipping_rectangle);
}

// Wrapper function for the games to call.
void sdl_draw_text(void *handle, int x, int y, int fonttype, int fontsize, int align, int colour, char *text)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_draw_text()\n");
#endif

    frontend *fe = (frontend *)handle;
    sdl_actual_draw_text(handle, x + fe->ox, y + fe->oy, fonttype, fontsize, align, colour, text);
}

// Draws coloured text - The games only ever really use fonttype=FONT_VARIABLE (maze3d has one 
// instance of fixed-width fonts) but this supports all possible combinations.  
// This is also used to draw text for various internal functions like the pause menu, help file,
// configuration options etc.
void sdl_actual_draw_text(void *handle, int x, int y, int fonttype, int fontsize, int align, int colour, char *text)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_actual_draw_text()\n");
#endif
    frontend *fe = (frontend *)handle;

#ifdef DEBUG_DRAWING
    printf("Writing '%s' to %u, %u in colour %u (RGB: %u, %u, %u)\n", text, x, y, colour, fe->sdlcolours[colour].r, fe->sdlcolours[colour].g, fe->sdlcolours[colour].b);
#endif

    SDL_Surface *text_surface;
    SDL_Rect blit_rectangle;
    int font_w,font_h;
    int font_index;

    // Only if we're being asked to actually draw some text...
    // Sometimes the midend does this to us.
    if( (text!=NULL) && strlen(text) )
    {
        font_index=find_and_cache_font(fe, fonttype, fontsize);

        // Retrieve the actual size of the rendered text for that particular font
        if(TTF_SizeText(fe->fonts[font_index].font,text,&font_w,&font_h))
        {
            printf("Error getting size of font text: %s\n", TTF_GetError());
	    cleanup_and_exit(fe, EXIT_FAILURE);
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
        if(!(text_surface=TTF_RenderUTF8_Blended(fe->fonts[font_index].font,text,fe->sdlcolours[colour])))
        {
            printf("Error rendering font text: %s\n", TTF_GetError());
	    cleanup_and_exit(fe, EXIT_FAILURE);
        }
        else
        {
            // Blit the text surface onto the screen at the right position.
            Unlock_SDL_Surface(fe);
            SDL_BlitSurface(text_surface,NULL,fe->screen,&blit_rectangle);
            Lock_SDL_Surface(fe);

            // Remove the temporary text surface from memory.
            SDL_FreeSurface(text_surface);
        };
    };
}

// Wrapper function for the games to call.
void sdl_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_draw_rect()\n");
#endif
    frontend *fe = (frontend *)handle;
    sdl_actual_draw_rect(handle, x + fe->ox, y + fe->oy, w, h, colour);
}

void sdl_actual_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_actual_draw_rect()\n");
#endif

#ifdef DEBUG_DRAWING
    printf("Rectangle: %u,%u - %u,%u Colour %u\n", x, y, x+w, y+h, colour);
#endif

    frontend *fe = (frontend *)handle;
    if( !(x < 0) && !(y < 0) && !(x > (int) screen_width) && !(y > (int) screen_height))
        boxRGBA(fe->screen, (Sint16) x, (Sint16) y, (Sint16) (x+w-1), (Sint16) (y+h-1), fe->sdlcolours[colour].r, fe->sdlcolours[colour].g, fe->sdlcolours[colour].b, (Uint8) 255);
}

// Wrapper function for the games to call.
void sdl_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_draw_line()\n");
#endif

    frontend *fe = (frontend *)handle;
    sdl_actual_draw_line(handle, x1 + fe->ox, y1 + fe->oy, x2 + fe->ox, y2 + fe->oy, colour);
}

void sdl_actual_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_actual_draw_line()\n");
#endif

#ifdef DEBUG_DRAWING
    printf("Line: %u,%u - %u,%u Colour %u\n", x1, y1, x2, y2, colour);
#endif

    frontend *fe = (frontend *)handle;

    // Draw an anti-aliased line.
    if( !(x1 < 0) && !(y1 < 0) && !(x1 > (int) screen_width) && !(y1 > (int) screen_height))
        if( !(x2 < 0) && !(y2 < 0) && !(x2 > (int) screen_width) && !(y2 > (int) screen_height))
            aalineRGBA(fe->screen, (Sint16) x1, (Sint16) y1, (Sint16) x2, (Sint16) y2, fe->sdlcolours[colour].r, fe->sdlcolours[colour].g, fe->sdlcolours[colour].b, 255);
}

void sdl_draw_poly(void *handle, int *coords, int npoints, int fillcolour, int outlinecolour)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_draw_poly()\n");
#endif

    frontend *fe = (frontend *)handle;
    Sint16 *xpoints = snewn(npoints,Sint16);
    Sint16 *ypoints = snewn(npoints,Sint16);
    int i;
#ifdef DEBUG_DRAWING
    printf("Polygon: ");
#endif

    // Convert points list from game format (x1,y1,x2,y2,...,xn,yn) to SDL_gfx format 
    // (x1,x2,x3,...,xn,y1,y2,y3,...,yn)
    for (i = 0; i < npoints; i++)
    {
        xpoints[i]=(Sint16) (coords[i*2] + fe->ox);
        ypoints[i]=(Sint16) (coords[i*2+1] + fe->oy);
#ifdef DEBUG_DRAWING
    printf("%u,%u ",xpoints[i],ypoints[i]);
#endif
    };

#ifdef DEBUG_DRAWING
    printf(" Colours: %u, %u\n", fillcolour, outlinecolour);
#endif

    // Draw a filled polygon (without outline).
    if (fillcolour >= 0)
        filledPolygonRGBA(fe->screen, xpoints, ypoints, npoints, fe->sdlcolours[fillcolour].r, fe->sdlcolours[fillcolour].g, fe->sdlcolours[fillcolour].b, 255);

    assert(outlinecolour >= 0);

    // This draws the outline of the polygon using calls to SDL_gfx anti-aliased routines.
    aapolygonRGBA(fe->screen, xpoints, ypoints, npoints,fe->sdlcolours[outlinecolour].r, fe->sdlcolours[outlinecolour].g, fe->sdlcolours[outlinecolour].b, 255);

    sfree(xpoints);
    sfree(ypoints);

/*
UNUSED
    // This draws the outline of the polygon using calls to the other existing frontend functions.
    // It seems to function identically to the above but is probably a lot slower.  It helped during 
    // initial debugging.
    for (i = 0; i < npoints; i++)
    {
	sdl_draw_line(fe, xpoints[i], ypoints[i], xpoints[(i+1)%npoints], ypoints[(i+1)%npoints],outlinecolour);
    }
*/
}

void sdl_draw_circle(void *handle, int cx, int cy, int radius, int fillcolour, int outlinecolour)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_draw_circle()\n");
#endif

#ifdef DEBUG_DRAWING
    printf("Circle: %i,%i radius %i Colours: %i %i\n", cx, cy, radius, fillcolour, outlinecolour);
#endif

    frontend *fe = (frontend *)handle;

    // Draw a filled circle with no outline
    // We don't anti-alias because it looks ugly when things try to draw circles over circles
    if(fillcolour >=0)
        if( !(cx < 0) && !(cy < 0) && !(cx > (int) screen_width) && !(cy > (int) screen_height))
            filledCircleRGBA(fe->screen, (Sint16) (cx + fe->ox), (Sint16) (cy + fe->oy), (Sint16)radius, fe->sdlcolours[fillcolour].r, fe->sdlcolours[fillcolour].g, fe->sdlcolours[fillcolour].b, 255);

    assert(outlinecolour >= 0);
  
    // Draw just an outline circle in the same place
    // We don't anti-alias because it looks ugly when things try to draw circles over circles
    if( !(cx < 0) && !(cy < 0) && !(cx > (int)screen_width) && !(cy > (int)screen_height))
        circleRGBA(fe->screen, (Sint16) (cx + fe->ox), (Sint16) (cy + fe->oy), (Sint16)radius, fe->sdlcolours[outlinecolour].r, fe->sdlcolours[outlinecolour].g, fe->sdlcolours[outlinecolour].b, 255);
}

void clear_statusbar(void *handle)
{
    frontend *fe = (frontend *)handle;
    sdl_no_clip(fe);

    // If we've drawn text on the status bar before
    if(fe->last_status_bar_w || fe->last_status_bar_h)
    {
        if((current_screen != GAMELISTMENU) && (current_screen != MUSICCREDITSMENU))
        {
            if(fe->paused)
            {
                // Blank over the last status bar messages using a black rectangle.
                sdl_actual_draw_rect(fe, 0, 0, fe->last_status_bar_w, fe->last_status_bar_h, fe->black_colour);
            }
            else
            {
                // Blank over the last status bar messages using a background-coloured
                // rectangle.
                sdl_actual_draw_rect(fe, 0, 0, fe->last_status_bar_w, fe->last_status_bar_h, fe->background_colour);
            };                
            sdl_actual_draw_update(fe, 0, 0, fe->last_status_bar_w, fe->last_status_bar_h);
        };
    };
    
    // Update variables so that we know how much screen to "blank" next time round.
    fe->last_status_bar_w=0;
    fe->last_status_bar_h=0;
 
    if(!fe->paused)
        sdl_unclip(fe);
};


// Routine to handle showing "status bar" messages to the user.
// These are used by both the midend to signal game information and the SDL interface to 
// signal quitting, restarting, entering text etc.
void sdl_status_bar(void *handle, char *text)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_status_bar()\n");
#endif

    frontend *fe = (frontend *)handle;
    SDL_Color fontcolour;
    SDL_Surface *text_surface;
    SDL_Rect blit_rectangle;
    int font_index;

#ifdef DEBUG_DRAWING
    printf("Status bar: %s Length:%u\n", text, strlen(text));
#endif

    sdl_no_clip(fe);

    // Keep the (overly-fussy) compiler happy.
    fontcolour.unused=(Uint8) 0;

    // If we haven't got anything to display, don't do anything.
    // Sometimes the midend does this to us.
    if( (text!=NULL) && strlen(text) )
    {
        // Print messages in the top-left corner of the screen.
        blit_rectangle.x = (Sint16) 0;
        blit_rectangle.y = (Sint16) 0;

        // SDL can automatically handle the width/height for us as we are 
        // blitting the "entire" text surface.
        blit_rectangle.w = (Uint16) 0;
        blit_rectangle.h = (Uint16) 0;

        if((current_screen == INGAME) || (current_screen == GAMELISTMENU))
        {
            // Print black text
            fontcolour=fe->sdlcolours[fe->black_colour];
        }
        else
        {
            // Print white text
            fontcolour=fe->sdlcolours[fe->white_colour];
        };

#ifdef DEBUG_DRAWING
        printf(" Colour: %u %u %u\n", fontcolour.r, fontcolour.g, fontcolour.b);
#endif

        font_index=find_and_cache_font(fe, FONT_VARIABLE, STATUSBAR_FONT_SIZE);

        // Render the text to a temporary text surface.
        if(!(text_surface=TTF_RenderUTF8_Blended(fe->fonts[font_index].font, text, fontcolour)))
        {
            printf("Error rendering font text: %s\n", TTF_GetError());
     	    cleanup_and_exit(fe, EXIT_FAILURE);
        }
        else
        {
            // If we've drawn text on the status bar before
            if(fe->last_status_bar_w || fe->last_status_bar_h)
            {
                if(current_screen != GAMELISTMENU)
                {
                    if(fe->paused)
                    {
                        // Blank over the last status bar messages using a black rectangle.
                        sdl_actual_draw_rect(fe, 0, 0, fe->last_status_bar_w, fe->last_status_bar_h, fe->black_colour);
                    }
                    else
                    {
                        // Blank over the last status bar messages using a background-coloured
                        // rectangle.
                        sdl_actual_draw_rect(fe, 0, 0, fe->last_status_bar_w, fe->last_status_bar_h, fe->background_colour);
                    };                
                };
            };

            // Blit the new text into place.
            Unlock_SDL_Surface(fe);
            if(SDL_BlitSurface(text_surface,NULL,fe->screen,&blit_rectangle))
            {
                printf("Error blitting text surface to screen: %s\n", TTF_GetError());
                cleanup_and_exit(fe, EXIT_FAILURE);
            };
            Lock_SDL_Surface(fe);

            // Cause a screen update over the relevant area.
            sdl_actual_draw_update(fe, 0, 0, text_surface->w, text_surface->h);
#ifdef SCALELARGESCREEN
  sdl_end_draw(fe);
#endif

            // Update variables so that we know how much screen to "blank" next time round.
            fe->last_status_bar_w=text_surface->w;
            fe->last_status_bar_h=text_surface->h;
 
            gettimeofday(&fe->last_statusbar_update,NULL);

            // Remove the temporary text surface
            SDL_FreeSurface(text_surface);
        };
    };

    if(!fe->paused)
        sdl_unclip(fe);
}

// Create a new "blitter" (temporary surface) of width w, height h.
// These are used by the midend to do things like save what's under the mouse cursor so it can 
// be redrawn later.
blitter *sdl_blitter_new(void *handle, int w, int h)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_blitter_new()\n");
#endif

#ifdef DEBUG_DRAWING
    printf("New blitter created of size %i, %i", w, h);
#endif
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
#ifdef DEBUG_FUNCTIONS
    printf("sdl_blitter_free()\n");
#endif

    if (bl->pixmap)
        SDL_FreeSurface(bl->pixmap);

#ifdef DEBUG_DRAWING
        printf("Blitter freed.\n");
#endif
    sfree(bl);
}

// Save the contents of the screen starting at x, y into a "blitter" with size bl->w, bl->h
void sdl_blitter_save(void *handle, blitter *bl, int x, int y)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_blitter_save()\n");
#endif

    frontend *fe = (frontend *)handle;
    SDL_Rect srcrect, destrect;

#ifdef DEBUG_DRAWING
    printf("Saving screen portion at %i, %i (%i, %i)\n", x, y, bl->w, bl->h);
#endif

    // Create the actual blitter surface now that we know all the details.
    if (!bl->pixmap)
        bl->pixmap=SDL_CreateRGBSurface(SDL_SURFACE_FLAGS & (!SDL_DOUBLEBUF), bl->w, bl->h, SCREEN_DEPTH, 0, 0, 0, 0);

    bl->x = x;
    bl->y = y;

    // Blit the relevant area of the screen into the blitter.
    srcrect.x = x + fe->ox;
    srcrect.y = y + fe->oy;
    srcrect.w = bl->w;
    srcrect.h = bl->h;
    destrect.x = 0;
    destrect.y = 0;
    destrect.w = bl->w;
    destrect.h = bl->h;
    Unlock_SDL_Surface(fe);
    SDL_BlitSurface(fe->screen, &srcrect, bl->pixmap, &destrect);
    Lock_SDL_Surface(fe);
}

// Load the contents of the screen starting at X, Y from a "blitter" with size bl->w, bl->h
void sdl_blitter_load(void *handle, blitter *bl, int x, int y)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_blitter_load()\n");
#endif

    frontend *fe = (frontend *)handle;
    SDL_Rect srcrect, destrect;

    assert(bl->pixmap);

    // If we've been asked to just "use the blitter size", do that.
    if ((x == BLITTER_FROMSAVED) && (y == BLITTER_FROMSAVED))
    {
        x = bl->x;
        y = bl->y;
    };

#ifdef DEBUG_DRAWING
    printf("Restoring screen portion at %u, %u, (%u, %u)\n", x+fe->ox, y+fe->oy, bl->w, bl->h);
#endif

    // Blit the contents of the blitter into the relevant area of the screen.
    srcrect.x = 0;
    srcrect.y = 0;
    srcrect.w = bl->w;
    srcrect.h = bl->h;
    destrect.x = x + fe->ox;
    destrect.y = y + fe->oy;
    destrect.w = bl->w;
    destrect.h = bl->h;
    Unlock_SDL_Surface(fe);
    SDL_BlitSurface(bl->pixmap, &srcrect, fe->screen, &destrect);
    Lock_SDL_Surface(fe);
}

// Informs the front end that a rectangular portion of the puzzle window 
// has been drawn on and needs to be updated.
void sdl_draw_update(void *handle, int x, int y, int w, int h)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_draw_update()\n");
#endif

    frontend *fe = (frontend *)handle;
    sdl_actual_draw_update(fe, x + fe->ox, y + fe->oy, w, h);
}

void sdl_actual_draw_update(void *handle, int x, int y, int w, int h)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_actual_draw_update()\n");
#endif

    frontend *fe = (frontend *)handle;

    if(x<0)
        x=0;
    if(y<0)
        y=0;
    if(x > (int)screen_width)
        x=screen_width;
    if(y > (int)screen_height)
        y=screen_height;
    if((x + w) > screen_width)
        w = screen_width - x;
    if((y + h) > screen_height)
        h = screen_height - y;

#ifdef DEBUG_DRAWING
    printf("Partial screen update: %i, %i, %i, %i.\n", x+fe->ox, y+fe->oy, w, h);
#endif

    // Request a partial screen update of the relevant rectangle.
    Unlock_SDL_Surface(fe);
    SDL_UpdateRect(fe->screen, (Sint32) x, (Sint32) y, (Sint32) w, (Sint32) h);
    Lock_SDL_Surface(fe);
}

// This function is called at the end of drawing (i.e. a frame).
void sdl_end_draw(void *handle)
{
#ifdef DEBUG_FUNCTIONS
    printf("sdl_end_draw()\n");
#endif

    frontend *fe = (frontend *)handle;

#ifdef DEBUG_DRAWING
    printf("End of frame.  Updating screen.\n");
#endif

    // In SDL software surfaces, SDL_Flip() is just SDL_UpdateRect(everything).
    // In double-buffered SDL hardware surfaces, this does the flip between the two screen 
    // buffers.  We don't do double-buffering properly yet anyway, so we make it always
    // do the UpdateRect version.

#ifdef SCALELARGESCREEN
    if(screen_width == SCREEN_WIDTH_LARGE)
    {
        SDL_Rect blit_rectangle;
        blit_rectangle.w=0;
        blit_rectangle.h=0;
        blit_rectangle.x=0;
        blit_rectangle.y=0;
        SDL_Surface *zoomed_surface=zoomSurface(screen, 0.5, 0.5, SMOOTHING_ON);
        SDL_BlitSurface(zoomed_surface, NULL, real_screen, &blit_rectangle);
        SDL_FreeSurface(zoomed_surface);
        SDL_Flip(real_screen);
    };
#endif

    Unlock_SDL_Surface(fe);
    if(SDL_SURFACE_FLAGS & SDL_DOUBLEBUF)
        SDL_UpdateRect(fe->screen, 0, 0, fe->screen->w, fe->screen->h);
    else
        SDL_Flip(fe->screen);
    Lock_SDL_Surface(fe);   
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
#ifdef DEBUG_FUNCTIONS
    printf("configure_area()\n");
#endif

    frontend *fe = (frontend *)data;

    fe->w=x;
    fe->h=y;

    // Get the new size of the puzzle.
    midend_size(fe->me, &x, &y, FALSE);
    fe->pw = x;
    fe->ph = y;

    // This calculates a new offset for the puzzle screen.
    // This would be used for "centering" the puzzle on the screen.
    fe->ox = (fe->w - fe->pw) / 2;
    fe->oy = (fe->h - fe->ph) / 2;    

#ifdef DEBUG_DRAWING
    printf("New puzzle size is %u x %u\n", fe->pw, fe->ph);
    printf("New puzzle offset is %u x %u\n", fe->ox, fe->oy);
#endif
}

// Called once a second to perform menial tasks.
Uint32 sdl_second_timer_func(Uint32 interval, void *data)
{
    frontend *fe = (frontend *)data;
    SDL_Event event;

    gettimeofday(&fe->last_time, NULL);

    // Generate a user event and push it to the event queue so that the event does
    // the work and not the timer (you can't run SDL code inside an SDL timer)

    event.type = SDL_USEREVENT;
    event.user.code = RUN_SECOND_TIMER_LOOP;
    event.user.data1 = 0;
    event.user.data2 = 0;
            
    Push_SDL_Event(&event);

    // Specify to run this event again in interval milliseconds.
    return interval;
}

// Called regularly whenever the midend enables a timer.
Uint32 sdl_timer_func(Uint32 interval, void *data)
{
    frontend *fe = (frontend *)data;
    if(fe->paused)
    {
        // If we are paused, don't do anything except update the timer (so that timed 
        // games don't tick on when paused), but just let the timer re-fire again
        // after another X ms.
        gettimeofday(&fe->last_time, NULL);
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
            event.user.code = RUN_GAME_TIMER_LOOP;
            event.user.data1 = 0;
            event.user.data2 = 0;
            
            Push_SDL_Event(&event);

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
     
    Push_SDL_Event(&event);

    // Specify to run this event again in interval milliseconds.
    return interval;
}

// Stop a timer from firing again.
void deactivate_timer(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("deactivate_timer()\n");
#endif
    if(fe->timer_active)
    {
        // Set a flag so that any remaining timer fires end up removing the timer too.
        fe->timer_active = FALSE;

	// Remove the timer.
        SDL_RemoveTimer(fe->sdl_timer_id);
#ifdef DEBUG_TIMER
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
#ifdef DEBUG_FUNCTIONS
    printf("activate_timer()\n");
#endif

    // If a timer isn't already running
    if(!fe->timer_active)
    {
#ifdef DEBUG_TIMER
        printf("Timer activated (wasn't already running).\n");
#endif

        // Set the game timer to fire every SDL_GAME_TIMER_INTERVAL ms.
        // This is an arbitrary number, chosen to make the animation smooth without
        // generating too many events for the GP2X to process properly.

        fe->sdl_timer_id=SDL_AddTimer(SDL_GAME_TIMER_INTERVAL, sdl_timer_func, fe);
        if(!fe->sdl_timer_id)
        {
            printf("Error - Timer Creation failed.\n");
            cleanup_and_exit(fe, EXIT_FAILURE);
        };

        // Update the last time the event fired so that the midend knows how long it's been.
        gettimeofday(&fe->last_time, NULL);

        fe->timer_active = TRUE;
#ifdef DEBUG_TIMER
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
#ifdef DEBUG_FUNCTIONS
    printf("get_size()\n");
#endif

    int x, y;

    // Set both to INT_MAX to disable scaling, or window size to enable scaling.
    // We set these to the GP2X screen size, larger multiples automatically 
    // scales it to fit in screen_height x screen_width.

    x = screen_width;
    y = screen_height;

    midend_size(fe->me, &x, &y, FALSE);

#ifdef DEBUG_DRAWING
    printf("Puzzle wants size %u, %u.\n", x, y);
#endif

    *px = x;
    *py = y;
}

int show_file_on_screen(frontend *fe, char *filename)
{
#ifdef DEBUG_FUNCTIONS
    printf("show_file_on_screen()\n");
#endif

    const int buffer_length=60;
    FILE *textfile;
    char str_buf[buffer_length];
    int line=1;
    int i;
	
    // Open the text file.
    textfile = fopen(filename, "r");
    if(textfile == NULL)
    {
        printf("Cannot open text file: %s\n",filename);
        return(FALSE);
    }
    else
    {
#ifdef DEBUG_FILE_ACCESS
        printf("Opened text file: %s\n",filename);
#endif

        // Blank the entire screen
        sdl_actual_draw_rect(fe, 0, 0, screen_width, screen_height, fe->black_colour);

        memset(str_buf, 0, buffer_length * sizeof(char));

        // Read one line at a time from the file into a buffer
        while(fgets(str_buf, buffer_length, textfile) != NULL)
        {
            // Strip all line returns (char 10) from the buffer because they show up as
            // squares.  Probably have to do this for carriage returns (char 13) too if
            // someone edits the text files in DOS/Windows?
            for(i=0; i<buffer_length; i++)
            {
                if(str_buf[i]==10)
                    str_buf[i]=0;
                if(str_buf[i]==13)
                    str_buf[i]=0;
            };
		
            // Draw the buffer to the screen.
            sdl_actual_draw_text(fe, 0, line * (HELP_FONT_SIZE+2), FONT_VARIABLE, HELP_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, str_buf);
            line++;
        };

        // Update screen.
        sdl_end_draw(fe);

        // Cleanup
        fclose(textfile);
        return(TRUE);
    };
};

// Loads and shows the text file of the game's name for instructions.
// This should only ever be called while the game is already paused!
int read_game_helpfile(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("read_game_helpfile()\n");
#endif

    char* filename;
    int result;

    filename=snewn(MAX_GAMENAME_SIZE + sizeof(MENU_HELPFILES) + 1,char);
    sprintf(filename, MENU_HELPFILES, fe->sanitised_game_name);
    result=show_file_on_screen(fe, filename);
    sfree(filename);
    return(result);
}

// Initial setting up of the games.
static frontend *new_window(char *arg, int argtype, char **error)
{
#ifdef DEBUG_FUNCTIONS
    printf("new_window()\n");
#endif

    frontend *fe;

#ifdef DEBUG_DRAWING
    printf("New window called.\n");
#endif

    // Create a new frontend
    fe = snew(frontend);
    memset(fe, 0, sizeof(struct frontend));

    // Probably redundant now because of the memset.
    fe->ini_dict=NULL;
    fe->cfg=NULL;
    fe->config_window_options=NULL;

    fe->black_colour=-1;
    fe->white_colour=-1;

    fe->timer_active = FALSE;

    return fe;
}

void draw_menu(frontend *fe, uint menu_index)
{
#ifdef DEBUG_FUNCTIONS
    printf("draw_menu()\n");
#endif

    int j=0;
    int current_y;
    int i;
    char *preset_name;
    SDL_Rect font_clip_rectangle;
    SDL_Surface *credits_window;

    // Set the clipping region to the whole physical screen
    sdl_no_clip(fe);

    // Black out the whole screen using the drawing routines.
    sdl_actual_draw_rect(fe, 0, 0, screen_width, screen_height, fe->black_colour);

    // Update the current screen now so that we know what screen we are trying to become in the following code.
    current_screen=menu_index;

    // Set the (soon-to-be-deprecated) pause flag so that it doesn't get out of sync with the screen we are on.

    show_keyboard_icon(fe);
    if(menu_index == INGAME)
    {
        fe->paused=FALSE;
        if(global_config->control_system == CURSOR_KEYS_EMULATION)
        {
            SDL_ShowCursor(SDL_DISABLE);
        };
    }
    else
    {
        fe->paused=TRUE;
        SDL_ShowCursor(SDL_ENABLE);
    };

    helpfile_showing=FALSE;

    switch(menu_index)
    {
        case GAMELISTMENU:
            menu_loop(fe);
            break;

        case INGAME:
            // Cover the whole screen in background colour using the drawing routines.
            sdl_actual_draw_rect(fe, 0, 0, screen_width, screen_height, fe->background_colour);

            // Make the game redraw itself.
            midend_force_redraw(fe->me);

            // Restore clipping rectangle to the puzzle screen
            sdl_unclip(fe);
            break;

        case GAMEMENU:
            sdl_status_bar(fe,"Paused.");

            // Print STPPC2x and the game name at the top of the screen.
            sdl_actual_draw_text(fe, screen_width / 2, 2*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->white_colour, stppc2x_ver);
            sdl_actual_draw_text(fe, screen_width / 2, 3*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->white_colour, (char *) gamelist[current_game_index]->name);

            // Draw a seperating line
            sdl_actual_draw_line(fe, 0, 3*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2, screen_width, 3*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2,fe->white_colour);

            // Draw the menu items.
            sdl_actual_draw_text(fe, 10, 5*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Return to Game");
            sdl_actual_draw_text(fe, 10, 6*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "New Game");
            sdl_actual_draw_text(fe, 10, 7*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Restart Current Game");
            sdl_actual_draw_text(fe, 10, 8*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Configure Game");
            sdl_actual_draw_text(fe, 10, 9*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Save / Load Game");
            sdl_actual_draw_text(fe, 10, 10*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Global Settings");
            sdl_actual_draw_text(fe, 10, 11*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Help");
            sdl_actual_draw_text(fe, 10, 12*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Credits");
            sdl_actual_draw_text(fe, 10, 13*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Quit to main menu");

            // Draw a seperating line
            sdl_actual_draw_line(fe, 0, 14*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2, screen_width, 14*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2,fe->white_colour);

            // Draw the SVN version number at the bottom of the screen.
            char *number_of_games, *version;
            version=snewn(45,char);
            sprintf(version, "Based on SVN %s, compiled on", ver);

            number_of_games=snewn(sizeof(__DATE__) + sizeof(__TIME__) + 28,char);
            sprintf(number_of_games, "%s, %s and containing %u games", __DATE__, __TIME__, gamecount);
            sdl_actual_draw_text(fe, screen_width / 2, 16*(MENU_FONT_SIZE+2), FONT_VARIABLE, HELP_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->white_colour, version);
            sdl_actual_draw_text(fe, screen_width / 2, 17*(MENU_FONT_SIZE+2), FONT_VARIABLE, HELP_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->white_colour, number_of_games);
            sfree(version);
            sfree(number_of_games);
            break;

        case SAVEMENU:
            // Draw the menu items.
            sdl_actual_draw_text(fe, screen_width / 2, 2*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->white_colour, "Save/Load");
            sdl_actual_draw_text(fe, 10, 3*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Return To Main Menu");

            // Draw a seperating line
            sdl_actual_draw_line(fe, 0, 3*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2, screen_width, 3*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2,fe->white_colour);

            savegame_to_delete=-1;

            // Print 10 save / load slots on the screen.
            for(j=0;j<10;j++)
            {
                char *saveslot_string;
                saveslot_string=snewn(11, char);
                memset(saveslot_string, 0, 11 * sizeof(char));
                sprintf(saveslot_string, "Saveslot %u", j);
                sdl_actual_draw_text(fe, 10, (5+j)*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, saveslot_string);
                sdl_actual_draw_text(fe, (screen_width==SCREEN_WIDTH_LARGE)?400:200, (5+j)*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Save");
                if(savefile_exists(fe->sanitised_game_name, j))
                {
                    sdl_actual_draw_text(fe, (screen_width==SCREEN_WIDTH_LARGE)?500:250, (5+j)*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Load");
                    sdl_actual_draw_text(fe, (screen_width==SCREEN_WIDTH_LARGE)?600:300, (5+j)*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, UNICODE_SKULL_CROSSBONES);
                };
                sfree(saveslot_string);
            };

            sdl_actual_draw_text(fe, 10, 15*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Autosave");
            if(!global_config->autosave_on_exit)
            {
               sdl_actual_draw_line(fe, 10, 14*(MENU_FONT_SIZE+2), (screen_width==SCREEN_WIDTH_LARGE)?140:70, 15*(MENU_FONT_SIZE+2),fe->white_colour);
               sdl_actual_draw_line(fe, 10, 15*(MENU_FONT_SIZE+2), (screen_width==SCREEN_WIDTH_LARGE)?140:70, 14*(MENU_FONT_SIZE+2),fe->white_colour);
            };

            if(autosave_file_exists(fe->sanitised_game_name))
            {
                sdl_actual_draw_text(fe, (screen_width==SCREEN_WIDTH_LARGE)?500:250, 15*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Load");
                sdl_actual_draw_text(fe, (screen_width==SCREEN_WIDTH_LARGE)?600:300, 15*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, UNICODE_SKULL_CROSSBONES);
            };
            break;

        case CONFIGMENU:
            // Print the game's configuration window title
            sdl_actual_draw_text(fe, screen_width / 2, 2*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->white_colour, fe->configure_window_title);
 
            // Print the menu items.
            sdl_actual_draw_text(fe, 10, 3*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Return To Main Menu");
            sdl_actual_draw_text(fe, 10, 4*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Save My Preset");
            sdl_actual_draw_text(fe, 10, 5*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "View Presets");

            // Draw a seperating line
            sdl_actual_draw_line(fe, 0, 5*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2, screen_width, 5*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2,fe->white_colour);

            // This next bit gets complicated but it basically goes through the game's
            // configuration structures and prints suitable "toggles", "sliders" etc.
            // on the screen for each of them (size, complexity etc. depending on the
            // game)

            fe->ncfg=0;

            if(fe->config_window_options)
               sfree(fe->config_window_options);

            fe->config_window_options=snew(struct config_window_option);

            current_y=7*(MENU_FONT_SIZE+2);
	
            // This part collects all the configuration options for the current game
            // and dynamically adds the options/current values to the "menu".
            while(fe->cfg[j].type!=C_END)
            {
                char *token_pointer;
                char delimiter[2];
                char current_string[255];
                char number_of_strings=0;

                // Dynamically increase the size of the config options
                fe->ncfg++;
                fe->config_window_options=sresize(fe->config_window_options, fe->ncfg, struct config_window_option);
                fe->config_window_options[fe->ncfg-1].config_index=j;
                fe->config_window_options[fe->ncfg-1].config_option_index=0;

                // Setup a clipping rectangle to stop text overlapping.
                font_clip_rectangle.x=0;
                font_clip_rectangle.w=screen_width - 100;
                font_clip_rectangle.y=current_y-(MENU_FONT_SIZE+2);
                font_clip_rectangle.h=MENU_FONT_SIZE+2;

                sdl_actual_clip(fe, font_clip_rectangle.x, font_clip_rectangle.y, font_clip_rectangle.w, font_clip_rectangle.h);

                // Draw the configuration option name.
                sdl_actual_draw_text(fe, 5, current_y, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, fe->cfg[j].name);

                // Setup a clipping rectangle to stop text overlapping.
                font_clip_rectangle.x=screen_width * 7 / 10;
                font_clip_rectangle.w=screen_width * 3 / 10;

                font_clip_rectangle.y=current_y-(MENU_FONT_SIZE+2);
                font_clip_rectangle.h=MENU_FONT_SIZE+2;

                sdl_actual_clip(fe, font_clip_rectangle.x, font_clip_rectangle.y, font_clip_rectangle.w, font_clip_rectangle.h);

                // Depending on the type of the configuration option (string, boolean, dropdown options)
                switch(fe->cfg[j].type)
                {
                    case C_STRING:
#ifdef OPTION_KLUDGES
                        // Kludge - if the option name is "Barrier probablity", it's probably 
                        // expecting decimal numbers instead of integers, so add the relevant 
                        // ".0" to the end of the value.
                        // This is for "net" and "netslide".
                        if(!strcmp(fe->cfg[j].name,"Barrier probability"))
                            sprintf(fe->cfg[j].sval, "%.2f", atof(fe->cfg[j].sval));

                        // Kludge - if the option name is "Expansion factor", it's probably 
                        // expecting decimal numbers instead of integers, so add the relevant 
                        // ".0" to the end of the value.
                        // This is for "rect", not Bridges, which has a C_CHOICES named the same.
                        if(!strcmp(fe->cfg[j].name,"Expansion factor"))
                            sprintf(fe->cfg[j].sval, "%.2f", atof(fe->cfg[j].sval));
#endif
                        // Just draw the current setting straight to the menu.
                        sdl_actual_draw_text(fe, screen_width * 7 / 10, current_y, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, fe->cfg[j].sval);

                        // Store y position of this menu item
                        fe->config_window_options[fe->ncfg-1].y=current_y;
#ifdef DEBUG_MISC
                        printf("Config option: %u - %s, default value = %s\n", j, fe->cfg[j].name, fe->cfg[j].sval);
#endif
                        break;

                    case C_BOOLEAN:
                        // Print true (unicode tick) or false (unicode cross) depending on 
                        // the current value.
                        sdl_actual_draw_text(fe, screen_width * 7 / 10, current_y, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, fe->cfg[j].ival?UNICODE_TICK_CHAR:UNICODE_CROSS_CHAR);

                        // Store y position of this menu item
                        fe->config_window_options[fe->ncfg-1].y=current_y;
#ifdef DEBUG_MISC
                        printf("Config option: %u - %s, default value = %u\n", j, fe->cfg[j].name, fe->cfg[j].ival);
#endif
                        break;

                    case C_CHOICES:
                        // The options for the drop down box are presented as a delimited 
                        // list of strings, with the first character being the delimiter.

                        // Extract the delimiter from the string.
                        delimiter[0]=fe->cfg[j].sval[0];
                        delimiter[1]=0;
                        strncpy(current_string, &fe->cfg[j].sval[0], 250);

                        token_pointer=strtok(current_string, delimiter);
                        while(token_pointer != NULL)
                        {
                            // If it isn't an empty string...
                            if(strlen(token_pointer))
                            {
                                // If it's the currently selected option string from the choices
                                // available
                                if(fe->cfg[j].ival == number_of_strings)
                                    // Draw it on screen.
                                    sdl_actual_draw_text(fe, screen_width * 7 / 10, current_y, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, token_pointer);

                                // Add it to the array of all config sub-options for this 
                                // config option.
                                number_of_strings++;
                                fe->ncfg++;
                                fe->config_window_options=sresize(fe->config_window_options,fe->ncfg, struct config_window_option);
                                fe->config_window_options[fe->ncfg-1].y=current_y;
                                fe->config_window_options[fe->ncfg-1].config_option_index=number_of_strings;
                                fe->config_window_options[fe->ncfg-1].config_index=j;
#ifdef DEBUG_MISC
                                printf("Config option: %u.%u - %s\n", j, number_of_strings, current_string);
#endif
                            };
                            token_pointer=strtok(NULL, delimiter);
                        };
#ifdef DEBUG_MISC
                        printf("Config option: %u - %s, default value = %u\n", j, fe->cfg[j].name, fe->cfg[j].ival);
#endif
                    break;
                }; // end switch

                // Increment screen y position of this menu item
                current_y+=MENU_FONT_SIZE + 2;
                j++;
            }; // end while

            // Set the clipping region to the whole physical screen
            sdl_no_clip(fe);
            break;

        case PRESETSMENU:
            // Print the game's configuration window title
            sdl_actual_draw_text(fe, screen_width / 2, 2*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->white_colour, fe->configure_window_title);

            // Draw the rest of the menu items.
            sdl_actual_draw_text(fe, 10, 3*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Return to Configuration Menu");

            // Draw an item to load the user's default config.
            sdl_actual_draw_text(fe, 10, 4*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "My Saved Preset");

            // Draw a seperating line
            sdl_actual_draw_line(fe, 0, 4*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2, screen_width, 4*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2,fe->white_colour);
            sdl_actual_draw_text(fe, screen_width / 2, 6*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->white_colour, "Built-in Presets");
            if(fe->first_preset_showing > 0)
                sdl_actual_draw_text(fe, screen_width / 2, 7*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->white_colour, UNICODE_UP_ARROW);
            if(midend_num_presets(fe->me) > 9)
                sdl_actual_draw_text(fe, screen_width / 2, 17*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->white_colour, UNICODE_DOWN_ARROW);

#ifdef DEBUG_MISC
            printf("Loading presets...\n");
#endif
            current_y = 8*(MENU_FONT_SIZE+2);

            // Loop over all the presets...
            for(i=fe->first_preset_showing;i<midend_num_presets(fe->me);i++)
            {
                if(current_y < (17 * (MENU_FONT_SIZE +2)))
                {
                    game_params *preset_params;

                    // Get that preset's configuaration (and, incidentally, the name of the preset).
                    midend_fetch_preset(fe->me, i, &preset_name, &preset_params);
#ifdef DEBUG_MISC
                    printf("Preset %i: %s\n", i, preset_name);
#endif
                    // Print the name of the preset to the screen
                    sdl_actual_draw_text(fe, 5, current_y, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, preset_name);
                    if(midend_which_preset(fe->me) == i)
                    {
#ifdef DEBUG_MISC
                        printf("Preset %i is currently used.\n", midend_which_preset(fe->me));
#endif
                        // Put a tick beside the preset that it matches.
                        sdl_actual_draw_text(fe, screen_width * 9 / 10, current_y, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, UNICODE_TICK_CHAR);
                    };
                    current_y+=MENU_FONT_SIZE + 2;
                };
            };
            break;

        case HELPMENU:
            // Draw the menu items.
            sdl_actual_draw_text(fe, screen_width / 2, 2*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->white_colour, "Help");
            sdl_actual_draw_text(fe, 10, 3*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Return to Main Menu");

            // Draw a seperating line.
            sdl_actual_draw_line(fe, 0, 3*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2, screen_width, 3*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2,fe->white_colour);

            sdl_actual_draw_text(fe, 10, 5*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Game Help");
            sdl_actual_draw_text(fe, 10, 6*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "In-game Keys Help");
            sdl_actual_draw_text(fe, 10, 7*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "In-menu Keys Help");
            break;

        case CREDITSMENU:
            if(!show_file_on_screen(fe, MENU_CREDITS_FILENAME))
                sdl_status_bar(fe,"Could not open credits file.");
            break;

        case MUSICCREDITSMENU:
            credits_window=IMG_Load(MENU_MUSIC_CREDITS_IMAGE);
            if(credits_window!=NULL)
            {
                SDL_Rect blit_rectangle;
                blit_rectangle.w=0;
                blit_rectangle.h=0;
                if(screen_width == SCREEN_WIDTH_LARGE)
                {
                    SDL_Surface *zoomed_surface=zoomSurface(credits_window, 2, 2, SMOOTHING_ON);
                    blit_rectangle.x=(screen_width - zoomed_surface->w) / 2;
                    blit_rectangle.y=(screen_height - zoomed_surface->h) / 2;
                    SDL_BlitSurface(zoomed_surface, NULL, screen, &blit_rectangle);
                    SDL_FreeSurface(zoomed_surface);
                }
                else
                {
                    blit_rectangle.x=(screen_width - credits_window->w) / 2;
                    blit_rectangle.y=(screen_height - credits_window->h) / 2;
                    SDL_BlitSurface(credits_window, NULL, screen, &blit_rectangle);
                };
                SDL_FreeSurface(credits_window);
                sdl_actual_draw_text(fe, 10, 2*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->black_colour, "Music courtesy of:");
            }
            else
            {
                printf("Could not open music credits image: %s\n", MENU_MUSIC_CREDITS_IMAGE);
                sdl_actual_draw_text(fe, 10, 20, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Music courtesy of:");
                sdl_actual_draw_text(fe, 10, 100, FONT_VARIABLE, MENU_FONT_SIZE * 3, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Mr Lou");
            };
            sdl_end_draw(fe);
            break;

        case SETTINGSMENU:
            // Draw the menu items.
            sdl_actual_draw_text(fe, screen_width / 2, 2*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->white_colour, "Global Settings");
            sdl_actual_draw_text(fe, 10, 3*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Return to Config Menu");
            sdl_actual_draw_text(fe, 10, 4*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Save Global Config");

            // Draw a seperating line.
            sdl_actual_draw_line(fe, 0, 4*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2, screen_width, 4*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2,fe->white_colour);

            sdl_actual_draw_text(fe, 10, 6*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Music:");
            sdl_actual_draw_text(fe, 20, 7*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Play Music");
            sdl_actual_draw_text(fe, 20, 8*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Select Tracks...");
            sdl_actual_draw_text(fe, 10, 9*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Screenshots");
            sdl_actual_draw_text(fe, 20, 10*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Include cursor");
            sdl_actual_draw_text(fe, 20, 11*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Include status text");
            sdl_actual_draw_text(fe, 10, 12*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Auto-save Game on Exit");
            sdl_actual_draw_text(fe, 10, 13*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Start with Autosave");
            sdl_actual_draw_text(fe, 10, 14*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Control System");
            sdl_actual_draw_text(fe, 20, 15*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Mouse Emulation");
            sdl_actual_draw_text(fe, 20, 16*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Cursor Keys Emulation");

            sdl_actual_draw_text(fe, screen_width * 7 / 10, 7*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, global_config->play_music?UNICODE_TICK_CHAR:UNICODE_CROSS_CHAR);
            sdl_actual_draw_text(fe, screen_width * 7 / 10, 9*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, global_config->screenshots_enabled?UNICODE_TICK_CHAR:UNICODE_CROSS_CHAR);
            sdl_actual_draw_text(fe, screen_width * 7 / 10, 10*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, global_config->screenshots_include_cursor?UNICODE_TICK_CHAR:UNICODE_CROSS_CHAR);
            sdl_actual_draw_text(fe, screen_width * 7 / 10, 11*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, global_config->screenshots_include_statusbar?UNICODE_TICK_CHAR:UNICODE_CROSS_CHAR);
            sdl_actual_draw_text(fe, screen_width * 7 / 10, 12*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, global_config->autosave_on_exit?UNICODE_TICK_CHAR:UNICODE_CROSS_CHAR);
            sdl_actual_draw_text(fe, screen_width * 7 / 10, 13*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, global_config->always_load_autosave?UNICODE_TICK_CHAR:UNICODE_CROSS_CHAR);
            sdl_actual_draw_text(fe, screen_width * 7 / 10, 15*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, (global_config->control_system == MOUSE_EMULATION)?UNICODE_TICK_CHAR:UNICODE_CROSS_CHAR);
            sdl_actual_draw_text(fe, screen_width * 7 / 10, 16*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, (global_config->control_system == CURSOR_KEYS_EMULATION)?UNICODE_TICK_CHAR:UNICODE_CROSS_CHAR);

            break;

        case MUSICMENU:
            // Draw the menu items.
            sdl_actual_draw_text(fe, screen_width / 2, 2*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->white_colour, "Music Tracks");
            sdl_actual_draw_text(fe, 10, 3*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Return to Global Settings Menu");
            sdl_actual_draw_text(fe, 10, 4*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Save Music Tracks Config");

            // Draw a seperating line.
            sdl_actual_draw_line(fe, 0, 4*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2, screen_width, 4*(MENU_FONT_SIZE+2) + (MENU_FONT_SIZE+2)/2,fe->white_colour);

            sdl_actual_draw_text(fe, 10, 6*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "Music Volume");

            char *music_volume_as_string=snewn(4, char);
            sprintf(music_volume_as_string, "%u", global_config->music_volume);
            sdl_actual_draw_text(fe, screen_width * 9 / 10, 6*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, music_volume_as_string);

            for(i=0;i<10;i++)
            {
                char *track_name=get_music_filename(i+1);
                if(track_name != NULL)
                {
                    sdl_actual_draw_text(fe, 10, (7+i)*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, track_name);
                    sfree(track_name);
                    if((i+1) == current_music_track)
                        sdl_actual_draw_text(fe, 0, (7+i)*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, UNICODE_MUSICAL_NOTE);
                    sdl_actual_draw_text(fe, screen_width * 9 / 10, (7+i)*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, global_config->tracks_to_play[i]?UNICODE_TICK_CHAR:UNICODE_CROSS_CHAR);
                };
            };

            break;

        default:
            printf("Invalid menu code passed to draw_menu.\n");
            cleanup_and_exit(fe, EXIT_FAILURE);
    };

    // Update screen
    sdl_end_draw(fe);
};


// Called on whenever the game is paused or unpaused.  
void game_pause(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("game_pause()\n");
#endif

    // If the game needs to be paused.
    if(fe->paused)
    {
#ifdef DEBUG_MISC
        printf("Game paused.\n");
#endif
        draw_menu(fe, GAMEMENU);
    }
    else
    {
#ifdef DEBUG_MISC
        printf("Game unpaused.\n");
#endif
        draw_menu(fe, INGAME);
        sdl_status_bar(fe,"Resumed.");
    };
};

void game_quit()
{
#ifdef DEBUG_FUNCTIONS
    printf("game_quit()\n");
#endif

    // Turn the cursor off to stop it lingering after program exit (e.g. in the GP2X menu)
    SDL_ShowCursor(SDL_DISABLE);

    // Flush all files to disk (just in case).
#ifndef WIN32    
    sync();
#endif

    // We *could* also run the menu again, but it's preferable for this program to just
    // terminate.  This way, GMenu2x etc. users don't get thrown back into the GP2X menu
    // and GP2X menu users can use a basic wrapper script to run the game and then re-execute 
    // the menu.

#ifdef TARGET_GP2X
    if(respawn_gp2x_menu)
    {
        printf("Respawning GP2X menu...\n");
        printf("(Pass any command line argument to stop me doing that.)\n");

        chdir("/usr/gp2x");
        execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);
    };
#endif
};

// Adjust the virtual mouse cursor to be within the confines it needs to be
uint contain_mouse(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("contain_mouse()\n");
#endif

    signed int old_mouse_x=mouse_x;
    signed int old_mouse_y=mouse_y;

    // If we are in a menu...
    if(current_screen != INGAME)
    {
        // Confine the mouse to the whole screen
        if(mouse_x < 0)
            mouse_x = 0;
        if(mouse_y < 0)
            mouse_y = 0;
#ifdef SCALELARGESCREEN
        if(screen_width == SCREEN_WIDTH_LARGE)
        {
            if(mouse_x*2 > (int) (screen_width - 1))
                mouse_x = (screen_width - 1) / 2;
            if(mouse_y*2 > (int) (screen_height - 1))
                mouse_y = (screen_height - 1) / 2;
        }
        else
        {
#endif
            if(mouse_x > (int) (screen_width - 1))
                mouse_x = screen_width - 1;
            if(mouse_y > (int) (screen_height - 1))
                mouse_y = screen_height - 1;
#ifdef SCALELARGESCREEN
        };
#endif
    }
    else
    {
        // Confine the mouse to the puzzle area.
#ifdef SCALELARGESCREEN
        if(screen_width == SCREEN_WIDTH_LARGE)
        {
            if(mouse_x*2 < (signed int) (fe->ox))
                mouse_x = fe->ox / 2;
            if(mouse_y < (fe->oy))
                mouse_y = fe->oy / 2;

            if(mouse_x *2 > (fe->pw + fe->ox - 1))
                mouse_x = (fe->pw + fe->ox - 1) / 2;
            if(mouse_y *2 > (fe->ph + fe->oy - 1))
                mouse_y = (fe->ph + fe->oy - 1) / 2;
        }
        else
        {
#endif
            if(mouse_x < (signed int) (fe->ox))
                mouse_x = fe->ox;
            if(mouse_y < (fe->oy))
                mouse_y = fe->oy;

            if(mouse_x > (fe->pw + fe->ox - 1))
                mouse_x = fe->pw + fe->ox - 1;
            if(mouse_y > (fe->ph + fe->oy - 1))
                mouse_y = fe->ph + fe->oy - 1;
#ifdef SCALELARGESCREEN
        };
#endif
    };

    // If the mouse was moved by those checks...
    if((mouse_x != old_mouse_x) || (mouse_y != old_mouse_y))
    {
#ifdef DEBUG_MOUSE
        printf("Mouse had to be contained inside screen (was %u, %u - changed to %u, %u)\n", old_mouse_x, old_mouse_y, mouse_x, mouse_y);
#endif
        return(TRUE);
    }
    else
    {
        return(FALSE);
    };
}

// Generates a fake SDL event for a mouse or joystick button
// Used by the game itself for mouse emulation, and by the
// PC version to simulate the GP2X controls.
void emulate_event(Uint8 type, Uint16 x, Uint16 y, Uint8 button)
{
    SDL_Event emulated_event;

    if(type==SDL_MOUSEBUTTONUP)
    {
#ifdef DEBUG_MOUSE
        printf("Mouse button release emulated: %i, %i - %i\n", x, y, button);
#endif
        // Plug most of the information straight into the event.
        emulated_event.button.button = button;
        emulated_event.button.x = x;
        emulated_event.button.y = y;

        // Generate the remaining three pieces of information depending on 
        // if we want "Pressed" or "Released".
        emulated_event.button.type=SDL_MOUSEBUTTONUP;
        emulated_event.button.state=SDL_RELEASED;
        emulated_event.type=SDL_MOUSEBUTTONUP;
    }
    else if(type==SDL_MOUSEBUTTONDOWN)
    {
#ifdef DEBUG_MOUSE
        printf("Mouse button press emulated: %i, %i - %i\n", x, y, button);
#endif
        emulated_event.button.button = button;
        emulated_event.button.x = x;
        emulated_event.button.y = y;

        emulated_event.button.type=SDL_MOUSEBUTTONDOWN;
        emulated_event.button.state=SDL_PRESSED;
        emulated_event.type=SDL_MOUSEBUTTONDOWN;
    }
    else if(type==SDL_JOYBUTTONUP)
    {
#ifdef DEBUG_MOUSE
        printf("Joystick button release emulated: %i\n", button);
#endif
        emulated_event.type=SDL_JOYBUTTONUP;
        emulated_event.jbutton.type=SDL_JOYBUTTONUP;
        emulated_event.jbutton.which = 0;
        emulated_event.jbutton.state = SDL_RELEASED;
        emulated_event.jbutton.button = button;
    }
    else if(type==SDL_JOYBUTTONDOWN)
    {
#ifdef DEBUG_MOUSE
        printf("Joystick button press emulated: %i\n", button);
#endif
        emulated_event.type=SDL_JOYBUTTONDOWN;
        emulated_event.jbutton.type=SDL_JOYBUTTONDOWN;
        emulated_event.jbutton.which = 0;
        emulated_event.jbutton.state = SDL_PRESSED;
        emulated_event.jbutton.button = button;
    };

    // Push the event onto the event queue.
    Push_SDL_Event(&emulated_event);
}

void Main_SDL_Loop(void *handle)
{
#ifdef DEBUG_FUNCTIONS
    printf("Main_SDL_Loop()\n");
#endif

    frontend *fe = (frontend *)handle;

    // Indicate the direction that the virtual mouse cursor needs to move in.
    signed int mouse_direction_x=0;
    signed int mouse_direction_y=0;

    // The speed of the virtual mouse cursor.
    uint mouse_velocity=0;
    int button=0;
    SDL_Event event;
    bs=snew(struct button_status);
    memset(bs, 0, sizeof(struct button_status));

    max_digit_to_input=-1;
    min_digit_to_input=0;
    uint digit_to_input=1;
    char digit_to_input_as_string[21];
    char current_save_slot_as_string[2];
    int keyval;
    struct timeval now;
    float elapsed;
    int debounce_start_button=0;

    signed int old_mouse_x;
    signed int old_mouse_y;

    // Move the mouse to the center of the puzzle screen.
    mouse_x=fe->ox +(fe->pw / 2);
    mouse_y=fe->oy +(fe->ph / 2);
    SDL_WarpMouse(mouse_x, mouse_y);

    // Start off a timer every SDL_MOUSE_TIMER_INTERVAL ms to update the mouse co-ordinates.
    fe->sdl_mouse_timer_id=SDL_AddTimer(SDL_MOUSE_TIMER_INTERVAL, sdl_mouse_timer_func, fe);
    if(fe->sdl_mouse_timer_id)
    {
#ifdef DEBUG_MOUSE
        printf("Mouse timer created.\n");
#endif
    }
    else
    {
        printf("Error creating mouse timer.\n");
        cleanup_and_exit(fe, EXIT_FAILURE);
    };

    // Start off a timer every 1000 ms
    fe->sdl_second_timer_id=SDL_AddTimer(1000, sdl_second_timer_func, fe);

    if(fe->sdl_mouse_timer_id)
    {
#ifdef DEBUG_MISC
        printf("Second timer created.\n");
#endif
    }
    else
    {
        printf("Error creating second timer.\n");
        cleanup_and_exit(fe, EXIT_FAILURE);
    };


    // Main program loop
    while(TRUE)
    {
#ifdef FAST_SDL_EVENTS
  #ifdef WAIT_FOR_EVENTS
        while(FE_WaitEvent(&event))
  #else
        while(FE_PollEvent(&event))
  #endif
#else
  #ifdef WAIT_FOR_EVENTS  
        while(SDL_WaitEvent(&event))
  #else
        while(SDL_PollEvent(&event))
  #endif
#endif
        {     
                switch(event.type)
                {
                    uint current_line;
                    char *result;
                    int x;

                    case SDL_USEREVENT:
                        switch(event.user.code)
                        {
                            // Mouse timer calls this
                            case RUN_MOUSE_TIMER_LOOP:
#ifdef DEBUG_TIMER
                                    print_time();
                                    printf("Mouse timer fired\n");
#endif
                                    old_mouse_x=mouse_x;
                                    old_mouse_y=mouse_y;

/*
   A note on GP2X joystick button handling (F-100 specific):

   There are actually eight buttons, up, upleft, left, etc. but they are switched on
   in combination to provide greater directional accuracy.  E.g. upleft + left means 
   W-NW, not just a leftover press from either W or NW.

   In these games, we effectively remove such accuracy to provide greater control
   by treating W-NW, W and W-SW the same as just W.  This gives us four large areas
   which are seen as up, down, left, right and four smaller areas seen as the diagonals.
*/

                                    // Generate an acceleration direction (-1,0,1) based on the 
                                    // joystick's x and y axes.
                                    mouse_direction_x=0;
                                    mouse_direction_y=0;
                                    if(bs->joy_upleft || bs->joy_up || bs->joy_upright)
                                        mouse_direction_y=-1;
                                    if(bs->joy_downleft || bs->joy_down || bs->joy_downright)
                                        mouse_direction_y=1;
                                    if(bs->joy_upleft || bs->joy_left || bs->joy_downleft)
                                        mouse_direction_x=-1;
                                    if(bs->joy_upright || bs->joy_right || bs->joy_downright)
                                        mouse_direction_x=1;

                                    if(mouse_direction_x || mouse_direction_y)
                                    {
                                        mouse_velocity++;
                                        if(mouse_velocity > MAX_MOUSE_ACCELERATION)
                                             mouse_velocity = MAX_MOUSE_ACCELERATION;

                                        // Move mouse according to motion recorded on GP2X joystick
                                        mouse_x += (mouse_direction_x * ((signed int) mouse_velocity / 2));
                                        mouse_y += (mouse_direction_y * ((signed int) mouse_velocity / 2));
                                    }
                                    else
                                    {
                                        mouse_velocity=0;
                                    };

                                    if((mouse_x!=old_mouse_x) || (mouse_y!=old_mouse_y))
                                    {
                                        contain_mouse(fe);
                                        SDL_WarpMouse((Uint16) mouse_x, (Uint16) mouse_y);
                                    };

                                break; // switch( event.user.code) case MOUSE_TIMER_LOOP

                            // Game midend timer calls this
                            case RUN_GAME_TIMER_LOOP:
                                // If we are paused, we just pretend the timer didn't fire at all.
                                // This stops timed games from ticking on during the pause menu.
                                if(!fe->paused)
                                {
#ifdef DEBUG_TIMER
                                    print_time();
                                    printf("Game timer fired, %f elapsed since last timer. %u, %u, %u\n", elapsed,fe->paused, fe->timer_active, (int) fe->sdl_timer_id);
#endif
                                    // Update the time elapsed since the last timer, so that the 
                                    // midend can take account of this.
                                    gettimeofday(&now, NULL);
                                    elapsed = ((now.tv_usec - fe->last_time.tv_usec) * 0.000001F + (now.tv_sec - fe->last_time.tv_sec));

                                    // Run the midend timer routine
                                    midend_timer(fe->me, elapsed);

                                    // Update the time the timer was last fired.
                                    fe->last_time = now;
                                };
	                        break; // switch( event.user.code ) case RUN_GAME_TIMER_LOOP

                            case RUN_SECOND_TIMER_LOOP:
#ifdef DEBUG_TIMER
                                print_time();
                                printf("Second timer fired.\n");
#endif

                                // Keep track of the display of the current music track in the Music menu.
                                if(current_screen == MUSICMENU)
                                { 
                                    if(music_track_changed)
                                    {
                                        music_track_changed=FALSE;
                                        draw_menu(fe,MUSICMENU);
                                    };
                                };
                                  
                                // If it's been 5 seconds since the last statusbar change, clear
                                // the statusbar.
                                gettimeofday(&now, NULL);
                                elapsed = ((now.tv_usec - fe->last_statusbar_update.tv_usec) * 0.000001F + (now.tv_sec - fe->last_statusbar_update.tv_sec));

                                if(elapsed > STATUSBAR_TIMEOUT)
                                    clear_statusbar(fe);
                                if(debounce_start_button > 0)
                                    debounce_start_button--;
	                        break; // switch( event.user.code ) case RUN_SECOND_TIMER_LOOP

                        }; // switch( event.user.code)
                    break; // switch( event.type ) case SDL_USEREVENT

                case SDL_JOYBUTTONUP:
                    switch(event.jbutton.button)
                    {
                        case GP2X_BUTTON_UP:
                            bs->joy_up=0;
                            break;

                        case GP2X_BUTTON_UPLEFT:
                            bs->joy_upleft=0;
                            break;

                        case GP2X_BUTTON_UPRIGHT:
                            bs->joy_upright=0;
                            break;

                        case GP2X_BUTTON_DOWN:
                            bs->joy_down=0;
                            break;

                        case GP2X_BUTTON_DOWNLEFT:
                            bs->joy_downleft=0;
                            break;

                        case GP2X_BUTTON_DOWNRIGHT:
                            bs->joy_downright=0;
                            break;

                        case GP2X_BUTTON_LEFT:
                            bs->joy_left=0;
                            break;

                        case GP2X_BUTTON_RIGHT:
                            bs->joy_right=0;
                            break;

                        case GP2X_BUTTON_A:
                            bs->joy_a=0;
                            //�Simulate releasing left mouse button
                            if(current_screen != GAMELISTMENU)
                                emulate_event(SDL_MOUSEBUTTONUP, mouse_x, mouse_y, SDL_BUTTON_LEFT);
                            break;

                        case GP2X_BUTTON_Y:
                            bs->joy_y=0;

                            // Simulate a middle-mouse button release
                            if(current_screen != GAMELISTMENU)
                                emulate_event(SDL_MOUSEBUTTONUP, mouse_x, mouse_y, SDL_BUTTON_MIDDLE);
                            break;

                        case GP2X_BUTTON_X:
                            bs->joy_x=0;
                            break;

                        case GP2X_BUTTON_B:
                            bs->joy_b=0;

                            // Simulate a right-mouse button release
                            if(current_screen != GAMELISTMENU)
                                emulate_event(SDL_MOUSEBUTTONUP, mouse_x, mouse_y, SDL_BUTTON_RIGHT);
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
                    }; // switch(event.jbutton.button)
                    break; // switch( event.type ) case SDL_JOYBUTTONUP
    
                case SDL_JOYBUTTONDOWN:
                    switch(event.jbutton.button)
                    {	
                        case GP2X_BUTTON_UP:
                            if(current_screen == GAMELISTMENU)
                            {
                                // If we're still showing the about screen, select the first game.
                                if(current_game_index < 0)
                                    current_game_index = 0;
                                else
                                    if(current_game_index == 0)
                                        current_game_index=gamecount-1;
                                    else
                                        current_game_index--;
                                draw_menu(fe, GAMELISTMENU);
                            }
                            else if((current_screen == INGAME) && (global_config->control_system == CURSOR_KEYS_EMULATION))
                            {
                                process_key(fe, 0, 0, CURSOR_UP);
                            };
                            bs->joy_up=1;
                            break;

                        case GP2X_BUTTON_UPLEFT:
                            if((current_screen == INGAME) && (global_config->control_system == CURSOR_KEYS_EMULATION) && !(bs->joy_up) && !(bs->joy_left))
                            {
                                process_key(fe, 0, 0, MOD_NUM_KEYPAD | '7');
                            };
                            bs->joy_upleft=1;
                            break;

                        case GP2X_BUTTON_UPRIGHT:
                            if((current_screen == INGAME) && (global_config->control_system == CURSOR_KEYS_EMULATION) && !(bs->joy_up) && !(bs->joy_right))
                            {
                                process_key(fe, 0, 0, MOD_NUM_KEYPAD | '9');
                            };
                            bs->joy_upright=1;
                            break;

                        case GP2X_BUTTON_DOWN:
                            if(current_screen == GAMELISTMENU)
                            {
                                // If we're still showing the about screen, select the first game.
                                if(current_game_index < 0)
                                    current_game_index=0;
                                else
                                    if(current_game_index == (gamecount-1))
                                        current_game_index=0;
                                    else
                                        current_game_index++;
                                draw_menu(fe, GAMELISTMENU);
                            }
                            else if((current_screen == INGAME) && (global_config->control_system == CURSOR_KEYS_EMULATION))
                            {
                                process_key(fe, 0, 0, CURSOR_DOWN);
                            };
                            bs->joy_down=1;
                            break;

                        case GP2X_BUTTON_DOWNLEFT:
                            if((current_screen == INGAME) && (global_config->control_system == CURSOR_KEYS_EMULATION) && !(bs->joy_down) && !(bs->joy_left))
                            {
                                process_key(fe, 0, 0, MOD_NUM_KEYPAD | '1');
                            };
                            bs->joy_downleft=1;
                            break;

                        case GP2X_BUTTON_DOWNRIGHT:
                            if((current_screen == INGAME) && (global_config->control_system == CURSOR_KEYS_EMULATION) && !(bs->joy_down) && !(bs->joy_right))
                            {
                                process_key(fe, 0, 0, MOD_NUM_KEYPAD | '3');
                            };
                            bs->joy_downright=1;
                            break;

                        case GP2X_BUTTON_LEFT:
                            if((current_screen == INGAME) && (global_config->control_system == CURSOR_KEYS_EMULATION))
                            {
                                process_key(fe, 0, 0, CURSOR_LEFT);
                            };
                            bs->joy_left=1;
                            break;

                        case GP2X_BUTTON_RIGHT:
                            if((current_screen == INGAME) && (global_config->control_system == CURSOR_KEYS_EMULATION))
                            {
                                process_key(fe, 0, 0, CURSOR_RIGHT);
                            };
                            bs->joy_right=1;
                            break;

                        case GP2X_BUTTON_A:
#ifdef DEBUG_MOUSE
                            printf("Simulating left-mouse-button press\n");
#endif
                            bs->joy_a=1;

                            if((current_screen == INGAME) && (global_config->control_system == CURSOR_KEYS_EMULATION))
                            {
                                process_key(fe, 0, 0, CURSOR_SELECT);
                            }
                            else if(current_screen != GAMELISTMENU)
                            {
                                // Simulate a left-mouse click
                                emulate_event(SDL_MOUSEBUTTONDOWN, mouse_x, mouse_y, SDL_BUTTON_LEFT);
                            };
                            break;

                        case GP2X_BUTTON_Y:
#ifdef DEBUG_MOUSE
                            printf("Simulating middle-mouse-button press\n");
#endif
                            bs->joy_y=1;

                            // Simulate a middle-mouse click
                            emulate_event(SDL_MOUSEBUTTONDOWN, mouse_x, mouse_y, SDL_BUTTON_MIDDLE);
                            break;

                        case GP2X_BUTTON_B: 
#ifdef DEBUG_MOUSE
                            printf("Simulating right-mouse-button press\n");
#endif
                            bs->joy_b=1;
                            if((current_screen == INGAME) && (global_config->control_system == CURSOR_KEYS_EMULATION))
                            {
                                process_key(fe, 0, 0, CURSOR_SELECT2);
                            }
                            else if(current_screen != GAMELISTMENU)
                            {
                                // Simulate a right-mouse click
                                emulate_event(SDL_MOUSEBUTTONDOWN, mouse_x, mouse_y, SDL_BUTTON_RIGHT);
                            };

                            break;

                        case GP2X_BUTTON_X:
                            bs->joy_x=1;

                            // Save config / Enter digit
                            if(current_screen == GAMEMENU)
                            {
                                char *result;

                                // Try to plug the current config into the game and see if it will
                                // allow it.
                                result=midend_set_config(fe->me, CFG_SETTINGS, fe->cfg);

                                // If the midend won't accept the new config
                                if(result)
                                {
                                    // Present the user with the midend's error text.
                                    sdl_status_bar(fe, result);
                                }
                                else
                                {
                                    // Save config.
                                    if(save_config_to_INI(fe))
                                    {
                                        sdl_status_bar(fe,"Default config saved.");
                                    }
                                    else
                                    {
                                        sdl_status_bar(fe,"Default config could not be saved.");
                                    };
                                };
                            }
                            else if(current_screen == INGAME)
                            {
                                if(this_game.flags & REQUIRE_NUMPAD)
                                {
                                    // Convert the current digit to ASCII, then make it look like it 
                                    // was typed in at the keyboard, to allow entering numbers into
                                    // solo, unequal, etc.  (48=ASCII value of the 0 key)

                                    if((digit_to_input >= 0) && (digit_to_input <= max_digit_to_input))
                                    {
                                        // If it's a letter from a to z that need inputting...
                                        if((digit_to_input > 9) && (digit_to_input < 36))
                                        {
                                            // 97 = ASCII value of "a"
                                            keyval = digit_to_input + 97 - 10;
                                            process_key(fe, 0, 0, keyval);
                                        }
                                        else
                                        {
                                            keyval = MOD_NUM_KEYPAD | (digit_to_input + 48);                                        
                                            process_key(fe, 0, 0, keyval);
                                        };
                                    };
                                }
                                else
                                {
                                    sdl_status_bar(fe,"Digit entry not supported in this game.");
                                };
                            };
                            break;

                        case GP2X_BUTTON_L:
                            // Quit (if pressed with R)
                            // Cycle digit to enter
                            // Cycle savegame slot

                            // If we're holding the other shoulder button too...
                            if(bs->joy_r)
                            {
                                if(current_screen == GAMELISTMENU)
                                    delete_ini_file(fe, current_game_index);
                                else
                                {
                                    if(global_config->autosave_on_exit)
                                    {
                                        sdl_status_bar(fe, autosave_game(fe));
                                    };
                                    // Quit back to the menu
                                    menu_loop(fe);
                                }
                            }
                            else
                            {
                                bs->joy_l=1;

                                if(current_screen == GAMEMENU)
                                {
                                    // If we are in the pause menu, let L and R change the save slot
                                    if(current_save_slot == 0)
                                        current_save_slot = 9;
                                    else                         
                                        current_save_slot -= 1;
                                    sprintf(current_save_slot_as_string, "Saveslot %u selected", current_save_slot);
                                    sdl_status_bar(fe,current_save_slot_as_string);
                                }
                                else if(current_screen == INGAME)
                                {
                                    if(this_game.flags & REQUIRE_NUMPAD)
                                    {
                                        // Cycle the number we would input
                                        if(digit_to_input <= min_digit_to_input)
                                            digit_to_input = max_digit_to_input;
                                        else if(digit_to_input <= max_digit_to_input)
                                            digit_to_input -= 1;
                                        else
                                            digit_to_input = max_digit_to_input;

                                        if((digit_to_input > 9) && (digit_to_input < 36))
                                        {
                                            // 65 = ASCII value of "A"
                                            sprintf(digit_to_input_as_string, "Press X to enter a \'%c\'", digit_to_input + 65 - 10);
                                        }
                                        else
                                        {
                                            sprintf(digit_to_input_as_string, "Press X to enter a \'%u\'", digit_to_input);
                                        };
                                        sdl_status_bar(fe,digit_to_input_as_string);
                                    }
                                    else
                                    {
                                        sdl_status_bar(fe,"Digit entry not supported in this game.");
                                    };
                                };
                            };
                            break;

                        case GP2X_BUTTON_R:
                            // Quit (if pressed with L)
                            // Cycle digit to enter
                            // Cycle savegame slot

                            // If we're holding the other shoulder button too...
                            if(bs->joy_l)
                            {
                                if(current_screen == GAMELISTMENU)
                                    delete_ini_file(fe, current_game_index);
                                else
                                {
                                    if(global_config->autosave_on_exit)
                                    {
                                        sdl_status_bar(fe, autosave_game(fe));
                                    };
                                    // Quit back to the menu
                                    menu_loop(fe);
                                };
                            }
                            else
                            {
                                bs->joy_r=1;

                                if(current_screen == GAMEMENU)
                                {
                                    // If we are in the pause menu, let L and R change the save slot
                                    current_save_slot += 1;
                                    if(current_save_slot > 9)
                                        current_save_slot = 0;
                                    sprintf(current_save_slot_as_string, "Saveslot %u selected", current_save_slot);
                                    sdl_status_bar(fe,current_save_slot_as_string);
                                }
                                else if(current_screen == INGAME)
                                {
                                    if(this_game.flags & REQUIRE_NUMPAD)
                                    {
                                        // Cycle the number we would input
                                        digit_to_input += 1;

                                        if((digit_to_input > max_digit_to_input) || (digit_to_input < min_digit_to_input))
                                            digit_to_input = min_digit_to_input;

                                        if((digit_to_input > 9) && (digit_to_input < 36))
                                        {
                                            // 65 = ASCII value of "a"
                                            sprintf(digit_to_input_as_string, "Press X to enter a \'%c\'", digit_to_input + 65 - 10);
                                        }
                                        else
                                        {
                                            sprintf(digit_to_input_as_string, "Press X to enter a \'%u\'", digit_to_input);
                                        };
                                        sdl_status_bar(fe,digit_to_input_as_string);
                                    }
                                    else
                                    {
                                        sdl_status_bar(fe,"Digit entry not supported in this game.");
                                    };
                                };
                            };
                            break;

                        case GP2X_BUTTON_VOLDOWN:
                            // Solve (if pressed with Vol+)
                            // Load (if in pause menu)

                            bs->joy_voldown=1;
                            if(current_screen != GAMELISTMENU)
                            {
                                // If we're holding the other volume button too...
                                if(bs->joy_volup)
                                {
                                    if(fe->paused)
                                    {
                                        load_game(fe,current_save_slot);
                                    }
                                    else
                                    {
                                        // Solve the puzzle
                                        sdl_status_bar(fe,"Solving...");
                                        char *solve_error = midend_solve(fe->me);
                                        if(solve_error)
                                            sdl_status_bar(fe, solve_error);
                                        else
                                            sdl_status_bar(fe,"Solved.");
                                    };
                                }
                                // If we're holding the other L button too...
                                else if(bs->joy_l)
                                {
                                    // Send the "Undo" key to the midend
                                    process_key(fe, 0, 0, 'u');
                                };
                            };
                            break;

                        case GP2X_BUTTON_VOLUP:
                            // Solve (if pressed with Vol-)
    
                            bs->joy_volup=1;
 
                            if(current_screen != GAMELISTMENU)
                            {
                                // If we're holding the other volume button too...
                                if(bs->joy_voldown)
                                {
                                    if(fe->paused)
                                    {
                                        load_game(fe,current_save_slot);
                                    }
                                    else
                                    {
                                        // Solve the puzzle
                                        sdl_status_bar(fe,"Solving...");
                                        char *solve_error = midend_solve(fe->me);
                                        if(solve_error)
                                            sdl_status_bar(fe, solve_error);
                                        else
                                            sdl_status_bar(fe,"Solved.");
                                    };
                                }
                                // If we're holding the L button too...
                                else if(bs->joy_l)
                                {
                                    // Send the "Redo" key to the midend.
                                    process_key(fe, 0, 0, '\x12');
                                };
                            };
                            break;

                        case GP2X_BUTTON_START:
                            if(debounce_start_button==0)
                            {

                            if(current_screen == GAMELISTMENU)
                            {
                                // If we're still showing the about screen (or have acquired an invalid game index)
                                if(current_game_index < 0)
                                {
                                    // Redraw the menu with the first game selected.
                                    current_game_index=0;
                                    draw_menu(fe, GAMELISTMENU);
                                }
                                else
                                {
                                    debounce_start_button=1;
                                    // If we're holding down the L or R button
                                    if((bs->joy_l == 1) || (bs->joy_r == 1))
                                    {
                                       // Start a new game WITHOUT reading in the saved .INI
                                       start_game(fe, current_game_index, TRUE);
                                    }
                                    else
                                    {
                                       // Start a new game normally.
                                       start_game(fe, current_game_index, FALSE);
                                    };
                                };
                            }
                            else
                            {
                                // Pause / Unpause
                                fe->paused=!fe->paused;
                                game_pause(fe);
                            };
                            };
                            break;

                        case GP2X_BUTTON_SELECT:
                            if(current_screen == GAMELISTMENU)
                            {
                                clear_statusbar(fe);
                                sdl_status_bar(fe,"Quitting...");
                                cleanup_and_exit(fe, EXIT_SUCCESS);
                            }
                            else if(current_screen == GAMEMENU)
                            {
                                char *result;
                                result=save_game(fe,current_save_slot);
                                sdl_status_bar(fe, result);
                                sfree(result);
                            }
                            else if(current_screen == INGAME)
                            {
                                // Restart the current game
                                sdl_status_bar(fe,"Restarting game...");
                                midend_restart_game(fe->me);
                                midend_force_redraw(fe->me);
                                sdl_status_bar(fe,"Restarted game.");
                            };
                            break;

                        case GP2X_BUTTON_CLICK:
                            if(global_config->screenshots_enabled)
                            {
                            char save_filename[20];
                            int i;
                            for(i=0;i<10000;i++)
                            {
                                sprintf(save_filename, SCREENSHOT_FILENAME,i);
                                if(file_exists(save_filename) == FALSE)
                                    break;
                            };
#ifdef DEBUG_FILE_ACCESS
                            printf("Screenshot filename: %s\n",save_filename);
#endif

                            if(i != 10000)
                            {
                                if((bs->joy_l) || (bs->joy_r))
                                {
                                    if(current_screen == INGAME)
                                    {
                                        SDL_Surface *screenshot_surface;
                                        SDL_Rect blit_rectangle;

                                        if(!global_config->screenshots_include_cursor)
                                            SDL_ShowCursor(SDL_DISABLE);

                                        if(!global_config->screenshots_include_statusbar);
                                            // Make the game redraw itself.	
                                            midend_force_redraw(fe->me);
       
                                        screenshot_surface=SDL_CreateRGBSurface(SDL_SURFACE_FLAGS & (!SDL_DOUBLEBUF), fe->pw, fe->ph, SCREEN_DEPTH, 0, 0, 0, 0);
                                        blit_rectangle.x=fe->ox;
                                        blit_rectangle.y=fe->oy;
                                        blit_rectangle.w=fe->pw;
                                        blit_rectangle.h=fe->ph;

                                        SDL_BlitSurface(screen, &blit_rectangle, screenshot_surface, NULL);
                                        if(SDL_SaveBMP(screenshot_surface,save_filename) == 0)
                                            sdl_status_bar(fe,"Game screenshot saved.");
                                        else
                                            sdl_status_bar(fe,"Error saving game screenshot.");

                                        if(!global_config->screenshots_include_cursor)
                                            SDL_ShowCursor(SDL_ENABLE);
                                    };
                                }
                                else
                                {
                                    if(SDL_SaveBMP(fe->screen,save_filename) == 0)
                                        sdl_status_bar(fe,"Entire screenshot saved.");
                                    else
                                        sdl_status_bar(fe,"Error saving entire screenshot.");
                                };
                            };
                            }
                            break;
                    }; // switch(event.jbutton.button)
                    break; // switch( event.type ) case SDL_JOYBUTTONDOWN

#ifndef TARGET_GP2X
// Used for debugging so that we can quit the game when using a Linux machine
// running on the same code!
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_z:
                            //�Simulate pressing the Select key
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_SELECT);
                            break;

                        case SDLK_y:
                            //�Simulate pressing the Y key
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_Y);
                            break;

                        case SDLK_u:
                            //�Simulate pressing the Vol- key
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_VOLDOWN);
                            break;

                        case SDLK_i:
                            //�Simulate pressing the Vol+ key
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_VOLUP);
                            break;

                        case SDLK_p:
                            //�Simulate pressing the Start key
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_START);
                            break;

                        case SDLK_q:
                            clear_statusbar(fe);
                            sdl_status_bar(fe,"Quitting...");
                            cleanup_and_exit(fe, EXIT_SUCCESS);
                            break;

                        case SDLK_e:
                            //�Simulate pressing stick-click
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_CLICK);
                            break;

                        case SDLK_s:
                            //�Simulate pressing the Vol+ and Vol- keys
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_VOLUP);
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_VOLDOWN);
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_VOLUP);
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_VOLDOWN);
                            break;

                        case SDLK_l:
                            //�Simulate pressing the L key
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_L);
                            break;

                        case SDLK_a:
                            //�Simulate pressing the A key
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_A);
                            break;

                        case SDLK_b:
                            //�Simulate pressing the B key
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_B);
                            break;

                        case SDLK_x:
                            //�Simulate pressing the X key
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_X);
                            break;

                        case SDLK_r:
                            //�Simulate pressing the R key
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_R);
                            break;

                        case SDLK_UP:
                            //�Simulate pressing Up
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_UP);
                            break;

                        case SDLK_DOWN:
                            //�Simulate pressing Down
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_DOWN);
                            break;

                        case SDLK_LEFT:
                            //�Simulate pressing Left
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_LEFT);
                            break;

                        case SDLK_RIGHT:
                            //�Simulate pressing Right
                            emulate_event(SDL_JOYBUTTONDOWN, 0, 0, GP2X_BUTTON_RIGHT);
                           break;

                        default:
                            break;
                    }; // switch(event.key.keysym.sym)
                    break; // switch( event.type ) case SDL_KEYDOWN

                case SDL_KEYUP:
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_z:
                            //�Simulate pressing the Select key
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_SELECT);
                            break;

                        case SDLK_p:
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_START);
                            break;

                        case SDLK_u:
                            //�Simulate releasing the Vol- key
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_VOLDOWN);
                            break;

                        case SDLK_i:
                            //�Simulate releasing the Vol+ key
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_VOLUP);
                            break;

                        case SDLK_l:
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_L);
                            break;

                        case SDLK_a:
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_A);
                            break;

                        case SDLK_b:
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_B);
                            break;

                        case SDLK_y:
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_Y);
                            break;

                        case SDLK_x:
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_X);
                            break;

                        case SDLK_r:
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_R);
                            break;

                        case SDLK_UP:
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_UP);
                            break;

                        case SDLK_DOWN:
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_DOWN);
                            break;

                        case SDLK_LEFT:
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_LEFT);
                            break;

                        case SDLK_RIGHT:
                            emulate_event(SDL_JOYBUTTONUP, 0, 0, GP2X_BUTTON_RIGHT);
                            break;

                        default:
                            break;
                    }; // switch(event.key.keysym.sym)
                    break; // switch( event.type ) case SDL_KEYUP
#endif

                case SDL_MOUSEBUTTONUP:
                    if(!fe->paused)
                    {
                        if(current_screen == INGAME)
                        {
                            button=0;
                            if(event.button.button == 1)
                            {
#ifdef DEBUG_MOUSE
                                printf("Left-mouse-button release recieved\n");
#endif
                                button = LEFT_BUTTON;
                                bs->mouse_left=0;
                            }
                            else if(event.button.button == 2)
                            {
#ifdef DEBUG_MOUSE
                                printf("Middle-mouse-button release recieved\n");
#endif
                                button = MIDDLE_BUTTON;
                                bs->mouse_middle=0;
                            }
                            else if (event.button.button == 3)
                            {
#ifdef DEBUG_MOUSE
                                printf("Right-mouse-button release recieved\n");
#endif
                                button = RIGHT_BUTTON;
                                bs->mouse_right=0;
                            };

                            // If one of the "buttons" was valid...
                            if(button)
                            {
                                // Send a mouse button release to the midend.
                                button += LEFT_RELEASE - LEFT_BUTTON;
                                process_key(fe, event.button.x - fe->ox, event.button.y - fe->oy, button);
                            };
                        };
                    };
                    break; // switch( event.type ) case SDL_MOUSEBUTTONUP

                case SDL_MOUSEBUTTONDOWN:
#ifdef SCALELARGESCREEN
  if(screen_width == SCREEN_WIDTH_LARGE)
  {
    event.motion.x=event.motion.x * 2;
    event.motion.y=event.motion.y * 2;
  };
#endif
                        current_line=(event.button.y / (MENU_FONT_SIZE+2)+1);

                        switch(current_screen)
                        {
                            case INGAME:
                                button=0;
                                if (event.button.button == 1)
                                {
#ifdef DEBUG_MOUSE
                                    printf("Left-mouse-button press recieved at %i, %i\n", event.button.x, event.button.y);
#endif
                                    button = LEFT_BUTTON;
                                    bs->mouse_left=1;
                                }
                                else if (event.button.button == 2)
                                {
#ifdef DEBUG_MOUSE
                                    printf("Middle-mouse-button press recieved at %i, %i\n", event.button.x, event.button.y);
#endif
                                    button = MIDDLE_BUTTON;
                                    bs->mouse_middle=1;
                                }
                                else if (event.button.button == 3)
                                {
#ifdef DEBUG_MOUSE
                                    printf("Right-mouse-button press recieved at %i, %i\n", event.button.x, event.button.y);
#endif
                                    button = RIGHT_BUTTON;
                                    bs->mouse_right=1;
                                };

                                // If one of the "buttons" was valid...
                                if(button)
                                    // Send a mouse button press to the midend.
                                    process_key(fe, event.button.x - fe->ox, event.button.y - fe->oy, button);
                                break;

                            case GAMELISTMENU:
                                // If we're still showing the about screen (or have acquired an invalid game index)
                                if(current_game_index < 0)
                                {
                                    // Redraw the menu with the first game selected.
                                    current_game_index=0;
                                    draw_menu(fe, GAMELISTMENU);
                                }
                                else
                                {
                                    // If we clicked on the left hand side (list of games)
                                    if(event.button.x < 160)
                                    {
                                        // If we clicked the up arrow...
                                        if(current_line == 2)
                                            current_game_index--;
                                        // If we clicked a game...
                                        else if((current_line > 2) && (current_line < 16))
                                            current_game_index = current_game_index + (current_line - 3);
                                        // If we clicked the down arrow...
                                        else if(current_line == 16)
                                            current_game_index++;

                                        // Make sure the current_game_index stays within bound.
                                        if(current_game_index < 0)
                                            current_game_index = gamecount - 1;
                                        if(current_game_index > (gamecount -1))
                                            current_game_index = 0;
 
                                        // Redraw the menu.
                                        redraw_gamelist_menu(fe);
                                    }
                                    else
                                    {
                                        // If we clicked near the bottom.
                                        if(event.button.y > 220)
                                        {
                                            // If we clicked the words "Start game".
                                            if((event.button.x > 160) && (event.button.x < 250))
                                            {
                                                // If we're holding down the L or R button
                                                if((bs->joy_l == 1) || (bs->joy_r == 1))
                                                {
                                                    // Start a new game WITHOUT reading in the saved .INI
                                                    start_game(fe, current_game_index, TRUE);
                                                }
                                                else
                                                {
                                                    // Start a new game normally.
                                                    start_game(fe, current_game_index, FALSE);
                                                };
                                            } 
                                            // If we clicked the word "Exit".
                                            else if(event.button.x > 250)
                                            {
                                                clear_statusbar(fe);
                                                sdl_status_bar(fe,"Quitting...");
                                                cleanup_and_exit(fe, EXIT_SUCCESS);
                                            };
                                        }; 
                                    };
                                };
                                break;

                            case HELPMENU:
                                if(helpfile_showing == TRUE)
                                {
                                    draw_menu(fe, HELPMENU);
                                }
                                else if(current_line == 3)
                                {
                                    draw_menu(fe, GAMEMENU);
                                }
                                else if(current_line == 5)
                                {
                                    if(read_game_helpfile(fe))
                                    {
                                        helpfile_showing=TRUE;
                                    }
                                    else
                                    {
                                        sdl_status_bar(fe,"Could not open help file.");
                                    };
                                }
                                else if(current_line == 6)
                                {
                                    if(show_file_on_screen(fe, MENU_INGAME_KEY_HELPFILE))
                                    {
                                        helpfile_showing=TRUE;
                                    }
                                    else
                                    {
                                        sdl_status_bar(fe,"Could not open ingame keys help file.");
                                    };
                                }
                                else if(current_line == 7)
                                {
                                    if(show_file_on_screen(fe, MENU_KEY_HELPFILE))
                                    {
                                        helpfile_showing=TRUE;
                                    }
                                    else
                                    {
                                        sdl_status_bar(fe,"Could not open keys help file.");
                                    };
                                };
                                break;

                            case CREDITSMENU:
                                draw_menu(fe, MUSICCREDITSMENU);
                                break;

                            case MUSICCREDITSMENU:
                                // Go back to the game menu screen.
                                draw_menu(fe, GAMEMENU);
                                break;

                            case CONFIGMENU:
                                if(current_line == 4)
                                {
                                    // Save my preset
                                    char *result;

                                    // Try to plug the current config into the game and see if it will
                                    // allow it.
                                    result=midend_set_config(fe->me, CFG_SETTINGS, fe->cfg);

                                    // If the midend won't accept the new config
                                    if(result)
                                    {
                                        // Present the user with the midend's error text.
                                        sdl_status_bar(fe, result);
                                    }
                                    else
                                    {
                                        // Save config.
                                        if(save_config_to_INI(fe))
                                            sdl_status_bar(fe,"Default config saved.");
                                        else
                                            sdl_status_bar(fe,"Default config could not be saved.");
                                    };
                                } 
                                else if(current_line == 3)
                                {
                                    // Save settings
                                    char *result;
    
                                // Try to plug the current config into the game and see if it will
                                // allow it.
                                result=midend_set_config(fe->me, CFG_SETTINGS, fe->cfg);

                                // If the midend won't accept the new config
                                if(result)
                                {
                                    // Present the user with the midend's error text.
                                    sdl_status_bar(fe,result);
                                }
                                else
                                {
                                    // Return to the main menu.
                                    draw_menu(fe, GAMEMENU);
                                };
                            }
                            else if(current_line == 5)
                            {
                                // Return to the main menu.
                                draw_menu(fe, PRESETSMENU);
                            }
                            else if(current_line > 5)
                            { 
                                // If someone is clicking where there might be config options...
                                uint i,j;
                                int config_item_found=FALSE;

                                // Loop over all config options
                                for(i=0; i < fe->ncfg; i++)
                                {
                                    // Find the config option that relate to the position clicked
                                    if( (((fe->config_window_options[i].y-1) / (MENU_FONT_SIZE+2))+1) == current_line && !config_item_found)
                                    {
                                        int current_cfg_item_index=fe->config_window_options[i].config_index;
                                        int current_number=0;
                                        uint max_config_option_index;
                                        int current_config_option_index;

                                        config_item_found=TRUE;

                                        switch(fe->cfg[current_cfg_item_index].type)
                                        {
                                            case C_STRING:
                                                // If it's got a decimal point in it
                                                if(strchr(fe->cfg[current_cfg_item_index].sval,'.'))
                                                {
                                                    // Add or remove 0.01 from it
                                                    float current_probability=atof(fe->cfg[current_cfg_item_index].sval);
                                                    if (event.button.button == 1)
                                                        current_probability-=0.01;
                                                    if (event.button.button == 3)
                                                        current_probability+=0.01;
                                                    sprintf(fe->cfg[current_cfg_item_index].sval, "%.2f", current_probability);
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
#ifdef DEBUG_MISC
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
                                                if(current_config_option_index>((int)max_config_option_index-1))
                                                    current_config_option_index=0;

                                                // Set the new option
                                                fe->cfg[current_cfg_item_index].ival=current_config_option_index;
#ifdef DEBUG_MISC
                                                printf("is %u, out of %u\n",current_config_option_index, max_config_option_index);
#endif
                                            break;
                                        }; // switch(fe->cfg[current_cfg_item_index].type)
                                    };// if
                                };// for

                                // Redraw the pause/config menu
                                draw_menu(fe, CONFIGMENU);
                            }; //if
                            break;

                            case SETTINGSMENU:
                                switch(current_line)
                                {
                                case 3:
                                    draw_menu(fe, GAMEMENU);
                                    break;

                                case 4:
                                    if(save_global_config_to_INI(FALSE))
                                    {
                                        sdl_status_bar(fe,"Default config saved.");
                                    }
                                    else
                                    {
                                        sdl_status_bar(fe,"Default config could not be saved.");
                                    };
                                    break;

                                case 7:
                                    global_config->play_music=1-global_config->play_music;
                                    if(global_config->play_music)
                                    { 
#ifdef BACKGROUND_MUSIC            
                                        start_background_music();
#endif
                                    }
                                    else
                                    {
#ifdef BACKGROUND_MUSIC            
                                        stop_background_music();
#endif
                                    };                                        
                                    draw_menu(fe, SETTINGSMENU);
                                    break;
  
                                case 8:
                                    draw_menu(fe, MUSICMENU);
                                    break;

                                case 9:
                                    global_config->screenshots_enabled=1-global_config->screenshots_enabled;
                                    draw_menu(fe, SETTINGSMENU);
                                    break;

                                case 10:
                                    global_config->screenshots_include_cursor=1-global_config->screenshots_include_cursor;
                                    draw_menu(fe, SETTINGSMENU);
                                    break;

                                case 11:
                                    global_config->screenshots_include_statusbar=1-global_config->screenshots_include_statusbar;
                                    draw_menu(fe, SETTINGSMENU);
                                    break;

                                case 12:
                                    global_config->autosave_on_exit=1-global_config->autosave_on_exit;
                                    draw_menu(fe, SETTINGSMENU);
                                    break;

                                case 13:
                                    global_config->always_load_autosave=1-global_config->always_load_autosave;
                                    draw_menu(fe, SETTINGSMENU);
                                    break;

                                case 15:
                                    global_config->control_system=MOUSE_EMULATION;
                                    draw_menu(fe, SETTINGSMENU);
                                    break;

                                case 16:
                                    global_config->control_system=CURSOR_KEYS_EMULATION;
                                    draw_menu(fe, SETTINGSMENU);
                                    break;
                              };
                              break;

                          case MUSICMENU:
                              if(current_line == 3)
                            {
                                draw_menu(fe, SETTINGSMENU);
                            }
                            else if(current_line == 4)
                            {
                                if(save_global_config_to_INI(TRUE))
                                {
                                    sdl_status_bar(fe,"Music config saved.");
                                }
                                else
                                {
                                    sdl_status_bar(fe,"Music config could not be saved.");
                                };
                                break;
                            }
                            else if(current_line == 6)
                            {
                                if (event.button.button == 1)
                                {
                                    if(global_config->music_volume < (MIX_MAX_VOLUME - 10))
                                        global_config->music_volume = global_config->music_volume + 10;
                                    else
                                        global_config->music_volume = MIX_MAX_VOLUME;
                                }
                                else if(event.button.button == 3)
                                {
                                    if(global_config->music_volume > 10)
                                        global_config->music_volume = global_config->music_volume - 10;
                                    else
                                        global_config->music_volume = 0;     
                                };
#ifdef BACKGROUND_MUSIC
                                Mix_VolumeMusic(global_config->music_volume);
#endif
                                draw_menu(fe, MUSICMENU);
                            }
                            else if((current_line > 6) && (current_line <16))
                            {
                                if(event.button.x < 10)
                                {
#ifdef BACKGROUND_MUSIC
                                    play_music_track(current_line - 6);
#endif
                                }
                                else
                                {
                                    global_config->tracks_to_play[current_line - 7] = !global_config->tracks_to_play[current_line - 7];
                                };
                                draw_menu(fe, MUSICMENU);
                            };
                            break;
                        
                        case PRESETSMENU:
                            if(current_line == 3)
                            {
                                draw_menu(fe, CONFIGMENU);
                            }
                            else if(current_line == 4)
                            {
                                if(load_config_from_INI(fe))
                                {
                                    // Size the game for the current screen.
                                    configure_area(screen_width, screen_height, fe);

                                    draw_menu(fe, INGAME);
  
                                    // Update the user in case the next bit takes a long time.
                                    sdl_status_bar(fe,"Loaded.");

                                    current_screen = INGAME;
                                }
                                else
                                {
                                    sdl_status_bar(fe,"Cannot load your preset - do you have one saved?");
                                };
                            }
                            else if(current_line == 7)
                            {
                                if(fe->first_preset_showing > 0)
                                    fe->first_preset_showing--;
                                draw_menu(fe, PRESETSMENU);
                            }
                            else if((current_line > 7) && (current_line < 17))
                            {
                                int preset_to_load;
                                game_params *preset_params;
                                char *preset_name;

                                preset_to_load = current_line - 8 + fe->first_preset_showing;

                                if(preset_to_load < midend_num_presets(fe->me))
                                {
#ifdef DEBUG_MISC
                                    printf("Loading preset %u\n", preset_to_load);
#endif
                                    midend_fetch_preset(fe->me, preset_to_load, &preset_name, &preset_params);
                                    midend_set_params(fe->me, preset_params);

                                    if(fe->cfg != NULL)
                                        free_cfg(fe->cfg);

                                    if(fe->configure_window_title != NULL)
                                        sfree(fe->configure_window_title);

                                    fe->cfg=midend_get_config(fe->me, CFG_SETTINGS, &fe->configure_window_title);

                                    // Resize/scale the game screen
                                    configure_area(screen_width, screen_height, fe);

                                    // Set the clipping region to the whole physical screen
                                    sdl_no_clip(fe);

                                    // Blank over the whole screen using the frontend "rect" routine with background colour
                                    sdl_actual_draw_rect(fe, 0, 0, screen_width, screen_height, fe->background_colour);

                                    sdl_status_bar(fe,"Generating a new game...");

                                    // Update the screen.
                                    sdl_end_draw(fe);

                                    // Set the clipping region to the whole physical screen
                                    sdl_no_clip(fe);

                                    start_loading_animation(fe);

                                    // Start a new game with the new config
                                    // This will probably mess up the clipping region.
                                    midend_new_game(fe->me);

                                    stop_loading_animation();
  
                                    draw_menu(fe, INGAME);
                                };
                            }
                            else if((current_line > 16) && (midend_num_presets(fe->me) > 9))
                            {
                                fe->first_preset_showing++;
                                if(fe->first_preset_showing > (midend_num_presets(fe->me)-1))
                                    fe->first_preset_showing=midend_num_presets(fe->me)-1;                                    
                                draw_menu(fe, PRESETSMENU);
                            };
                            break;

                        case GAMEMENU:

                            switch(current_line)
                            {
                                case 5:
                                    // Return to game menu item
                                    fe->paused=FALSE;
                                    game_pause(fe);
                                    break;

                                case 6:
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
                                        configure_area(screen_width, screen_height, fe);

                                        // Set the clipping region to the whole physical screen
                                        sdl_no_clip(fe);

                                        // Blank over the whole screen using the frontend "rect" routine with background colour
                                        sdl_actual_draw_rect(fe, 0, 0, screen_width, screen_height, fe->background_colour);

                                        sdl_status_bar(fe,"Generating a new game...");
 
                                        // Update the screen.
                                        sdl_end_draw(fe);

                                        // Set the clipping region to the whole physical screen
                                        sdl_no_clip(fe);

                                        start_loading_animation(fe);

                                        // Start a new game with the new config
                                        // This will probably mess up the clipping region.
                                        midend_new_game(fe->me);

                                        stop_loading_animation();

                                        // Set the clipping region to the whole physical screen
                                        sdl_no_clip(fe);
 
                                        // Blank over the whole screen using the frontend "rect" routine with background colour
                                        sdl_actual_draw_rect(fe, 0, 0, screen_width, screen_height, fe->background_colour);

                                        // Make the game redraw itself.	
                                        midend_force_redraw(fe->me);

                                        // Set the clipping region to the game window.
                                        sdl_unclip(fe);

                                        draw_menu(fe, INGAME);
                                    };
                                    break;

                                case 7:
//INDENTING NEEDED BELOW HERE (4 spaces)
                                // Restart

                                // Come out of the pause menu and restart the game.
                                fe->paused=FALSE;
                                game_pause(fe);
                                midend_restart_game(fe->me);
                                midend_force_redraw(fe->me);
                                sdl_status_bar(fe,"Restarting game...");
                                break;

                            case 8:
                                draw_menu(fe, CONFIGMENU);
                                break;
                            case 9:
                                draw_menu(fe, SAVEMENU);
                                break;
                            case 10:
                                draw_menu(fe, SETTINGSMENU);
                                break;
                           
                            case 11:
                                draw_menu(fe, HELPMENU);
                                break;

                            case 12:
                                draw_menu(fe, CREDITSMENU);
                                break;

                            case 13:
                                if(global_config->autosave_on_exit)
                                {
                                    sdl_status_bar(fe, autosave_game(fe));
                                };
                                draw_menu(fe, GAMELISTMENU);
                                break;

                            default:
                                // Redraw the pause/config menu
                                draw_menu(fe, GAMEMENU);                                
                                break;
                        }; // switch(current_line)
                        break;

                    case SAVEMENU:
                        x = event.button.x;
                        if(screen_width==SCREEN_WIDTH_LARGE)
                            x = event.button.x / 2;
                        if(current_line == 3)
                        {
                            draw_menu(fe, GAMEMENU);
                        }
                        else if((current_line >= 5) && (current_line <15))
                        {
                            int saveslot_number = current_line -5;
                            if(saveslot_number < 10)
                            {
                                if((x > 200) && (x <240))
                                {
                                    // Save the game, redraw the menu (to show the new save game)
                                    // and THEN show a status bar message of whether it worked
                                    // or not.
                                    char *result;
                                    savegame_to_delete = -1;
                                    result=save_game(fe,saveslot_number);
                                    draw_menu(fe, SAVEMENU);
                                    sdl_status_bar(fe, result);
                                    sfree(result);
                                }
                                else if((x > 250) && (x < 290))
                                {
                                    savegame_to_delete = -1;
                                    if(savefile_exists(fe->sanitised_game_name, saveslot_number))
                                        load_game(fe, saveslot_number);
                                }
                                else if((x > 300) && (savefile_exists(fe->sanitised_game_name, saveslot_number)))
                                {
                                    char *message;
                                    message=snewn(sizeof("Press the delete icon again to delete save slot 0"), char);
                                    if((savegame_to_delete <0) || (savegame_to_delete != saveslot_number))
                                    {
                                        savegame_to_delete = saveslot_number;
                                        sprintf(message, "Press the delete icon again to delete save slot %u",saveslot_number);
                                        sdl_status_bar(fe, message);
                                        sfree(message);
                                    }
                                    else
                                    {
                                        delete_savegame(fe, savegame_to_delete);
                                        draw_menu(fe, SAVEMENU);
                                    };                                    
                                };
                            }
                            else
                            {
                                savegame_to_delete = -1;
                            };
                        }
                        else if(current_line == 15)
                        {
                            if((event.button.x > 250) && (event.button.x < 290))
                            {
                                if(autosave_file_exists(fe->sanitised_game_name))
                                    load_autosave_game(fe);
                            }
                            else if((event.button.x > 300) && (autosave_file_exists(fe->sanitised_game_name)))
                            {
                                delete_autosave_game(fe);
                                draw_menu(fe, SAVEMENU);
                            };
                        };
                        break;
                };
                break; // switch( event.type ) case SDL_MOUSEBUTTONDOWN

            case SDL_MOUSEMOTION:
                // Both joystick "warps" and real mouse movement (touchscreen etc.) end up here.
/*
                if((event.motion.x == mouse_x) && (event.motion.y == mouse_y))
                {

                }
                else
*/
                {
                    // Use the co-ords from the actual event, because the mouse/touchscreen wouldn't
                    // update our internal co-ords variables itself otherwise.
                    mouse_x=event.motion.x;
                    mouse_y=event.motion.y;

#ifdef DEBUG_MOUSE
  #ifdef DEBUG_TIMER
                    print_time();
  #endif
                    printf("Mouse moved to %i, %i\n", mouse_x, mouse_y);
#endif
                    // Contain the mouse cursor inside the edges of the puzzle.
                    if(contain_mouse(fe))
                    {
                        SDL_WarpMouse((Uint16)mouse_x, (Uint16) mouse_y);
                    }
                    else
                    {
                        if(!fe->paused)
                        {
                            // Send a "drag" event to the midend if necessary, 
                            // or just a motion event.
                            button=0;
                            if(bs->mouse_middle)
                                button = MIDDLE_DRAG;
                            else if(bs->mouse_right)
                                button = RIGHT_DRAG;
                            else if(bs->mouse_left)
                                button = LEFT_DRAG;
                            if(button)
                                process_key(fe, mouse_x - fe->ox, mouse_y - fe->oy, button);
                        };
                    };
                };
                break; // switch( event.type ) case SDL_MOUSEMOTION

            case SDL_QUIT:
                clear_statusbar(fe);
                sdl_status_bar(fe,"Quitting...");
                cleanup_and_exit(fe, EXIT_SUCCESS);
                break;
             } // switch(event.type)

        }; // while( SDL_PollEvent( &event ))
    }; // while(TRUE)
}

// Returns a dynamically allocated name of the game, suitable for use in filenames (i.e
// no spaces, not too long etc.)
char* sanitise_game_name(char *unclean_game_name)
{
#ifdef DEBUG_FUNCTIONS
    printf("sanitise_game_name()\n");
#endif

    int i;

    // Allocate a new string
    char* game_name=snewn(MAX_GAMENAME_SIZE+1,char);
    memset(game_name, 0, (MAX_GAMENAME_SIZE+1) * sizeof(char));

    // Put the first MAX_GAMENAME_SIZE characters of the game name into a variable we can change
    sprintf(game_name, "%.*s", MAX_GAMENAME_SIZE, unclean_game_name);

    // Sanitise the game name - if it contains spaces, replace them with underscores in 
    // the filename and convert the filename to lowercase (doesn't matter on the GP2X
    // because it uses case-insensitive FAT32, but you never know what might happen and
    // this is better for development).
    for(i=0;i<MAX_GAMENAME_SIZE;i++)
    {
        if(isspace(game_name[i]) || (game_name[i]=='\\') || (game_name[i]=='/'))
        {
            game_name[i]='_';
        }
        else
        {
            game_name[i]=tolower(game_name[i]);
        };
    };
    return(game_name);
};

// Load a game configuration from the game's INI file.
int load_config_from_INI(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("load_config_from_INI()\n");
#endif

    char *filename;
    char *result;
    char *keyword_name;
    char *string_value;
    int boolean_value;
    int int_value;
    int j=0;

    // Dynamically allocate space for a filename and add .ini to the end of the 
    // sanitised game name to get the filename we will use.
    filename=snewn(MAX_GAMENAME_SIZE + 5,char);
    sprintf(filename, "%.*s.ini", MAX_GAMENAME_SIZE, fe->sanitised_game_name);
#ifdef DEBUG_FILE_ACCESS
    printf("Attempting to load game INI file: %s\n",filename);
#endif

    // If we have an INI file already loaded into RAM
    if(fe->ini_dict)
    {
        // Free it.
        iniparser_freedict(fe->ini_dict);
        fe->ini_dict=NULL;
    };

    // Load the INI file into an in-memory structure.
    fe->ini_dict=iniparser_load(filename);

    // If we didn't succeed
    if(fe->ini_dict==NULL)
    {
#ifdef DEBUG_FILE_ACCESS
        printf("Cannot parse INI file: %s\nReverting to game defaults.\n", filename);
#endif
        // Allocate an empty INI structure, just to make the thing work.
        // This way, if the user saves, we have something to populate and then save.
        fe->ini_dict=dictionary_new(0);
        
        // Free the dynamically-allocated filename
        sfree(filename);
        return(FALSE);
    }
    else
    {
#ifdef DEBUG_FILE_ACCESS
        printf("INI file successfully parsed: %s\n", filename);
#else
        printf("Configuration loaded from %s\n", filename);
#endif
    };

    // Free the dynamically-allocated filename    
    sfree(filename);

    // Loop over all of the configuration options
    while(fe->cfg[j].type!=C_END)
    {
        // Allocate a new string of the form "Configuration:<keyname>"
        keyword_name=snewn(255,char);
        sprintf(keyword_name, "Configuration:%s",fe->cfg[j].name);

        // Depending on the type of the configuration option
        switch(fe->cfg[j].type)
        {
            case C_STRING:
                // Dynamically allocate a new string to hold the value we load, so we can
                // play with it and store it ourselves.
		string_value=snewn(255,char);

                // Copy the value read into our string (we can't play with the result
                // directly because it goes into a dictionary structure).
                strncpy(string_value,iniparser_getstring(fe->ini_dict, keyword_name, "\n"),250);

                // Populate the configuration value with the read string.
		fe->cfg[j].sval=string_value;
#ifdef DEBUG_FILE_ACCESS
		printf("INI: %s was read as %s\n", keyword_name, string_value);
#endif
                break;

            case C_BOOLEAN:
                // Read a boolean value from the INI file, return -1 if not found.
                boolean_value=iniparser_getboolean(fe->ini_dict, keyword_name,-1);
		if(boolean_value==-1)
                {
                    // Do nothing.  The INI key was not found, and specifying any particular
                    // default would overwrite the game's default options.
                }
                else
                {
                    // Populate the configuration value with the read boolean
                    fe->cfg[j].ival=boolean_value;
#ifdef DEBUG_FILE_ACCESS
                    printf("INI: %s was read as %i\n", keyword_name, boolean_value);
#endif
                };
                break;

            case C_CHOICES:
                // Read in the number associated with the selected configuration item, 
                // from the choices available, which is stored in the INI file under 
                // the key "<configuration_item_name>.default".  Return -1 if not found.
                strcat(keyword_name,".default");
                int_value=iniparser_getint(fe->ini_dict, keyword_name,-1);
		if(int_value==-1)
                {
                    // Do nothing.  The INI value was not found, and specifying any particular
                    // default will overwrite the game's default options.
                }
                else
                {
                    // Populate the configuration value with the read integer.
                    fe->cfg[j].ival=int_value;
#ifdef DEBUG_FILE_ACCESS
                    printf("INI: %s was read as %i\n", keyword_name, int_value);
#endif
                };
                break;
        }; // end switch

        j++;
        sfree(keyword_name);
    }; // end while
 
    // Plug the current config (i.e. the new INI config) back into the game
    result=midend_set_config(fe->me, CFG_SETTINGS, fe->cfg);

    // If the midend won't accept the new config
    if(result)
    {
        // Present the user with the midend's error text.
        sdl_status_bar(fe,result);
#ifdef DEBUG_FILE_ACCESS
        printf("Loaded INI config could not be used (corrupt).\nReverting to game defaults.\n");
#endif
	return(FALSE);
    }
    else
    {
#ifdef DEBUG_FILE_ACCESS
        printf("Loaded INI config was successfully used.\n");
#endif

        // Start a new game to let the configuration options take effect.
        start_loading_animation(fe);
        midend_new_game(fe->me);
        stop_loading_animation();
    };

    return(TRUE);
};

void load_global_config_from_INI()
{
#ifdef DEBUG_FUNCTIONS
    printf("load_global_config_from_INI()\n");
#endif

    int boolean_value;
    int int_value;
    dictionary *global_ini_dict;
    int i;

#ifdef DEBUG_FILE_ACCESS
    printf("Attempting to load global INI file: %s\n",GLOBAL_CONFIG_FILENAME);
#endif

    // Load the INI file into an in-memory structure.
    global_ini_dict=iniparser_load(GLOBAL_CONFIG_FILENAME);

    // If we didn't succeed
    if(global_ini_dict==NULL)
    {
#ifdef DEBUG_FILE_ACCESS
        printf("Cannot parse INI file: %s\nReverting to global defaults.\n", GLOBAL_CONFIG_FILENAME);
#endif
        // Allocate an empty INI structure, just to make the thing work.
        global_ini_dict=dictionary_new(0);
    }
    else
    {
#ifdef DEBUG_FILE_ACCESS
        printf("INI file successfully parsed: %s\n", GLOBAL_CONFIG_FILENAME);
#else
        printf("Global configuration loaded from %s\n", GLOBAL_CONFIG_FILENAME);
#endif
    };

    boolean_value=iniparser_getboolean(global_ini_dict, "Configuration:autosave",-1);
    if(boolean_value==-1)
    {
        // Do nothing.  The INI key was not found, so use the normal default.
    }
    else
    {
        if(boolean_value==0)
            global_config->autosave_on_exit=FALSE;
        else
            global_config->autosave_on_exit=TRUE;
    };

    boolean_value=iniparser_getboolean(global_ini_dict, "Configuration:always_load_autosave",-1);
    if(boolean_value==-1)
    {
        // Do nothing.  The INI key was not found, so use the normal default.
    }
    else
    {
        if(boolean_value==0)
            global_config->always_load_autosave=FALSE;
        else
            global_config->always_load_autosave=TRUE;
    };

    boolean_value=iniparser_getboolean(global_ini_dict, "Configuration:play_music",-1);
    if(boolean_value==-1)
    {
        // Do nothing.  The INI key was not found, so use the normal default.
    }
    else
    {
        if(boolean_value==0)
            global_config->play_music=FALSE;
        else
            global_config->play_music=TRUE;
    };

    boolean_value=iniparser_getboolean(global_ini_dict, "Configuration:screenshots_enabled",-1);
    if(boolean_value==-1)
    {
        // Do nothing.  The INI key was not found, so use the normal default.
    }
    else
    {
        if(boolean_value==0)
            global_config->screenshots_enabled=FALSE;
        else
            global_config->screenshots_enabled=TRUE;
    };

    boolean_value=iniparser_getboolean(global_ini_dict, "Configuration:screenshots_include_cursor",-1);
    if(boolean_value==-1)
    {
        // Do nothing.  The INI key was not found, so use the normal default.
    }
    else
    {
        if(boolean_value==0)
            global_config->screenshots_include_cursor=FALSE;
        else
            global_config->screenshots_include_cursor=TRUE;
    };

    boolean_value=iniparser_getboolean(global_ini_dict, "Configuration:screenshots_include_statusbar",-1);
    if(boolean_value==-1)
    {
        // Do nothing.  The INI key was not found, so use the normal default.
    }
    else
    {
        if(boolean_value==0)
            global_config->screenshots_include_statusbar=FALSE;
        else
            global_config->screenshots_include_statusbar=TRUE;
    };

    boolean_value=iniparser_getboolean(global_ini_dict, "Configuration:control_system",-1);
    if(boolean_value==-1)
    {
        // Do nothing.  The INI key was not found, so use the normal default.
    }
    else
    {
        if(boolean_value==0)
            global_config->control_system=MOUSE_EMULATION;
        else
            global_config->control_system=CURSOR_KEYS_EMULATION;
    };

    int_value=iniparser_getint(global_ini_dict, "Configuration:music_volume",-1);
    if(int_value==-1)
    {
        // Do nothing.  The INI key was not found, so use the normal default.
    }
    else
    {
        global_config->music_volume=int_value;
    };

    for(i=0;i<10;i++)
    {
        char *track_number_as_string=snewn(21, char);
        sprintf(track_number_as_string, "Configuration:track%u", i);
        boolean_value=iniparser_getboolean(global_ini_dict, track_number_as_string,-1);
        if(boolean_value==-1)
        {
            // Do nothing.  The INI key was not found, so use the normal default.
        }
        else
        {
            if(boolean_value==0)
                global_config->tracks_to_play[i]=FALSE;
            else
                global_config->tracks_to_play[i]=TRUE;
        };
        sfree(track_number_as_string);
    };

    iniparser_freedict(global_ini_dict);
};

uint save_global_config_to_INI(uint save_music_config)
{
#ifdef DEBUG_FUNCTIONS
    printf("save_global_config_to_INI()\n");
#endif

    FILE *inifile;
    dictionary *global_ini_dict;
    int i;

#ifdef DEBUG_FILE_ACCESS
    printf("Attempting to save global INI file: %s\n",GLOBAL_CONFIG_FILENAME);
#endif

    // Load the INI file into an in-memory structure.
    global_ini_dict=iniparser_load(GLOBAL_CONFIG_FILENAME);

    // If we didn't succeed
    if(global_ini_dict==NULL)
    {
#ifdef DEBUG_FILE_ACCESS
        printf("Cannot parse INI file: %s\n", GLOBAL_CONFIG_FILENAME);
#endif
        // Allocate an empty INI structure, just to make the thing work.
        global_ini_dict=dictionary_new(0);
    };

    // Create an INI section, in case one does not already exist.
    // If we don't do this, INIParser tends not to write entries under that
    // section at all.
    iniparser_setstring(global_ini_dict, "Configuration", NULL);

    if(!save_music_config)
    {
        iniparser_setstring(global_ini_dict, "Configuration:autosave", global_config->autosave_on_exit?"T":"F");
        iniparser_setstring(global_ini_dict, "Configuration:always_load_autosave", global_config->always_load_autosave?"T":"F");
        iniparser_setstring(global_ini_dict, "Configuration:play_music", global_config->play_music?"T":"F");
        iniparser_setstring(global_ini_dict, "Configuration:screenshots_enabled", global_config->screenshots_enabled?"T":"F");
        iniparser_setstring(global_ini_dict, "Configuration:screenshots_include_cursor", global_config->screenshots_include_cursor?"T":"F");
        iniparser_setstring(global_ini_dict, "Configuration:screenshots_include_statusbar", global_config->screenshots_include_statusbar?"T":"F");
        iniparser_setstring(global_ini_dict, "Configuration:control_system", (global_config->control_system==CURSOR_KEYS_EMULATION)?"T":"F");
    }
    else
    {
        char *music_volume_as_string=snewn(4, char);
        sprintf(music_volume_as_string, "%u", global_config->music_volume);
        iniparser_setstring(global_ini_dict, "Configuration:music_volume", music_volume_as_string);
        sfree(music_volume_as_string);

        for(i=0;i<10;i++)
        {
            char *track_number_as_string=snewn(21, char);
            sprintf(track_number_as_string, "Configuration:track%u", i);
            iniparser_setstring(global_ini_dict, track_number_as_string, global_config->tracks_to_play[i]?"T":"F");
            sfree(track_number_as_string);
        };
    };

#ifdef DEBUG_FILE_ACCESS
    printf("INI: Attempting to open %s for writing.\n", GLOBAL_CONFIG_FILENAME);
#endif
    // Open the INI file for writing.
    inifile = fopen(GLOBAL_CONFIG_FILENAME, "w");
    
    // If we succeeded
    if(inifile)
    {
#ifdef DEBUG_FILE_ACCESS
        printf("INI: %s opened successfully for writing.\n", GLOBAL_CONFIG_FILENAME);
#endif
        // Write the in-memory INI "dictionary" to disk as an INI file.
        iniparser_dump_ini(global_ini_dict,inifile);

        // Flush the data straight to disk.
        fflush(inifile);

        // Close the file
        fclose(inifile);
#ifdef DEBUG_FILE_ACCESS
        printf("INI: %s written to disk.\n", GLOBAL_CONFIG_FILENAME);
#endif
        return(TRUE);
    }
    else
    {
#ifdef DEBUG_FILE_ACCESS
        printf("INI: Error opening %s for writing.\n", GLOBAL_CONFIG_FILENAME);
#endif
        return(FALSE);
    };
    iniparser_freedict(global_ini_dict);
};


// Callback function used by the midend to read a game from an actual file.
// This is called many times by the midend itself to read individual lines of a savefile
int savefile_read(void *wctx, void *buf, int len)
{
#ifdef DEBUG_FUNCTIONS
    printf("savefile_read()\n");
#endif
    FILE *fp = (FILE *)wctx;
    int ret;

    ret = fread(buf, 1, len, fp);
    return (ret == len);
};

uint file_exists(char *filename)
{
#ifdef DEBUG_FUNCTIONS
    printf("file_exists()\n");
#endif
    // Open the file, readonly
    FILE *fp = fopen(filename, "r");

    // If it didn't open
    if (!fp)
        return(FALSE);

    // Close the file
    fclose(fp);

    return(TRUE);
};

uint savefile_exists(char *game_name, uint saveslot_number)
{
#ifdef DEBUG_FUNCTIONS
    printf("savefile_exists()\n");
#endif

    char *save_filename;
    uint result;
    save_filename=generate_save_filename(game_name,saveslot_number);

    result=file_exists(save_filename);
    sfree(save_filename);
    return(result);
};

char *generate_save_filename(char *game_name, uint saveslot_number)
{
#ifdef DEBUG_FUNCTIONS
    printf("generate_save_filename()\n");
#endif

    char *save_filename;

    // Dynamically allocate space for a filename and add .ini to the end of the 
    // sanitised game name to get the filename we will use.
    save_filename=snewn(MAX_GAMENAME_SIZE+6,char);
    memset(save_filename, 0, (MAX_GAMENAME_SIZE+6) * sizeof(char));
    sprintf(save_filename, "%.*s%u.sav", MAX_GAMENAME_SIZE, game_name, saveslot_number);
    return(save_filename);
};

// Load a game from a saveslot
void load_game(frontend *fe, uint saveslot_number)
{
#ifdef DEBUG_FUNCTIONS
    printf("load_game()\n");
#endif

    char *err;
    int x,y;

    char *message_text;
    char *save_filename;

    save_filename=generate_save_filename(fe->sanitised_game_name, saveslot_number);

    printf("Loading savefile from: %s\n", save_filename);

    // Open the file, readonly
    FILE *fp = fopen(save_filename, "r");
    sfree(save_filename);

    // If it didn't open
    if (!fp)
    {
        // Generate an error message.
        message_text=snewn(38, char);
        sprintf(message_text, "Could not load game from save slot %u.", saveslot_number);

        // Display it to the user       
        sdl_status_bar(fe, message_text);
        sfree(message_text);
        return;
    };

    // Instruct the midend to start reading in the savefile
    // This will call savefile_read itself multiple times to read the file in bit-by-bit.
    err = midend_deserialise(fe->me, savefile_read, fp);

    // Close the file
    fclose(fp);

    // If the midend generated an error while loading the file
    if(err)
    {
        // Generate an error message
        message_text=snewn(39,char);
        sprintf(message_text, "Could not parse game from save slot %u.", saveslot_number);
 
        // Display it to the user.
        sdl_status_bar(fe, message_text);
        sfree(message_text);
        return;
    };

    // Get the new puzzle size
    get_size(fe, &x, &y);
    fe->w=x;
    fe->h=y;

    // Make the puzzle redraw everything with the new game
    sdl_no_clip(fe);
    configure_area(screen_width,screen_height,fe);
    sdl_actual_draw_rect(fe, 0, 0, screen_width, screen_height, fe->background_colour);
    midend_force_redraw(fe->me);
    sdl_unclip(fe);

    draw_menu(fe, INGAME);

    // Generate a success message and show it to the user.
    message_text=snewn(30,char);
    sprintf(message_text, "Game loaded from save slot %u.", saveslot_number);
    sdl_status_bar(fe, message_text);
    sfree(message_text);
}

// Load a game from a saveslot
void load_autosave_game(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("load_autosave_game()\n");
#endif

    char *err;
    int x,y;

    char *save_filename;

    save_filename=snewn(MAX_GAMENAME_SIZE+10,char);
    sprintf(save_filename, "%.*s.autosave", MAX_GAMENAME_SIZE, fe->sanitised_game_name);

    printf("Loading autosave game from: %s\n", save_filename);

    // Open the file, readonly
    FILE *fp = fopen(save_filename, "r");
    sfree(save_filename);

    // If it didn't open
    if(!fp)
    {   
        sdl_status_bar(fe, "Could not load game from autosave slot.");
        return;
    };

    // Instruct the midend to start reading in the savefile
    // This will call savefile_read itself multiple times to read the file in bit-by-bit.
    err = midend_deserialise(fe->me, savefile_read, fp);

    // Close the file
    fclose(fp);

    // If the midend generated an error while loading the file
    if(err)
    {
        sdl_status_bar(fe, "Could not parse autosave game.");
        return;
    };

    // Get the new puzzle size
    get_size(fe, &x, &y);
    fe->w=x;
    fe->h=y;

    // Make the puzzle redraw everything with the new game
    sdl_no_clip(fe);
    configure_area(screen_width,screen_height,fe);
    sdl_actual_draw_rect(fe, 0, 0, screen_width, screen_height, fe->background_colour);
    midend_force_redraw(fe->me);
    sdl_unclip(fe);

    draw_menu(fe, INGAME);

    sdl_status_bar(fe, "Game loaded from autosave slot.");
}

// Callback function used by the midend to save a game into an actual file.
// This is called many times by the midend itself to save individual lines of a savefile
void savefile_write(void *wctx, void *buf, int len)
{
    FILE *fp = (FILE *)wctx;
    fwrite(buf, 1, len, fp);
}

int autosave_file_exists(char *game_name)
{
#ifdef DEBUG_FUNCTIONS
    printf("autosave_file_exists()\n");
#endif

    char *save_filename;
    uint result;
    save_filename=snewn(MAX_GAMENAME_SIZE+10, char);
    sprintf(save_filename, "%.*s.autosave", MAX_GAMENAME_SIZE, game_name);
    result=file_exists(save_filename);
    sfree(save_filename);
    return(result);
};

char *autosave_game(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("autosave_game()\n");
#endif

    char *save_filename;
    char *message_text;
    FILE *fp;

    save_filename=snewn(MAX_GAMENAME_SIZE + 10, char);
    memset(save_filename, 0, (MAX_GAMENAME_SIZE+10) * sizeof(char));
    sprintf(save_filename, "%.*s.autosave", MAX_GAMENAME_SIZE, fe->sanitised_game_name);

    fp = fopen(save_filename, "w");
    sfree(save_filename);
    if (!fp)
    {
        message_text="Could not write to autosave file.";
#ifdef DEBUG_FILE_ACCESS
        printf("Could not write to autosave file.\n");
#endif
        return(message_text);
    };
    midend_serialise(fe->me, savefile_write, fp);
    fclose(fp);
    message_text="Game auto-saved.";
#ifdef DEBUG_FILE_ACCESS
    printf("Game auto-saved.\n");
#endif
    return(message_text);
}

char *save_game(frontend *fe, uint saveslot_number)
{
#ifdef DEBUG_FUNCTIONS
    printf("save_game()\n");
#endif

    char *save_filename;
    char *message_text;

    save_filename=generate_save_filename(fe->sanitised_game_name, saveslot_number);

    FILE *fp;
    fp = fopen(save_filename, "w");
    sfree(save_filename);
    if (!fp)
    {
        message_text=snewn(32,char);
        sprintf(message_text, "Could not write to save slot %u.", saveslot_number);
    }
    else
    {
        midend_serialise(fe->me, savefile_write, fp);
        fclose(fp);
        message_text=snewn(27,char);
        sprintf(message_text, "Game saved to save slot %u.", saveslot_number);
    };
    return(message_text);
}

// Saves the current configuration to an INI file.
// The current configuration is assumed to be valid (i.e. the game midend accepts it)
int save_config_to_INI(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("save_config_to_INI()\n");
#endif

    int j=0;
    char *keyword_name;
    FILE *inifile;
    char *ini_filename;
    char digit_as_string[32];

    // Dynamically allocate space for a filename and add .ini to the end of the 
    // sanitised game name to get the filename we will use.
    ini_filename=snewn(MAX_GAMENAME_SIZE + 5,char);
    sprintf(ini_filename, "%.*s.ini", MAX_GAMENAME_SIZE, fe->sanitised_game_name);
  
    // Create an INI section, in case one does not already exist.
    // If we don't do this, INIParser tends not to write entries under that
    // section at all.
    iniparser_setstring(fe->ini_dict, "Configuration", NULL);

    // Loop over all the configuration items in this game
    while(fe->cfg[j].type!=C_END)
    {
        // Dynamically allocate a new string and set it to the name that the
        // configuration option would be using in an INI file.
        keyword_name=snewn(255,char);
        sprintf(keyword_name, "Configuration:%s", fe->cfg[j].name);

        switch(fe->cfg[j].type)
        {
            case C_STRING:
#ifdef DEBUG_FILE_ACCESS
                printf("INI: %s is now %s in file %s\n",keyword_name,fe->cfg[j].sval,ini_filename);
#endif
                // Write the current setting to the in-memory INI dictionary.
                iniparser_setstring(fe->ini_dict, keyword_name, fe->cfg[j].sval);
                break;

            case C_BOOLEAN:
                // Write the current setting to the in-memory INI dictionary.
                if(fe->cfg[j].ival)
                {
#ifdef DEBUG_FILE_ACCESS
                    printf("INI: %s is now %s in file %s\n",keyword_name,"T",ini_filename);
#endif
                    iniparser_setstring(fe->ini_dict, keyword_name, "T");
                }
                else
                {
#ifdef DEBUG_FILE_ACCESS
                    printf("INI: %s is now %s in file %s\n",keyword_name,"F",ini_filename);
#endif
                    iniparser_setstring(fe->ini_dict, keyword_name, "F");
                };
                break;

            case C_CHOICES:
                 // Write the current setting to the in-memory INI dictionary.
#ifdef DEBUG_FILE_ACCESS
                printf("INI: %s is now %s in file %s\n",keyword_name,fe->cfg[j].sval,ini_filename);
#endif
                iniparser_setstring(fe->ini_dict, keyword_name, fe->cfg[j].sval);

                // Because "CHOICES" consist of a string of options and a number to 
                // indicate the currently selected option, we have to write the number into
                // the INI too.

                // We do this using a key called "<configuration option>.default".
                strcat(keyword_name, ".default");
                sprintf(digit_as_string, "%d", fe->cfg[j].ival);
                iniparser_setstring(fe->ini_dict, keyword_name, digit_as_string);
                
                break;
        }; // end switch

        // Free the string used for generated the keyword.
        sfree(keyword_name);
        j++;
    }; // end while

#ifdef DEBUG_FILE_ACCESS
    printf("INI: Attempting to open %s for writing.\n", ini_filename);
#endif
    // Open the INI file for writing.
    inifile = fopen(ini_filename, "w");
    
    // If we succeeded
    if(inifile)
    {
#ifdef DEBUG_FILE_ACCESS
        printf("INI: %s opened successfully for writing.\n", ini_filename);
#endif
        // Write the in-memory INI "dictionary" to disk as an INI file.
        iniparser_dump_ini(fe->ini_dict,inifile);

        // Flush the data straight to disk.
        fflush(inifile);

        // Close the file
        fclose(inifile);
#ifdef DEBUG_FILE_ACCESS
        printf("INI: %s written to disk.\n", ini_filename);
#endif
        // Free the dynamically-allocated string used for the INI filename.
        sfree(ini_filename);

        return(TRUE);
    }
    else
    {
#ifdef DEBUG_FILE_ACCESS
        printf("INI: Error opening %s for writing.\n", ini_filename);
#endif
        // Free the dynamically-allocated string used for the INI filename.
        sfree(ini_filename);

        return(FALSE);
    };
};

int get_mouse_type()
{
#ifdef TARGET_GP2X
 #ifdef OPTION_CHECK_HARDWARE
    return(SDL_GP2X_MouseType());
 #else
    return(GP2X_MOUSE_STD);    
 #endif
#else
    return(0);
#endif
}

void start_loading_animation(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("start_loading_animation()\n");
#endif

    if(*loading_flag)
    {
#ifdef DEBUG_MISC
        printf("start_loading_animation called recursively!\n");
#endif
    }
    else
    {
        SDL_Rect blit_rectangle;
        blit_rectangle.w=0;
        blit_rectangle.h=0;
        if(screen_width == SCREEN_WIDTH_LARGE)
        {
            SDL_Surface *zoomed_surface=zoomSurface(loading_screen, 2, 2, SMOOTHING_ON);
            blit_rectangle.x=(screen_width - zoomed_surface->w) / 2;
            blit_rectangle.y=(screen_height - zoomed_surface->h) / 2;
            actual_unlock_surface(screen);
            SDL_BlitSurface(zoomed_surface, NULL, screen, &blit_rectangle);
            actual_lock_surface(screen);
            SDL_FreeSurface(zoomed_surface);
        }
        else
        {
            blit_rectangle.x=0;
            blit_rectangle.y=0;
            actual_unlock_surface(screen);
            SDL_BlitSurface(loading_screen, NULL, screen, &blit_rectangle);
            actual_lock_surface(screen);
        };

        sdl_end_draw(fe);

        *loading_flag=TRUE;

#ifdef OPTION_USE_THREADS
        // Spawn a thread to do fancy stuff while we carry on with the thing we want the 
        // user to wait for.
  #ifdef DEBUG_MISC
        printf("Spawning animation thread...\n");
  #endif
        splashscreen_animation_thread = SDL_CreateThread(splashscreen_thread_func, fe);
        if(splashscreen_animation_thread == NULL)
        {
            printf("Unable to create animation thread: %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        };
#endif
    };
}

void stop_loading_animation()
{
#ifdef DEBUG_FUNCTIONS
    printf("stop_loading_animation()\n");
#endif

    if(*loading_flag)
    {
        *loading_flag=FALSE;
#ifdef OPTION_USE_THREADS
  #ifdef DEBUG_MISC
        printf("Waiting for thread to terminate...\n");
  #endif
        SDL_WaitThread(splashscreen_animation_thread, NULL);
  #ifdef DEBUG_MISC
        printf("Animation thread terminated.\n");
  #endif
#endif
    }
    else
    {
#ifdef DEBUG_MISC
        printf("stop_loading_animation called recursively!\n");
#endif
    };
};

int splashscreen_thread_func(void *data)
{
    Sint16 x, y;
    int i, current=0;

    frontend *fe=(frontend *) data;

    // Loop until we are asked to terminate
    while(*loading_flag)
    {
        // Animate some square blocks to look fancy (numbers are hardcoded for now).
        y=screen_height * 6 / 10;

        // Blank over the old squares.
        boxRGBA(screen, 70, y - 10, 230, y + 10, 0, 0, 0, (Uint8) 255);
        for(i=0;i<8;i++)
        {
            x=70+i*20;
            if(current==i)
            {
                boxRGBA(screen, x, y, x + 12, y + 12, 200, 200, 200, (Uint8) 255);
            }
            else
            {
                boxRGBA(screen, x, y, x + 10, y + 10, 100, 100, 100, (Uint8) 255);
            };
        };
        current++;
        if(current>7)
            current=0;

        sdl_end_draw(fe);

        // Sleep so that the things we're hiding behind the loading screen can 
        // actually do their work, e.g. game creation, etc.
        SDL_Delay(ANIMATION_DELAY);
    };
    return(0);
};

enum { ARG_EITHER, ARG_SAVE, ARG_ID }; /* for argtype */

// Main Program Entry-point
int main(int argc, char **argv)
{
#ifdef DEBUG_FUNCTIONS
    printf("main()\n");
#endif

    frontend *fe;
    Uint32 SDL_Init_Flags;
    char *error;
    int i;

    if(argc > 1)
        respawn_gp2x_menu=FALSE;

    InitMemPool();

    // Initialise the loading flag.
    loading_flag = snew(uint);
    *loading_flag=FALSE;

    // Make sure that SDL etc. is cleaned up should anything fail.
    atexit(SDL_Quit);
#ifdef FAST_SDL_EVENTS
    atexit(FE_Quit);
#endif
    atexit(TTF_Quit);
    atexit(game_quit);

    SDL_Init_Flags = SDL_INIT_VIDEO|SDL_INIT_JOYSTICK|SDL_INIT_TIMER;
#ifdef EVENTS_IN_SEPERATE_THREAD
    SDL_Init_Flags |= SDL_INIT_EVENTTHREAD;

    printf("Using threaded events.\n");
#else
    printf("Not using threaded events.\n");
#endif

#ifdef BACKGROUND_MUSIC
    SDL_Init_Flags |= SDL_INIT_AUDIO;
#endif

    // Initialise the various components of SDL that we intend to use.
    if(SDL_Init(SDL_Init_Flags))
    {
        printf("Error initialising SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    };

    printf("Initialised SDL.\n");

#ifdef BACKGROUND_MUSIC
    initialise_audio();
#endif
    
    // Initialise video 
    screen = SDL_SetVideoMode(SCREEN_WIDTH_SMALL, SCREEN_HEIGHT_SMALL, SCREEN_DEPTH, SDL_SURFACE_FLAGS);

    // If the video mode was initialised successfully
    if(screen)
    {
        printf("Initialised %u x %u @ %u bit video mode.\n", screen->w, screen->h, SCREEN_DEPTH);
#ifdef DEBUG_DRAWING
        if(SDL_MUSTLOCK(screen))
        {
            printf("Screen surface requires locking.\n");
        }
        else
        {
            printf("Screen surface does not require locking.\n");
        };            
#endif
    }
    else
    {
        printf("Error initialising %u x %u @ %u bit video mode: %s\n", screen_width, screen_height, SCREEN_DEPTH, SDL_GetError());
        exit(EXIT_FAILURE);
    };

#if defined(_WIN32) || defined(__CYGWIN__)
    SDL_WM_SetCaption(stppc2x_ver, stppc2x_ver);
#endif

    screen_width=screen->w;
    screen_height=screen->h;

    // Turn off the cursor as soon as we can.
    SDL_ShowCursor(SDL_DISABLE);

    if((loading_screen = IMG_Load(LOADING_SCREEN_FILENAME)) == NULL)
    {
        printf("Unable to load %s: %s\n", LOADING_SCREEN_FILENAME, SDL_GetError());
        exit(EXIT_FAILURE);
    };
    
#ifdef FAST_SDL_EVENTS
    if(FE_Init())
    {
        printf("Error initialising SDL Fast Events: %s\n", FE_GetError());
        exit(EXIT_FAILURE);
    };

    printf("Initialised SDL Fast Events.\n");
#endif

    // Initialise SDL_TTF.
    if(TTF_Init()) 
    {
        printf("Error intialising SDL_ttf: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Initialised SDL TTF library.\n");
    };    

    // Create a new game
    fe = new_window(NULL, ARG_EITHER, &error);
    if(!fe)
    {
        printf("Error initialising frontend: %s\n", error);
        exit(EXIT_FAILURE);
    };

    gettimeofday(&fe->last_statusbar_update,NULL);

    // Copy our temporary placeholder for the screen into the frontend structure.
    fe->screen = screen;
#ifdef SCALELARGESCREEN
    real_screen = screen;
#endif

    // Start the loading animation.
    start_loading_animation(fe); 

#ifdef DEBUG_DRAWING
    if((fe->screen->flags & SDL_HWSURFACE) == 0)
    {
        printf("Using a software surface.\n");
    }
    else
    {
        printf("Using a hardware surface.\n");
    };
#endif

#ifdef OPTION_CHECK_HARDWARE
    printf("Detecting hardware...\n");
#endif

    // Check for a joystick
    if(SDL_NumJoysticks()>0)
    {
        // Open first joystick
        fe->joy=SDL_JoystickOpen(0);

        if(fe->joy)
        {
            printf("Initialised Joystick 0.\n");
        }
        else
        {
            printf("Error initialising Joystick 0.\n");
            cleanup_and_exit(fe, EXIT_FAILURE);
        };
    }
    else
    {
        printf("No joysticks were found on this system.  This is not a real GP2X.\n");

#ifdef OPTION_CHECK_HARDWARE
        hardware_type=NOT_GP2X;
#endif
    };

#ifdef OPTION_CHECK_HARDWARE
 #ifdef TARGET_GP2X
    printf("I appear to be running on a real GP2X.\n");
    if( (access("/dev/touchscreen/wm97xx", F_OK)==0) || (get_mouse_type()==GP2X_MOUSE_TOUCHSCREEN))
    {
        printf("Found a touchscreen device.  Assuming it's an F-200 GP2X.\n");
        hardware_type=GP2X_F200;
    };

    if(hardware_type==UNKNOWN)
    {
        printf("Could not determine hardware type.  Assuming it's an F-100 GP2X.\n");
        hardware_type=GP2X_F100;
    };
 #else
    printf("I am not running on a real GP2X.\n");
    hardware_type=NOT_GP2X;
 #endif

    printf("Hardware detection complete.\n");
#endif


    global_config=snew(struct global_configuration);
    global_config->autosave_on_exit=FALSE;
    global_config->always_load_autosave=FALSE;
    global_config->play_music=FALSE;
    global_config->screenshots_enabled=FALSE;
    global_config->screenshots_include_cursor=FALSE;
    global_config->screenshots_include_statusbar=FALSE;
    global_config->control_system=FALSE;
    global_config->music_volume=MIX_MAX_VOLUME;
    for(i=0;i<10;i++)
        global_config->tracks_to_play[i]=FALSE;

    load_global_config_from_INI();
#ifdef BACKGROUND_MUSIC
    if(global_config->play_music)
        start_background_music();
#endif

    // Kill the loading animation *before* we let the game touch the screen.
    stop_loading_animation();

    // Start the menu
    menu_loop(fe);

    first_run=FALSE;

    printf("Starting Event Loop...\n");
    Main_SDL_Loop(fe);

    // This line is never reached but keeps the compiler happy.
    return(EXIT_SUCCESS);
}

void menu_loop(frontend *fe)
{  
#ifdef DEBUG_FUNCTIONS
    printf("menu_loop()\n");
#endif

    current_screen=GAMELISTMENU;
    fe->timer_active=FALSE;

    if((screen->w != SCREEN_WIDTH_SMALL) || (screen->h != SCREEN_HEIGHT_SMALL))
    {
        // SDL docs say that the surface should NOT be FreeSurface'd or anything else when changing resolutions.
        screen = SDL_SetVideoMode(SCREEN_WIDTH_SMALL, SCREEN_HEIGHT_SMALL, SCREEN_DEPTH, SDL_SURFACE_FLAGS);

        // If the video mode was initialised successfully
        if(screen)
        {
            fe->screen = screen;
#ifdef SCALELARGESCREEN
            real_screen = screen;
#endif

            printf("Initialised %u x %u @ %u bit video mode.\n", screen->w, screen->h, SCREEN_DEPTH);
#ifdef DEBUG_DRAWING
            if(SDL_MUSTLOCK(screen))
            {
                printf("Screen surface requires locking.\n");
            }
            else
            {
                printf("Screen surface does not require locking.\n");
            };
#endif
        }
        else
        {
            printf("Error initialising %u x %u @ %u bit video mode: %s\n", screen->w, screen->h, SCREEN_DEPTH, SDL_GetError());
            exit(EXIT_FAILURE);
        };

        STATUSBAR_FONT_SIZE = DEFAULT_STATUSBAR_FONT_SIZE;
        MENU_FONT_SIZE = DEFAULT_MENU_FONT_SIZE;
        HELP_FONT_SIZE = DEFAULT_HELP_FONT_SIZE;
        KEYBOARD_ICON_FONT_SIZE = DEFAULT_KEYBOARD_ICON_FONT_SIZE;

        screen_width=SCREEN_WIDTH_SMALL;
        screen_height=SCREEN_HEIGHT_SMALL;
    };    

    // Turn off the cursor.
    SDL_ShowCursor(SDL_DISABLE);

    // Remove all clipping.
    sdl_no_clip(fe);

    // If we haven't already loaded the menu image...
    if(!menu_screen)
    {
        // Do so.
        if((menu_screen = IMG_Load(MENU_BACKGROUND_IMAGE)) == NULL)
        {
            printf("Unable to load %s: %s\n", LOADING_SCREEN_FILENAME, SDL_GetError());
            exit(EXIT_FAILURE);
        };
    };

    redraw_gamelist_menu(fe);
}

char *get_game_preview_data(frontend *fe, int game_index)
{
#ifdef DEBUG_FUNCTIONS
    printf("get_game_preview_data()\n");
#endif

    FILE *datafile;
    int comma_position,i;
    const int buffer_length=255;
    char str_buf[buffer_length];
    char *returned_string;

    returned_string=snewn(buffer_length,char);
    memset(returned_string, 0, buffer_length * sizeof(char));
    memset(str_buf, 0, buffer_length * sizeof(char));

    datafile = fopen(MENU_DATA_FILENAME, "r");
    if(datafile == NULL)
    {
        printf("Cannot open datafile for menu: %s\n",MENU_DATA_FILENAME);
        return(NULL);
    }
    else
    {
#ifdef DEBUG_FILE_ACCESS
        printf("Opened datafile for menu: %s\n",MENU_DATA_FILENAME);
#endif
    };

    // Read one line at a time from the file into a buffer
    while(fgets(str_buf, buffer_length, datafile) != NULL)
    {
        comma_position=0;

        for(i=0; i<buffer_length; i++)
        {
            if(str_buf[i]==',')
                comma_position=i;
            if(str_buf[i]=='\n')
                str_buf[i]='#';
        };

        if(comma_position == 0)
        {
            // Wasn't a valid data line
        }
        else
        {
            // If the first portion of the string (before the comma) matches the game
            // name that we are looking for.
            if(!strncmp(str_buf, (char *)gamelist[game_index]->htmlhelp_topic,max(sizeof((char *)gamelist[game_index]->htmlhelp_topic)-1,comma_position-1)))
            {
                // Return the portion of the string after the comma
                // WARNING: Potential buffer overflow?
                strncpy(returned_string,str_buf + comma_position+1,250);
            };
        };
    };

    fclose(datafile);
    return(returned_string);
};

void process_key(frontend *fe, int x, int y, int button)
{
#ifdef DEBUG_FUNCTIONS
    printf("process_key()\n");
#endif

    // We wrap this function because the midend returns 0 when it wants us to
    // quit (which for now is only if *we* press Q but you never know what might 
    // happen in the future).  It's called from lots of places so it's easier to
    // wrap it in a check function.

    if( midend_process_key(fe->me, x, y, button) == 0 )
    {
#ifdef DEBUG_MISC
        printf("Midend asked us to quit\n.");
#endif
        // Don't autosave because we'd auto-save a game that's asked us to quit!
        draw_menu(fe, GAMELISTMENU);
    };
}

void redraw_gamelist_menu(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("redraw_gamelist_menu()\n");
#endif

    int text_colour;
    char *preview_filename;
    char *sanitised_game_name;
    char *game_data;
    char *token_pointer;
    SDL_Surface *preview_window;
    uint current_y=0, data_current_y=0;
    SDL_Rect blit_rectangle;
    uint i,j;

    SDL_BlitSurface(menu_screen,NULL,screen,NULL);

    current_y=3*(MENU_FONT_SIZE + 2);

    fe->ncolours = 3;
    if(fe->sdlcolours != NULL)
        sfree(fe->sdlcolours);
    fe->sdlcolours = snewn(fe->ncolours, SDL_Color);
 
    // Background colour corresponding to 0.75
    fe->sdlcolours[0].r = 191;
    fe->sdlcolours[0].g = 191;
    fe->sdlcolours[0].b = 191;

    // White
    fe->sdlcolours[1].r = 255;
    fe->sdlcolours[1].g = 255;
    fe->sdlcolours[1].b = 255;

    // Black
    fe->sdlcolours[2].r = 0;
    fe->sdlcolours[2].g = 0;
    fe->sdlcolours[2].b = 0;

    fe->background_colour=0;
    fe->white_colour=1;
    fe->black_colour=2;

    if(first_run)
    {
        char *number_of_games;
        number_of_games=snewn(32,char);
        sprintf(number_of_games, "%u Games", gamecount);
        sdl_actual_draw_text(fe, 240-80, 190, FONT_FIXED, HELP_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->black_colour, stppc2x_ver);
        sdl_actual_draw_text(fe, 240-80, 190 + HELP_FONT_SIZE + 2, FONT_FIXED, HELP_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->black_colour, ver);
        sdl_actual_draw_text(fe, 240-80, 190 + 2*(HELP_FONT_SIZE + 2), FONT_FIXED, HELP_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->black_colour, number_of_games);
        sfree(number_of_games);

        preview_window=IMG_Load(MENU_ABOUT_IMAGE);
        if(preview_window!=NULL)
        {
            blit_rectangle.x=(screen_width - preview_window->w) / 2;
            blit_rectangle.y=(screen_height - preview_window->h) / 2;
            blit_rectangle.w=0;
            blit_rectangle.h=0;

            SDL_BlitSurface(preview_window, NULL, screen, &blit_rectangle);
            SDL_FreeSurface(preview_window);
        }
        else
        {
            printf("Could not open About image: %s\n", MENU_ABOUT_IMAGE);
        };
    }
    else
    {
        sdl_actual_draw_text(fe, 80, 2*MENU_FONT_SIZE+2, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->black_colour, UNICODE_UP_ARROW);
        sdl_actual_draw_text(fe, 100, 2*MENU_FONT_SIZE+2, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, "?");
        sdl_actual_draw_text(fe, 115, 2*MENU_FONT_SIZE+2, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, UNICODE_NUMERO);
#ifdef OPTION_SHOW_IF_MOUSE_NEEDED
        sdl_actual_draw_text(fe, 135, 2*MENU_FONT_SIZE+2, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, fe->white_colour, UNICODE_KEYBOARD);
#endif

        // Search through the list of games in the combined executable and look for a match.
        // Start from the selected game so that we give the impression of the menu scrolling.
        for(i=current_game_index, j=0;j<15;i=((i+1) % gamecount), j++)
        {
            if((i == (gamecount-1)) && (j<13))
            {
                sdl_actual_draw_line(fe, 10, current_y, 70, current_y, fe->black_colour);
            };

            if((int) i == current_game_index)
            {
                text_colour=fe->white_colour;
                game_data=get_game_preview_data(fe, current_game_index);
#ifdef DEBUG_FILE_ACCESS
                printf("Datafile line: %s\n", game_data);
#endif
                if((game_data == NULL) || (strlen(game_data) == 0))
                {
                    data_current_y=180;
                    sdl_actual_draw_text(fe, 240-80, data_current_y, FONT_FIXED, HELP_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->black_colour, "(no description)");
                    data_current_y+=HELP_FONT_SIZE+2;
                    sdl_actual_draw_text(fe, 240-80, data_current_y, FONT_FIXED, HELP_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->black_colour, stppc2x_ver);
                    data_current_y+=HELP_FONT_SIZE+2;
                    sdl_actual_draw_text(fe, 240-80, data_current_y, FONT_FIXED, HELP_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->black_colour, ver);
                }
                else
                {
                    data_current_y=180;
                    token_pointer=strtok(game_data, "#");
                    while(token_pointer != NULL)
                    {
                        sdl_actual_draw_text(fe, 240-80, data_current_y, FONT_FIXED, HELP_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->black_colour, token_pointer);
#ifdef DEBUG_FILE_ACCESS
                        printf("Datafile token: %s\n", token_pointer);
#endif
                        token_pointer=strtok(NULL, "#");
                        data_current_y+=HELP_FONT_SIZE+2;
                    };
                };
                sfree(game_data);
            }
            else
            {
                text_colour=fe->black_colour;
            };
            if(current_y < (MENU_FONT_SIZE+2)*16)
            {
                sdl_actual_draw_text(fe, 10, current_y, FONT_FIXED, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, text_colour, (char *)gamelist[i]->name);
#ifdef OPTION_SHOW_IF_MOUSE_NEEDED
                if(!(gamelist[i]->flags & REQUIRE_MOUSE_INPUT))
                    sdl_actual_draw_text(fe, 140, current_y, FONT_FIXED, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, text_colour, UNICODE_TICK_CHAR);
#endif
//                if(gamelist[i]->flags & REQUIRE_NUMPAD)
//                    sdl_actual_draw_text(fe, 120, current_y, FONT_FIXED, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, text_colour, UNICODE_TICK_CHAR);
//                if(gamelist[i]->can_solve)
//                    sdl_actual_draw_text(fe, 100, current_y, FONT_FIXED, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HLEFT, text_colour, UNICODE_TICK_CHAR);
            }
            current_y+=(MENU_FONT_SIZE+2);
        };

        sdl_actual_draw_text(fe, 80, (MENU_FONT_SIZE+2)*16, FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->black_colour, UNICODE_DOWN_ARROW);

        preview_filename=snewn(MAX_GAMENAME_SIZE+sizeof(MENU_PREVIEW_IMAGES)+1,char);
        sanitised_game_name=sanitise_game_name((char *) gamelist[current_game_index]->htmlhelp_topic);
        sprintf(preview_filename, MENU_PREVIEW_IMAGES, sanitised_game_name);
        sfree(sanitised_game_name);

#ifdef DEBUG_FILE_ACCESS
        printf("Loading preview: %s\n", preview_filename);
#endif

        preview_window=IMG_Load(preview_filename);
        if(preview_window!=NULL)
        {
            blit_rectangle.w=0;
            blit_rectangle.h=0;
            blit_rectangle.x=164-80;
            blit_rectangle.y=8;

            if(preview_window->w < 150)
                blit_rectangle.x += (150 - preview_window->w) / 2;

            if(preview_window->h < 150)
                blit_rectangle.y += (150 - preview_window->h) / 2;

            SDL_BlitSurface(preview_window, NULL, screen, &blit_rectangle);
            SDL_FreeSurface(preview_window);
        }
        else
        {
            // 164 = preview offset x
            // 8 = preview offset y
            // 150 = preview png width/height
            // 239 = 164 + (150 / 2)
            sdl_actual_draw_rect(fe, 164-80, 8, 150, 150, fe->background_colour);
            sdl_actual_draw_text(fe, 239-80, 8 + 5*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->black_colour, "No preview");
            sdl_actual_draw_text(fe, 239-80, 8 + 6*(MENU_FONT_SIZE+2), FONT_VARIABLE, MENU_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HCENTRE, fe->black_colour, "image available");
        };
        sfree(preview_filename);
    };
    sdl_end_draw(fe);
}

void start_game(frontend *fe, int game_index, uint skip_config)
{
#ifdef DEBUG_FUNCTIONS
    printf("start_game()\n");
#endif

    int black_found=FALSE, white_found=FALSE;
    int x, y;

    int i, ncolours;
    float *colours;

    cleanup(fe);

    this_game=*gamelist[game_index];
    printf("Starting game: %s\n", this_game.name);
    if(skip_config)   
        printf("(Skipping config by user request)\n");

    // If this game requires a large screen
    if(this_game.flags & REQUIRE_LARGE_SCREEN)
    {
        printf("Game requires a larger screen.\n");
        screen_width=SCREEN_WIDTH_LARGE;
        screen_height=SCREEN_HEIGHT_LARGE;

#ifdef SCALELARGESCREEN
        real_screen=screen;
        screen = SDL_CreateRGBSurface(SDL_SURFACE_FLAGS & (!SDL_DOUBLEBUF), SCREEN_WIDTH_LARGE, SCREEN_HEIGHT_LARGE, SCREEN_DEPTH, 0, 0, 0, 0);
#else
        // This line crashes as root, okay as normal user.
        screen = SDL_SetVideoMode(SCREEN_WIDTH_LARGE, SCREEN_HEIGHT_LARGE, SCREEN_DEPTH, SDL_SURFACE_FLAGS);
#endif

        // If the video mode was initialised successfully
        if(screen)
        {
            fe->screen = screen;
            printf("Initialised %u x %u @ %u bit video mode.\n", screen->w, screen->h, SCREEN_DEPTH);
#ifdef SCALELARGESCREEN
            printf("(FAKED)!\n");
#endif 
        }
        else
        {
            printf("Error initialising %u x %u @ %u bit video mode: %s\n", screen->w, screen->h, SCREEN_DEPTH, SDL_GetError());
#ifdef SCALELARGESCREEN
            printf("(FAKED)!\n");
#endif 
            exit(EXIT_FAILURE);
        };

        STATUSBAR_FONT_SIZE = 2 * DEFAULT_STATUSBAR_FONT_SIZE;
        MENU_FONT_SIZE = 2 * DEFAULT_MENU_FONT_SIZE;
        HELP_FONT_SIZE = 2 * DEFAULT_HELP_FONT_SIZE;
        KEYBOARD_ICON_FONT_SIZE = 2 * DEFAULT_KEYBOARD_ICON_FONT_SIZE;
    };
    printf("Initialising '%s'.\n", this_game.name);

    start_loading_animation(fe);

    fe->me = midend_new(fe, &this_game, &sdl_drawing, fe);

    // Get the colours that the midend thinks it needs.
    colours = midend_colours(fe->me, &ncolours);

    gettimeofday(&fe->last_statusbar_update,NULL);

    fe->ncolours = ncolours;
#ifdef DEBUG_PALETTE
    printf("Game palette has %u colours.\n", ncolours);
#endif
    fe->sdlcolours = snewn(fe->ncolours, SDL_Color);

    // Plug them into an array of SDL colours, so that we can use either
    // interchangably when given a colour index.
    for (i = 0; i < ncolours; i++)
    {
        // Convert them from game-style floats from 0.0 to 1.0 to SDL-style bytes.
        fe->sdlcolours[i].r = (Uint8)(colours[i*3] * 0xFF);
        fe->sdlcolours[i].g = (Uint8)(colours[i*3+1] * 0xFF);
        fe->sdlcolours[i].b = (Uint8)(colours[i*3+2] * 0xFF);

#ifdef DEBUG_PALETTE
        if(i==0)
            printf("Background: %u, %u, %u\n", fe->sdlcolours[i].r, fe->sdlcolours[i].g, fe->sdlcolours[i].b);
#endif

        // If we find a pure black colour, remember it
        if((!black_found) && (fe->sdlcolours[i].r == 0) && (fe->sdlcolours[i].g == 0) && (fe->sdlcolours[i].b==0))
        {
            fe->black_colour=i;
#ifdef DEBUG_PALETTE
            printf("Black is colour index %u\n",i);
#endif
            black_found=TRUE;
        };

        // If we find a white-like colour, remember it
        if((!white_found) && (fe->sdlcolours[i].r == 0xFF) && (fe->sdlcolours[i].g == 0xFF) && (fe->sdlcolours[i].b==0xFF))
        {
            fe->white_colour=i;
#ifdef DEBUG_PALETTE
            printf("White is colour index %u\n",i);
#endif
            white_found=TRUE;
        };
    };

    // If we didn't find white
    if(!white_found)
    {
#ifdef DEBUG_PALETTE
        printf("Did not find white colour in the palette.\nTacking it to the end of the %u colour palette.\n",fe->ncolours);
#endif

        // Create a white colour and add it to the end of the SDL palette.
        fe->ncolours++;
        fe->sdlcolours = sresize(fe->sdlcolours, fe->ncolours, SDL_Color);

        fe->sdlcolours[fe->ncolours - 1].r = 255;
        fe->sdlcolours[fe->ncolours - 1].g = 255;
        fe->sdlcolours[fe->ncolours - 1].b = 255;

        fe->white_colour=fe->ncolours -1;
#ifdef DEBUG_PALETTE
        printf("White is now colour index %u\n",fe->white_colour);
#endif
    };

    // If we didn't find black
    if(!black_found)
    {
#ifdef DEBUG_PALETTE
        printf("Did not find black colour in the palette.\nTacking it to the end of the %u colour palette.\n",fe->ncolours);
#endif
        // Create a black colour and add it to the end of the SDL palette.
        fe->ncolours++;
        fe->sdlcolours = sresize(fe->sdlcolours, fe->ncolours, SDL_Color);

        fe->sdlcolours[fe->ncolours - 1].r = 0;
        fe->sdlcolours[fe->ncolours - 1].g = 0;
        fe->sdlcolours[fe->ncolours - 1].b = 0;

        fe->black_colour=fe->ncolours - 1;
#ifdef DEBUG_PALETTE
        printf("Black is now colour index %u\n",fe->black_colour);
#endif
    };

    sfree(colours);

    // Generate a new game.
    midend_new_game(fe->me);

    // Get the size of the game.
    get_size(fe, &x, &y);
    fe->w=x;
    fe->h=y;

    // Various initialisations that the compiler should moan about but doesn't.  Without
    // these, things start to crash because they assume they are 0/NULL/False at program
    // start.
    fe->last_status_bar_w=0;
    fe->last_status_bar_h=0;
    fe->paused=FALSE;

    // Check for a joystick
    if(SDL_NumJoysticks()>0)
    {
        // Open first joystick
        fe->joy=SDL_JoystickOpen(0);

        if(fe->joy)
        {
            printf("Initialised Joystick 0.\n");
        }
        else
        {
            printf("Error initialising Joystick 0.\n");
            cleanup_and_exit(fe, EXIT_FAILURE);
        };
    };

    // Cache the sanitised game name.
    fe->sanitised_game_name=sanitise_game_name((char *) this_game.htmlhelp_topic);

    // Index of the background colour in colours array.
    // (I'm not sure this is "defined" as being the background colour but it works)
    fe->background_colour=0;

    fe->first_preset_showing=0;

    // Initialise the font structure so that we can load our first font.
    fe->fonts=snewn(1,struct font);
    fe->nfonts=0;

    // Cache all the important fonts.
    find_and_cache_font(fe, FONT_VARIABLE, STATUSBAR_FONT_SIZE);
    find_and_cache_font(fe, FONT_VARIABLE, MENU_FONT_SIZE);
    find_and_cache_font(fe, FONT_VARIABLE, HELP_FONT_SIZE);

    // Get configuration options from the midend.
    fe->cfg=midend_get_config(fe->me, CFG_SETTINGS, &fe->configure_window_title);

    if(!skip_config)
    {
        // Load up the default config from an INI file (if it exists, works, etc.)
        // This will overwrite the default configuration and start a new game if it works.
        load_config_from_INI(fe);
        if(autosave_file_exists(fe->sanitised_game_name) && (global_config->always_load_autosave))
            load_autosave_game(fe);
    };

    // Size the game for the current screen.
    configure_area(screen_width, screen_height, fe);

    // Blank over the whole screen using the frontend "rect" routine with background colour.
    sdl_no_clip(fe);
    sdl_actual_draw_rect(fe, 0, 0, screen_width, screen_height, fe->background_colour);

    // Set the clipping region for SDL so that we start with a clipping region
    // that prevents blit garbage at the edges of the game window.
    sdl_unclip(fe);

    // Force the game to redraw itself.
    midend_force_redraw(fe->me);

    draw_menu(fe, INGAME);

    // Turn on the cursor.
    if(global_config->control_system != CURSOR_KEYS_EMULATION)
        SDL_ShowCursor(SDL_ENABLE);

    stop_loading_animation();
}


void delete_ini_file(frontend *fe, int game_index)
{
// SAFER ALTERNATIVE WAS IMPLEMENTED.
/*
    // Delete the config for the selected game.

    if(game_index >= 0)
    {
        char *filename;

        // Double-check that we have the sanitised game name loaded.
        fe->sanitised_game_name=sanitise_game_name((char *) gamelist[game_index]->htmlhelp_topic);

        // Dynamically allocate space for a filename and add .ini to 
        // the end of the sanitised game name to get the filename 
        // we will use.
        filename=snewn(MAX_GAMENAME_SIZE + 5,char);
        sprintf(filename, "%.*s.ini", MAX_GAMENAME_SIZE, fe->sanitised_game_name);
  
        // Check the file exists by opening the file, readonly
        FILE *fp = fopen(filename, "r");

        // If it didn't open
        if(fp)
        {
            // Close the file
            fclose(fp);
            if(remove(filename) == 0)
                printf("Configuration file %s has been deleted at user's request.\n", filename);
            else
                printf("Configuration file %s could not be deleted (write-protected?).\n", filename);
        }
        else
            printf("Configuration file %s could not be deleted (not found).\n", filename);
        sfree(filename);
    };
*/
};

void delete_savegame(frontend *fe, uint saveslot_number)
{
#ifdef DEBUG_FUNCTIONS
    printf("delete_savegame()\n");
#endif

    // Delete the selected savegame.

    if((saveslot_number >= 0) && (savefile_exists(fe->sanitised_game_name, saveslot_number)))
    {
        char *filename;

        if(fe->sanitised_game_name)
            sfree(fe->sanitised_game_name);

        // Double-check that we have the sanitised game name loaded.
        fe->sanitised_game_name=sanitise_game_name((char *) gamelist[current_game_index]->htmlhelp_topic);

        // Dynamically allocate space for a filename and add .ini to 
        // the end of the sanitised game name to get the filename 
        // we will use.
        filename=generate_save_filename(fe->sanitised_game_name, saveslot_number);
  
#ifdef DEBUG_FILE_ACCESS
        printf("DELETING %s!!\n", filename);
#endif

        // Check the file exists by opening the file, readonly
        FILE *fp = fopen(filename, "r");

        // If it didn't open
        if(fp)
        {
            // Close the file
            fclose(fp);
            if(remove(filename) == 0)
                printf("Configuration file %s has been deleted at user's request.\n", filename);
            else
                printf("Configuration file %s could not be deleted (write-protected?).\n", filename);
        }
        else
            printf("Configuration file %s could not be deleted (not found).\n", filename);
        sfree(filename);
    };
};

void delete_autosave_game(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("delete_autosave_game()\n");
#endif

    char *save_filename;

    save_filename=snewn(MAX_GAMENAME_SIZE + 10, char);

    // Double-check that we have the sanitised game name loaded.
    fe->sanitised_game_name=sanitise_game_name((char *) gamelist[current_game_index]->htmlhelp_topic);

    if(autosave_file_exists(fe->sanitised_game_name))
    {
        sprintf(save_filename, "%.*s.autosave", MAX_GAMENAME_SIZE, fe->sanitised_game_name);
#ifdef DEBUG_FILE_ACCESS
        printf("DELETING %s!!\n", save_filename);
#endif
        if(remove(save_filename) == 0)
            printf("Autosave %s has been deleted at user's request.\n", save_filename);
        else
            printf("Autosave %s could not be deleted (write-protected?).\n", save_filename);
    }
    else
    {
            printf("Autosave %s could not be deleted (not found).\n", save_filename);
    };
    sfree(save_filename);
};

#ifdef BACKGROUND_MUSIC

void play_first_track()
{
#ifdef DEBUG_FUNCTIONS
    printf("play_first_track()\n");
#endif

    int first_track=0;
    int i;

    for(i=0;i<10;i++)
    {
        if(global_config->tracks_to_play[i]==TRUE)
        {
            first_track=i+1;
            break;
        };
    };

    if(first_track > 0)
    {
        play_music_track(first_track);
    }
    else
    {
        printf("No available music tracks selected for playing.\n");
        global_config->play_music=FALSE;
        current_music_track=0;
        music_track_changed=TRUE;
    };
};

void play_next_track()
{
#ifdef DEBUG_FUNCTIONS
    printf("play_next_track()\n");
#endif

    uint i;
    uint found_next_track=FALSE;
    for(i=current_music_track;i<10;i++)
    {
        char *current_track_filename = get_music_filename(i);
        char *current_track_full_filename = snewn(255,char);
        sprintf(current_track_full_filename, "%s%s", MUSIC_PATH, current_track_filename);
        if(file_exists(current_track_full_filename) && (global_config->tracks_to_play[i]==TRUE))
        {
            play_music_track(i+1);
            found_next_track=TRUE;
            sfree(current_track_filename);
            sfree(current_track_full_filename);
            break;
        }
        else
        {
            sfree(current_track_filename);
            sfree(current_track_full_filename);
        };
    };
    if(!found_next_track)
        play_first_track();
}


void play_music_track(uint track_number)
{
#ifdef DEBUG_FUNCTIONS
    printf("play_music_track()\n");
#endif

    printf("Starting music track %u\n", track_number);
    stop_background_music();
    while(Mix_PlayingMusic());

    global_config->play_music=TRUE;
    if(music == NULL)
    {
        char *track_filename = get_music_filename(track_number);
        if(track_filename !=NULL)
        {
            char *full_track_filename=snewn(255,char);
            memset(full_track_filename, 0, 255 * sizeof(char));
            sprintf(full_track_filename, "%s%s", MUSIC_PATH, track_filename);
            printf("Music track %u info: %s\n", track_number, full_track_filename);

            music = Mix_LoadMUS(full_track_filename);
            if(!music)
            {
                printf("Loading music failed: %s\n", Mix_GetError());
                current_music_track=0;
            }
            else
            {
                // -1 = loop infinitely.
                Mix_PlayMusic(music, 1);
                Mix_VolumeMusic(global_config->music_volume);
                current_music_track = track_number;

                Mix_HookMusicFinished(background_music_callback);
            };
            sfree(track_filename);
            sfree(full_track_filename);
            music_track_changed=TRUE;
        };
    };
};

void initialise_audio()
{
#ifdef DEBUG_FUNCTIONS
    printf("initialise_audio()\n");
#endif

    int audio_rate = MIX_DEFAULT_FREQUENCY;
    Uint16 audio_format = MIX_DEFAULT_FORMAT;
    int audio_channels = 2;
    int audio_buffers = 8192;

    printf("Initialising audio...\n");

    if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0)
    {
        printf("Unable to open audio!\n");
        exit(1);
    };

    printf("%i audio channels available\n", Mix_AllocateChannels(-1));
};

void stop_audio()
{
#ifdef DEBUG_FUNCTIONS
    printf("stop_audio()\n");
#endif

    Mix_CloseAudio();
};

void start_background_music()
{
#ifdef DEBUG_FUNCTIONS
    printf("start_background_music()\n");
#endif

    play_first_track();
};

void stop_background_music()
{
#ifdef DEBUG_FUNCTIONS
    printf("stop_background_music()\n");
#endif

    if(music != NULL)
    {
        fade_out_background_music();
        Mix_HaltMusic();
        Mix_FreeMusic(music);
        music = NULL;
        current_music_track = 0;
    };
};

/*
void play_sound()
{
#ifdef DEBUG_FUNCTIONS
    printf("play_sound()\n");
#endif

    Mix_Chunk *sound = NULL;
    int channel;
 
    sound = Mix_LoadWAV("sound.wav");
    if(sound == NULL)
    {
	printf("Unable to load sound file: %s\n", Mix_GetError());
    }
    else
    {
        channel = Mix_PlayChannel(-1, sound, 0);
        if(channel == -1)
        printf("Unable to play WAV file: %s\n", Mix_GetError());

        while(Mix_Playing(channel) != 0);
 
        Mix_FreeChunk(sound); 
    };
};
*/

void background_music_callback()
{
#ifdef DEBUG_FUNCTIONS
    printf("background_music_callback()\n");
#endif

    play_next_track();
};

void fade_out_background_music()
{
#ifdef DEBUG_FUNCTIONS
    printf("fade_out_background_music()\n");
#endif

    while(!Mix_FadeOutMusic(1000) && Mix_PlayingMusic())
    {
        // wait for any fades to complete
        SDL_Delay(100);
    };
};
#endif //ifdef BACKGROUND_MUSIC

void list_music_files()
{
#ifdef DEBUG_FUNCTIONS
    printf("list_music_files()\n");
#endif

    struct dirent *file_list;
    DIR *directory;
    directory = opendir(MUSIC_PATH);
    if(directory == NULL)
    {
#ifdef DEBUG_FILE_ACCESS
       printf("Reading the music folder %s failed\n", MUSIC_PATH);
#endif
       return;
    };

    file_list = readdir(directory);
    while(file_list != NULL)
    {
        if( (strcmp(".", file_list->d_name)==0) || (strcmp("..", file_list->d_name)==0))
        {
             // Ignore it, it's a folder
        }
        else
        {
#ifdef DEBUG_FILE_ACCESS
            printf("%s\n", file_list->d_name);
#endif
        };
        file_list = readdir(directory);
    };
    closedir(directory);
};

char *get_music_filename(uint track_number)
{
#ifdef DEBUG_FUNCTIONS
    printf("get_music_filename()\n");
#endif

    struct dirent *file_list;
    DIR *directory;
    directory = opendir(MUSIC_PATH);
    char *track_filename=snewn(255,char);
    char *track_number_as_string=snewn(3, char);
    uint track_found=FALSE;

    sprintf(track_number_as_string, "%.2u", track_number);

#ifdef DEBUG_FILE_ACCESS
    printf("Looking for track %s\n", track_number_as_string);
#endif

    if(directory == NULL)
    {
#ifdef DEBUG_FILE_ACCESS
       printf("Reading the music folder %s failed\n", MUSIC_PATH);
#endif
       return(NULL);
    };

    file_list = readdir(directory);
    while(file_list != NULL)
    {
        if( (strcmp(".", file_list->d_name)==0) || (strcmp("..", file_list->d_name)==0))
        {
             // Ignore it, it's a folder
        }
        else
        {
            // If the first two character match the number we are looking for...
            if(strncmp(track_number_as_string, file_list->d_name, 2) == 0)
            {
                strncpy(track_filename, file_list->d_name, 250);
#ifdef DEBUG_FILE_ACCESS
                printf("Found: %s\n", file_list->d_name);
#endif
                track_found=TRUE;
            };
        };
        file_list = readdir(directory);
    };
    sfree(track_number_as_string);
    closedir(directory);
    if(track_found==TRUE)
        return(track_filename);
    else
    {
        sfree(track_filename);
        return(NULL);
    }
};

void print_game_as_text(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("print_game_as_text()\n");
#endif

    char *text_format=midend_text_format(fe->me);
    if(text_format != NULL)
        printf(text_format);
    sfree(text_format);
};

void show_keyboard_icon(frontend *fe)
{
#ifdef DEBUG_FUNCTIONS
    printf("show_keyboard_icon()\n");
#endif

    if((current_screen == GAMELISTMENU) || (current_screen == INGAME))
        return;
    if(global_config->control_system == CURSOR_KEYS_EMULATION)
    {
        sdl_actual_draw_text(fe, screen_width, KEYBOARD_ICON_FONT_SIZE, FONT_VARIABLE, KEYBOARD_ICON_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HRIGHT, fe->white_colour, UNICODE_KEYBOARD);
    }
    else
    {
#ifdef OPTION_CHECK_HARDWARE
        if(hardware_type == GP2X_F200)
#endif 
        sdl_actual_draw_text(fe, screen_width, KEYBOARD_ICON_FONT_SIZE, FONT_VARIABLE, KEYBOARD_ICON_FONT_SIZE, ALIGN_VNORMAL | ALIGN_HRIGHT, fe->white_colour, UNICODE_STYLUS);
    };
};

// Called if the game thinks it has been completed.
// Not called from Black Box, Loopy, Maze 3D, Slide, Sokoban
void game_completed()
{
#ifdef DEBUG_FUNCTIONS
    printf("game_completed()\n");
#endif
  // printf("Game completed!\n");
}

// Function prototypes

#ifdef DEBUGGING
void dputs(char *buf);
void debug_printf(char *fmt, ...);
#endif

void sdl_actual_draw_text(void *handle, int x, int y, int fonttype, int fontsize, int align, int colour, char *text);
void fatal(char *fmt, ...);
void Lock_SDL_Surface(SDL_Surface *screen_to_lock);
void Unlock_SDL_Surface(SDL_Surface *screen_to_unlock);
void get_random_seed(void **randseed, int *randseedsize);
void frontend_default_colour(frontend *fe, float *output);
void sdl_start_draw(void *handle);
void sdl_no_clip(void *handle);
void sdl_clip(void *handle, int x, int y, int w, int h);
void sdl_actual_clip(void *handle, int x, int y, int w, int h);
void sdl_unclip(void *handle);
void sdl_draw_text(void *handle, int x, int y, int fonttype, int fontsize, int align, int colour, char *text);
void sdl_actual_draw_text(void *handle, int x, int y, int fonttype, int fontsize, int align, int colour, char *text);
void sdl_draw_rect(void *handle, int x, int y, int w, int h, int colour);
void sdl_actual_draw_rect(void *handle, int x, int y, int w, int h, int colour);
void sdl_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour);
void sdl_actual_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour);
void sdl_draw_poly(void *handle, int *coords, int npoints, int fillcolour, int outlinecolour);
void sdl_draw_circle(void *handle, int cx, int cy, int radius, int fillcolour, int outlinecolour);
void sdl_status_bar(void *handle, char *text);
blitter *sdl_blitter_new(void *handle, int w, int h);
void sdl_blitter_free(void *handle, blitter *bl);
void sdl_blitter_save(void *handle, blitter *bl, int x, int y);
void sdl_blitter_load(void *handle, blitter *bl, int x, int y);
void sdl_draw_update(void *handle, int x, int y, int w, int h);
void sdl_end_draw(void *handle);
static void configure_area(int x, int y, void *data);
Uint32 sdl_timer_func(Uint32 interval, void *data);
Uint32 sdl_mouse_timer_func(Uint32 interval, void *data);
void deactivate_timer(frontend *fe);
void activate_timer(frontend *fe);
static void get_size(frontend *fe, int *px, int *py);
int read_game_helpfile(void *handle);
static frontend *new_window(char *arg, int argtype, char **error);
void game_pause(frontend *fe);
void game_quit();
void Main_SDL_Loop(void *handle);
int main(int argc, char **argv);
int load_config_from_INI(frontend *fe);
int save_config_to_INI(frontend *fe);
char* sanitise_game_name();
void save_game(frontend *fe, Uint8 saveslot_number);
void load_game(frontend *fe, Uint8 saveslot_number);

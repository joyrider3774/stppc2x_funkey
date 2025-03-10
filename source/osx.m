/*
 * Mac OS X / Cocoa front end to puzzles.
 *
 * Still to do:
 * 
 *  - I'd like to be able to call up context help for a specific
 *    game at a time.
 * 
 * Mac interface issues that possibly could be done better:
 * 
 *  - is there a better approach to frontend_default_colour?
 *
 *  - do we need any more options in the Window menu?
 *
 *  - can / should we be doing anything with the titles of the
 *    configuration boxes?
 * 
 *  - not sure what I should be doing about default window
 *    placement. Centring new windows is a bit feeble, but what's
 *    better? Is there a standard way to tell the OS "here's the
 *    _size_ of window I want, now use your best judgment about the
 *    initial position"?
 *     + there's a standard _policy_ on window placement, given in
 * 	 the HI guidelines. Have to implement it ourselves though,
 * 	 bah.
 *
 *  - a brief frob of the Mac numeric keypad suggests that it
 *    generates numbers no matter what you do. I wonder if I should
 *    try to figure out a way of detecting keypad codes so I can
 *    implement UP_LEFT and friends. Alternatively, perhaps I
 *    should simply assign the number keys to UP_LEFT et al?
 *    They're not in use for anything else right now.
 *
 *  - see if we can do anything to one-button-ise the multi-button
 *    dependent puzzle UIs:
 *     - Pattern is a _little_ unwieldy but not too bad (since
 * 	 generally you never need the middle button unless you've
 * 	 made a mistake, so it's just click versus command-click).
 *     - Net is utterly vile; having normal click be one rotate and
 * 	 command-click be the other introduces a horrid asymmetry,
 * 	 and yet requiring a shift key for _each_ click would be
 * 	 even worse because rotation feels as if it ought to be the
 * 	 default action. I fear this is why the Flash Net had the
 * 	 UI it did...
 * 	  + I've tried out an alternative dragging interface for
 * 	    Net; it might work nicely for stylus-based platforms
 * 	    where you have better hand/eye feedback for the thing
 * 	    you're clicking on, but it's rather unwieldy on the
 * 	    Mac. I fear even shift-clicking is better than that.
 *
 *  - Should we _return_ to a game configuration sheet once an
 *    error is reported by midend_set_config, to allow the user to
 *    correct the one faulty input and keep the other five OK ones?
 *    The Apple `one sheet at a time' restriction would require me
 *    to do this by closing the config sheet, opening the alert
 *    sheet, and then reopening the config sheet when the alert is
 *    closed; and the human interface types, who presumably
 *    invented the one-sheet-at-a-time rule for good reasons, might
 *    look with disfavour on me trying to get round them to fake a
 *    nested sheet. On the other hand I think there are good
 *    practical reasons for wanting it that way. Uncertain.
 * 
 *  - User feedback dislikes nothing happening when you start the
 *    app; they suggest a finder-like window containing an icon for
 *    each puzzle type, enabling you to start one easily. Needs
 *    thought.
 * 
 * Grotty implementation details that could probably be improved:
 * 
 *  - I am _utterly_ unconvinced that NSImageView was the right way
 *    to go about having a window with a reliable backing store! It
 *    just doesn't feel right; NSImageView is a _control_. Is there
 *    a simpler way?
 * 
 *  - Resizing is currently very bad; rather than bother to work
 *    out how to resize the NSImageView, I just splatter and
 *    recreate it.
 */

#include <ctype.h>
#include <sys/time.h>
#import <Cocoa/Cocoa.h>
#include "puzzles.h"

/* ----------------------------------------------------------------------
 * Global variables.
 */

/*
 * The `Type' menu. We frob this dynamically to allow the user to
 * choose a preset set of settings from the current game.
 */
NSMenu *typemenu;

/*
 * Forward reference.
 */
extern const struct drawing_api osx_drawing;

/* ----------------------------------------------------------------------
 * Miscellaneous support routines that aren't part of any object or
 * clearly defined subsystem.
 */

void fatal(char *fmt, ...)
{
    va_list ap;
    char errorbuf[2048];
    NSAlert *alert;

    va_start(ap, fmt);
    vsnprintf(errorbuf, lenof(errorbuf), fmt, ap);
    va_end(ap);

    alert = [NSAlert alloc];
    /*
     * We may have come here because we ran out of memory, in which
     * case it's entirely likely that that alloc will fail, so we
     * should have a fallback of some sort.
     */
    if (!alert) {
	fprintf(stderr, "fatal error (and NSAlert failed): %s\n", errorbuf);
    } else {
	alert = [[alert init] autorelease];
	[alert addButtonWithTitle:@"Oh dear"];
	[alert setInformativeText:[NSString stringWithCString:errorbuf]];
	[alert runModal];
    }
    exit(1);
}

void frontend_default_colour(frontend *fe, float *output)
{
    /* FIXME: Is there a system default we can tap into for this? */
    output[0] = output[1] = output[2] = 0.8F;
}

void get_random_seed(void **randseed, int *randseedsize)
{
    time_t *tp = snew(time_t);
    time(tp);
    *randseed = (void *)tp;
    *randseedsize = sizeof(time_t);
}

static void savefile_write(void *wctx, void *buf, int len)
{
    FILE *fp = (FILE *)wctx;
    fwrite(buf, 1, len, fp);
}

static int savefile_read(void *wctx, void *buf, int len)
{
    FILE *fp = (FILE *)wctx;
    int ret;

    ret = fread(buf, 1, len, fp);
    return (ret == len);
}

/*
 * Since this front end does not support printing (yet), we need
 * this stub to satisfy the reference in midend_print_puzzle().
 */
void document_add_puzzle(document *doc, const game *game, game_params *par,
			 game_state *st, game_state *st2)
{
}

/*
 * setAppleMenu isn't listed in the NSApplication header, but an
 * NSApp responds to it, so we're adding it here to silence
 * warnings. (This was removed from the headers in 10.4, so we
 * only need to include it for 10.4+.)
 */
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1040
@interface NSApplication(NSAppleMenu)
- (void)setAppleMenu:(NSMenu *)menu;
@end
#endif

/* ----------------------------------------------------------------------
 * Tiny extension to NSMenuItem which carries a payload of a `void
 * *', allowing several menu items to invoke the same message but
 * pass different data through it.
 */
@interface DataMenuItem : NSMenuItem
{
    void *payload;
}
- (void)setPayload:(void *)d;
- (void *)getPayload;
@end
@implementation DataMenuItem
- (void)setPayload:(void *)d
{
    payload = d;
}
- (void *)getPayload
{
    return payload;
}
@end

/* ----------------------------------------------------------------------
 * Utility routines for constructing OS X menus.
 */

NSMenu *newmenu(const char *title)
{
    return [[[NSMenu allocWithZone:[NSMenu menuZone]]
	     initWithTitle:[NSString stringWithCString:title]]
	    autorelease];
}

NSMenu *newsubmenu(NSMenu *parent, const char *title)
{
    NSMenuItem *item;
    NSMenu *child;

    item = [[[NSMenuItem allocWithZone:[NSMenu menuZone]]
	     initWithTitle:[NSString stringWithCString:title]
	     action:NULL
	     keyEquivalent:@""]
	    autorelease];
    child = newmenu(title);
    [item setEnabled:YES];
    [item setSubmenu:child];
    [parent addItem:item];
    return child;
}

id initnewitem(NSMenuItem *item, NSMenu *parent, const char *title,
	       const char *key, id target, SEL action)
{
    unsigned mask = NSCommandKeyMask;

    if (key[strcspn(key, "-")]) {
	while (*key && *key != '-') {
	    int c = tolower((unsigned char)*key);
	    if (c == 's') {
		mask |= NSShiftKeyMask;
	    } else if (c == 'o' || c == 'a') {
		mask |= NSAlternateKeyMask;
	    }
	    key++;
	}
	if (*key)
	    key++;
    }

    item = [[item initWithTitle:[NSString stringWithCString:title]
	     action:NULL
	     keyEquivalent:[NSString stringWithCString:key]]
	    autorelease];

    if (*key)
	[item setKeyEquivalentModifierMask: mask];

    [item setEnabled:YES];
    [item setTarget:target];
    [item setAction:action];

    [parent addItem:item];

    return item;
}

NSMenuItem *newitem(NSMenu *parent, char *title, char *key,
		    id target, SEL action)
{
    return initnewitem([NSMenuItem allocWithZone:[NSMenu menuZone]],
		       parent, title, key, target, action);
}

/* ----------------------------------------------------------------------
 * About box.
 */

@class AboutBox;

@interface AboutBox : NSWindow
{
}
- (id)init;
@end

@implementation AboutBox
- (id)init
{
    NSRect totalrect;
    NSView *views[16];
    int nviews = 0;
    NSImageView *iv;
    NSTextField *tf;
    NSFont *font1 = [NSFont systemFontOfSize:0];
    NSFont *font2 = [NSFont boldSystemFontOfSize:[font1 pointSize] * 1.1];
    const int border = 24;
    int i;
    double y;

    /*
     * Construct the controls that go in the About box.
     */

    iv = [[NSImageView alloc] initWithFrame:NSMakeRect(0,0,64,64)];
    [iv setImage:[NSImage imageNamed:@"NSApplicationIcon"]];
    views[nviews++] = iv;

    tf = [[NSTextField alloc]
	  initWithFrame:NSMakeRect(0,0,400,1)];
    [tf setEditable:NO];
    [tf setSelectable:NO];
    [tf setBordered:NO];
    [tf setDrawsBackground:NO];
    [tf setFont:font2];
    [tf setStringValue:@"Simon Tatham's Portable Puzzle Collection"];
    [tf sizeToFit];
    views[nviews++] = tf;

    tf = [[NSTextField alloc]
	  initWithFrame:NSMakeRect(0,0,400,1)];
    [tf setEditable:NO];
    [tf setSelectable:NO];
    [tf setBordered:NO];
    [tf setDrawsBackground:NO];
    [tf setFont:font1];
    [tf setStringValue:[NSString stringWithCString:ver]];
    [tf sizeToFit];
    views[nviews++] = tf;

    /*
     * Lay the controls out.
     */
    totalrect = NSMakeRect(0,0,0,0);
    for (i = 0; i < nviews; i++) {
	NSRect r = [views[i] frame];
	if (totalrect.size.width < r.size.width)
	    totalrect.size.width = r.size.width;
	totalrect.size.height += border + r.size.height;
    }
    totalrect.size.width += 2 * border;
    totalrect.size.height += border;
    y = totalrect.size.height;
    for (i = 0; i < nviews; i++) {
	NSRect r = [views[i] frame];
	r.origin.x = (totalrect.size.width - r.size.width) / 2;
	y -= border + r.size.height;
	r.origin.y = y;
	[views[i] setFrame:r];
    }

    self = [super initWithContentRect:totalrect
	    styleMask:(NSTitledWindowMask | NSMiniaturizableWindowMask |
		       NSClosableWindowMask)
	    backing:NSBackingStoreBuffered
	    defer:YES];

    for (i = 0; i < nviews; i++)
	[[self contentView] addSubview:views[i]];

    [self center];		       /* :-) */

    return self;
}
@end

/* ----------------------------------------------------------------------
 * The front end presented to midend.c.
 * 
 * This is mostly a subclass of NSWindow. The actual `frontend'
 * structure passed to the midend contains a variety of pointers,
 * including that window object but also including the image we
 * draw on, an ImageView to display it in the window, and so on.
 */

@class GameWindow;
@class MyImageView;

struct frontend {
    GameWindow *window;
    NSImage *image;
    MyImageView *view;
    NSColor **colours;
    int ncolours;
    int clipped;
};

@interface MyImageView : NSImageView
{
    GameWindow *ourwin;
}
- (void)setWindow:(GameWindow *)win;
- (BOOL)isFlipped;
- (void)mouseEvent:(NSEvent *)ev button:(int)b;
- (void)mouseDown:(NSEvent *)ev;
- (void)mouseDragged:(NSEvent *)ev;
- (void)mouseUp:(NSEvent *)ev;
- (void)rightMouseDown:(NSEvent *)ev;
- (void)rightMouseDragged:(NSEvent *)ev;
- (void)rightMouseUp:(NSEvent *)ev;
- (void)otherMouseDown:(NSEvent *)ev;
- (void)otherMouseDragged:(NSEvent *)ev;
- (void)otherMouseUp:(NSEvent *)ev;
@end

@interface GameWindow : NSWindow
{
    const game *ourgame;
    midend *me;
    struct frontend fe;
    struct timeval last_time;
    NSTimer *timer;
    NSWindow *sheet;
    config_item *cfg;
    int cfg_which;
    NSView **cfg_controls;
    int cfg_ncontrols;
    NSTextField *status;
}
- (id)initWithGame:(const game *)g;
- (void)dealloc;
- (void)processButton:(int)b x:(int)x y:(int)y;
- (void)keyDown:(NSEvent *)ev;
- (void)activateTimer;
- (void)deactivateTimer;
- (void)setStatusLine:(char *)text;
- (void)resizeForNewGameParams;
@end

@implementation MyImageView

- (void)setWindow:(GameWindow *)win
{
    ourwin = win;
}

- (BOOL)isFlipped
{
    return YES;
}

- (void)mouseEvent:(NSEvent *)ev button:(int)b
{
    NSPoint point = [self convertPoint:[ev locationInWindow] fromView:nil];
    [ourwin processButton:b x:point.x y:point.y];
}

- (void)mouseDown:(NSEvent *)ev
{
    unsigned mod = [ev modifierFlags];
    [self mouseEvent:ev button:((mod & NSCommandKeyMask) ? RIGHT_BUTTON :
				(mod & NSShiftKeyMask) ? MIDDLE_BUTTON :
				LEFT_BUTTON)];
}
- (void)mouseDragged:(NSEvent *)ev
{
    unsigned mod = [ev modifierFlags];
    [self mouseEvent:ev button:((mod & NSCommandKeyMask) ? RIGHT_DRAG :
				(mod & NSShiftKeyMask) ? MIDDLE_DRAG :
				LEFT_DRAG)];
}
- (void)mouseUp:(NSEvent *)ev
{
    unsigned mod = [ev modifierFlags];
    [self mouseEvent:ev button:((mod & NSCommandKeyMask) ? RIGHT_RELEASE :
				(mod & NSShiftKeyMask) ? MIDDLE_RELEASE :
				LEFT_RELEASE)];
}
- (void)rightMouseDown:(NSEvent *)ev
{
    unsigned mod = [ev modifierFlags];
    [self mouseEvent:ev button:((mod & NSShiftKeyMask) ? MIDDLE_BUTTON :
				RIGHT_BUTTON)];
}
- (void)rightMouseDragged:(NSEvent *)ev
{
    unsigned mod = [ev modifierFlags];
    [self mouseEvent:ev button:((mod & NSShiftKeyMask) ? MIDDLE_DRAG :
				RIGHT_DRAG)];
}
- (void)rightMouseUp:(NSEvent *)ev
{
    unsigned mod = [ev modifierFlags];
    [self mouseEvent:ev button:((mod & NSShiftKeyMask) ? MIDDLE_RELEASE :
				RIGHT_RELEASE)];
}
- (void)otherMouseDown:(NSEvent *)ev
{
    [self mouseEvent:ev button:MIDDLE_BUTTON];
}
- (void)otherMouseDragged:(NSEvent *)ev
{
    [self mouseEvent:ev button:MIDDLE_DRAG];
}
- (void)otherMouseUp:(NSEvent *)ev
{
    [self mouseEvent:ev button:MIDDLE_RELEASE];
}
@end

@implementation GameWindow
- (void)setupContentView
{
    NSRect frame;
    int w, h;

    if (status) {
	frame = [status frame];
	frame.origin.y = frame.size.height;
    } else
	frame.origin.y = 0;
    frame.origin.x = 0;

    w = h = INT_MAX;
    midend_size(me, &w, &h, FALSE);
    frame.size.width = w;
    frame.size.height = h;

    fe.image = [[NSImage alloc] initWithSize:frame.size];
    [fe.image setFlipped:YES];
    fe.view = [[MyImageView alloc] initWithFrame:frame];
    [fe.view setImage:fe.image];
    [fe.view setWindow:self];

    midend_redraw(me);

    [[self contentView] addSubview:fe.view];
}
- (id)initWithGame:(const game *)g
{
    NSRect rect = { {0,0}, {0,0} }, rect2;
    int w, h;

    ourgame = g;

    fe.window = self;

    me = midend_new(&fe, ourgame, &osx_drawing, &fe);
    /*
     * If we ever need to open a fresh window using a provided game
     * ID, I think the right thing is to move most of this method
     * into a new initWithGame:gameID: method, and have
     * initWithGame: simply call that one and pass it NULL.
     */
    midend_new_game(me);
    w = h = INT_MAX;
    midend_size(me, &w, &h, FALSE);
    rect.size.width = w;
    rect.size.height = h;

    /*
     * Create the status bar, which will just be an NSTextField.
     */
    if (midend_wants_statusbar(me)) {
	status = [[NSTextField alloc] initWithFrame:NSMakeRect(0,0,100,50)];
	[status setEditable:NO];
	[status setSelectable:NO];
	[status setBordered:YES];
	[status setBezeled:YES];
	[status setBezelStyle:NSTextFieldSquareBezel];
	[status setDrawsBackground:YES];
	[[status cell] setTitle:@""];
	[status sizeToFit];
	rect2 = [status frame];
	rect.size.height += rect2.size.height;
	rect2.size.width = rect.size.width;
	rect2.origin.x = rect2.origin.y = 0;
	[status setFrame:rect2];
    } else
	status = nil;

    self = [super initWithContentRect:rect
	    styleMask:(NSTitledWindowMask | NSMiniaturizableWindowMask |
		       NSClosableWindowMask)
	    backing:NSBackingStoreBuffered
	    defer:YES];
    [self setTitle:[NSString stringWithCString:ourgame->name]];

    {
	float *colours;
	int i, ncolours;

	colours = midend_colours(me, &ncolours);
	fe.ncolours = ncolours;
	fe.colours = snewn(ncolours, NSColor *);

	for (i = 0; i < ncolours; i++) {
	    fe.colours[i] = [[NSColor colorWithDeviceRed:colours[i*3]
			      green:colours[i*3+1] blue:colours[i*3+2]
			      alpha:1.0] retain];
	}
    }

    [self setupContentView];
    if (status)
	[[self contentView] addSubview:status];
    [self setIgnoresMouseEvents:NO];

    [self center];		       /* :-) */

    return self;
}

- (void)dealloc
{
    int i;
    for (i = 0; i < fe.ncolours; i++) {
	[fe.colours[i] release];
    }
    sfree(fe.colours);
    midend_free(me);
    [super dealloc];
}

- (void)processButton:(int)b x:(int)x y:(int)y
{
    if (!midend_process_key(me, x, y, b))
	[self close];
}

- (void)keyDown:(NSEvent *)ev
{
    NSString *s = [ev characters];
    int i, n = [s length];

    for (i = 0; i < n; i++) {
	int c = [s characterAtIndex:i];

	/*
	 * ASCII gets passed straight to midend_process_key.
	 * Anything above that has to be translated to our own
	 * function key codes.
	 */
	if (c >= 0x80) {
	    int mods = FALSE;
	    switch (c) {
	      case NSUpArrowFunctionKey:
		c = CURSOR_UP;
		mods = TRUE;
		break;
	      case NSDownArrowFunctionKey:
		c = CURSOR_DOWN;
		mods = TRUE;
		break;
	      case NSLeftArrowFunctionKey:
		c = CURSOR_LEFT;
		mods = TRUE;
		break;
	      case NSRightArrowFunctionKey:
		c = CURSOR_RIGHT;
		mods = TRUE;
		break;
	      default:
		continue;
	    }

	    if (mods) {
		if ([ev modifierFlags] & NSShiftKeyMask)
		    c |= MOD_SHFT;
		if ([ev modifierFlags] & NSControlKeyMask)
		    c |= MOD_CTRL;
	    }
	}

	if (c >= '0' && c <= '9' && ([ev modifierFlags] & NSNumericPadKeyMask))
	    c |= MOD_NUM_KEYPAD;

	[self processButton:c x:-1 y:-1];
    }
}

- (void)activateTimer
{
    if (timer != nil)
	return;

    timer = [NSTimer scheduledTimerWithTimeInterval:0.02
	     target:self selector:@selector(timerTick:)
	     userInfo:nil repeats:YES];
    gettimeofday(&last_time, NULL);
}

- (void)deactivateTimer
{
    if (timer == nil)
	return;

    [timer invalidate];
    timer = nil;
}

- (void)timerTick:(id)sender
{
    struct timeval now;
    float elapsed;
    gettimeofday(&now, NULL);
    elapsed = ((now.tv_usec - last_time.tv_usec) * 0.000001F +
	       (now.tv_sec - last_time.tv_sec));
    midend_timer(me, elapsed);
    last_time = now;
}

- (void)showError:(char *)message
{
    NSAlert *alert;

    alert = [[[NSAlert alloc] init] autorelease];
    [alert addButtonWithTitle:@"Bah"];
    [alert setInformativeText:[NSString stringWithCString:message]];
    [alert beginSheetModalForWindow:self modalDelegate:nil
     didEndSelector:nil contextInfo:nil];
}

- (void)newGame:(id)sender
{
    [self processButton:'n' x:-1 y:-1];
}
- (void)restartGame:(id)sender
{
    midend_restart_game(me);
}
- (void)saveGame:(id)sender
{
    NSSavePanel *sp = [NSSavePanel savePanel];

    if ([sp runModal] == NSFileHandlingPanelOKButton) {
       const char *name = [[sp filename] UTF8String];

        FILE *fp = fopen(name, "w");

        if (!fp) {
            [self showError:"Unable to open save file"];
            return;
        }

        midend_serialise(me, savefile_write, fp);

        fclose(fp);
    }
}
- (void)loadSavedGame:(id)sender
{
    NSOpenPanel *op = [NSOpenPanel openPanel];

    [op setAllowsMultipleSelection:NO];

    if ([op runModalForTypes:nil] == NSOKButton) {
	const char *name = [[[op filenames] objectAtIndex:0] cString];
	char *err;

        FILE *fp = fopen(name, "r");

        if (!fp) {
            [self showError:"Unable to open saved game file"];
            return;
        }

        err = midend_deserialise(me, savefile_read, fp);

        fclose(fp);

        if (err) {
            [self showError:err];
            return;
        }

	[self resizeForNewGameParams];
    }
}
- (void)undoMove:(id)sender
{
    [self processButton:'u' x:-1 y:-1];
}
- (void)redoMove:(id)sender
{
    [self processButton:'r'&0x1F x:-1 y:-1];
}

- (void)copy:(id)sender
{
    char *text;

    if ((text = midend_text_format(me)) != NULL) {
	NSPasteboard *pb = [NSPasteboard generalPasteboard];
	NSArray *a = [NSArray arrayWithObject:NSStringPboardType];
	[pb declareTypes:a owner:nil];
	[pb setString:[NSString stringWithCString:text]
	 forType:NSStringPboardType];
    } else
	NSBeep();
}

- (void)solveGame:(id)sender
{
    char *msg;

    msg = midend_solve(me);

    if (msg)
	[self showError:msg];
}

- (BOOL)validateMenuItem:(NSMenuItem *)item
{
    if ([item action] == @selector(copy:))
	return (ourgame->can_format_as_text ? YES : NO);
    else if ([item action] == @selector(solveGame:))
	return (ourgame->can_solve ? YES : NO);
    else
	return [super validateMenuItem:item];
}

- (void)clearTypeMenu
{
    while ([typemenu numberOfItems] > 1)
	[typemenu removeItemAtIndex:0];
}

- (void)becomeKeyWindow
{
    int n;

    [self clearTypeMenu];

    [super becomeKeyWindow];

    n = midend_num_presets(me);

    if (n > 0) {
	[typemenu insertItem:[NSMenuItem separatorItem] atIndex:0];
	while (n--) {
	    char *name;
	    game_params *params;
	    DataMenuItem *item;

	    midend_fetch_preset(me, n, &name, &params);

	    item = [[[DataMenuItem alloc]
		     initWithTitle:[NSString stringWithCString:name]
		     action:NULL keyEquivalent:@""]
		    autorelease];

	    [item setEnabled:YES];
	    [item setTarget:self];
	    [item setAction:@selector(presetGame:)];
	    [item setPayload:params];

	    [typemenu insertItem:item atIndex:0];
	}
    }
}

- (void)resignKeyWindow
{
    [self clearTypeMenu];
    [super resignKeyWindow];
}

- (void)close
{
    [self clearTypeMenu];
    [super close];
}

- (void)resizeForNewGameParams
{
    NSSize size = {0,0};
    int w, h;

    w = h = INT_MAX;
    midend_size(me, &w, &h, FALSE);
    size.width = w;
    size.height = h;

    if (status) {
	NSRect frame = [status frame];
	size.height += frame.size.height;
	frame.size.width = size.width;
	[status setFrame:frame];
    }

    NSDisableScreenUpdates();
    [self setContentSize:size];
    [self setupContentView];
    NSEnableScreenUpdates();
}

- (void)presetGame:(id)sender
{
    game_params *params = [sender getPayload];

    midend_set_params(me, params);
    midend_new_game(me);

    [self resizeForNewGameParams];
}

- (void)startConfigureSheet:(int)which
{
    NSButton *ok, *cancel;
    int actw, acth, leftw, rightw, totalw, h, thish, y;
    int k;
    NSRect rect, tmprect;
    const int SPACING = 16;
    char *title;
    config_item *i;
    int cfg_controlsize;
    NSTextField *tf;
    NSButton *b;
    NSPopUpButton *pb;

    assert(sheet == NULL);

    /*
     * Every control we create here is going to have this size
     * until we tell it to calculate a better one.
     */
    tmprect = NSMakeRect(0, 0, 100, 50);

    /*
     * Set up OK and Cancel buttons. (Actually, MacOS doesn't seem
     * to be fond of generic OK and Cancel wording, so I'm going to
     * rename them to something nicer.)
     */
    actw = acth = 0;

    cancel = [[NSButton alloc] initWithFrame:tmprect];
    [cancel setBezelStyle:NSRoundedBezelStyle];
    [cancel setTitle:@"Abandon"];
    [cancel setTarget:self];
    [cancel setKeyEquivalent:@"\033"];
    [cancel setAction:@selector(sheetCancelButton:)];
    [cancel sizeToFit];
    rect = [cancel frame];
    if (actw < rect.size.width) actw = rect.size.width;
    if (acth < rect.size.height) acth = rect.size.height;

    ok = [[NSButton alloc] initWithFrame:tmprect];
    [ok setBezelStyle:NSRoundedBezelStyle];
    [ok setTitle:@"Accept"];
    [ok setTarget:self];
    [ok setKeyEquivalent:@"\r"];
    [ok setAction:@selector(sheetOKButton:)];
    [ok sizeToFit];
    rect = [ok frame];
    if (actw < rect.size.width) actw = rect.size.width;
    if (acth < rect.size.height) acth = rect.size.height;

    totalw = SPACING + 2 * actw;
    h = 2 * SPACING + acth;

    /*
     * Now fetch the midend config data and go through it creating
     * controls.
     */
    cfg = midend_get_config(me, which, &title);
    sfree(title);		       /* FIXME: should we use this somehow? */
    cfg_which = which;

    cfg_ncontrols = cfg_controlsize = 0;
    cfg_controls = NULL;
    leftw = rightw = 0;
    for (i = cfg; i->type != C_END; i++) {
	if (cfg_controlsize < cfg_ncontrols + 5) {
	    cfg_controlsize = cfg_ncontrols + 32;
	    cfg_controls = sresize(cfg_controls, cfg_controlsize, NSView *);
	}

	thish = 0;

	switch (i->type) {
	  case C_STRING:
	    /*
	     * Two NSTextFields, one being a label and the other
	     * being an edit box.
	     */

	    tf = [[NSTextField alloc] initWithFrame:tmprect];
	    [tf setEditable:NO];
	    [tf setSelectable:NO];
	    [tf setBordered:NO];
	    [tf setDrawsBackground:NO];
	    [[tf cell] setTitle:[NSString stringWithCString:i->name]];
	    [tf sizeToFit];
	    rect = [tf frame];
	    if (thish < rect.size.height + 1) thish = rect.size.height + 1;
	    if (leftw < rect.size.width + 1) leftw = rect.size.width + 1;
	    cfg_controls[cfg_ncontrols++] = tf;

	    tf = [[NSTextField alloc] initWithFrame:tmprect];
	    [tf setEditable:YES];
	    [tf setSelectable:YES];
	    [tf setBordered:YES];
	    [[tf cell] setTitle:[NSString stringWithCString:i->sval]];
	    [tf sizeToFit];
	    rect = [tf frame];
	    /*
	     * We impose a minimum and maximum width on editable
	     * NSTextFields. If we allow them to size themselves to
	     * the contents of the text within them, then they will
	     * look very silly if that text is only one or two
	     * characters, and equally silly if it's an absolutely
	     * enormous Rectangles or Pattern game ID!
	     */
	    if (rect.size.width < 75) rect.size.width = 75;
	    if (rect.size.width > 400) rect.size.width = 400;

	    if (thish < rect.size.height + 1) thish = rect.size.height + 1;
	    if (rightw < rect.size.width + 1) rightw = rect.size.width + 1;
	    cfg_controls[cfg_ncontrols++] = tf;
	    break;

	  case C_BOOLEAN:
	    /*
	     * A checkbox is an NSButton with a type of
	     * NSSwitchButton.
	     */
	    b = [[NSButton alloc] initWithFrame:tmprect];
	    [b setBezelStyle:NSRoundedBezelStyle];
	    [b setButtonType:NSSwitchButton];
	    [b setTitle:[NSString stringWithCString:i->name]];
	    [b sizeToFit];
	    [b setState:(i->ival ? NSOnState : NSOffState)];
	    rect = [b frame];
	    if (totalw < rect.size.width + 1) totalw = rect.size.width + 1;
	    if (thish < rect.size.height + 1) thish = rect.size.height + 1;
	    cfg_controls[cfg_ncontrols++] = b;
	    break;

	  case C_CHOICES:
	    /*
	     * A pop-up menu control is an NSPopUpButton, which
	     * takes an embedded NSMenu. We also need an
	     * NSTextField to act as a label.
	     */

	    tf = [[NSTextField alloc] initWithFrame:tmprect];
	    [tf setEditable:NO];
	    [tf setSelectable:NO];
	    [tf setBordered:NO];
	    [tf setDrawsBackground:NO];
	    [[tf cell] setTitle:[NSString stringWithCString:i->name]];
	    [tf sizeToFit];
	    rect = [tf frame];
	    if (thish < rect.size.height + 1) thish = rect.size.height + 1;
	    if (leftw < rect.size.width + 1) leftw = rect.size.width + 1;
	    cfg_controls[cfg_ncontrols++] = tf;

	    pb = [[NSPopUpButton alloc] initWithFrame:tmprect pullsDown:NO];
	    [pb setBezelStyle:NSRoundedBezelStyle];
	    {
		char c, *p;

		p = i->sval;
		c = *p++;
		while (*p) {
		    char *q;

		    q = p;
		    while (*p && *p != c) p++;

		    [pb addItemWithTitle:[NSString stringWithCString:q
					  length:p-q]];

		    if (*p) p++;
		}
	    }
	    [pb selectItemAtIndex:i->ival];
	    [pb sizeToFit];

	    rect = [pb frame];
	    if (rightw < rect.size.width + 1) rightw = rect.size.width + 1;
	    if (thish < rect.size.height + 1) thish = rect.size.height + 1;
	    cfg_controls[cfg_ncontrols++] = pb;
	    break;
	}

	h += SPACING + thish;
    }

    if (totalw < leftw + SPACING + rightw)
	totalw = leftw + SPACING + rightw;
    if (totalw > leftw + SPACING + rightw) {
	int excess = totalw - (leftw + SPACING + rightw);
	int leftexcess = leftw * excess / (leftw + rightw);
	int rightexcess = excess - leftexcess;
	leftw += leftexcess;
	rightw += rightexcess;
    }

    /*
     * Now go through the list again, setting the final position
     * for each control.
     */
    k = 0;
    y = h;
    for (i = cfg; i->type != C_END; i++) {
	y -= SPACING;
	thish = 0;
	switch (i->type) {
	  case C_STRING:
	  case C_CHOICES:
	    /*
	     * These two are treated identically, since both expect
	     * a control on the left and another on the right.
	     */
	    rect = [cfg_controls[k] frame];
	    if (thish < rect.size.height + 1)
		thish = rect.size.height + 1;
	    rect = [cfg_controls[k+1] frame];
	    if (thish < rect.size.height + 1)
		thish = rect.size.height + 1;
	    rect = [cfg_controls[k] frame];
	    rect.origin.y = y - thish/2 - rect.size.height/2;
	    rect.origin.x = SPACING;
	    rect.size.width = leftw;
	    [cfg_controls[k] setFrame:rect];
	    rect = [cfg_controls[k+1] frame];
	    rect.origin.y = y - thish/2 - rect.size.height/2;
	    rect.origin.x = 2 * SPACING + leftw;
	    rect.size.width = rightw;
	    [cfg_controls[k+1] setFrame:rect];
	    k += 2;
	    break;

	  case C_BOOLEAN:
	    rect = [cfg_controls[k] frame];
	    if (thish < rect.size.height + 1)
		thish = rect.size.height + 1;
	    rect.origin.y = y - thish/2 - rect.size.height/2;
	    rect.origin.x = SPACING;
	    rect.size.width = totalw;
	    [cfg_controls[k] setFrame:rect];
	    k++;
	    break;
	}
	y -= thish;
    }

    assert(k == cfg_ncontrols);

    [cancel setFrame:NSMakeRect(SPACING+totalw/4-actw/2, SPACING, actw, acth)];
    [ok setFrame:NSMakeRect(SPACING+3*totalw/4-actw/2, SPACING, actw, acth)];

    sheet = [[NSWindow alloc]
	     initWithContentRect:NSMakeRect(0,0,totalw + 2*SPACING,h)
	     styleMask:NSTitledWindowMask | NSClosableWindowMask
	     backing:NSBackingStoreBuffered
	     defer:YES];

    [[sheet contentView] addSubview:cancel];
    [[sheet contentView] addSubview:ok];

    for (k = 0; k < cfg_ncontrols; k++)
	[[sheet contentView] addSubview:cfg_controls[k]];

    [NSApp beginSheet:sheet modalForWindow:self
     modalDelegate:nil didEndSelector:nil contextInfo:nil];
}

- (void)specificGame:(id)sender
{
    [self startConfigureSheet:CFG_DESC];
}

- (void)specificRandomGame:(id)sender
{
    [self startConfigureSheet:CFG_SEED];
}

- (void)customGameType:(id)sender
{
    [self startConfigureSheet:CFG_SETTINGS];
}

- (void)sheetEndWithStatus:(BOOL)update
{
    assert(sheet != NULL);
    [NSApp endSheet:sheet];
    [sheet orderOut:self];
    sheet = NULL;
    if (update) {
	int k;
	config_item *i;
	char *error;

	k = 0;
	for (i = cfg; i->type != C_END; i++) {
	    switch (i->type) {
	      case C_STRING:
		sfree(i->sval);
		i->sval = dupstr([[[(id)cfg_controls[k+1] cell]
                                  title] UTF8String]);
		k += 2;
		break;
	      case C_BOOLEAN:
		i->ival = [(id)cfg_controls[k] state] == NSOnState;
		k++;
		break;
	      case C_CHOICES:
		i->ival = [(id)cfg_controls[k+1] indexOfSelectedItem];
		k += 2;
		break;
	    }
	}

	error = midend_set_config(me, cfg_which, cfg);
	if (error) {
	    NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	    [alert addButtonWithTitle:@"Bah"];
	    [alert setInformativeText:[NSString stringWithCString:error]];
	    [alert beginSheetModalForWindow:self modalDelegate:nil
	     didEndSelector:nil contextInfo:nil];
	} else {
	    midend_new_game(me);
	    [self resizeForNewGameParams];
	}
    }
    sfree(cfg_controls);
    cfg_controls = NULL;
}
- (void)sheetOKButton:(id)sender
{
    [self sheetEndWithStatus:YES];
}
- (void)sheetCancelButton:(id)sender
{
    [self sheetEndWithStatus:NO];
}

- (void)setStatusLine:(char *)text
{
    [[status cell] setTitle:[NSString stringWithCString:text]];
}

@end

/*
 * Drawing routines called by the midend.
 */
static void osx_draw_polygon(void *handle, int *coords, int npoints,
			     int fillcolour, int outlinecolour)
{
    frontend *fe = (frontend *)handle;
    NSBezierPath *path = [NSBezierPath bezierPath];
    int i;

    [[NSGraphicsContext currentContext] setShouldAntialias:YES];

    for (i = 0; i < npoints; i++) {
	NSPoint p = { coords[i*2] + 0.5, coords[i*2+1] + 0.5 };
	if (i == 0)
	    [path moveToPoint:p];
	else
	    [path lineToPoint:p];
    }

    [path closePath];

    if (fillcolour >= 0) {
	assert(fillcolour >= 0 && fillcolour < fe->ncolours);
	[fe->colours[fillcolour] set];
	[path fill];
    }

    assert(outlinecolour >= 0 && outlinecolour < fe->ncolours);
    [fe->colours[outlinecolour] set];
    [path stroke];
}
static void osx_draw_circle(void *handle, int cx, int cy, int radius,
			    int fillcolour, int outlinecolour)
{
    frontend *fe = (frontend *)handle;
    NSBezierPath *path = [NSBezierPath bezierPath];

    [[NSGraphicsContext currentContext] setShouldAntialias:YES];

    [path appendBezierPathWithArcWithCenter:NSMakePoint(cx + 0.5, cy + 0.5)
        radius:radius startAngle:0.0 endAngle:360.0];

    [path closePath];

    if (fillcolour >= 0) {
	assert(fillcolour >= 0 && fillcolour < fe->ncolours);
	[fe->colours[fillcolour] set];
	[path fill];
    }

    assert(outlinecolour >= 0 && outlinecolour < fe->ncolours);
    [fe->colours[outlinecolour] set];
    [path stroke];
}
static void osx_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour)
{
    frontend *fe = (frontend *)handle;
    NSBezierPath *path = [NSBezierPath bezierPath];
    NSPoint p1 = { x1 + 0.5, y1 + 0.5 }, p2 = { x2 + 0.5, y2 + 0.5 };

    [[NSGraphicsContext currentContext] setShouldAntialias:NO];

    assert(colour >= 0 && colour < fe->ncolours);
    [fe->colours[colour] set];

    [path moveToPoint:p1];
    [path lineToPoint:p2];
    [path stroke];
}
static void osx_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
    frontend *fe = (frontend *)handle;
    NSRect r = { {x,y}, {w,h} };
    
    [[NSGraphicsContext currentContext] setShouldAntialias:NO];

    assert(colour >= 0 && colour < fe->ncolours);
    [fe->colours[colour] set];

    NSRectFill(r);
}
static void osx_draw_text(void *handle, int x, int y, int fonttype,
			  int fontsize, int align, int colour, char *text)
{
    frontend *fe = (frontend *)handle;
    NSString *string = [NSString stringWithCString:text];
    NSDictionary *attr;
    NSFont *font;
    NSSize size;
    NSPoint point;

    [[NSGraphicsContext currentContext] setShouldAntialias:YES];

    assert(colour >= 0 && colour < fe->ncolours);

    if (fonttype == FONT_FIXED)
	font = [NSFont userFixedPitchFontOfSize:fontsize];
    else
	font = [NSFont userFontOfSize:fontsize];

    attr = [NSDictionary dictionaryWithObjectsAndKeys:
	    fe->colours[colour], NSForegroundColorAttributeName,
	    font, NSFontAttributeName, nil];

    point.x = x;
    point.y = y;

    size = [string sizeWithAttributes:attr];
    if (align & ALIGN_HRIGHT)
	point.x -= size.width;
    else if (align & ALIGN_HCENTRE)
	point.x -= size.width / 2;
    if (align & ALIGN_VCENTRE)
	point.y -= size.height / 2;
    else
	point.y -= size.height;

    [string drawAtPoint:point withAttributes:attr];
}
struct blitter {
    int w, h;
    int x, y;
    NSImage *img;
};
static blitter *osx_blitter_new(void *handle, int w, int h)
{
    blitter *bl = snew(blitter);
    bl->x = bl->y = -1;
    bl->w = w;
    bl->h = h;
    bl->img = [[NSImage alloc] initWithSize:NSMakeSize(w, h)];
    [bl->img setFlipped:YES];
    return bl;
}
static void osx_blitter_free(void *handle, blitter *bl)
{
    [bl->img release];
    sfree(bl);
}
static void osx_blitter_save(void *handle, blitter *bl, int x, int y)
{
    frontend *fe = (frontend *)handle;
    [fe->image unlockFocus];
    [bl->img lockFocus];
    [fe->image drawInRect:NSMakeRect(0, 0, bl->w, bl->h)
	fromRect:NSMakeRect(x, y, bl->w, bl->h)
	operation:NSCompositeCopy fraction:1.0];
    [bl->img unlockFocus];
    [fe->image lockFocus];
    bl->x = x;
    bl->y = y;
}
static void osx_blitter_load(void *handle, blitter *bl, int x, int y)
{
    /* frontend *fe = (frontend *)handle; */
    if (x == BLITTER_FROMSAVED && y == BLITTER_FROMSAVED) {
        x = bl->x;
        y = bl->y;
    }
    [bl->img drawInRect:NSMakeRect(x, y, bl->w, bl->h)
	fromRect:NSMakeRect(0, 0, bl->w, bl->h)
	operation:NSCompositeCopy fraction:1.0];
}
static void osx_draw_update(void *handle, int x, int y, int w, int h)
{
    frontend *fe = (frontend *)handle;
    [fe->view setNeedsDisplayInRect:NSMakeRect(x,y,w,h)];
}
static void osx_clip(void *handle, int x, int y, int w, int h)
{
    frontend *fe = (frontend *)handle;
    NSRect r = { {x,y}, {w,h} };
    
    if (!fe->clipped)
	[[NSGraphicsContext currentContext] saveGraphicsState];
    [NSBezierPath clipRect:r];
    fe->clipped = TRUE;
}
static void osx_unclip(void *handle)
{
    frontend *fe = (frontend *)handle;
    if (fe->clipped)
	[[NSGraphicsContext currentContext] restoreGraphicsState];
    fe->clipped = FALSE;
}
static void osx_start_draw(void *handle)
{
    frontend *fe = (frontend *)handle;
    [fe->image lockFocus];
    fe->clipped = FALSE;
}
static void osx_end_draw(void *handle)
{
    frontend *fe = (frontend *)handle;
    [fe->image unlockFocus];
}
static void osx_status_bar(void *handle, char *text)
{
    frontend *fe = (frontend *)handle;
    [fe->window setStatusLine:text];
}

const struct drawing_api osx_drawing = {
    osx_draw_text,
    osx_draw_rect,
    osx_draw_line,
    osx_draw_polygon,
    osx_draw_circle,
    osx_draw_update,
    osx_clip,
    osx_unclip,
    osx_start_draw,
    osx_end_draw,
    osx_status_bar,
    osx_blitter_new,
    osx_blitter_free,
    osx_blitter_save,
    osx_blitter_load,
    NULL, NULL, NULL, NULL, NULL, NULL, /* {begin,end}_{doc,page,puzzle} */
    NULL,			       /* line_width */
};

void deactivate_timer(frontend *fe)
{
    [fe->window deactivateTimer];
}
void activate_timer(frontend *fe)
{
    [fe->window activateTimer];
}

/* ----------------------------------------------------------------------
 * AppController: the object which receives the messages from all
 * menu selections that aren't standard OS X functions.
 */
@interface AppController : NSObject
{
}
- (void)newGameWindow:(id)sender;
- (void)about:(id)sender;
@end

@implementation AppController

- (void)newGameWindow:(id)sender
{
    const game *g = [sender getPayload];
    id win;

    win = [[GameWindow alloc] initWithGame:g];
    [win makeKeyAndOrderFront:self];
}

- (void)about:(id)sender
{
    id win;

    win = [[AboutBox alloc] init];
    [win makeKeyAndOrderFront:self];    
}

- (NSMenu *)applicationDockMenu:(NSApplication *)sender
{
    NSMenu *menu = newmenu("Dock Menu");
    {
	int i;

	for (i = 0; i < gamecount; i++) {
	    id item =
		initnewitem([DataMenuItem allocWithZone:[NSMenu menuZone]],
			    menu, gamelist[i]->name, "", self,
			    @selector(newGameWindow:));
	    [item setPayload:(void *)gamelist[i]];
	}
    }
    return menu;
}

@end

/* ----------------------------------------------------------------------
 * Main program. Constructs the menus and runs the application.
 */
int main(int argc, char **argv)
{
    NSAutoreleasePool *pool;
    NSMenu *menu;
    NSMenuItem *item;
    AppController *controller;
    NSImage *icon;

    pool = [[NSAutoreleasePool alloc] init];

    icon = [NSImage imageNamed:@"NSApplicationIcon"];
    [NSApplication sharedApplication];
    [NSApp setApplicationIconImage:icon];

    controller = [[[AppController alloc] init] autorelease];
    [NSApp setDelegate:controller];

    [NSApp setMainMenu: newmenu("Main Menu")];

    menu = newsubmenu([NSApp mainMenu], "Apple Menu");
    item = newitem(menu, "About Puzzles", "", NULL, @selector(about:));
    [menu addItem:[NSMenuItem separatorItem]];
    [NSApp setServicesMenu:newsubmenu(menu, "Services")];
    [menu addItem:[NSMenuItem separatorItem]];
    item = newitem(menu, "Hide Puzzles", "h", NSApp, @selector(hide:));
    item = newitem(menu, "Hide Others", "o-h", NSApp, @selector(hideOtherApplications:));
    item = newitem(menu, "Show All", "", NSApp, @selector(unhideAllApplications:));
    [menu addItem:[NSMenuItem separatorItem]];
    item = newitem(menu, "Quit", "q", NSApp, @selector(terminate:));
    [NSApp setAppleMenu: menu];

    menu = newsubmenu([NSApp mainMenu], "File");
    item = newitem(menu, "Open", "o", NULL, @selector(loadSavedGame:));
    item = newitem(menu, "Save As", "s", NULL, @selector(saveGame:));
    item = newitem(menu, "New Game", "n", NULL, @selector(newGame:));
    item = newitem(menu, "Restart Game", "r", NULL, @selector(restartGame:));
    item = newitem(menu, "Specific Game", "", NULL, @selector(specificGame:));
    item = newitem(menu, "Specific Random Seed", "", NULL,
                   @selector(specificRandomGame:));
    [menu addItem:[NSMenuItem separatorItem]];
    {
	NSMenu *submenu = newsubmenu(menu, "New Window");
	int i;

	for (i = 0; i < gamecount; i++) {
	    id item =
		initnewitem([DataMenuItem allocWithZone:[NSMenu menuZone]],
			    submenu, gamelist[i]->name, "", controller,
			    @selector(newGameWindow:));
	    [item setPayload:(void *)gamelist[i]];
	}
    }
    [menu addItem:[NSMenuItem separatorItem]];
    item = newitem(menu, "Close", "w", NULL, @selector(performClose:));

    menu = newsubmenu([NSApp mainMenu], "Edit");
    item = newitem(menu, "Undo", "z", NULL, @selector(undoMove:));
    item = newitem(menu, "Redo", "S-z", NULL, @selector(redoMove:));
    [menu addItem:[NSMenuItem separatorItem]];
    item = newitem(menu, "Cut", "x", NULL, @selector(cut:));
    item = newitem(menu, "Copy", "c", NULL, @selector(copy:));
    item = newitem(menu, "Paste", "v", NULL, @selector(paste:));
    [menu addItem:[NSMenuItem separatorItem]];
    item = newitem(menu, "Solve", "S-s", NULL, @selector(solveGame:));

    menu = newsubmenu([NSApp mainMenu], "Type");
    typemenu = menu;
    item = newitem(menu, "Custom", "", NULL, @selector(customGameType:));

    menu = newsubmenu([NSApp mainMenu], "Window");
    [NSApp setWindowsMenu: menu];
    item = newitem(menu, "Minimise Window", "m", NULL, @selector(performMiniaturize:));

    menu = newsubmenu([NSApp mainMenu], "Help");
    item = newitem(menu, "Puzzles Help", "?", NSApp, @selector(showHelp:));

    [NSApp run];
    [pool release];

    return 0;
}

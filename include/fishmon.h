#ifndef _FISHMON_H_
#define _FISHMON_H_

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#define POWER2 12
#define REALY(y) ((y) >> POWER2)
#define MAKEY(y) ((y) << POWER2)

/* we are going to try and make this independent from 56x56 problem,
 * so that someone interested can zoom this to say 256x256 */

/* perform clipping outside this range, also this is the size of the
 * drawing area */
#define XMAX 56
#define YMAX 56
/* this is the max size of the RGB buffer: 56 * 56 * 3
 * used for memcpy, memset, etc operations */
#define RGBSIZE (XMAX * YMAX * 3)
/* size of the colormap based image */
#define CMAPSIZE (XMAX * YMAX)
/* where WEED is located */
#define WEEDY (YMAX - 12)

/* width of sprite header, sprite cmap alias, sprite master block alias */
#define HWIDTH width
#define THE_CMAP header_data_cmap
#define THE_DATA header_data

/* how many fishes? */
#define NRFISH 4
#define MAXBUBBLE 32

/* main structure holding all sprites used in this dockapp */
typedef struct {
    int w;			/* sprite width */
    int h;			/* sprite height */
    int srcx;			/* sprite start in sprites.h */
    int srcy;			/* sprite start in sprites.h */
    unsigned char *data;	/* pointer to sprite image */
} Sprite;

/* structure describing each fish */
typedef struct {
    int speed;			/* amount of pixels to add for next move */
    int tx;			/* current x position */
    int y;			/* current y position */
    int travel;			/* how far to move beyond the screen */
    int rev;			/* going left or right? */
    int frame;			/* current animation frame */
    int delay;			/* how quick we swap frames */
    int turn;			/* in "turn" mode */
} Fish;

/* structure describing weed */
typedef struct {
    int where;			/* x position */
    int frame;			/* which frame are we on now */
    int delay;			/* delay for each weed */
} Weed;

typedef struct {
    int x;			/* x position */
    int y;			/* y position */
    int dy;			/* vertical velocity */
    int frame;			/* current frame type */
} Bubble;

/* main dockapp info structure.  windows, buffers, etc */
typedef struct {
    Display *display;		/* X11 display */
    GdkWindow *win;		/* main window */
    GdkWindow *iconwin;		/* icon window */
    GdkGC *gc;			/* drawing GC */
    GdkPixmap *pixmap;		/* main dockapp pixmap */
    GdkBitmap *mask;		/* dockapp mask */

    Fish fishes[NRFISH];	/* fishes */
	/* Pigeon */
    Weed weeds[2];		/* 2 weeds */

    Bubble bubbles[MAXBUBBLE];	/* bubbles */
    int nr_bubbles;		/* current bubble count */

    /* main image buffer */
    unsigned char rgb[RGBSIZE];

    /* cmap based buffer for sprites, etc - notice not RGB buffer */
    unsigned char image[CMAPSIZE];

    /* back buffer - stores things we dont want to redraw all the time */
    unsigned char bgr[RGBSIZE];

} FishMonData;
#endif				/* _FISHMON_H_ */

/*  BubbleFishyMon dockapp
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */
/* Huw Giddens 2003-10-28: Added missing copyright boilerplate. Corrected the
 *     calculation that keeps the fish in the water, and put it in its own
 *     function adjust_y().
 */
#define _GNU_SOURCE
#define VERSION "1.23"

/* general includes */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>

/* x11 includes */
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xresource.h>

#include "include/sprites.h"
#include "include/fishmon.h"
#include "include/chars.h"

#include "include/bubblemon.h"
#include "include/sys_include.h"

/* *INDENT-OFF* */
/* drawing */
void draw_cmap_image(void);
void draw_sprite(int x, int y, int idx);
void draw_sprite_alpha(int x, int y, int idx, int alpha);
void draw_string(int dx, int dy, char *string);
void draw_ascii(int x, int y, char digit);
void putpixel(int x, int y, float i, int linewidth, int color);

/* stuff */
void fishmon_update(void);
void bubble_update(void);
void fish_update(void);
void weed_update(void);

#ifdef ENABLE_TIME
void time_update(void);
void anti_line(int x1, int y1, int x2, int y2, int linewidth, int color);
#endif

/* misc support functions */
void prepare_sprites(void);

/* global variables */
extern BubbleMonData bm;
extern int fish_traffic;
extern int time_enabled;

/* 34 sprites:
 * 0, 2, 4, 6, 8, 10, 12, 14 - fish left
 * 1, 3, 5, 7, 9, 11, 13, 15 - fish right
 * 16, 17, 18, 19, 20 - grass
 * 21, 22, 23, 24, 25 - small fish
 * 26, 27, 28, 29, 30 - bubbles
 * 31 - thermometer and the "liquid"
 */
Sprite sp[34] = {
/* 00 */    { 17, 14, 0, 0, NULL },		/* left swim fish */
/* 01 */    { 17, 14, 18, 0, NULL },		/* right swim fish */
/* 02 */    { 17, 14, 0, 15, NULL },		/* left swim fish 1 */
/* 03 */    { 17, 14, 18, 15, NULL },		/* right swim fish 1 */
/* 04 */    { 17, 14, 0, 30, NULL },		/* left swim fish 2 */
/* 05 */    { 17, 14, 18, 30, NULL },		/* right swim fish 2 */
/* 06 */    { 17, 14, 0, 45, NULL },		/* left turn begin */
/* 07 */    { 17, 14, 18, 45, NULL },		/* right turn begin */
/* 08 */    { 17, 14, 0, 60, NULL },		/* facing */
/* 09 */    { 17, 14, 18, 60, NULL },		/* facing */
/* 10 */    { 17, 14, 0, 75, NULL },		/* left mouth open */
/* 11 */    { 17, 14, 18, 75, NULL },		/* right mouth open */
/* 12 */    { 17, 14, 0, 90, NULL },		/* left mouth open 1 */
/* 13 */    { 17, 14, 18, 90, NULL },		/* right mouth open 1 */
/* 14 */    { 17, 14, 0, 105, NULL },		/* left mouth open 2 */
/* 15 */    { 17, 14, 18, 105, NULL },		/* right mouth open 2 */

/* 16 */    { 17, 12, 0, 120, NULL },		/* grass 1 */
/* 17 */    { 17, 12, 18, 120, NULL },		/* grass 2 */
/* 18 */    { 17, 12, 0, 135, NULL },		/* grass 3 */
/* 19 */    { 17, 12, 18, 135, NULL },		/* grass 4 */
/* 20 */    { 17, 12, 0, 150, NULL },		/* grass 5 */

/* 21 */    { 11, 8, 0, 180, NULL },		/* small fish huge */
/* 22 */    { 11, 7, 0, 189, NULL },		/* small fish big */
/* 23 */    { 9, 6, 0, 197, NULL },		/* small fish small */
/* 24 */    { 7, 4, 10, 197, NULL },		/* small fish little */
/* 25 */    { 7, 3, 10, 202, NULL },		/* small fish tiny */

/* 26 */    { 5, 5, 19, 196, NULL },		/* bubble huge */
/* 27 */    { 5, 3, 19, 204, NULL },		/* bubble big */
/* 28 */    { 3, 3, 27, 196, NULL },		/* bubble small */
/* 29 */    { 2, 2, 27, 202, NULL },		/* bubble little */
/* 30 */    { 1, 1, 33, 196, NULL },		/* bubble tiny */

/* 31 */    { 6, 35, 26, 151, NULL },		/* thermometer case */

            { 0, 0, 0, 0, NULL }		/* terminator */
};

/* animation sequence loop for fish [4 frames, loop] */
int fish_animation[4] = { 0, 2, 4, 2 };
/* animation sequence loop for weeds [8 frames, loop] */
int weed_animation[8] = { 16, 17, 18, 19, 20, 19, 18, 17 };
/* animation sequence loop for turning fish [4 frames, no loop] */
int turn_animation[8] = { 6, 8, 7, 1,    7, 9, 6, 0 };
/* "x offset" sequence for bubbles moving upward */
int bubble_sequence[5] = { -2, -2, -1, 0, 0 };
/* keeps track of mouse focus */
/* keeps track of distance between bubble state changes (depends on YMAX) */
extern int bubble_state_change;
/* day of week */
/* *INDENT-ON* */

void traffic_fish_update(void);

/* update fish, bubbles, temperature, etc */
void fishmon_update(void)
{
    memset(&bm.image, 0, CMAPSIZE);

    /* check for new mail if enabled and no new mail exists */
    /*
	check_mail();
	if (new_mail)
	    */
    weed_update();

    /* update each fish position */
	if(fish_traffic)
	{
		traffic_fish_update();
	}
	else
	{
		fish_update();
	}

    /* done with sprites - now draw colormap-based image */
    draw_cmap_image(); 

    /* draw bubbles - done after everything else because we blend them
     * on top of everything */

    /* draw thermometer 80 - alpha 
    draw_sprite_alpha(49, 20, 31, 80); */


}				/* fishmon_update */


void weed_update(void)
{
    int i;

    for (i = 0; i < 2; i++) {

	if (bm.weeds[i].delay++ <= 20) {
	    draw_sprite(bm.weeds[i].where, WEEDY,
			weed_animation[bm.weeds[i].frame]);
	    continue;
	}

	/* process frame shifting */
	bm.weeds[i].delay = 0;
	draw_sprite(bm.weeds[i].where, WEEDY,
		    weed_animation[bm.weeds[i].frame]);
	bm.weeds[i].frame++;
	if (bm.weeds[i].frame > 7)
	    bm.weeds[i].frame = 0;
    }
}

void bubble_update(void)
{
    Bubble *bubbles = bm.bubbles;
    int i, x, y;
    int seq;

    /* make a new bubble, if needed */
    if ((bm.nr_bubbles < MAXBUBBLE) && ((rand() % 101) <= 32)) {
	bm.bubbles[bm.nr_bubbles].x = (rand() % XMAX);
	bm.bubbles[bm.nr_bubbles].y = MAKEY(YMAX);
	bm.bubbles[bm.nr_bubbles].dy = 0;
	bm.nr_bubbles++;
    }

    /* Update and draw the bubbles */
    for (i = 0; i < bm.nr_bubbles; i++) {
	/* Accelerate the bubble */
	bubbles[i].dy -= 64;

	/* Move the bubble vertically */
	bubbles[i].y += bubbles[i].dy;

	/* Did we lose it? */
	if (bubbles[i].y < 0) {
	    /* Yes; nuke it */
	    bubbles[i].x = bubbles[bm.nr_bubbles - 1].x;
	    bubbles[i].y = bubbles[bm.nr_bubbles - 1].y;
	    bubbles[i].dy = bubbles[bm.nr_bubbles - 1].dy;
	    bm.nr_bubbles--;

	    /* We must check the previously last bubble, which is
	     * now the current bubble, also. */
	    i--;
	    continue;
	}
	/* Draw the bubble */
	x = bubbles[i].x;
	y = bubbles[i].y;
	
	/* convert to standard dimension */
	y = REALY(y);
	/* calculate bubble sequence - 0 to 4 (determine bubble sprite idx) */
	seq = y / bubble_state_change;

	/* draw the bubble, using offset-to-center calculated from current
	 * sequence, and make the bubble bigger as we go up. 120 - alpha */
	draw_sprite_alpha(x + bubble_sequence[seq], y, 26 + seq, 120);
    }
}

/* This function adjusts a passed y value so that it's not less than the 
 * water level. 0 on the y axis is at the top of the frame. This is called by
 * fish_update() and traffic_fish_update(). It returns the corrected value.
 */
static int adjust_y(int y)
{
	/* Pigeon
	   Make sure the fish is in the water :) */
	int min_y = (YMAX * (100 - bm.mem_percent))/100;

	if(y <= min_y) return min_y;
	else return y;
}

void fish_update(void)
{
    int i, j;

    for (i = 0; i < NRFISH; i++) {

	/* frozen fish doesn't need to be handled, or drawn */
	if (bm.fishes[i].speed == 0)
	    continue;

	/* randomly make fish change direction mid-way
	 * but only if it isn't turning already and if it isn't
	 * scared */
	if (((rand() % 255) == 128) && (bm.fishes[i].turn != 1)) {
	    bm.fishes[i].turn = 1;
	    bm.fishes[i].frame = 0;
	    bm.fishes[i].speed = 1;
	    bm.fishes[i].delay = 0;
	}

	/* move fish in horizontal direction, left or right */
	if (!bm.fishes[i].rev) {
	    bm.fishes[i].tx -= bm.fishes[i].speed;
	    if (bm.fishes[i].tx < -18 - bm.fishes[i].travel) {
		/* we moved out of bounds. change direction,
		 * position, speed. */
		bm.fishes[i].travel = rand() % 32;
		bm.fishes[i].tx = -18 - bm.fishes[i].travel;
		bm.fishes[i].rev = 1;
		bm.fishes[i].y = rand() % (YMAX - 14);
		bm.fishes[i].speed = (rand() % 2) + 1;
	    }
	} else {
	    bm.fishes[i].tx += bm.fishes[i].speed;
	    if (bm.fishes[i].tx > XMAX + bm.fishes[i].travel) {
		/* we moved out of bounds. change direction,
		 * position, speed. */
		bm.fishes[i].travel = rand() % 32;
		bm.fishes[i].tx = XMAX + bm.fishes[i].travel;
		bm.fishes[i].rev = 0;
		bm.fishes[i].y = rand() % (YMAX - 14);
		bm.fishes[i].speed = (rand() % 2) + 1;
	    }
	}

	/* move fish in vertical position randomly by one pixel up or down */
	j = rand() % 16;
	if (j <= 4)
	{
	    bm.fishes[i].y--;
	}
	else if (j > 12)
	{
	    bm.fishes[i].y++;
	}

	bm.fishes[i].y = adjust_y(bm.fishes[i].y);

	/* handle fish currently turning around */
	if (bm.fishes[i].turn) {
	    int turnframe;

	    /* turn_animation array keeps sprite indexes for turning fish
	     * around.  0-3 - ltr turn, 4-7 = rtl turn - rev determines
	     * if we use ltr or rtl parts by multiplying by 4 */
	    turnframe = turn_animation[bm.fishes[i].frame + ((bm.fishes[i].rev) * 4)];
	    draw_sprite(bm.fishes[i].tx, bm.fishes[i].y, turnframe);

	    /* because this is a special case, handle updating things here.
	     * notice, delay is lowered (quicker turn), and rev is also
	     * flipped after turn completion */
	    bm.fishes[i].delay += bm.fishes[i].speed;
	    if (bm.fishes[i].delay >= 5) {
		if (++bm.fishes[i].frame >= 4) {
		    bm.fishes[i].frame = 0;
		    bm.fishes[i].rev = !bm.fishes[i].rev;
		    bm.fishes[i].turn = 0;
		    bm.fishes[i].speed = (rand() % 2) + 1;
		}
		bm.fishes[i].delay = 0;
	    }
	    /* get to the next fish */
	    continue;

	} else {
	    /* animate fishes using fish_animation array */
	    draw_sprite(bm.fishes[i].tx, bm.fishes[i].y,
			bm.fishes[i].rev +
			fish_animation[bm.fishes[i].frame]);
	}

	/* switch to next swimming frame */
	bm.fishes[i].delay += bm.fishes[i].speed;
	if (bm.fishes[i].delay >= 10) {
	    if (++bm.fishes[i].frame > 3)
		bm.fishes[i].frame = 0;
	    bm.fishes[i].delay = 0;
	}
    }
}

void copy_sprite_data(int sx, int sy, int w, int h, unsigned char *to)
{
    int i, j;

    for (i = 0; i < h; i++) {
	for (j = 0; j < w; j++) {
	    to[(i * w) + j] = THE_DATA[((sy + i) * HWIDTH) + (sx + j)];
	}
    }
}

void prepare_sprites(void)
{
    int i = 0;

    i = height;
    i = 0;

    while (sp[i].w) {
	if (sp[i].data)
	    free(sp[i].data);
	sp[i].data = calloc(1, sp[i].w * sp[i].h);
	copy_sprite_data(sp[i].srcx, sp[i].srcy, sp[i].w, sp[i].h,
			 sp[i].data);
	i++;
    }

	if(fish_traffic)
	{
		for (i = 0; i < NRFISH; i++)
		{
			if(i < (NRFISH / 2))
			{
				bm.fishes[i].tx = -18 - rand() % XMAX;
				bm.fishes[i].y = 50;
				bm.fishes[i].rev = 1;
				bm.fishes[i].speed = 0;
			}
			else
			{
				bm.fishes[i].tx = XMAX + rand() % XMAX; 
				bm.fishes[i].y = 50;
				bm.fishes[i].rev = 0;
				bm.fishes[i].speed = 0;
			}
		}

	}
	else
	{
		for (i = 0; i < NRFISH; i++) {
		bm.fishes[i].y = 50;
		bm.fishes[i].rev = (i % 2) ? 1 : 0;
		bm.fishes[i].tx = rand() % XMAX;
		bm.fishes[i].speed = (rand() % 2) + 1;
		}
	}

    bm.weeds[0].where = -5;
    bm.weeds[0].frame = rand() % 7;

    bm.weeds[1].where = 42;
    bm.weeds[1].frame = rand() % 7;

}

/********************************************************************
 *                                                                  *
 * Drawing Functions below                                          *
 *                                                                  *
 ********************************************************************/

/* draw XMAX x YMAX colormap image into the main rgb buffer */
void draw_cmap_image(void)
{
    int i;
    unsigned char cmap;

    for (i = 0; i < CMAPSIZE; i++) {
	if ((cmap = bm.image[i]) != 0) {
	    int pos = i * 3;
	    bm.rgb_buf[pos++] = THE_CMAP[cmap][0];
	    bm.rgb_buf[pos++] = THE_CMAP[cmap][1];
/*
 * Looks nicer without this :)
		bm.rgb_buf[pos] = THE_CMAP[cmap][2];
 */
	}
    }
}

/* draw a sprite into bm.image (palette-based) */
void draw_sprite(int x, int y, int idx)
{
    /* bounding box of the clipped sprite */
    int dw, di, dh, ds;
    /* loop counters */
    int w, h;
    /* offset into image buffer */
    int pos;

    /* cmap reference for each pixel */
    unsigned char cmap;

    assert(idx >= 0);

    /* completely off the screen, don't bother drawing */
    if ((y < -(sp[idx].h)) || (y > YMAX) ||
	(x > XMAX) || (x < -(sp[idx].w)))
	return;

    /* do clipping for top side */
    ds = 0;
    if (y < 0)
	ds = -(y);

    /* do clipping for bottom side */
    dh = sp[idx].h;
    if ((y + sp[idx].h) > YMAX)
	dh = YMAX - y;

    /* do clipping for right side */
    dw = sp[idx].w;
    if (x > (XMAX - sp[idx].w))
	dw = sp[idx].w - (x - (XMAX - sp[idx].w));

    /* do clipping for left side */
    di = 0;
    if (x < 0)
	di = -(x);

    for (h = ds; h < dh; h++) {
	/* offset to beginning of current row */
	int ypos = (h + y) * XMAX;
	for (w = di; w < dw; w++) {
	    if ((cmap = sp[idx].data[h * sp[idx].w + w]) != 0) {
		pos = ypos + w + x;
		bm.image[pos] = cmap;
	    }
	}
    }
}

/* draw a sprite into bm.rgb_buf with alpha-blend */
void draw_sprite_alpha(int x, int y, int idx, int alpha)
{
    /* bounding box of the clipped sprite */
    int dw, di, dh, ds;
    /* loop counters */
    int w, h;
    /* offset into rgb buffer */
    int pos;

    /* cmap reference for each pixel */
    unsigned char cmap;

    /* completely off the screen, don't bother drawing */
    if ((y < -(sp[idx].h)) || (y > YMAX) ||
	(x > XMAX) || (x < -(sp[idx].w)))
	return;

    /* do clipping for top side */
    ds = 0;
    if (y < 0)
	ds = -(y);

    /* do clipping for bottom side */
    dh = sp[idx].h;
    if ((y + sp[idx].h) > YMAX)
	dh = YMAX - y;

    /* do clipping for right side */
    dw = sp[idx].w;
    if (x > (XMAX - sp[idx].w))
	dw = sp[idx].w - (x - (XMAX - sp[idx].w));

    /* do clipping for left side */
    di = 0;
    if (x < 0)
	di = -(x);

    for (h = ds; h < dh; h++) {
	/* offset to beginning of current row */
	int ypos = (h + y) * XMAX;
	for (w = di; w < dw; w++) {
	    if ((cmap = sp[idx].data[h * sp[idx].w + w]) != 0) {
		unsigned char r, g, b;
		
		pos = (ypos + w + x) * 3;
		r = THE_CMAP[cmap][0];
		g = THE_CMAP[cmap][1];
		b = THE_CMAP[cmap][2];

		bm.rgb_buf[pos] = (alpha * (int) bm.rgb_buf[pos]
				       + (256 - alpha) * (int) r) >> 8;
		
		bm.rgb_buf[pos + 1] = (alpha * (int) bm.rgb_buf[pos + 1]
			 + (256 - alpha) * (int) g) >> 8;
		
		bm.rgb_buf[pos + 2] = (alpha * (int) bm.rgb_buf[pos + 2]
			 + (256 - alpha) * (int) b) >> 8;
	    }
	}
    }
}

/* draw string using the draw_ascii function below.  Please make sure the
 * output doesn't go outside the dockapp as no checking is performed! */
void draw_string(int dx, int dy, char *string)
{
    char c = 0;

    while ((c = *string++)) {
	draw_ascii(dx, dy, c);
	if (c != '-')
	    dx += 6;
	else
	    dx += 5;
    }
}

/* draw digits 0..9, letters A..Z, and "-"
 * Clipping not performed!  Must be inside dockapp at all times! */
void draw_ascii(int dx, int dy, char digit)
{
    char *source = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ- ";
    int idx = strchr(source, digit) - source;
    int x, y;

    /* handle "space" here */
    if (idx == 37)
	return;

    for (y = 0; y < 7; y++) {
	for (x = 0; x < 6; x++) {
	    int src = font_data[(y * 222 + x) + (idx * 6)];
	    if (src) {
		int pos = ((y + dy) * XMAX * 3) + (x + dx) * 3;
		bm.rgb_buf[pos + 0] = font_cmap[src][0];
		bm.rgb_buf[pos + 1] = font_cmap[src][1];
		bm.rgb_buf[pos + 2] = font_cmap[src][2];
	    }
	}
    }
}


/* put alpha-blended pixel on the backbuffer.  Uses floats, could be
 * optimized, probably */
void putpixel(int x, int y, float i, int linewidth, int color)
{
    int pos = (y * XMAX * 3) + x * 3;

    unsigned char r = ((color >> 16) & 0xff) * i + (bm.rgb_buf[pos]) * (1 - i);
    unsigned char g = ((color >> 8) & 0xff) * i + (bm.rgb_buf[pos + 1]) * (1 - i);
    unsigned char b = (color & 0xff) * i + (bm.rgb_buf[pos + 2]) * (1 - i);

    if (linewidth == 1) {
	bm.rgb_buf[pos] = r;
	bm.rgb_buf[pos + 1] = g;
	bm.rgb_buf[pos + 2] = b;
    } else {
	int dx, dy;
	for (dx = x; dx < x + linewidth; dx++) {
	    for (dy = y; dy < y + linewidth; dy++) {
		pos = (dy * XMAX * 3) + dx * 3;
		bm.rgb_buf[pos] = r;
		bm.rgb_buf[pos + 1] = g;
		bm.rgb_buf[pos + 2] = b;
	    }
	}
    }
}


void traffic_fish_update()
{
    int i, j;

	int rx_speed = net_rx_speed();
	int tx_speed = net_tx_speed();

    for (i = 0; i < NRFISH; i++)
	{
		/* No traffic, do nothing */
		if (bm.fishes[i].speed == 0 && rx_speed == 0 && tx_speed == 0)
		{
			continue;
		}

		if(i < (NRFISH / 2))
		{
			/* tx traffic */
			if(bm.fishes[i].tx < XMAX)
			{
				if(bm.fishes[i].speed < tx_speed)
				{
					/* Slowly accelerate */
					bm.fishes[i].speed += 1;
				}

				bm.fishes[i].tx += bm.fishes[i].speed;
			}
			else
			{
				/* Done once, go back */
				bm.fishes[i].tx = -18 - rand() % XMAX;
				bm.fishes[i].y = (rand() % (YMAX - 14));
				if(tx_speed == 0)
				{
					/* Stop the fish when it's at the end... */
					bm.fishes[i].speed = 0;
				}
				else
				{
					bm.fishes[i].speed = tx_speed;
				}
			}
		}
		else
		{
			/* rx traffic */
			if(bm.fishes[i].tx > -18)
			{
				if(bm.fishes[i].speed < rx_speed)
				{
					/* Slowly accelerate */
					bm.fishes[i].speed += 1;
				}
				bm.fishes[i].tx -= bm.fishes[i].speed;
			}
			else
			{
				/* Done once, go back */
				bm.fishes[i].tx = XMAX + rand() % XMAX;
				bm.fishes[i].y = (rand() % (YMAX - 14));
				if(rx_speed == 0)
				{
					/* Stop the fish when it's at the end... */
					bm.fishes[i].speed = 0;
				}
				else
				{
					bm.fishes[i].speed = rx_speed;
					bm.fishes[i].tx -= bm.fishes[i].speed;
				}
			}
		}

		/* move fish in vertical position randomly by one pixel up or down */
		j = rand() % 16;
		if (j <= 4)
		{
			bm.fishes[i].y--;
		}
		else if (j > 12)
		{
			bm.fishes[i].y++;
		}

		bm.fishes[i].y = adjust_y(bm.fishes[i].y);

		/* animate fishes using fish_animation array */
		draw_sprite(bm.fishes[i].tx, bm.fishes[i].y,
			bm.fishes[i].rev +
			fish_animation[bm.fishes[i].frame]);

		/* switch to next swimming frame */
		bm.fishes[i].delay += bm.fishes[i].speed;
		if (bm.fishes[i].delay >= 10) {
			if (++bm.fishes[i].frame > 3)
			bm.fishes[i].frame = 0;
			bm.fishes[i].delay = 0;
		}
    }

}

#ifdef ENABLE_TIME
void time_update(void)
{
    struct tm *data;
    time_t cur_time;
    static time_t old_time;

    int hr, min, sec;
    static int osec = -1;
    static int oday = -1;
    static int hdx, hdy, mdx, mdy, sdx, sdy;

    double psi;

    cur_time = time(NULL);

    if (cur_time != old_time) {
	old_time = cur_time;

	data = localtime(&cur_time);

	hr = data->tm_hour % 12;
	min = data->tm_min;
	sec = data->tm_sec;

	/* hours */
	if ((sec % 15) == 0) {
	    psi = hr * (M_PI / 6.0);
	    psi += min * (M_PI / 360);
	    hdx = floor(sin(psi) * 26 * 0.55) + 28;
	    hdy = floor(-cos(psi) * 22 * 0.55) + 24;
	}

	/* minutes */
	if ((sec % 15) == 0) {
	    psi = min * (M_PI / 30.0);
	    psi += sec * (M_PI / 1800);
	    mdx = floor(sin(psi) * 26 * 0.7) + 28;
	    mdy = floor(-cos(psi) * 22 * 0.7) + 24;
	}
	
	/* seconds */
	if (osec != sec) {
	    psi = sec * (M_PI / 30.0);
	    sdx = floor(sin(psi) * 26 * 0.9) + 28;
	    sdy = floor(-cos(psi) * 22 * 0.9) + 24;
	    osec = sec;
	}

	/* see if we need to redraw the day/month/weekday deal */
	if (data->tm_mday != oday) {
	    oday = data->tm_mday;
	    /* redundant calculation for a reason */
	    psi = hr * (M_PI / 6.0);
	    psi += min * (M_PI / 360);
	    hdx = floor(sin(psi) * 26 * 0.55) + 28;
	    hdy = floor(-cos(psi) * 22 * 0.55) + 24;
	    psi = min * (M_PI / 30.0);
	    psi += sec * (M_PI / 1800);
	    mdx = floor(sin(psi) * 26 * 0.7) + 28;
	    mdy = floor(-cos(psi) * 22 * 0.7) + 24;

	    /* reflash the backbuffer / date / weekday */
	    /* prepare_backbuffer(0); */
	}
    }

    /* must redraw each frame */
    anti_line(28, 24, mdx, mdy, 1, 0xeeeeee);
    anti_line(28, 24, hdx, hdy, 1, 0xbf0000);
    anti_line(28, 24, sdx, sdy, 1, 0xc79f2b);

}

/* draw antialiased line from (x1, y1) to (x2, y2), with width linewidth
 * color is an int like 0xRRGGBB */
void anti_line(int x1, int y1, int x2, int y2, int linewidth, int color)
{
    int dx = abs(x1 - x2); 
    int dy = abs(y1 - y2);
    int error, sign, tmp;
    float ipix;
    int step = linewidth;

    if (dx >= dy) {
	if (x1 > x2) {
	    tmp = x1;
	    x1 = x2;
	    x2 = tmp;
	    tmp = y1;
	    y1 = y2;
	    y2 = tmp;
	}
	error = dx / 2;
	if (y2 > y1)
	    sign = step;
	else
	    sign = -step;

	putpixel(x1, y1, 1, linewidth, color);

	while (x1 < x2) {
	    if ((error -= dy) < 0) {
		y1 += sign;
		error += dx;
	    }
	    x1 += step;
	    ipix = (float)error / dx;

	    if (sign == step)
		ipix = 1 - ipix;

	    putpixel(x1, y1, 1, linewidth, color);
	    putpixel(x1, y1 - step, (1 - ipix), linewidth, color);
	    putpixel(x1, y1 + step, ipix, linewidth, color);
	}
	putpixel(x2, y2, 1, linewidth, color);
    } else {
	if (y1 > y2) {
	    tmp = x1;
	    x1 = x2;
	    x2 = tmp;
	    tmp = y1;
	    y1 = y2;
	    y2 = tmp;
	}
	error = dy / 2;

	if (x2 > x1)
	    sign = step;
	else
	    sign = -step;

	putpixel(x1, y1, 1, linewidth, color);

	while (y1 < y2) {
	    if ((error -= dx) < 0) {
		x1 += sign;
		error += dy;
	    }
	    y1 += step;
	    ipix = (float)error / dy;
	    
	    if (sign == step)
		ipix = 1 - ipix;

	    putpixel(x1 ,y1, 1, linewidth, color);
	    putpixel(x1 - step, y1, (1 - ipix), linewidth, color);
	    putpixel(x1 + step, y1, ipix, linewidth, color);
	}
	putpixel(x2, y2, 1, linewidth, color);
    }
}
#endif


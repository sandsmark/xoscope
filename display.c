/*
 * @(#)$Id: display.c,v 1.1 1996/01/23 07:52:07 twitham Exp $
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see scope.c and the file COPYING for more details)
 *
 * This file implements the display for both scope and xscope
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "scope.h"		/* program defaults */
#include "display.h"

#ifdef XSCOPE
char fontname[80] = DEF_FX;
Widget w[1];
XFont font;
char x11_key = '\0';
int XX[] = {320,640,640,640,800,1024,1280};
int XY[] = {200,200,350,480,480, 480, 480};
void
animate(void *data);
#else
char fontname[80] = DEF_F;
int screen_modes[] = {		/* allowed modes */
    G320x200x16,		/* 0 */
    G640x200x16,		/* 1 */
    G640x350x16,		/* 2 */
    G640x480x16,		/* 3 */
    G800x600x16,		/* 4 */
    G1024x768x16,		/* 5 */
    G1280x1024x16,		/* 6 */
};
#ifdef HAVEVGAMISC
font_t font;			/* pointer to the font structure */
#endif
#endif

/* clear the display and redraw any text */
void
clear()
{
#ifdef XSCOPE
  ClearDrawArea();
#else
  vga_clear();
#endif
  draw_text();
}

/* restore text mode and close sound device */
void
cleanup()
{
#ifdef XSCOPE
  FreeFont(font);
#else
  vga_setmode(TEXT);		/* restore text screen */
#endif
  close(snd);			/* close sound device */
}

/* draw text to graphics screen */
void
draw_text()
{
#if defined XSCOPE || defined HAVEVGAMISC
  VGA_WRITE(running ? "RUN " : "STOP",
	    COL(0), ROW(0),  font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
  if (verbose) {
    VGA_WRITE(error,  COL(0), ROW(5),  font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
  } else {
    VGA_WRITE("                                                                                ",
	      COL(0), ROW(5), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
  }
#endif
}

/* if verbose mode, show current parameter settings on standard out */
void
show_info(unsigned char c) {
  if (verbose) {
    sprintf(error, "%1c %5dHz:  %2s  -r %5d  -s %2d  -t %3d  -c %2d  -m %2d  "
	    "-d %1d  %2s  %2s  %2s%s",
	    c, actual,
	    channels == 1 ? "-1" : "-2",
	    sampling, scale, trigger, colour, mode, dma,
	    point_mode ? "-p" : "-l",

	    /* reverse logic if these booleans are on by default in scope.h */
#if DEF_G
	    graticule ? "" : "-g",
#else
	    graticule ? "-g" : "",
#endif

#if DEF_B
	    behind ? "" : "-b",
#else
	    behind ? "-b" : "",
#endif

#if DEF_V
	    verbose ? "" : "  -v"
#else
	    verbose ? "  -v" : ""
#endif
	    );
    draw_text();
    printf("%s\n", error);
  }
}

/* if graticule mode, draw graticule, always draw frame */
static inline void
draw_graticule()
{
  static int i, j;

  VGA_SETCOLOR(colour+2);
  VGA_DRAWLINE(0, offset-1, h_points-1, offset-1);
  VGA_DRAWLINE(0, offset+256, h_points-1, offset+256);
  VGA_DRAWLINE(0, offset-1, 0, offset+256);
  VGA_DRAWLINE(h_points-1, offset, h_points-1, offset+256);

  if (graticule) {

    /* horizontial line at mid-scale */
    VGA_SETCOLOR(colour+2);
    VGA_DRAWLINE(0, offset+128, h_points-1, offset+128);

    /* 1 pixel dots at 0.1 msec intervals */
    for (i = 0 ; (j = i / 10000) < h_points ; i += (actual * scale)) {
      VGA_DRAWPIXEL(j, offset);
      VGA_DRAWLINE(j, offset+127, j, offset+129);
      VGA_DRAWPIXEL(j, offset + 255);

      /* 5 pixel marks at 0.5 msec intervals */
      if ((j = i / 2000) < h_points) {
	VGA_DRAWLINE(j, offset, j, offset+4);
	VGA_DRAWLINE(j, offset+123, j, offset+133);
	VGA_DRAWLINE(j, offset+251, j, offset+255);
      }
      /* 20 pixel marks at 1 msec intervals */
      if ((j = i / 1000) < h_points) {
	VGA_DRAWLINE(j, offset, j, offset+20);
	VGA_DRAWLINE(j, offset+107, j, offset+149);
	VGA_DRAWLINE(j, offset+235, j, offset+255);
      }
      /* vertical major divs at 5 msec intervals */
      if ((j = i / 200) < h_points)
	VGA_DRAWLINE(j, offset, j, offset+255);
    }
    /* a tick mark where the trigger level is */
    if (trigger > -1)
      VGA_DRAWLINE(0, offset+trigger, 5, offset+trigger);

  }
}

#ifdef XSCOPE
/* callback to redisplay the X11 screen */
void
redisplay(Widget w, int new_width, int new_height, void * data) {
  draw_text();
  animate(NULL);
}

/* callback for keypress events on the X11 screen */
void
keys_x11(Widget w, char *input, int up_or_down, void *data)
{
  if (!up_or_down)		/* 0 press, 1 release */
    return;
  if (input[1] == '\0') {	/* single character to handle */
    handle_key(input[0]);
  }
}
#endif

/* [re]initialize graphics screen */
void
init_screen(int firsttime)
{
  if (firsttime) {
#ifdef XSCOPE
    h_points = XX[mode];
    v_points = XY[mode];
    w[0] = MakeDrawArea(h_points, v_points, redisplay, NULL);
    SetKeypressCB(w[0], keys_x11);
    ShowDisplay();
    GetStandardColors();
    SetBgColor(w[0], BLACK);
    clear();
    font = GetFont(fontname);
    SetWidgetFont(w[0], font);
#else
/*     vga_disabledriverreport(); */
    vga_init();
#ifdef HAVEVGAMISC
    vga_initfont (fontname, &font, 1, 1);
#endif
#endif
  }
#ifndef XSCOPE
  vga_setmode(screen_modes[mode]);
  v_points = vga_getydim();
  h_points = vga_getxdim();
#endif
  offset = v_points / 2 - 127;
  clear();
  draw_graticule();
}

/* graph the data on the display */
static inline void
draw_data()
{
  static int i, j, k;

  if (point_mode) {
    for (j = 0 ; j < channels ; j++) {
      for (i = 1 ; i < (h_points / scale - 1) ; i++) {
	k = i * channels + j;	/* calc this offset once for efficiency */
	VGA_SETCOLOR(BLACK);	/* erase previous dot */
	VGA_DRAWPIXEL(i * scale,
		      old[k] + offset);
	VGA_SETCOLOR(colour + j); /* draw dot */
	VGA_DRAWPIXEL(i * scale,
		      buffer[k] + offset);
	old[k] = buffer[k];
      }
    }
  } else {			/* line mode, a little slower */
    for (j = 0 ; j < channels ; j++) {
      for (i = 1 ; i < (h_points / scale - 2) ; i++) {
	k = i * channels + j;	/* calc this offset once for efficiency */
	VGA_SETCOLOR(BLACK);	/* erase previous line */
	VGA_DRAWLINE(i * scale,
		     old[k] + offset,
		     i * scale + scale,
		     old[k + channels] + offset);
	VGA_SETCOLOR(colour + j); /* draw line */
	VGA_DRAWLINE(i * scale,
		     buffer[k] + offset,
		     i * scale + scale,
		     buffer[k + channels] + offset);
	old[k] = buffer[k];
      }
      old[i * channels + j] = buffer[i * channels + j];
    }
  }
#ifdef XSCOPE
  SyncDisplay();
#endif
}

/* get and plot one screen full of data and draw the graticule */
void
animate(void *data)
{
  if (running) {
    get_data();
    if (behind) {
      draw_graticule();		/* plot data on top of graticule */
      draw_data();
    } else {
      draw_data();		/* plot graticule on top of data */
      draw_graticule();
    }
  }
#ifdef XSCOPE
  AddTimeOut(0, animate, NULL);
  if (quit_key_pressed) {
    cleanup();
    exit(0);
  }
#endif
}

/* loop until finished */
void
mainloop()
{
#ifdef XSCOPE
  MainLoop();
#else
  while (!quit_key_pressed) {
    handle_key(vga_getkey());
    animate(NULL);
  }
#endif
}

/* parse X11 args and connect to the display or do nothing much */
int
opendisplay(int argc, char **argv)
{
#ifdef XSCOPE
  if ((argc = OpenDisplay(argc, argv)) == FALSE)
    exit(1);
#endif
  return(argc);
}

/* return a string indicating the source of fonts for the -f usage */
char *
fonts()
{
#ifdef XSCOPE
  return "xlsfonts";
#else
  return "/usr/lib/kbd/consolefonts";
#endif
}

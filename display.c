/*
 * @(#)$Id: display.c,v 1.7 1996/01/28 23:41:24 twitham Exp $
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

/* libsx vs. svgalib drawing routines */
#ifdef XSCOPE

#include <libsx.h>
#define VGA_DRAWPIXEL	DrawPixel
#define VGA_DRAWLINE	DrawLine
#define VGA_SETCOLOR	SetColor
#define ALIGN_RIGHT	1
#define ALIGN_LEFT	2
#define ALIGN_CENTER	3
int
VGA_WRITE(char *s, short x, short y, XFont f, short fg, short bg, char p)
{
  SetColor(fg);
  if (p == ALIGN_CENTER)
    x -= (TextWidth(f, s) / 2);
  else if (p == ALIGN_RIGHT)
    x -= (TextWidth(f, s));
  DrawText(s, x, y + FontHeight(f));
  return(1);
}
#else

#include <vga.h>
#define VGA_DRAWPIXEL	vga_drawpixel
#define VGA_DRAWLINE	vga_drawline
#define VGA_SETCOLOR	vga_setcolor
#define VGA_WRITE(s,x,y,f,fg,bg,p)	;
#ifdef HAVEVGAMISC
#include <fontutils.h>
#undef VGA_WRITE
#define VGA_WRITE(s,x,y,f,fg,bg,p)	vga_write(s,x,y,&f,fg,bg,p)
#endif

#endif

int color[16];
char *colors[] = {		/* X colors similar to 16 console colors */
  "black",
  "blue",
  "green",
  "cyan",
  "red",
  "magenta",
  "orange",
  "gray75",
  "gray25",
  "blue4",
  "green4",
  "cyan4",
  "red4",
  "magenta4",
  "yellow",
  "white"
};

#ifdef XSCOPE
char fontname[80] = DEF_FX;
Widget w[1];
XFont font;
int XX[] = {640,800,1024,1280};
int XY[] = {480,480, 480, 480};	/* user can resize it taller if really want */
#else
char fontname[80] = DEF_F;
int screen_modes[] = {		/* allowed modes */
    G640x480x16,
    G800x600x16,
    G1024x768x16,
    G1280x1024x16
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

/* how to convert text column to graphics position */
int
col(int x)
{
  if (x < 13)			/* left side; absolute */
    return (x * 8);
  if (x > 67)			/* right side; absolute */
    return (h_points - ((80 - x) * 8));
  return (x * h_points / 80);	/* middle; spread it out proportionally */
}

/* how to convert text row(0-30) to graphics position */
int
row(int y)
{
  return (y * v_points / 30);
}

/* draw text to graphics screen */
void
draw_text()
{
#if defined XSCOPE || defined HAVEVGAMISC
  char speed[20];

  VGA_WRITE(running ? "RUN " : "STOP",
	    col(68), row(15),  font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
  if (verbose) {
    VGA_WRITE(error,  col(0), row(0),  font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
  }
  sprintf(speed, "%5d us/div", 44000000 / sampling / scope.scale);
  VGA_WRITE(speed, col(39), row(25), font, TEXT_FG, TEXT_BG, ALIGN_CENTER);
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
	    sampling, scope.scale, trigger, colour, mode, dma,
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

  VGA_SETCOLOR(color[colour]);
  VGA_DRAWLINE(100, offset-160, h_points-100, offset-160);
  VGA_DRAWLINE(100, offset+160, h_points-100, offset+160);
  VGA_DRAWLINE(100, offset-160, 100, offset+160);
  VGA_DRAWLINE(h_points-100, offset-160, h_points-100, offset+160);

  if (graticule) {

    /* cross-hairs */
    for (i = 320 ; i < h_points - 100 ; i += 220) {
      VGA_DRAWLINE(i, offset-160, i, offset+160);
    }
    VGA_DRAWLINE(100, offset, h_points-100, offset);

    /* vertical dotted lines */
    for (i = 144 ; i < h_points-100 ; i += 44) {
      for (j = (offset - 160) * 10 ; j < (offset + 160) * 10 ; j += 64) {
	VGA_DRAWPIXEL(i, j / 10);
      }
    }

    /* horizontal dotted lines */
    for (i = offset - 160 ; i < (offset + 160) ; i += 32) {
      for (j = 1088 ; j < (h_points-100) * 10 ; j += 88) {
	VGA_DRAWPIXEL(j / 10, i);
      }
    }
    /* a tick mark where the trigger level is */
    if (trigger > -1)
      VGA_DRAWLINE(100, offset+trigger-128, 105, offset+trigger-128);

  }
}

/* graph the data on the display */
static inline void
draw_data()
{
  static int i, j;

  if (point_mode < 2) {
    for (j = 0 ; j < channels ; j++) {
      for (i = 0 ; i <= (h_points - 200) / scope.scale ; i++) {
	if (point_mode == 0) {
	  VGA_SETCOLOR(color[0]);	/* erase previous dot */
	  VGA_DRAWPIXEL(i * scope.scale + 100,
			offset + ch[j].old[i]);
	}
	VGA_SETCOLOR(ch[j].color); /* draw dot */
	VGA_DRAWPIXEL(i * scope.scale + 100,
		      offset + ch[j].data[i]);
	ch[j].old[i] = ch[j].data[i];
      }
    }
  } else {			/* line mode, a little slower */
    for (j = 0 ; j < channels ; j++) {
      for (i = 0 ; i <= (h_points - 200) / scope.scale - 1 ; i++) {
	if (point_mode == 2) {
	  VGA_SETCOLOR(color[0]);	/* erase previous line */
	  VGA_DRAWLINE(i * scope.scale + 100,
		       offset + ch[j].old[i],
		       i * scope.scale + scope.scale + 100,
		       offset + ch[j].old[i + 1]);
	}
	VGA_SETCOLOR(ch[j].color); /* draw line */
	VGA_DRAWLINE(i * scope.scale + 100,
		     offset + ch[j].data[i],
		     i * scope.scale + scope.scale + 100,
		     offset + ch[j].data[i + 1]);
	ch[j].old[i] = ch[j].data[i];
      }
      ch[j].old[i] = ch[j].data[i];
    }
  }
#ifdef XSCOPE
  SyncDisplay();
#endif
}

/* get and plot one screen full of data and draw the graticule */
static inline void
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
  if (quit_key_pressed) {
    cleanup();
    exit(0);
  }
  AddTimeOut(10, animate, NULL);
#endif
}

#ifdef XSCOPE
/* callback to redisplay the X11 screen */
void
redisplay(Widget w, int new_width, int new_height, void *data) {
  h_points = new_width > 640
    ? new_width - (new_width - 200) % 44
    : 640;
  v_points = new_height > 480 ? new_height : 480;
  offset = v_points / 2;
  clear();
  draw_text();
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
  int i;

  if (firsttime) {
#ifdef XSCOPE
    h_points = XX[mode];
    v_points = XY[mode];
    w[0] = MakeDrawArea(h_points, v_points, redisplay, NULL);
    SetKeypressCB(w[0], keys_x11);
    ShowDisplay();
    for (i = 0 ; i < 16 ; i++) {
      color[i] = GetNamedColor(colors[i]);
    }
    SetBgColor(w[0], color[0]);
    clear();
    font = GetFont(fontname);
    SetWidgetFont(w[0], font);
#else
/*     vga_disabledriverreport(); */
    vga_init();
    for (i = 0 ; i < 16 ; i++) {
      color[i] = i;
    }
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
  offset = v_points / 2;
  clear();
  draw_graticule();
}

/* loop until finished */
void
mainloop()
{
#ifdef XSCOPE
  animate(NULL);
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

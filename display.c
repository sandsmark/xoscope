/*
 * @(#)$Id: display.c,v 1.13 1996/01/31 07:40:37 twitham Exp $
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see oscope.c and the file COPYING for more details)
 *
 * This file implements the display for both the console and X11
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "oscope.h"		/* program defaults */
#include "display.h"

/* libsx vs. svgalib drawing routines */
#ifdef XOSCOPE

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

#ifdef XOSCOPE
char fontname[80] = DEF_FX;
int XX[] = {640,800,1024,1280};
int XY[] = {480,600, 768,1024};
XFont font;
Widget w[1];
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

/* restore text mode and close sound device */
void
cleanup()
{
#ifdef XOSCOPE
  FreeFont(font);
#else
  vga_setmode(TEXT);		/* restore text screen */
#endif
  close(snd);			/* close sound device */
}

/* how to convert text column (0-79) to graphics position */
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
  if (y < 5)			/* top; absolute */
    return (y * 16);
  if (y > 24)			/* bottom; absolute */
    return (v_points - ((30 - y) * 16));
  return (y * v_points / 30);	/* center; spread out proportionally */
}

/* draw text to graphics screen */
void
draw_text()
{
#if defined XOSCOPE || defined HAVEVGAMISC
  static char string[81];
  static int i, j;
  static char *strings[] = {
    "Point",
    "Point Accum.",
    "Line",
    "Line Accum.",
    "1 2 3 4",
    "q w e r",
    "a s d f",
    "z x c v",
    "Position Up  ",
    "   Scale Up  ",
    "   Scale Down",
    "Position Down"
  };

  /* above graticule */
  VGA_WRITE("(Esc)", 0, 0, font, KEY_FG, TEXT_BG, ALIGN_LEFT);
  VGA_WRITE("Quit", col(5), 0, font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

  VGA_WRITE("(Tab)", 0, row(1), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
  sprintf(string, "Channels: %d", channels);
  VGA_WRITE(string, col(5), row(1), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

  VGA_WRITE("(Enter)", col(73), 0, font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
  VGA_WRITE("Refresh", col(80), 0, font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

  VGA_WRITE("([)", col(71), row(1), font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
  VGA_WRITE("Graticule", col(80), row(1), font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

  VGA_WRITE("(])", col(71), row(2), font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
  VGA_WRITE(scope.behind ? "Behind   " : "In Front ", col(80), row(2),
	    font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

  VGA_WRITE("(p)", 100, 62, font, KEY_FG, TEXT_BG, ALIGN_LEFT);
  VGA_WRITE(strings[plot_mode],
	    100+8*3, 62, font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

  VGA_WRITE("(9)               (0)", col(40), 62,
	    font, KEY_FG, TEXT_BG, ALIGN_CENTER);
  sprintf(string, "%d S/s * %d", actual, scope.scale);
  VGA_WRITE(string, col(40), 62, font, TEXT_FG, TEXT_BG, ALIGN_CENTER);

  VGA_WRITE("(space)", h_points - 100 - 8*4, 62,
	    font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
  VGA_WRITE(scope.run ? " RUN" : "STOP",
	    h_points - 100, 62, font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

  /* left of graticule */
  for (i = 0 ; i < 4 ; i++) {
    sprintf(string, "Channel %d", i+1);
    VGA_WRITE(string, 0, row(i*5 + 5),
	      font, ch[i].color, TEXT_BG, ALIGN_LEFT);
    sprintf(string, "Scale: %d/%d", mult[ch[i].scale], divi[ch[i].scale]);
    VGA_WRITE(string, 0, row(i*5 + 6),
	      font, ch[i].color, TEXT_BG, ALIGN_LEFT);
    sprintf(string, "Pos. : %d", -(ch[i].pos));
    VGA_WRITE(string, 0, row(i*5 + 7),
	      font, ch[i].color, TEXT_BG, ALIGN_LEFT);
  }
  for (i = 0 ; i < 4 ; i++) {
    for (j = 0 ; j < 4 ; j++) {
      VGA_WRITE(&strings[j+4][i*2], col(i*2+1), row(25+j),
		font, ch[i].color, TEXT_BG, ALIGN_LEFT);
    }
    VGA_WRITE("(", 0, row(25+i), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
    VGA_WRITE(")", col(8), row(25+i), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
    VGA_WRITE(strings[i+8], col(9), row(25+i),
	      font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
  }

  /* below graticule */
  VGA_WRITE("(9)               (0)", col(40), row(25),
	    font, KEY_FG, TEXT_BG, ALIGN_CENTER);
  i = 44000000 / actual / scope.scale;
  sprintf(string, "%d %cs/div",
	  i > 999 ? i / 1000 : i,
	  i > 999 ? 'm' : 'u');
  VGA_WRITE(string, col(40), row(25), font, TEXT_FG, TEXT_BG, ALIGN_CENTER);

  VGA_WRITE("(l)               (o)", col(40), row(27),
	    font, KEY_FG, TEXT_BG, ALIGN_CENTER);
  sprintf(string, "Trigger: %d", -(scope.trig - 128));
  VGA_WRITE(scope.trig > -1 ? string : "No Trigger", col(40), row(27),
	    font, TEXT_FG, TEXT_BG, ALIGN_CENTER);

#endif
}

/* clear the display and redraw any text */
void
clear()
{
#ifdef XOSCOPE
  ClearDrawArea();
#else
  vga_clear();
#endif
  draw_text();
}

/* if verbose mode, show current parameter settings on standard out */
void
show_info(unsigned char c) {
  if (verbose) {
    sprintf(error, "%1c %5dHz:  -%d  -r %5d  -s %2d  -t %3d  -c %2d  -m %2d  "
	    "-d %1d  %2s  %2s  %2s%s",
	    c, actual, channels,
	    scope.rate, scope.scale, scope.trig, scope.color, mode, dma,
	    plot_mode ? "-p" : "-l",

	    /* reverse logic if these booleans are on by default in oscope.h */
#if DEF_G
	    scope.grat ? "" : "-g",
#else
	    scope.grat ? "-g" : "",
#endif

#if DEF_B
	    scope.behind ? "" : "-b",
#else
	    scope.behind ? "-b" : "",
#endif

#if DEF_V
	    verbose ? "" : "  -v"
#else
	    verbose ? "  -v" : ""
#endif
	    );
    printf("%s\n", error);
  }
}

/* if graticule mode, draw graticule, always draw frame */
static inline void
draw_graticule()
{
  static int i, j;

  VGA_SETCOLOR(color[scope.color]);
  VGA_DRAWLINE(100, 80, h_points-100, 80);
  VGA_DRAWLINE(100, v_points - 80, h_points-100, v_points - 80);
  VGA_DRAWLINE(100, 80, 100, v_points - 80);
  VGA_DRAWLINE(h_points-100, 80, h_points-100, v_points - 80);

  /* a tick mark where the trigger level is */
  if (scope.trig > -1) {
    i = offset + scope.trig + ch[0].pos - 128;
    VGA_DRAWLINE(100, i, 110, i);
  }

  if (scope.grat) {

    /* cross-hairs every 5 x 4 divisions */
    for (i = 320 ; i < h_points - 100 ; i += 220) {
      VGA_DRAWLINE(i, 80, i, v_points - 80);
    }
    for (i = 0 ; (offset - i) > 80 ; i += 128) {
      VGA_DRAWLINE(100, offset - i, h_points - 100, offset - i);
      VGA_DRAWLINE(100, offset + i, h_points - 100, offset + i);
    }

    /* vertical dotted lines */
    for (i = 144 ; i < h_points-100 ; i += 44) {
      for (j = 800 ; j < (v_points - 80) * 10 ; j += 64) {
	VGA_DRAWPIXEL(i, j / 10);
      }
    }

    /* horizontal dotted lines */
    for (i = 80 ; i < v_points - 80 ; i += 32) {
      for (j = 1088 ; j < (h_points-100) * 10 ; j += 88) {
	VGA_DRAWPIXEL(j / 10, i);
      }
    }
  }
}

/* graph the data on the display */
static inline void
draw_data()
{
  static int i, j, off;
  static Signal *p;

  if (plot_mode < 2) {		/* point / point accumulate */
    for (j = 0 ; j < channels ; j++) {
      p = &ch[j];
      off = offset + p->pos;
      VGA_SETCOLOR(p->color);
      for (i = 0 ; i <= (h_points - 200) / scope.scale ; i++) {
	if (plot_mode == 0) {	/* erase previous dots */
	  VGA_SETCOLOR(color[0]);
	  VGA_DRAWPIXEL(i * scope.scale + 100,
			off + p->old[i] * mult[p->scale] / divi[p->scale]);
	  VGA_SETCOLOR(p->color); /* draw dots */
	}
	VGA_DRAWPIXEL(i * scope.scale + 100,
		      off + p->data[i] * mult[p->scale] / divi[p->scale]);
	p->old[i] = p->data[i];
      }
      VGA_DRAWLINE(90, off, 100, off);
    }
  } else {			/* line / line accumulate */
    for (j = 0 ; j < channels ; j++) {
      p = &ch[j];
      off = offset + p->pos;
      VGA_SETCOLOR(p->color);
      for (i = 0 ; i < (h_points - 200) / scope.scale ; i++) {
	if (plot_mode == 2) {	/* erase previous lines */
	  VGA_SETCOLOR(color[0]);
	  VGA_DRAWLINE(i * scope.scale + 100,
		       off + p->old[i] * mult[p->scale] / divi[p->scale],
		       i * scope.scale + 100 + scope.scale,
		       off + p->old[i + 1] * mult[p->scale] / divi[p->scale]);
	  VGA_SETCOLOR(p->color); /* draw lines */
	}
	VGA_DRAWLINE(i * scope.scale + 100,
		     off + p->data[i] * mult[p->scale] / divi[p->scale],
		     i * scope.scale + 100 + scope.scale,
		     off + p->data[i + 1] * mult[p->scale] / divi[p->scale]);
	p->old[i] = p->data[i];
      }
      VGA_DRAWLINE(90, off, 100, off);
      p->old[i] = p->data[i];
    }
  }
#ifdef XOSCOPE
  SyncDisplay();
#endif
}

/* get and plot one screen full of data and draw the graticule */
static inline void
animate(void *data)
{
  if (scope.run)
    get_data();
  if (scope.behind) {
    draw_graticule();		/* plot data on top of graticule */
    draw_data();
  } else {
    draw_data();		/* plot graticule on top of data */
    draw_graticule();
  }
#ifdef XOSCOPE
  if (quit_key_pressed) {
    cleanup();
    exit(0);
  }
  AddTimeOut(MSECREFRESH, animate, NULL);
#endif
}

#ifdef XOSCOPE
/* callback to redisplay the X11 screen */
void
redisplay(Widget w, int new_width, int new_height, void *data) {
  h_points = new_width > 640
    ? new_width - (new_width - 200) % 44
    : 640;
  v_points = new_height > 480
    ? new_height - (new_height - 160) % 64
    : 480;
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
#ifdef XOSCOPE
    h_points = XX[mode];
    v_points = XY[mode];
    w[0] = MakeDrawArea(h_points, v_points, redisplay, NULL);
    SetKeypressCB(w[0], keys_x11);
    ShowDisplay();
    for (i = 0 ; i < 16 ; i++) {
      color[i] = GetNamedColor(colors[i]);
    }
    SetBgColor(w[0], color[0]);
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
#ifndef XOSCOPE
  vga_setmode(screen_modes[mode]);
  v_points = vga_getydim();
  h_points = vga_getxdim();
#endif
  offset = v_points / 2;
}

/* loop until finished */
void
mainloop()
{
  draw_text();
#ifdef XOSCOPE
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
#ifdef XOSCOPE
  if ((argc = OpenDisplay(argc, argv)) == FALSE)
    exit(1);
#endif
  return(argc);
}

/* return a string indicating the source of fonts for the -f usage */
char *
fonts()
{
#ifdef XOSCOPE
  return "xlsfonts";
#else
  return "/usr/lib/kbd/consolefonts";
#endif
}

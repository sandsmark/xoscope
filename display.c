/*
 * @(#)$Id: display.c,v 1.32 1996/10/05 05:49:52 twitham Exp $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements display code common to console and X11
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "oscope.h"		/* program defaults */
#include "display.h"
#include "func.h"

/* libsx vs. svgalib drawing routines */
#ifdef XOSCOPE

#include "x11.h"

#else

#include <vga.h>
#define VGA_DRAWPIXEL(x,y)	vga_drawpixel(x, y > 0 ? y : 0)
#define VGA_DRAWLINE(x1,y1,x2,y2)	\
	vga_drawline(x1, y1 > 0 ? y1 : 0, x2, y2 > 0 ? y2 : 0)
#define VGA_SETCOLOR	vga_setcolor
#define VGA_WRITE(s,x,y,f,fg,bg,p)	;
#ifdef HAVEVGAMISC
#include <fontutils.h>
#include <miscutils.h>
#undef VGA_WRITE
#define VGA_WRITE(s,x,y,f,fg,bg,p)	vga_write(s,x,y,&f,fg,bg,p)
#endif

#endif

void
show_data();
int triggered = 0;		/* whether we've triggered or not */
int data_good = 1;		/* whether data may be "stale" or not */

#ifdef XOSCOPE
char fontname[80] = DEF_FX;
#else
char fontname[80] = DEF_F;
int screen_modes[] = {		/* allowed modes */
  G640x480x16,
  G800x600x16,
  G1024x768x16,
  G1280x1024x16
};
int color[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
#ifdef HAVEVGAMISC
font_t font;			/* pointer to the font structure */
#endif
#endif

/* restore text mode and close sound device */
void
cleanup()
{
  cleanup_math();
#ifdef XOSCOPE
  cleanup_x11();
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
  /* middle; spread it out proportionally */
  return ((x - 12) * (h_points - 200) / 55 + 100);
}

/* how to convert text row(0-29) to graphics position */
int
row(int y)
{
  if (y < 5)			/* top; absolute */
    return (y * 16);
  if (y > 24)			/* bottom; absolute */
    return (v_points - ((30 - y) * 16));
  /* center; spread out proportionally */
  return ((y - 5) * (v_points - 160) / 20 + 80);
}

#ifndef XOSCOPE
/* get a file name */
char *
GetFile(char *path)
{
#ifdef HAVEVGAMISC
  char *s;

  s = vga_prompt(col(20), v_points / 2,
		 40 * 8, 8 + font.font_height, "Filename:",
		 &font, &font, TEXT_FG, KEY_FG, TEXT_BG, PROMPT_SCROLLABLE);
  if (s[0] == '\e' || s[0] == '\0')
    return(NULL);
  return(s);
#else
  return(filename);
#endif
}

/* prompt for a string */
char *
GetString(char *msg, char *def)
{
#ifdef HAVEVGAMISC
  char *s;

  s = vga_prompt(0, v_points / 2,
		 80 * 8, 8 + font.font_height, msg,
		 &font, &font, TEXT_FG, KEY_FG, TEXT_BG, PROMPT_SCROLLABLE);
  if (s[0] == '\e' || s[0] == '\0')
    return(NULL);
  return(s);
#else
  return(NULL);
#endif
}
#endif

/* draw a temporary one-line message to center of screen */
void
message(char *message, int clr)
{
#if defined XOSCOPE || defined HAVEVGAMISC
  VGA_WRITE("                                                  ",
	    h_points / 2, v_points / 2,
	    font, clr, TEXT_BG, ALIGN_CENTER);
  VGA_WRITE(message, h_points / 2, v_points / 2,
	    font, clr, TEXT_BG, ALIGN_CENTER);
#endif
}

/* draw just dynamic or all text to graphics screen */
void
draw_text(int all)
{
#if defined XOSCOPE || defined HAVEVGAMISC
  static char string[81];
  static int i, j, k;
  static Signal *p;
  static char *strings[] = {
    "Point",
    "Point Accum.",
    "Line",
    "Line Accum."
  };
  static char *trigs[] = {
    "Auto",
    "Rising",
    "Falling"
  };

  p = &ch[scope.select];
  if (all) {			/* everything */

    /* above graticule */
    VGA_WRITE("(Esc)", 0, 0, font, KEY_FG, TEXT_BG, ALIGN_LEFT);
    VGA_WRITE("Quit", col(5), 0, font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

    VGA_WRITE("(@)", col(2), row(1), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
    VGA_WRITE("Load", col(5), row(1), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

    VGA_WRITE("(#)", col(2), row(2), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
    VGA_WRITE("Save", col(5), row(2), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

    sprintf(string, "%d x %d", h_points, v_points);
    VGA_WRITE(string, 0, row(3), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

    VGA_WRITE("(Enter)", col(70), 0, font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
    VGA_WRITE("Refresh", col(77), 0, font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

    VGA_WRITE("(&)", col(70), row(1), font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
    VGA_WRITE("Graticule", col(79), row(1),
	      font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

    VGA_WRITE("(_)(-)                      (=)(+)", col(40), row(2),
	      font, KEY_FG, TEXT_BG, ALIGN_CENTER);
    sprintf(string, "%s Trigger @ %d", trigs[scope.trige], scope.trig - 128);
    VGA_WRITE(string, col(40), row(2),
	      font, ch[scope.trigch].color, TEXT_BG, ALIGN_CENTER);

    VGA_WRITE("(*)", col(70), row(2), font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
    VGA_WRITE(scope.behind ? "Behind   " : "In Front ", col(79), row(2),
	      font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

    VGA_WRITE("(()      ())", col(79), row(3),
	      font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
    VGA_WRITE("Color", col(75), row(3), font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

    VGA_WRITE("(!)", 100, 62, font, KEY_FG, TEXT_BG, ALIGN_LEFT);
    VGA_WRITE(strings[scope.mode],
	      100 + 8 * 3, 62, font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

    VGA_WRITE("(space)", h_points - 100 - 8 * 4, 62,
	      font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
    VGA_WRITE(scope.run ? (scope.run > 1 ? "WAIT" : " RUN") : "STOP",
	      h_points - 100, 62, font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

    /* sides of graticule */
    for (i = 0 ; i < CHANNELS ; i++) {
      j = (i % 4) * 5 + 5;
      k = ch[i].color;

      VGA_WRITE("Channel", col(69 * (i / 4)), row(j),
		font, k, TEXT_BG, ALIGN_LEFT);
      sprintf(string, "(%d)", i + 1);
      VGA_WRITE(string, col(69 * (i / 4) + 7), row(j),
		font, KEY_FG, TEXT_BG, ALIGN_LEFT);

      sprintf(string, "Pos. :%4d", -(ch[i].pos));
      VGA_WRITE(string, col(69 * (i / 4)), row(j + 1),
		font, k, TEXT_BG, ALIGN_LEFT);

      sprintf(string, "Scale:%d/%d", ch[i].mult, ch[i].div);
      VGA_WRITE(string, col(69 * (i / 4)), row(j + 2),
		font, k, TEXT_BG, ALIGN_LEFT);

      VGA_WRITE(funcnames[ch[i].func], col(69 * (i / 4)), row(j + 3),
		font, k, TEXT_BG, ALIGN_LEFT);

      if (ch[i].func == FUNCMEM) {
	sprintf(string, "%c", ch[i].mem);
	VGA_WRITE(string, col(69 * (i / 4) + 7), row(j + 3),
		  font, memcolor[ch[i].mem - 'a'], TEXT_BG, ALIGN_LEFT);
      }

      if (scope.select == i) {
	VGA_SETCOLOR(k);
	k = i < 4 ? 11 : 80 - 11;
	VGA_DRAWLINE(col(i < 4 ? 0 : 79), row(j), col(k), row(j));
	VGA_DRAWLINE(col(k), row(j), col(k), row(j + 5) - 1);
	VGA_DRAWLINE(col(i < 4 ? 0 : 79), row(j + 5) - 1,
		     col(k), row(j + 5) - 1);
      }
    }

    /* below graticule */
    VGA_WRITE("(Tab)", 0, row(25), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
    VGA_WRITE(p->show ? "Visible" : "HIDDEN ", col(5), row(25),
	      font, p->color, TEXT_BG, ALIGN_LEFT);

    VGA_WRITE("({)        (})", 0, row(26), font,KEY_FG, TEXT_BG, ALIGN_LEFT);
    VGA_WRITE("Position", col(3), row(26), font, p->color, TEXT_BG, ALIGN_LEFT);

    VGA_WRITE("([)        (])", 0, row(27), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
    VGA_WRITE("Scale", col(4), row(27), font, p->color, TEXT_BG, ALIGN_LEFT);

    VGA_WRITE("(,)        (.)", 0, row(28), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
    sprintf(string, "DMA:%d", scope.dma);
    VGA_WRITE(string, col(4), row(28), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

    i = 44000000 / actual / scope.scale;
    sprintf(string, "%d %cs/div",
	    i > 999 ? i / 1000 : i,
	    i > 999 ? 'm' : 'u');
    VGA_WRITE(string, col(40), row(25), font, TEXT_FG, TEXT_BG, ALIGN_CENTER);

    VGA_WRITE("(9)                 (0)", col(40), row(26),
	      font, KEY_FG, TEXT_BG, ALIGN_CENTER);
    sprintf(string, "%d S/s * %d", actual, scope.scale);
    VGA_WRITE(string, col(40), row(26), font, TEXT_FG, TEXT_BG, ALIGN_CENTER);

    i = actual / 20 / scope.scale;
    sprintf(string, "%d Hz/div FFT", i);
    VGA_WRITE(string, col(40), row(27), font, TEXT_FG, TEXT_BG, ALIGN_CENTER);

    if (actual >= 44000) {
      VGA_WRITE("(A-Z)", col(72), row(27), font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      VGA_WRITE("Store", col(77), row(27),
		font, p->color, TEXT_BG, ALIGN_RIGHT);
    }

    if (scope.select > 1) {
      VGA_WRITE("($)", col(72), row(25),
		font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      VGA_WRITE("Extern", col(78), row(25),
		font, p->color, TEXT_BG, ALIGN_RIGHT);
      VGA_WRITE("(:)    (;)", col(79), row(26),
		font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      VGA_WRITE("Math", col(76), row(26),
		font, p->color, TEXT_BG, ALIGN_RIGHT);
      VGA_WRITE("(a-z)", col(72), row(28),
		font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      VGA_WRITE("Recall", col(78), row(28),
		font, p->color, TEXT_BG, ALIGN_RIGHT);
    }

    VGA_WRITE("(", col(26), row(28), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
    VGA_WRITE(")", col(53), row(28), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
    for (i = 0 ; i < 26 ; i++) {
      if (mem[i] != NULL) {
	sprintf(string, "%c", i + 'a');
	VGA_WRITE(string, col(27 + i), row(28),
		  font, memcolor[i], TEXT_BG, ALIGN_LEFT);
      }
    }
#ifdef XOSCOPE
    if (all == 1)
      fix_widgets();
#endif
    show_data();
    data_good = 0;		/* data may be bad if user just tweaked S/s */
    return;			/* show_data will call again to do the rest */
  }

  /* always draw the dynamic text */
  sprintf(string, "Period of %6d us = %5d Hz", p->time,  p->freq);
  VGA_WRITE(string, col(40), row(0), font, p->color, TEXT_BG, ALIGN_CENTER);
  
  sprintf(string, " Max:%3d - Min:%4d = %3d Pk-Pk ",
	  p->max, p->min, p->max - p->min);
  VGA_WRITE(string, col(40), row(1), font, p->color, TEXT_BG, ALIGN_CENTER);

  VGA_WRITE(triggered ? " Triggered " : "? TRIGGER ?", col(40), row(3),
	    font, ch[scope.trigch].color, TEXT_BG, ALIGN_CENTER);
#endif
}

/* clear the display and redraw all text */
void
clear()
{
#ifdef XOSCOPE
  ClearDrawArea();
#else
  vga_clear();
#endif
  draw_text(1);
}

/* if graticule mode, draw graticule, always draw frame */
static inline void
draw_graticule()
{
  static int i, j;
  static int tilt[] = {
    0, -10, 10
  };

  /* a mark where the trigger level is */
  i = scope.trigch;
  j = offset + ch[i].pos + (128 - scope.trig) * ch[i].mult / ch[i].div;
  VGA_SETCOLOR(ch[i].color);
  VGA_DRAWLINE(90, j + tilt[scope.trige], 110, j - tilt[scope.trige]);

  /* the frame */
  VGA_SETCOLOR(clip ? ch[clip - 1].color : color[scope.color]);
  VGA_DRAWLINE(100, 80, h_points - 100, 80);
  VGA_DRAWLINE(100, v_points - 80, h_points - 100, v_points - 80);
  VGA_DRAWLINE(100, 80, 100, v_points - 80);
  VGA_DRAWLINE(h_points - 100, 80, h_points - 100, v_points - 80);

  if (scope.grat) {

    /* cross-hairs every 5 x 5 divisions */
    if (scope.grat > 1) {
      for (i = 320 ; i < h_points - 100 ; i += 220) {
	VGA_DRAWLINE(i, 80, i, v_points - 80);
      }
      for (i = 0 ; (offset - i) > 80 ; i += 160) {
	VGA_DRAWLINE(100, offset - i, h_points - 100, offset - i);
	VGA_DRAWLINE(100, offset + i, h_points - 100, offset + i);
      }
    }

    /* vertical dotted lines */
    for (i = 144 ; i < h_points - 100 ; i += 44) {
      for (j = 864 ; j < (v_points - 80) * 10 ; j += 64) {
	VGA_DRAWPIXEL(i, j / 10);
      }
    }

    /* horizontal dotted lines */
    for (i = 112 ; i < v_points - 80 ; i += 32) {
      for (j = 1088 ; j < (h_points - 100) * 10 ; j += 88) {
	VGA_DRAWPIXEL(j / 10, i);
      }
    }
  }
}

/* graph the data on the display */
static inline void
draw_data()
{
  static int i, j, k, l, off, delay, old = 0, mult, div, mode;
  static Signal *p;
  static short *samples;

  mode = data_good ?  scope.mode : (scope.mode < 2 ? 0 : 2);
  /* interpolate a line between the sample just before and after trigger */
  off = 100 - scope.scale;	/* then shift all samples horizontally */
  if (scope.trige) {		/* to place time zero at trigger */
    p = &ch[scope.trigch];
    if ((i = p->data[0]) != (j = p->data[1])) /* avoid divide by zero */
      delay = 100 - ((scope.trig - 128 - i) * scope.scale / (j - i));
    if (delay < off)
      delay = old;		/* may be out of range if user just tweaked */
    if (delay > 100)		/* trigger but we're using old data */
      delay = old;
  } else			/* no trigger, leave delay as it was */
    delay = old;
  for (j = 0 ; j < CHANNELS ; j++) {
    p = &ch[j];
    if (p->show) {		/* plot each visible channel */
      mult = p->mult;
      div = p->div;
      off = offset + p->pos;
      for (k = !(mode % 2) ; k >= 0 ; k--) { /* once=accumulate, twice=erase */
	if (k) {		/* erase previous samples */
	  VGA_SETCOLOR(color[0]);
	  samples = p->old + 1;
	  l = old;
	} else {		/* plot new samples */
	  VGA_SETCOLOR(p->color);
	  samples = p->data + 1;
	  l = delay;
	}
	if (mode < 2)		/* point / point accumulate */
	  for (i = 1 ; i <= (h_points - 200) / scope.scale ; i++) {
	    VGA_DRAWPIXEL(i * scope.scale + l,
			  off - *samples * mult / div);
	    samples++;
	  }
	else			/* line / line accumulate */
	  for (i = 1 ; i < (h_points - 200) / scope.scale ; i++) {
	    VGA_DRAWLINE(i * scope.scale + l,
			 off - *samples * mult / div,
			 i * scope.scale + l + scope.scale,
			 off - *(samples + 1) * mult / div);
	    samples++;
	  }
      }
      memcpy(p->old, p->data,
	     sizeof(short) * (h_points - 200) / scope.scale + sizeof(short));
      VGA_DRAWLINE(90, off, 100, off);
      VGA_DRAWLINE(h_points - 100, off, h_points - 90, off);
    }
  }
  old = delay;
#ifdef XOSCOPE
  SyncDisplay();
#endif
}

/* calculate any math and plot the results and the graticule */
void
show_data()
{
  do_math();
  measure_data(&ch[scope.select]);
  draw_text(0);
  if (scope.behind) {
    draw_graticule();		/* plot data on top of graticule */
    draw_data();
  } else {
    draw_data();		/* plot graticule on top of data */
    draw_graticule();
  }
}

/* get and plot one screen full of data */
static inline void
animate(void *data)
{
  if (scope.run) {
    triggered = get_data();
    if (triggered) {
      if (scope.run > 1) {	/* auto-stop single-shot wait */
	scope.run = 0;
	draw_text(1);
      }
      show_data();
    } else			/* no need to redraw samples */
      draw_text(0);
    data_good = 1;
  }
#ifdef XOSCOPE
  if (quit_key_pressed) {
    cleanup();
    exit(0);
  }
  AddTimeOut(MSECREFRESH, animate, NULL);
#endif
}

/* [re]initialize graphics screen */
void
init_screen()
{
  int i, channelcolor[] = CHANNELCOLOR;

#ifdef XOSCOPE
  init_widgets();		/* get color definitions from x11.c */
#endif
  for (i = 0 ; i < CHANNELS ; i++) {
    ch[i].color = color[channelcolor[i]];
  }
#ifdef XOSCOPE
  fix_widgets();		/* reset widget colors */
#else
  /*     vga_disabledriverreport(); */
  vga_init();
#ifdef HAVEVGAMISC
  vga_initfont (fontname, &font, 1, 1);
#endif
  vga_setmode(screen_modes[scope.size]);
  v_points = vga_getydim();
  h_points = vga_getxdim();
#endif
  offset = v_points / 2;
}

/* loop until finished */
void
mainloop()
{
  draw_text(1);
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

/* parse command-line args and connect to the X display if necessary */
void
opendisplay(int argc, char **argv)
{
#ifdef XOSCOPE
  if ((argc = OpenDisplay(argc, argv)) == FALSE)
    exit(1);
#endif
  parse_args(argc, argv);
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

/*
 * @(#)$Id: display.c,v 1.38 1997/05/01 04:45:09 twitham Exp $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements display code common to console and X11
 *
 */

#include <stdio.h>
#include "oscope.h"		/* program defaults */
#include "display.h"
#include "func.h"

void	show_data();		/* other function prototypes */
int	vga_write();
void	DrawPixel();		/* these are defined in */
void	DrawLine();		/* a display-specific file */
void	SetColor();		/* like gr_vga.c or gr_sx.c */
void	init_widgets();
void	fix_widgets();
void	clear_display();
void	SyncDisplay();
void	AddTimeOut();

int triggered = 0;		/* whether we've triggered or not */
int data_good = 1;		/* whether data may be "stale" or not */
void *font;

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

/* draw a temporary one-line message to center of screen */
void
message(char *message, int clr)
{
  vga_write("                                                  ",
	    h_points / 2, v_points / 2,
	    font, clr, TEXT_BG, ALIGN_CENTER);
  vga_write(message, h_points / 2, v_points / 2,
	    font, clr, TEXT_BG, ALIGN_CENTER);
}

/* draw just dynamic or all text to graphics screen */
void
draw_text(int all)
{
  static char string[81];
  static int i, j, k;
  static Channel *p;
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
    if (scope.verbose) {
      vga_write("(Esc)", 0, 0, font, KEY_FG, TEXT_BG, ALIGN_LEFT);
      vga_write("Quit", col(5), 0, font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
      vga_write(progname,  col(12), 0, font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

      vga_write("(@)", col(2), row(1), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
      vga_write("Load   ver:", col(5), row(1),
		font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
      vga_write(version,  col(16), row(1), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

      vga_write("(#)", col(2), row(2), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
      vga_write("Save", col(5), row(2), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
      sprintf(string, "%d x %d", h_points, v_points);
      vga_write(string, col(12), row(2), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

      vga_write("(Enter)", col(70), 0, font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      vga_write("Refresh", col(77), 0, font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

      vga_write("(,)", col(70), row(1), font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      vga_write("Graticule", col(79), row(1),
		font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

      vga_write("(_)(-)                      (=)(+)", col(40), row(2),
		font, KEY_FG, TEXT_BG, ALIGN_CENTER);

      vga_write("(.)", col(70), row(2), font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      vga_write(scope.behind ? "Behind   " : "In Front ", col(79), row(2),
		font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

      vga_write("(<)      (>)", col(79), row(3),
		font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      vga_write("Color", col(75), row(3), font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

      vga_write("(!)", 100, 62, font, KEY_FG, TEXT_BG, ALIGN_LEFT);
      vga_write("(space)", h_points - 100 - 8 * 4, 62,
		font, KEY_FG, TEXT_BG, ALIGN_RIGHT);

    } else {			/* not verbose */

      vga_write("(?)", col(75), 0, font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      vga_write("Help", col(79), 0, font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);
    }

    sprintf(string, "%s Trigger @ %d", trigs[scope.trige], scope.trig - 128);
    vga_write(string, col(40), row(2),
	      font, ch[scope.trigch].color, TEXT_BG, ALIGN_CENTER);
    vga_write(strings[scope.mode],
	      100 + 8 * 3, 62, font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
    vga_write(scope.run ? (scope.run > 1 ? "WAIT" : " RUN") : "STOP",
	      h_points - 100, 62, font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

    /* sides of graticule */
    for (i = 0 ; i < CHANNELS ; i++) {

      j = (i % 4) * 5 + 5;
      k = ch[i].color;

      vga_write("Channel", col(69 * (i / 4)), row(j),
		font, k, TEXT_BG, ALIGN_LEFT);
      sprintf(string, "(%d)", i + 1);
      vga_write(string, col(69 * (i / 4) + 7), row(j),
		font, KEY_FG, TEXT_BG, ALIGN_LEFT);

      if (scope.verbose || ch[i].show || scope.select == i) {
	sprintf(string, "%d / %d", ch[i].mult, ch[i].div);
	vga_write(string, col(69 * (i / 4) + 5), row(j + 1),
		  font, k, TEXT_BG, ALIGN_CENTER);

	sprintf(string, "@ %d", -(ch[i].pos));
	vga_write(string, col(69 * (i / 4) + 5), row(j + 2),
		  font, k, TEXT_BG, ALIGN_CENTER);

	vga_write(funcnames[ch[i].func], col(69 * (i / 4)), row(j + 3),
		  font, ch[i].func < 2 ? ch[i].signal->color : k,
		  TEXT_BG, ALIGN_LEFT);

	if (ch[i].func == FUNCMEM) {
	  sprintf(string, "%c", ch[i].mem);
	  vga_write(string, col(69 * (i / 4) + 7), row(j + 3),
		    font, ch[i].signal->color, TEXT_BG, ALIGN_LEFT);
	}
      }
      if (scope.select == i) {
	SetColor(k);
	k = i < 4 ? 11 : 80 - 11;
	DrawLine(col(i < 4 ? 0 : 79), row(j), col(k), row(j));
	DrawLine(col(k), row(j), col(k), row(j + 5) - 1);
	DrawLine(col(i < 4 ? 0 : 79), row(j + 5) - 1,
		 col(k), row(j + 5) - 1);
      }
    }

    /* below graticule */
    if (scope.verbose) {
      vga_write("(Tab)", 0, row(25), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
      vga_write(p->show ? "Visible" : "HIDDEN ", col(5), row(25),
		font, p->color, TEXT_BG, ALIGN_LEFT);

      vga_write("({)        (})", 0, row(26), font,KEY_FG, TEXT_BG, ALIGN_LEFT);
      vga_write("Scale", col(4), row(26), font, p->color, TEXT_BG, ALIGN_LEFT);

      vga_write("([)        (])", 0, row(27), font, KEY_FG, TEXT_BG,ALIGN_LEFT);
      vga_write("Position", col(3), row(27), font, p->color,TEXT_BG,ALIGN_LEFT);

      vga_write("(&)        (*)", 0, row(28), font, KEY_FG, TEXT_BG,ALIGN_LEFT);
      sprintf(string, "DMA:%d", scope.dma);
      vga_write(string, col(4), row(28), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

      vga_write("(9)(()                 ())(0)", col(40), row(26),
		font, KEY_FG, TEXT_BG, ALIGN_CENTER);

      vga_write("(A-Z)", col(72), row(27), font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      vga_write("Store", col(77), row(27),
		font, p->color, TEXT_BG, ALIGN_RIGHT);

      if (scope.select > 1) {
	vga_write("($)", col(72), row(25),
		  font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
	vga_write("Extern", col(78), row(25),
		  font, p->color, TEXT_BG, ALIGN_RIGHT);
	vga_write("(:)    (;)", col(79), row(26),
		  font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
	vga_write("Math", col(76), row(26),
		  font, p->color, TEXT_BG, ALIGN_RIGHT);
      }

      vga_write("(a-z)", col(72), row(28),
		font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      vga_write("Recall", col(78), row(28),
		font, p->color, TEXT_BG, ALIGN_RIGHT);

      vga_write("(", col(26), row(28), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
      vga_write(")", col(53), row(28), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
    }
    i = 1000 / scope.scale;
    sprintf(string, "%d %cs/div", i > 999 ? i / 1000 : i, i > 999 ? 'm' : 'u');
    vga_write(string, col(40), row(25), font, TEXT_FG, TEXT_BG, ALIGN_CENTER);

    sprintf(string, "%d S/s * %d", p->signal->rate, scope.scale * 44000
	    / p->signal->rate);
    vga_write(string, col(40), row(26), font, p->color, TEXT_BG, ALIGN_CENTER);

    i = p->signal->rate / 20 * p->signal->rate / 44000 / scope.scale;
    sprintf(string, "%d Hz/div FFT", i);
    vga_write(string, col(40), row(27), font, p->color, TEXT_BG, ALIGN_CENTER);

    for (i = 0 ; i < 26 ; i++) {
      sprintf(string, "%c", i + 'a');
      vga_write(string, col(27 + i), row(28),
		font, mem[i].color, TEXT_BG, ALIGN_LEFT);
    }

    if (all == 1)
      fix_widgets();
    show_data();
    data_good = 0;		/* data may be bad if user just tweaked S/s */
    return;			/* show_data will call again to do the rest */
  }

  /* always draw the dynamic text */
  sprintf(string, "Period of %6d us = %5d Hz", p->time,  p->freq);
  vga_write(string, col(40), row(0), font, p->color, TEXT_BG, ALIGN_CENTER);
  
  sprintf(string, " Max:%3d - Min:%4d = %3d Pk-Pk ",
	  p->max, p->min, p->max - p->min);
  vga_write(string, col(40), row(1), font, p->color, TEXT_BG, ALIGN_CENTER);

  vga_write(triggered ? " Triggered " : "? TRIGGER ?", col(40), row(3),
	    font, ch[scope.trigch].color, TEXT_BG, ALIGN_CENTER);
}

/* clear the display and redraw all text */
void
clear()
{
  clear_display();
  draw_text(1);
}

/* if graticule mode, draw graticule, always draw frame */
void
draw_graticule()
{
  static int i, j;
  static int tilt[] = {
    0, -10, 10
  };

  /* a mark where the trigger level is, if the triggered channel is shown */
  i = -1;
  for (j = 7 ; j >= 0 ; j--) {
    if (ch[j].func == scope.trigch)
      i = j;
  }
  if (i > -1) {
    j = offset + ch[i].pos + (128 - scope.trig) * ch[i].mult / ch[i].div;
    SetColor(mem[scope.trigch + 23].color);
    DrawLine(90, j + tilt[scope.trige], 110, j - tilt[scope.trige]);
  }

  /* the frame */
  SetColor(clip ? ch[clip - 1].color : color[scope.color]);
  DrawLine(100, 80, h_points - 100, 80);
  DrawLine(100, v_points - 80, h_points - 100, v_points - 80);
  DrawLine(100, 80, 100, v_points - 80);
  DrawLine(h_points - 100, 80, h_points - 100, v_points - 80);

  if (scope.grat) {

    /* cross-hairs every 5 x 5 divisions */
    if (scope.grat > 1) {
      for (i = 320 ; i < h_points - 100 ; i += 220) {
	DrawLine(i, 80, i, v_points - 80);
      }
      for (i = 0 ; (offset - i) > 80 ; i += 160) {
	DrawLine(100, offset - i, h_points - 100, offset - i);
	DrawLine(100, offset + i, h_points - 100, offset + i);
      }
    }

    /* vertical dotted lines */
    for (i = 144 ; i < h_points - 100 ; i += 44) {
      for (j = 864 ; j < (v_points - 80) * 10 ; j += 64) {
	DrawPixel(i, j / 10);
      }
    }

    /* horizontal dotted lines */
    for (i = 112 ; i < v_points - 80 ; i += 32) {
      for (j = 1088 ; j < (h_points - 100) * 10 ; j += 88) {
	DrawPixel(j / 10, i);
      }
    }
  }
}

/* graph the data on the display */
void
draw_data()
{
  static int i, j, k, l, m, off, delay, old = 100, mult, div, mode, prev;
  static int x, y, X, Y;
  static Channel *p;
  static short *samples;

  mode = data_good ?  scope.mode : (scope.mode < 2 ? 0 : 2);
  /* interpolate a line between the sample just before and after trigger */
  off = 100 - scope.scale;	/* then shift all samples horizontally */
  if (scope.trige) {		/* to place time zero at trigger */
    samples = mem[k = scope.trigch + 23].data;
    if ((i = samples[0]) != (j = samples[1])) /* avoid divide by zero: */
      delay = 100 + (j - scope.trig + 128) * 44000 * scope.scale
	/ ((j - i) * mem[k].rate); /* y=mx+b  so  x=(y-b)/m */
    if ((delay < 100) || (delay > (100 + 44000
				   / mem[k].rate * scope.scale)))
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
	  SetColor(color[0]);
	  samples = p->old + 1;
	  l = old;
	} else {		/* plot new samples */
	  SetColor(p->color);
	  samples = p->signal->data + 1;
	  l = delay;
	}
	if (p->func == FUNCMEM)	/* should we use the offset? */
	  l = 100;		/* not if we're memory */
	else if (p->func > FUNCRIGHT /* yes if we're math & 1 or 2 is */
		 && (ch[0].func > FUNCRIGHT && ch[1].func > FUNCRIGHT))
	  l = 100;
	prev = 1;
	X = Y = 0;
	if (mode < 2)		/* point / point accumulate */
	  for (i = 0 ; i < h_points - 100 - l ; i++) {
	    if ((m = i * p->signal->rate / scope.scale / 44000 ) != prev)
	      DrawPixel(i + l, off - samples[m] * mult / div);
	    prev = m;
	  }
	else			/* line / line accumulate */
	  for (i = 0 ; i < h_points - 100 - l ; i++) {
	    if ((m = i * p->signal->rate / scope.scale / 44000) != prev) {
	      x = i + l; y = off - samples[m] * mult / div;
	      if (X)
		DrawLine(X, Y, x, y);
	      else
		DrawPixel(x, y);
	      X = x; Y = y;
	    }
	    prev = m;
	  }
      }
      memcpy(p->old, p->signal->data,
	     sizeof(short) * (h_points - 200) + sizeof(short));
      DrawLine(90, off, 100, off);
      DrawLine(h_points - 100, off, h_points - 90, off);
    }
  }
  old = delay;
  SyncDisplay();
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
void
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
  if (quit_key_pressed) {
    cleanup();
    exit(0);
  }
  AddTimeOut(MSECREFRESH, animate, NULL);
}

/* [re]initialize graphics screen */
void
init_screen()
{
  int i, channelcolor[] = CHANNELCOLOR;

  init_widgets();
  for (i = 0 ; i < CHANNELS ; i++) {
    ch[i].color = mem[i + 23].color = color[channelcolor[i]];
  }
  fix_widgets();
  offset = v_points / 2;
}

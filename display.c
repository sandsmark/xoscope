/*
 * @(#)$Id: display.c,v 1.51 1999/08/27 03:56:17 twitham Rel $
 *
 * Copyright (C) 1996 - 1999 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the UI-independent display code
 *
 */

#include <stdio.h>
#include <time.h>
#include "oscope.h"		/* program defaults */
#include "display.h"
#include "func.h"
#include "proscope.h"

void	show_data();
int	vga_write();
void	init_widgets();
void	fix_widgets();
void	clear_display();

int	triggered = 0;		/* whether we've triggered or not */
int	data_good = 1;		/* whether data may be "stale" or not */
void	*font;

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
	    h_points / 2, row(5),
	    font, clr, TEXT_BG, ALIGN_CENTER);
  vga_write(message, h_points / 2, row(5),
	    font, clr, TEXT_BG, ALIGN_CENTER);
}

/* draw just dynamic or all text to graphics screen */
void
draw_text(int all)
{
  static char string[81];
  static int i, j, k, frames = 0;
  static time_t sec, prev;
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
      vga_write(progname,  100, 0, font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

      vga_write("(@)", col(2), row(1), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
      vga_write("Load", col(5), row(1),
		font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

      sprintf(string, "ver: %s", version);
      vga_write(string,  100, row(1), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

      vga_write("(#)", col(2), row(2), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
      vga_write("Save", col(5), row(2), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

      sprintf(string, "%d x %d", h_points, v_points);
      vga_write(string, 100, row(2), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

      vga_write("(&)", col(2), row(3), font, KEY_FG, TEXT_BG,ALIGN_LEFT);
      sprintf(string, "Sound %s", snd ? "On" : "Off");
      vga_write(string, col(5), row(3), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
      vga_write("(*)", col(17), row(3), font, KEY_FG, TEXT_BG,ALIGN_RIGHT);
      sprintf(string, "DMA:%d", scope.dma);
      vga_write(string, col(17), row(3), font, TEXT_FG, TEXT_BG, ALIGN_LEFT);

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

      vga_write("(!)", 100, 62, font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      vga_write("(space)", h_points - 100 - 8 * 4, 62,
		font, KEY_FG, TEXT_BG, ALIGN_RIGHT);

    } else {			/* not verbose */

      vga_write("(?)", col(75), 0, font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      vga_write("Help", col(79), 0, font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);
    }

    sprintf(string, "%s Trigger @ %d", trigs[scope.trige], scope.trig - 128);
    vga_write(string, col(40), row(2),
	      font, ch[scope.trigch].color, TEXT_BG, ALIGN_CENTER);
    vga_write(strings[scope.mode], 100, 62, font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
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
	if (ch[i].func == FUNCPS ||
	    (ch[i].func >= FUNCEXT && ch[0].func == FUNCPS))
	  sprintf(string, "%g V/div",
		  (float)ps.volts * (float)ch[i].div / (float)ch[i].mult / 10);
	else
	  sprintf(string, "%d / %d", ch[i].mult, ch[i].div);
	vga_write(string, col(69 * (i / 4) + 5), row(j + 1),
		  font, k, TEXT_BG, ALIGN_CENTER);

	sprintf(string, "@ %d", -(ch[i].pos));
	vga_write(string, col(69 * (i / 4) + 5), row(j + 2),
		  font, k, TEXT_BG, ALIGN_CENTER);

	vga_write(funcnames[ch[i].func], col(69 * (i / 4)), row(j + 3),
		  font, ch[i].func < FUNCEXT ? ch[i].signal->color : k,
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

      vga_write("({)     (})", 0, row(26), font,KEY_FG, TEXT_BG, ALIGN_LEFT);
      vga_write("Scale", col(3), row(26), font, p->color, TEXT_BG, ALIGN_LEFT);

      vga_write("([)     (])", 0, row(27), font, KEY_FG, TEXT_BG,ALIGN_LEFT);
      vga_write("Pos.", col(3), row(27), font, p->color,TEXT_BG,ALIGN_LEFT);

      vga_write("(^)", 0, row(28), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
      sprintf(string, "ProbeScope %s", ps.found ? "On" : "Off");
      vga_write(string, col(3), row(28),
		font, mem[25].color, TEXT_BG, ALIGN_LEFT);

      vga_write("(9)              (0)", col(40), row(25),
		font, KEY_FG, TEXT_BG, ALIGN_CENTER);
      vga_write("(()              ())", col(40), row(26),
		font, KEY_FG, TEXT_BG, ALIGN_CENTER);

      vga_write("(A-W)", col(73), row(27), font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      vga_write("Store", col(78), row(27),
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

      vga_write("(a-z)", col(73), row(28),
		font, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      vga_write("Recall", col(79), row(28),
		font, p->color, TEXT_BG, ALIGN_RIGHT);

      vga_write("(", col(26), row(28), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
      vga_write(")", col(53), row(28), font, KEY_FG, TEXT_BG, ALIGN_LEFT);
    }
    i = 1000 * scope.div / scope.scale;
    sprintf(string, "%d %cs/div", i > 999 ? i / 1000 : i, i > 999 ? 'm' : 'u');
    vga_write(string, col(40), row(25), font, TEXT_FG, TEXT_BG, ALIGN_CENTER);

    sprintf(string, "%d S/s", p->signal->rate);
    vga_write(string, col(40), row(26), font, p->color, TEXT_BG, ALIGN_CENTER);

    sprintf(string, "%d Hz/div FFT", scope.div * p->signal->rate / 1000
	    * p->signal->rate / 880 / scope.scale);
    vga_write(string, col(40), row(27), font, p->color, TEXT_BG, ALIGN_CENTER);

    for (i = 0 ; i < 26 ; i++) {
      sprintf(string, "%c", i + 'a');
      vga_write(string, col(27 + i), row(28),
		font, mem[i].color, TEXT_BG, ALIGN_LEFT);
    }

    if (ps.found) {		/* ProbeScope on ? */
      j = mem[25].color;

      sprintf(string, "%d Volt Range", ps.volts);
      vga_write(string, 100, row(25), font, j, TEXT_BG, ALIGN_LEFT);

      vga_write(ps.trigger & PS_SINGLE ? "SINGLE" : "   RUN",
		h_points - 100, row(25), font, j, TEXT_BG, ALIGN_RIGHT);

      sprintf(string, "%s ~ %g V", ps.trigger & PS_PINT ? "+INTERN"
	      : ps.trigger & PS_MINT ? "-INTERN"
	      : ps.trigger & PS_PEXT ? "+EXTERN"
	      : ps.trigger & PS_MEXT ? "-EXTERN" : "AUTO",
	      (float)ps.level * (ps.trigger & PS_PEXT || ps.trigger & PS_MEXT
			  ? 1.0 : (float)ps.volts) / 10);
      vga_write(string, h_points - 100, row(27), font, j, TEXT_BG, ALIGN_RIGHT);

      if (ps.wait)
	vga_write("WAITING!", h_points - 100, row(28),
		  font, j, TEXT_BG, ALIGN_RIGHT);
    }

    if (all == 1)
      fix_widgets();
    show_data();
    data_good = 0;		/* data may be bad if user just tweaked S/s */
    return;			/* show_data will call again to do the rest */
  }

  /* always draw the dynamic text */
  sprintf(string, "Period of %6d us = %5d Hz", p->time,  p->freq);
  vga_write(string, h_points/2, row(0), font, p->color, TEXT_BG, ALIGN_CENTER);

  sprintf(string, " Max:%3d - Min:%4d = %3d Pk-Pk ",
	  p->max, p->min, p->max - p->min);
  vga_write(string, h_points/2, row(1), font, p->color, TEXT_BG, ALIGN_CENTER);

  vga_write(triggered ? " Triggered " : "? TRIGGER ?", h_points/2, row(3),
	    font, ch[scope.trigch].color, TEXT_BG, ALIGN_CENTER);

  if (ch[0].signal->rate != ch[1].signal->rate) {
    sprintf(string, "WARNING: math(%d,%d) is bogus!",
	    ch[0].signal->rate, ch[1].signal->rate);
    vga_write(string, h_points/2, 62, font, KEY_FG, TEXT_BG, ALIGN_CENTER);
  }

  if (ps.found) {		/* ProbeScope on ? */
    sprintf(string, "%s%g V %s      ", ps.flags & PS_OVERFLOW ? "/\\"
	    : ps.flags & PS_UNDERFLOW ? "\\/ " : "",
	    (float)ps.dvm * (float)ps.volts / 100,
	    ps.coupling ? ps.coupling : "?");
    vga_write(string, 100, row(27), font, mem[25].color, TEXT_BG, ALIGN_LEFT);
  }

  time(&sec);
  if (sec != prev) {		/* show frames per second refresh rate */
    sprintf(string, "fps:%3d", frames);
    vga_write(string, h_points - 100, row(2), font,
	      TEXT_FG, TEXT_BG, ALIGN_RIGHT);
    frames = 0;
    prev = sec;
  }
  frames++;
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

  /* marks where the physical ProbeScope display ends */
  i = 31 * 44000 * scope.scale / (mem[25].rate * scope.div) + 100;
  if (i > h_points - 100) i = h_points - 100;
  SetColor(mem[25].color);
  DrawLine(i, 70, i, 80);
  DrawLine(i, v_points - 70, i, v_points - 80);

  /* a mark where the trigger level is, if the triggered channel is shown */
  i = -1;
  for (j = 7 ; j >= 0 ; j--) {
    if (ch[j].show && ch[j].func == scope.trigch)
      i = j;
  }
  if (i > -1) {
    j = offset + ch[i].pos + (128 - scope.trig) * ch[i].mult / ch[i].div;
    SetColor(mem[scope.trigch + 23].color);
    DrawLine(90, j + tilt[scope.trige], 110, j - tilt[scope.trige]);
  }

  /* the frame */
  SetColor(clip ? mem[clip + 22].color : color[scope.color]);
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
  static int i, j, k, l, x, y, X, Y, mult, div, off;
  static int mode, time, prev, delay, old = 100;
  static Channel *p;
  static short *samples;

  mode = data_good ?  scope.mode : (scope.mode < 2 ? 0 : 2);

  /* interpolate a line between the sample just before and after trigger */
  if (scope.trige) {		/* to place time zero at trigger */
    samples = mem[k = scope.trigch + 23].data;
    if ((i = samples[0]) != (j = samples[1])) /* avoid divide by zero: */
      delay = 100 + (j - scope.trig + 128) * 44000 * scope.scale
	/ ((j - i) * mem[k].rate); /* y=mx+b  so  x=(y-b)/m */
    if (delay < 100 || delay > h_points - 200)
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
	if (p->func <= FUNCRIGHT /* use the delay? */
	    || (p->func >= FUNCEXT
		&& (ch[0].func <= FUNCRIGHT || ch[1].func <= FUNCRIGHT)))
	  l = l;		/* yes only if we're L or R or their math */
	else
	  l = 100;		/* no if we're memory or ProbeScope */
	X = Y = 0;
	prev = -1;
	if (mode < 2)		/* point / point accumulate */
	  for (i = 0 ; i < h_points - 100 - l ; i++) {
	    if ((time = i * (p->signal->rate / 100) * scope.div
		 / scope.scale / 440) > prev && time < h_points)
	      DrawPixel(i + l, off - samples[time] * mult / div);
	    if (time > prev) prev = time;
	  }
	else			/* line / line accumulate */
	  for (i = 0 ; i < h_points - 100 - l ; i++) {
	    if ((time = i * (p->signal->rate / 100) * scope.div
		 / scope.scale / 440) > prev && time < h_points) {
	      x = i + l; y = off - samples[time] * mult / div;
	      if (X) DrawLine(X, Y, x, y);
	      else DrawPixel(x, y);
	      X = x; Y = y;
	    }
	    if (time > prev) prev = time;
	  }
      }
      memcpy(p->old, p->signal->data,
	     sizeof(short) * h_points + sizeof(short));
      DrawLine(90, off, 100, off);
      DrawLine(h_points - 100, off, h_points - 90, off);
    }
  }
  old = delay;
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
  SyncDisplay();
}

/* get and plot one screen full of data */
int
animate(void *data)
{
  clip = 0;
  if (ps.found) probescope();
  if (scope.run) {
    triggered = get_data();
    if (triggered && scope.run > 1) {	/* auto-stop single-shot wait */
      scope.run = 0;
      draw_text(1);
    }
  }
  show_data();
  data_good = 1;
  if (quit_key_pressed) {
    cleanup();
    exit(0);
  }
  AddTimeOut(MSECREFRESH, animate, NULL);
  return FALSE;
}

/* [re]initialize graphics screen */
void
init_screen()
{
  int i, channelcolor[] = CHANNELCOLOR;

  for (i = 0 ; i < CHANNELS ; i++) {
    ch[i].color = mem[i + 23].color = color[channelcolor[i]];
  }
  fix_widgets();
  offset = v_points / 2;
}

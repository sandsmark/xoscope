/*
 * @(#)$Id: display.c,v 1.66 2000/07/11 23:01:25 twitham Exp $
 *
 * Copyright (C) 1996 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the UI-independent display code
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "oscope.h"		/* program defaults */
#include "display.h"
#include "func.h"
#include "proscope.h"
#include "bitscope.h"

void	show_data();
int	vga_write();
void	init_widgets();
void	fix_widgets();
void	clear_display();

int	triggered = 0;		/* whether we've triggered or not */
int	erase_data = 1;		/* whether data may be "stale" or not */
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

void
format(char *buf, const char *fmt, float num)
{
  sprintf(buf, fmt, num >= 1000 ? num / 1000 : num, num >= 1000 ? "" : "m");
}

/* draw just dynamic or all text to graphics screen */
void
draw_text(int all)
{
  static char string[81];
  static int i, j, k, rate, trige, frames = 0;
  static time_t sec, prev;
  static Channel *p;
  static char *strings[] = {
    "Point",
    "Point Accum.",
    "Line",
    "Line Accum.",
    "Step",
    "Step Accum.",
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
	if (!ch[i].bits && ch[i].signal->volts)
	  format(string, "%g %sV/div",
		 (float)ch[i].signal->volts * ch[i].div / ch[i].mult / 10);
	else
	  sprintf(string, "%d / %d", ch[i].mult, ch[i].div);
	vga_write(string, col(69 * (i / 4) + 5), row(j + 1),
		  font, k, TEXT_BG, ALIGN_CENTER);

	sprintf(string, "%d @ %d", ch[i].bits, -(ch[i].pos));
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
      sprintf(string, "Serial Scope %s", bs.found ? bs.bcid
	      : (ps.found ? "ProbeScope" : "Off"));
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

    fix_widgets();
    if (scope.rate != rate || scope.trige != trige)
      erase_data = 1;		/* bogus data if user just tweaked these */
    rate = scope.rate;
    trige = scope.trige;
    prev = -1;
    show_data();
    return;			/* show_data will call again to do the rest */
  }

  /* always draw the dynamic text */
  sprintf(string, "  Period of %6d us = %6d Hz  ", p->time,  p->freq);
  vga_write(string, h_points/2, row(0), font, p->color, TEXT_BG, ALIGN_CENTER);

  if (p->signal->volts)
    sprintf(string, "   %7.5g - %7.5g = %7.5g mV   ",
	    (float)p->max * p->signal->volts / 320,
	    (float)p->min * p->signal->volts / 320,
	    ((float)p->max - p->min) * p->signal->volts / 320);
  else
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
  if (sec != prev) {		/* fix "scribbled" text once a second */

    sprintf(string, "%s Trigger @ %d", trigs[scope.trige], scope.trig - 128);
    vga_write(string, col(40), row(2),
	      font, ch[scope.trigch].color, TEXT_BG, ALIGN_CENTER);
    vga_write(strings[scope.mode], 100, 62, font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
    vga_write(scope.run ? (scope.run > 1 ? "WAIT" : " RUN") : "STOP",
	      h_points - 100, 62, font, TEXT_FG, TEXT_BG, ALIGN_RIGHT);
    sprintf(string, "fps:%3d", frames);
    vga_write(string, h_points - 100, row(2), font,
	      TEXT_FG, TEXT_BG, ALIGN_RIGHT);

    i = 1000 * scope.div / scope.scale;
    sprintf(string, "%d %cs/div", i > 999 ? i / 1000: i, i > 999 ? 'm' : 'u');
    vga_write(string, col(40), row(25), font, TEXT_FG, TEXT_BG, ALIGN_CENTER);

    sprintf(string, "%d S/s", p->signal->rate);
    vga_write(string, col(40), row(26), font, p->color, TEXT_BG, ALIGN_CENTER);

/*      sprintf(string, "%d Hz/div FFT", scope.div * p->signal->rate / 1000 */
/*  	    * p->signal->rate / 880 / scope.scale); */
    sprintf(string, "%d Samples", samples(p->signal->rate) - 2);
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
  if (ps.found) {
    i = 31 * 44000 * scope.scale / (mem[25].rate * scope.div) + 100;
    if (i > h_points - 100) i = h_points - 100;
    SetColor(mem[25].color);
    DrawLine(i, 70, i, 80);
    DrawLine(i, v_points - 70, i, v_points - 80);
  }

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
  static int i, j, k, l, x, y, X, Y, mult, div, off, bit, start, end;
  static int time, prev, delay, bitoff;
  static long int num;
  static int old = 100, preva = 100, prevb = 100;
  static Channel *p;
  static short *samp;

  /* interpolate a line between the sample just before and after trigger */
  if (scope.trige) {		/* to place time zero at trigger */
    samp = mem[k = scope.trigch + 23].data;
    if ((i = samp[0]) != (j = samp[1])) /* avoid divide by zero: */
      delay = 100 + abs(j - scope.trig + 128) * 44000 * scope.scale
	/ (abs(j - i) * mem[k].rate); /* y=mx+b  so  x=(y-b)/m */
    if (delay < 100 || delay > h_points - 200)
      delay = old;
  } else			/* no trigger, leave delay as it was */
    delay = old;

  if (scope.curs) {		/* erase previous cursors */
    SetColor(color[0]);
    DrawLine(preva, 70, preva, v_points - 70);
    DrawLine(prevb, 70, prevb, v_points - 70);
  }

  for (j = 0 ; j < CHANNELS ; j++) { /* plot each visible channel */
    p = &ch[j];

    if (p->show) {
      mult = p->mult;
      div = p->div;
      off = offset + p->pos;
      num = 10 * p->signal->rate * scope.div / scope.scale / 44;

      if (!p->bits)		/* analog display mode: draw one line */
	start = end = -1;
      else {			/* logic analyzer mode: draw bits lines */
	start = 0;
	end = p->bits - 1;
      }

      for (k = !(scope.mode % 2) ; k >= 0 ; k--) { /* once=accum, twice=erase */

	if (k) {		/* erase previous samples */
	  SetColor(color[0]);
	  samp = p->old + 1;
	  l = old;
	} else {		/* plot new samples */
	  SetColor(erase_data ? color[0] : p->color);
	  samp = p->signal->data + 1;
	  l = delay;
	}

	if (!(p->func <= FUNCRIGHT /* use the delay? */
	    || (p->func >= FUNCEXT /* yes only if we're L or R or their math */
		&& (ch[0].func <= FUNCRIGHT || ch[1].func <= FUNCRIGHT))))
	  l = 100;		/* no if we're memory or ProbeScope */

	if (scope.curs && j == scope.select && !k) { /* draw new cursors */
	  if ((time = (scope.cursa - 1) * 10000 / num + l + 1) < h_points - 100)
	    DrawLine(time, 70, time, v_points - 70);
	  preva = time;
	  if ((time = (scope.cursb - 1) * 10000 / num + l + 1) < h_points - 100)
	    DrawLine(time, 70, time, v_points - 70);
	  prevb = time;
	}

	for (bit = start ; bit <= end ; bit++) {
	  prev = -1;
	  X = 0;
	  bitoff = bit * 16 - end * 8 + 4;
	  for (i = 0 ; i < h_points - 100 - l ; i++) {
	    if ((time = i * num / 10000) > prev && time < MAXWID) {
	      if ((x = i + l) > h_points - 100)
		break;
	      y = off - (bit < 0 ? samp[time]
			 : (bitoff - (samp[time] & (1 << bit) ? 0 : 8)))
		* mult / div;
	      if (scope.mode < 2)
		DrawPixel(x, y);
	      else if (X) {
		if (scope.mode < 4)
		  DrawLine(X, Y, x, y);
		else {
		  DrawLine(X, Y, x, Y);
		  DrawLine(x, Y, x, y);
		}
	      }
	      X = x; Y = y;
	    }
	    if (time > prev) prev = time;
	  }
	}
      }
      memcpy(p->old, p->signal->data, sizeof(short) * samples(p->signal->rate));
      DrawLine(90, off, 100, off);
      DrawLine(h_points - 100, off, h_points - 90, off);
    }
  }
  erase_data = 0;
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

/* get and plot some data */
int
animate(void *data)
{
  clip = 0;
  if (ps.found) probescope();
  if (scope.run) {
    triggered = bs.found ? bs_getdata(bs.fd) : get_data();
    if (triggered && scope.run > 1) { /* auto-stop single-shot wait */
      scope.run = 0;
      draw_text(1);
    }
  } else if (in_progress)
    bs.found ? bs_getdata(bs.fd) : get_data();
  else
    usleep(100000);		/* no need to suck all CPU cycles */
  show_data();
  if (quit_key_pressed) {
    cleanup();
    exit(0);
  }
  AddTimeOut(MSECREFRESH, animate, NULL);
  return TRUE;
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
  draw_text(1);
  clear();
}

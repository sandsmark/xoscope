/*
 * @(#)$Id: display.c,v 1.70 2003/06/20 19:51:32 baccala Exp $
 *
 * Copyright (C) 1996 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the UI-independent display code
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "oscope.h"		/* program defaults */
#include "display.h"
#include "func.h"

#define DEBUG 0

void	show_data();
void	text_write();
void	init_widgets();
void	fix_widgets();
void	clear_display();

int	triggered = 0;		/* whether we've triggered or not */
void	*font;
int	math_warning = 0;	/* TRUE if math has a problem */

struct signal_stats stats;

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
  if (y < 4)			/* top; absolute */
    return (y * 16);
  if (y == 4)
    return 62;
  if (y > 24)			/* bottom; absolute */
    return (v_points - ((30 - y) * 16));
  /* center; spread out proportionally */
  return ((y - 5) * (v_points - 160) / 20 + 80);
}

/* draw a temporary one-line message to center of screen */
void
message(char *message, int clr)
{
  text_write("                                                  ",
	     40, 5, 0, clr, TEXT_BG, ALIGN_CENTER);
  text_write(message,
	     40, 5, 0, clr, TEXT_BG, ALIGN_CENTER);
}

void
format(char *buf, const char *fmt, float num)
{
  int power=0;

  /* Round off num to nearest 1% */

  while (num > 100) num /= 10, power ++;
  while ((num > 0) && (num < 10)) num *= 10, power --;
  num = rint(num);
  num *= pow(10.0, power);

  sprintf(buf, fmt, num >= 1000 ? num / 1000 : num, num >= 1000 ? "" : "m");
}

/* draw just dynamic or all text to graphics screen */
void
draw_text(int all)
{
  static char string[81];
  static char *s;
  static int i, j, k, frames = 0;
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
      text_write("(Esc)", 0, 0, 0, KEY_FG, TEXT_BG, ALIGN_LEFT);
      text_write("Quit", 5, 0, 0, TEXT_FG, TEXT_BG, ALIGN_LEFT);
      text_write(progname, 12, 0, 0, TEXT_FG, TEXT_BG, ALIGN_LEFT);

      text_write("(@)", 2, 1, 0, KEY_FG, TEXT_BG, ALIGN_LEFT);
      text_write("Load", 5, 1, 0, TEXT_FG, TEXT_BG, ALIGN_LEFT);

      sprintf(string, "ver: %s", version);
      text_write(string, 12, 1, 0, TEXT_FG, TEXT_BG, ALIGN_LEFT);

      text_write("(#)", 2, 2, 0, KEY_FG, TEXT_BG, ALIGN_LEFT);
      text_write("Save", 5, 2, 0, TEXT_FG, TEXT_BG, ALIGN_LEFT);

      sprintf(string, "%d x %d", h_points, v_points);
      text_write(string, 12, 2, 0, TEXT_FG, TEXT_BG, ALIGN_LEFT);

      text_write("(&)", 2, 3, 0, KEY_FG, TEXT_BG,ALIGN_LEFT);

      if (datasrc && datasrc->option1str != NULL) {
	text_write("(*)", 17, 3, 0, KEY_FG, TEXT_BG,ALIGN_RIGHT);
      }

      text_write("(Enter)", 70, 0, 0, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      text_write("Refresh", 77, 0, 0, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

      text_write("(,)", 70, 1, 0, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      text_write("Graticule", 79, 1, 0, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

      text_write("(_)(-)                      (=)(+)", 40, 2,
		 0, KEY_FG, TEXT_BG, ALIGN_CENTER);

      text_write("(.)", 70, 2, 0, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      text_write(scope.behind ? "Behind   " : "In Front ", 79, 2,
		 0, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

      text_write("(<)      (>)", 79, 3,	0, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      text_write("Color", 75, 3, 0, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

      text_write("(!)", 12, 4, 0, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      text_write("(space)", 61, 4, 0, KEY_FG, TEXT_BG, ALIGN_RIGHT);

    } else {			/* not verbose */

      text_write("(?)", 75, 0, 0, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      text_write("Help", 79, 0, 0, TEXT_FG, TEXT_BG, ALIGN_RIGHT);
    }

    if (scope.trige) {
      Signal *trigsig = datasrc->chan(scope.trigch);

      if (trigsig->volts > 0) {
	char minibuf[256];
	format(minibuf, "%g %sV",
	       (scope.trig) * trigsig->volts / 320);
	sprintf(string, "%s Trigger @ %s", trigs[scope.trige], minibuf);
      } else {
	sprintf(string, "%s Trigger @ %d",
		trigs[scope.trige], scope.trig);
      }
      text_write(string, 40, 2,	0, TEXT_FG, TEXT_BG, ALIGN_CENTER);
      text_write(trigsig->name, 40, 3, 0, TEXT_FG, TEXT_BG, ALIGN_CENTER);
    } else {
      text_write("No Trigger", 40, 2, 0, TEXT_FG, TEXT_BG, ALIGN_CENTER);
    }

    text_write(datasrc ? datasrc->name : "No data source", 5, 3,
	       0, TEXT_FG, TEXT_BG, ALIGN_LEFT);
    text_write(strings[scope.mode], 12, 4, 0, TEXT_FG, TEXT_BG, ALIGN_LEFT);
    if (datasrc && (datasrc->option1str != NULL)
	&& ((s = datasrc->option1str()) != NULL)) {
      text_write(s, 17, 3, 0, TEXT_FG, TEXT_BG, ALIGN_LEFT);
    }


    /* sides of graticule */
    for (i = 0 ; i < CHANNELS ; i++) {

      j = (i % 4) * 5 + 5;
      k = ch[i].color;

      text_write("Channel", 69 * (i / 4), j, 0, k, TEXT_BG, ALIGN_LEFT);
      sprintf(string, "(%d)", i + 1);
      text_write(string, 69 * (i / 4) + 7, j, 0, KEY_FG, TEXT_BG, ALIGN_LEFT);

      if ((scope.verbose || ch[i].show || scope.select == i) && ch[i].signal) {

	/* XXX here and other places, need to make sure we clear the field */
	if (!ch[i].bits && ch[i].signal->volts)
	  format(string, "%g %sV/div",
		 (float)ch[i].signal->volts * ch[i].div / ch[i].mult / 10);
	else
	  sprintf(string, "%d / %d", ch[i].mult, ch[i].div);
	text_write(string, 69 * (i / 4), j + 1, 0, k, TEXT_BG, ALIGN_LEFT);

	sprintf(string, "%d @ %d", ch[i].bits, -(ch[i].pos));
	text_write(string, 69 * (i / 4) + 5, j + 2,
		   0, k, TEXT_BG, ALIGN_CENTER);

	text_write(ch[i].signal->name, 69 * (i / 4), j + 3,
		  0, ch[i].color, TEXT_BG, ALIGN_LEFT);

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
      text_write("(Tab)", 0, 25, 0, KEY_FG, TEXT_BG, ALIGN_LEFT);
      text_write(p->show ? "Visible" : "HIDDEN ", 5, 25,
		0, p->color, TEXT_BG, ALIGN_LEFT);

      text_write("({)     (})", 0, 26, 0, KEY_FG, TEXT_BG, ALIGN_LEFT);
      text_write("Scale", 3, 26, 0, p->color, TEXT_BG, ALIGN_LEFT);

      text_write("([)     (])", 0, 27, 0, KEY_FG, TEXT_BG,ALIGN_LEFT);
      text_write("Pos.", 3, 27, 0, p->color,TEXT_BG,ALIGN_LEFT);

      if (datasrc && datasrc->option2str != NULL) {
	text_write("(^)", 0, 28, 0, KEY_FG, TEXT_BG, ALIGN_LEFT);
      }

      text_write("(9)              (0)", 40, 25,
		0, KEY_FG, TEXT_BG, ALIGN_CENTER);
      text_write("(()              ())", 40, 26,
		0, KEY_FG, TEXT_BG, ALIGN_CENTER);

      text_write("(A-W)", 73, 27, 0, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      text_write("Store", 78, 27,
		0, p->color, TEXT_BG, ALIGN_RIGHT);

      if (scope.select > 1) {
	text_write("($)", 72, 25,
		  0, KEY_FG, TEXT_BG, ALIGN_RIGHT);
	text_write("Extern", 78, 25,
		  0, p->color, TEXT_BG, ALIGN_RIGHT);
	text_write("(:)    (;)", 79, 26,
		  0, KEY_FG, TEXT_BG, ALIGN_RIGHT);
	text_write("Math", 76, 26,
		  0, p->color, TEXT_BG, ALIGN_RIGHT);
      }

      /* XXX not exactly true anymore */
      text_write("(a-z)", 73, 28,
		0, KEY_FG, TEXT_BG, ALIGN_RIGHT);
      text_write("Recall", 79, 28,
		0, p->color, TEXT_BG, ALIGN_RIGHT);

      text_write("(", 26, 28, 0, KEY_FG, TEXT_BG, ALIGN_LEFT);
      text_write(")", 53, 28, 0, KEY_FG, TEXT_BG, ALIGN_LEFT);
    }

    if (datasrc && (datasrc->option2str != NULL)
	&& ((s = datasrc->option2str()) != NULL)) {
      text_write(s, 3, 28,
		0, TEXT_FG, TEXT_BG, ALIGN_LEFT);
    }

    fix_widgets();
    prev = -1;
    show_data();
    return;			/* show_data will call again to do the rest */
  }

  /* always draw the dynamic text, if signal is analog (bits == 0) */
  if (p->signal && (p->signal->bits == 0)) {

    sprintf(string, "  Period of %6d us = %6d Hz  ", stats.time,  stats.freq);
    text_write(string, 40, 0,
	      0, p->color, TEXT_BG, ALIGN_CENTER);

    if (p->signal->volts)
      sprintf(string, "   %7.5g - %7.5g = %7.5g mV   ",
	      (float)stats.max * p->signal->volts / 320,
	      (float)stats.min * p->signal->volts / 320,
	      ((float)stats.max - stats.min) * p->signal->volts / 320);
    else
      sprintf(string, " Max:%3d - Min:%4d = %3d Pk-Pk ",
	      stats.max, stats.min, stats.max - stats.min);
    text_write(string, 40, 1,
	      0, p->color, TEXT_BG, ALIGN_CENTER);
  }

  if (math_warning) {
#if 0
    sprintf(string, "WARNING: math(%d,%d) is bogus!",
	    ch[0].signal->rate, ch[1].signal->rate);
    text_write(string, 40, 4, 0, KEY_FG, TEXT_BG, ALIGN_CENTER);
#else
    text_write("WARNING: math is bogus!", 40, 4,
	       0, KEY_FG, TEXT_BG, ALIGN_CENTER);
#endif
  }

  if (datasrc && datasrc->status_str != NULL) {
    for (i=0; i<8; i++) {
      int fieldsize[] = {16,16,16,16,20,20,12,12};
      if ((s = datasrc->status_str(i)) != NULL) {
	if (i%2 == 0)
	  text_write(s, 12, 25 + i/2, fieldsize[i],
		     TEXT_FG, TEXT_BG, ALIGN_LEFT);
	else
	  text_write(s, 66, 25 + i/2, fieldsize[i],
		     TEXT_FG, TEXT_BG, ALIGN_RIGHT);
      }
    }
  }

  time(&sec);
  if (sec != prev) {		/* fix "scribbled" text once a second */

    text_write(scope.run ? (scope.run > 1 ? "WAIT" : " RUN") : "STOP", 68, 4,
	       0, TEXT_FG, TEXT_BG, ALIGN_RIGHT);
    sprintf(string, "fps:%3d", frames);
    text_write(string, 66, 2, 0, TEXT_FG, TEXT_BG, ALIGN_RIGHT);

    i = 1000 * scope.div / scope.scale;
    sprintf(string, "%d %cs/div", i > 999 ? i / 1000: i, i > 999 ? 'm' : 'u');
    text_write(string, 40, 25, 0, TEXT_FG, TEXT_BG, ALIGN_CENTER);

    if (p->signal) {

      /* XXX what do we want here - frame samples, samples per screen? */

      /* I cut and changed this line a half dozen times trying to decide
       * what number I wanted displayed as the "Samples" - p->signal->num
       * would give us the actual number of samples in the signal, but
       * that changes during the course of a sweep.  Now I've got the
       * number of samples per sweep, which isn't quite acceptable if
       * there's no data on the screen, or if we're displaying a memory
       * channel with a fixed number of sample
       */

      /* sprintf(string, "%d Samples", p->signal->num); */
      sprintf(string, "%d Samples", samples(p->signal->rate));
      text_write(string, 40, 27, 14, p->color, TEXT_BG, ALIGN_CENTER);

      if (p->signal->rate > 0) {

	sprintf(string, "%d S/s", p->signal->rate);
	text_write(string, 40, 26, 0, p->color, TEXT_BG, ALIGN_CENTER);

      } else if (p->signal->rate < 0) {

	/* Special case for a Fourier Transform.  p->signal->rate is
	 * the negative of the frequency step for each point in the
	 * transform, times 10.  Since there are 44 x-coordinates in a
	 * division, after applying the scope's current time base
	 * multiplier (scope.div / scope.scale), we multiply by 44/10
	 * to get the number of Hz in a division.
	 */

	sprintf(string, "%d Hz/div FFT",
		(- p->signal->rate) * 44 * scope.div / scope.scale / 10);
	text_write(string, 40, 26, 0, p->color, TEXT_BG, ALIGN_CENTER);

      }
    }

    for (i = 0 ; i < 26 ; i++) {
      sprintf(string, "%c", i + 'a');
      if (datasrc && i < datasrc->nchans()) {
	/* XXX Maybe here we should show color by channel if sig displayed */
#if 0
	text_write(string, 27 + i, 28,
		  0, chan[i].color, TEXT_BG, ALIGN_LEFT);
#else
	text_write(string, 27 + i, 28,
		  0, TEXT_FG, TEXT_BG, ALIGN_LEFT);
#endif
      } else if (mem[i].num > 0) {
	/* XXX different color here for memory? */
	text_write(string, 27 + i, 28,
		  0, TEXT_FG, TEXT_BG, ALIGN_LEFT);
      }
    }

    frames = 0;
    prev = sec;
  }
  frames++;
}

/* roundoff_multipliers() - set mult/div, based on target mult/div if
 * channel is displaying a signal with a voltage scale, then round
 * the multipliers to something conventional (i.e, 10 mV/div instead
 * of 9.7 mV/div), otherwise leave them alone.
 *
 * The rounding is done by computing the base ten logarithm of what
 * the mV-per-division value would be if we just used the target
 * mult/div ratio.  Then we throw away the integer part, leaving
 * a number between 0 and 1 corresponding to a leading digit between
 * 1 and 10.  By comparing this to the logarithms of 7.5 (.875),
 * 3.5 (.544), and 1.5 (.176), we pick a target of 10 (1.0),
 * 5 (0.7), 2 (0.3), or 1 (0.0), and subtract out the corresponding
 * logarithm.  The difference is the power of ten we need to multiply
 * mult/div by to get to our target.  At this point, we'd like a nice
 * algorithm to find the closest rational fraction to a given real
 * number, but I don't know of one.  Instead, we just multiply
 * mult/div by 1000, then multiply the logarithm's power into
 * the largest of either mult or div.  It works within 1%, which
 * is the accuracy we display the voltage scale with.
 */

void
roundoff_multipliers(Channel *p)
{

  if (p->signal && p->signal->volts && !p->bits) {

    double mV_per_div;
    double logmV;

    mV_per_div = (double)p->signal->volts
      * p->target_div / p->target_mult / 10;

    logmV = log10(mV_per_div);
    logmV -= floor(logmV);

    if (logmV > .875) logmV = logmV - 1.0;
    else if (logmV > .544) logmV = logmV - 0.7;
    else if (logmV > .176) logmV = logmV - 0.3;

    p->mult = p->target_mult * 1000;
    p->div = p->target_div * 1000;
 
    if (p->mult > p->div) {
      p->mult *= pow(10.0, logmV);
    } else {
      p->div *= pow(10.0, -logmV);
    }

#if 0
    printf("roundoff_multipliers() %d/%d -> %d/%d\n",
	   p->target_mult, p->target_div, p->mult, p->div);
#endif

  } else {

    p->mult = p->target_mult;
    p->div = p->target_div;

  }

}

/* clear() - one of the most important functions in the program,
 * called whenever something 'changes'
 *
 * Clear the display, clear data history on all display
 * channels, and redraw all text.  Since this clears data history
 * (both on the screen and in memory), it should only be called when
 * needed.
 */

void
clear()
{
  int i;

  clear_display();
  if (datasrc) {
    datasrc->reset();
    setinputfd(datasrc->fd());
  }

  /* This also updates the 'volts' and 'rate' fields in the math
   * signals
   */

  math_warning = update_math_signals();

  for (i = 0; i < CHANNELS; i++) {
    ch[i].old_frame = 0;

    roundoff_multipliers(&ch[i]);
  }

  show_data();
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
  if (scope.trige) {
    i = -1;
    for (j = 7 ; j >= 0 ; j--) {
      if (ch[j].show && ch[j].signal == datasrc->chan(scope.trigch))
	i = j;
    }
    if (i > -1) {
      j = offset + ch[i].pos - scope.trig * ch[i].mult / ch[i].div;
      SetColor(ch[i].color);
      DrawLine(90, j + tilt[scope.trige], 110, j - tilt[scope.trige]);
    }
  }

  /* the frame */
#if 0
  SetColor(clip ? mem[clip + 22].color : color[scope.color]);
#else
  SetColor(color[scope.color]);
#endif
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

/* draw_data()
 *
 * graph the data on the display, possibly erasing old data to make
 * room for the new.  To do this, we keep an array of data points,
 * half of which is the previous sweep that we're erasing as we go,
 * the other half of which is the new sweep that we're drawing as we
 * go.  At the end of a sweep, we flip pointers to the two halves.  If
 * we're in an accumulate mode, don't have to erase anything, because
 * we're just letting the traces accumulate on the screen.  Also draw
 * cursors here, and, near the end of the function, draw tick marks to
 * show zero levels.
 */

void
draw_data()
{
  static int i, j, l, x, y, X, Y, mult, div, off, bit, start, end;
  static int time, prev, bitoff;
  static long int num;
  static int preva = 100, prevb = 100;	/* previous cursor locations */
  static Channel *p;
  static SignalLine *sl;
  static short *samp;

  for (j = 0 ; j < CHANNELS ; j++) { /* plot each visible channel */
    p = &ch[j];

    if (p->show && p->signal) {

      mult = p->mult;
      div = p->div;
      off = offset + p->pos;

      samp = p->signal->data;

      /* Compute num, the number of samples per x-coordinate, times
       * 10000, based on the signal's rate (in samples/sec), the
       * scope's time base multiplier (scope.scale / scope.div), the
       * 44 x-coords ((640-200)/10) in each division, and a base rate
       * of 1 ms per div.  For example, if the signal rate is 44000 Hz,
       * and the time base multiplier is 1, num will be 10000 (or
       * 1, since it's times 10000), for one sample for x-coord, 44
       * samples per division, 1 ms per div.  If the signal rate
       * is zero (unspecified) or negative (a special case for
       * Fourier Transforms, meaning the x scale is in Hz), we
       * use a base rate of one sample per x-coord.
       */

      if (p->signal->rate > 0) {
	num = 10 * p->signal->rate * scope.div / scope.scale / 44;
      } else {
	num = 10000 * scope.div / scope.scale;
      }

      /* Compute left offset: 100 pixels to the side of the display
       * area, plus any additional delay specified by the signal.
       */

      l = 100 + p->signal->delay / num;

      /* Erase and redraw the cursors, if they've moved.
       *
       * We always draw them, for much the same reason as we always
       * draw signal lines - because we've never quite sure if
       * a signal that coincides with them might have been erased,
       * and part of the cursor along with it.
       *
       * There's several things I don't like about the cursors.
       * First, the cursor positions are stored as offsets into the
       * data arrays, which means that if we change to a different
       * signal with a different sampling rate, the cursors move
       * around on the screen!  We'd have to worry about that in this
       * code, except that handle_key() always does a clear() when it
       * changes channels - not that I think a clear() is necessary
       * when we change channels, but at least it makes sure that we
       * don't get doubly drawn cursors if we move to a channel with a
       * different sampling rate.
       *
       * Also, this math might be a bit flakey, I'm not sure (I didn't
       * write it).  Multiplication and division with computer
       * integers is always a lot of trouble, and 'num' was designed
       * to convert from time to x-coords, not the other way around.
       *
       * And, of course, we're doing all this in one of the program's
       * most critical code sections, from a performance standpoint.
       * These cursors get redrawn every single time we refresh the
       * screen.  It really could be somewhat smarter.
       */

      if (scope.curs && j == scope.select) {
	X = (scope.cursa - 1) * 10000 / num + l + 1;
	if (X != preva) {
	  SetColor(color[0]);
	  DrawLine(preva, 70, preva, v_points - 70);
	  preva = X;
	}
	if (X < h_points - 100) {
	  SetColor(p->color);
	  DrawLine(X, 70, X, v_points - 70);
	}

	X = (scope.cursb - 1) * 10000 / num + l + 1;
	if (X != prevb) {
	  SetColor(color[0]);
	  DrawLine(prevb, 70, prevb, v_points - 70);
	  prevb = X;
	}
	if (X < h_points - 100) {
	  SetColor(p->color);
	  DrawLine(X, 70, X, v_points - 70);
	}
      }

      /* XXX make sure that if we're displaying a digital signal,
       * we go into digital display mode.  Should be elsewhere.
       */

      if (p->bits == 0 && p->signal->bits != 0) {
	p->bits = p->signal->bits;
      }

      if (!p->bits)		/* analog display mode: draw one line */
	start = end = -1;
      else {			/* logic analyzer mode: draw bits lines */
	start = 0;
	end = p->bits - 1;
      }

      for (bit = start ; bit <= end ; bit++) {

	/* Hardwired: 16 y-coords between bits in digital mode */
	bitoff = bit * 16 - end * 8 + 4;

	/* SignalLine structures contain all the stored information
	 * about the (x,y) coordinates we've drawn already and may
	 * need to erase
	 */
	sl = p->signalline[bit < 0 ? 0 : bit];

	if (sl == NULL) {
	  sl = (SignalLine *) malloc(sizeof(SignalLine));
	  if (sl == NULL) {
	    perror("xoscope: malloc(SignalLine)");
	    break;
	  }
	  bzero(sl, sizeof(SignalLine));
	  p->signalline[bit < 0 ? 0 : bit] = sl;
	}

	if ((p->signal->frame != p->old_frame) || (p->old_frame == 0)) {

	  /* New frame.  Start all the way at the left side. */

	  prev = -1;
	  X = 0;
	    
	  /* If we're not in an accumulate mode, erase anything
           * lingering on the old line
	   */

	  if (!(scope.mode % 2) && (sl->prev_line_last > sl->prev_line_start)){
	    if (scope.mode < 2)
	      PolyPoint(color[0], sl->points + sl->prev_line_start,
			  sl->prev_line_last - sl->prev_line_start + 1);
	    else
	      PolyLine(color[0], sl->points + sl->prev_line_start,
			 sl->prev_line_last - sl->prev_line_start + 1);
	  }

	  /* Now swap the current line into the previous line
	   * and start a new current line.
	   */

	  sl->prev_line_start = sl->current_line_start;
	  sl->prev_line_last = sl->current_line_next - 1;

	  if (sl->current_line_start == 0) sl->current_line_start = 1024;
	  else sl->current_line_start = 0;

	  sl->current_line_next = sl->current_line_start;

	} else {

	  /* Continuation of a frame we've seen before.  Pick up where
	   * we left off.  We assume here that 'l' hasn't changed from
	   * one pass to next
	   */

	  if (sl->current_line_next > sl->current_line_start) {
	    X = sl->points[sl->current_line_next - 1].x - l;
	    Y = sl->points[sl->current_line_next - 1].y;
	  } else {
	    X = 0;
	    Y = 0;
	  }

	  prev = X * num / 10000;

	}

	/* Compute the points we want to draw on the current line and
	 * write them into the sl->points[] array.  'time' is an index
	 * into samp[] array, computed from the current x-coord.
	 * 'prev', the last 'time' we actually used to draw a point,
	 * is here to make sure that if a single sample is spread over
	 * several x-coords, we skip coords to draw one line across
	 * them all.
	 */

	/* XXX we'd really like to draw one point extra, so we're
	 * never left with a signal line that stops before the end of
	 * the screen.
	 */

	/* for (x = X; x <= h_points - 100 - l ; x++) { */
	for (x = X; 1; x++) {

	  time = x * num / 10000;

	  if ((time > prev) && (time <= p->signal->num - 1)) {

	    /* Hardwired: 8 y-coords is height of digital line */

	    y = off - (bit < 0 ? samp[time]
		       : (bitoff - (samp[time] & (1 << bit) ? 0 : 8)))
	      * mult / div;

	    if (scope.mode >= 4 && X) {

	      /* Step mode.  Draw a horizontal line segment,
	       * then a vertical one (instead of a single line)
	       */

	      sl->points[sl->current_line_next].x = l + x;
	      sl->points[sl->current_line_next].y = Y;
	      sl->current_line_next ++;
	    }

	    sl->points[sl->current_line_next].x = l + x;
	    sl->points[sl->current_line_next].y = y;
	    sl->current_line_next ++;

	    X = x; Y = y; prev = time;

	    if (x > h_points - 100 - l) break;

	  }

	  /* XXX this is here if we try to display a signal with no
	   * plotable data points (i.e, p->signal->num is zero,
	   * or we've started the draw past its last point)
	   */

	  if (time > p->signal->num + 1) break;

	}

	/* If we're not in an accumulate mode, erase everything on the
	 * previous line up to the last X coordinate we're going to
	 * draw on the current line.
	 */

	if (!(scope.mode % 2) && (sl->prev_line_last > sl->prev_line_start)) {
	  for (i = sl->prev_line_start; i < sl->prev_line_last; i ++) {
	    if (sl->points[i].x >= l + X) break;
	  }
	  if (scope.mode < 2)
	    PolyPoint(color[0], sl->points + sl->prev_line_start,
			i - sl->prev_line_start + 1);
	  else
	    PolyLine(color[0], sl->points + sl->prev_line_start,
		       i - sl->prev_line_start + 1);
	  sl->prev_line_start = i;
	}

	/* Draw the points on the current line
	 *
	 * Even if nothing changed on this line, another signal might
	 * have erased an overlapping portion, so we always draw
	 * the entire signal, not just the newly added part.
	 */

	if (sl->current_line_next > sl->current_line_start) {
	  if (scope.mode < 2)
	    PolyPoint(p->color, sl->points + sl->current_line_start,
			sl->current_line_next - sl->current_line_start);
	  else
	    PolyLine(p->color, sl->points + sl->current_line_start,
		       sl->current_line_next - sl->current_line_start);
	}

      }

      p->old_frame = p->signal->frame;

      /* Draw tick marks on left and right sides of display showing zero pos */
      SetColor(p->color);
      DrawLine(90, off, 100, off);
      DrawLine(h_points - 100, off, h_points - 90, off);
    }
  }
}

/* calculate any math and plot the results and the graticule */
void
show_data(void)
{

  /* Run any math functions, then measure statistics to be displayed,
   * like min, max, frequency.  If the timebase is fast enough (less
   * than 100 ms/div) do this only at the end of a frame.
   */

  do_math();

  if ((scope.scale >= 100) || !in_progress)
    measure_data(&ch[scope.select], &stats);

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

/* animate() - get and plot some data
 *
 */

int
animate(void *data)
{
  static struct timeval current_time, prev_time;

  /* To avoid hammering the X server, don't do anything if it's been
   * less than scope.min_interval milliseconds (default 50) since the
   * last time we ran this function.  If we do skip processing, then
   * set a timeout to make sure we run again scope.min_interval
   * milliseconds from now.
   */

  gettimeofday(&current_time, NULL);

  if ((prev_time.tv_sec <= current_time.tv_sec)
      && (prev_time.tv_sec + 10 > current_time.tv_sec)
      && (1000000 * (current_time.tv_sec - prev_time.tv_sec)
	  + current_time.tv_usec - prev_time.tv_usec
	  < 1000 * scope.min_interval)) {
    settimeout(scope.min_interval);
    setinputfd(-1);
    return;
  }

  prev_time = current_time;
  if (datasrc) setinputfd(datasrc->fd());
  settimeout(0);

  clip = 0;
  if (datasrc) {
    if (scope.run) {
      triggered = datasrc->get_data();
      if (triggered && scope.run > 1) { /* auto-stop single-shot wait */
	scope.run = 0;
	draw_text(1);
      }
    } else if (in_progress) {
      datasrc->get_data();
    } else {
      //usleep(100000);		/* no need to suck all CPU cycles */
      setinputfd(-1);		/* scope not running, so why listen? */
    }
  }
  show_data();

  return TRUE;
}

/* [re]initialize graphics screen */
void
init_screen()
{
  int i, channelcolor[] = CHANNELCOLOR;

  for (i = 0 ; i < CHANNELS ; i++) {
    ch[i].color = color[channelcolor[i]];
  }
  fix_widgets();
  offset = v_points / 2;
  draw_text(1);
  clear();
}

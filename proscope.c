/*
 * @(#)$Id: proscope.c,v 1.7 2000/07/11 23:01:25 twitham Rel $
 *
 * Copyright (C) 1997 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the ProbeScope protocol, as defined in its' hlp file.
 * Tested with a "V3.0 1995" RadioShack ProbeScope.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include "proscope.h"
#include "oscope.h"
#include "display.h"

unsigned int ps_rate[] = {20000000, 10000000, 2000000, 1000000,
			  200000, 100000, 20000, 10000, 2000, 1000};

ProbeScope ps;

/* initialize the probescope structure */
void
init_probescope()
{
  ps.found = ps.wait = ps.volts = ps.trigger = ps.level = ps.dvm = ps.flags = 0;
  ps.coupling = "?";
}

/* get one set of bytes from the ProbeScope, if possible */
void
probescope()
{
  int c, maybe, byte = -1, try = 0, cls = 0, dvm = 0, gotdvm = 0, flush = 1;

  while (byte < 138 && try < 280) { /* allow max 2 cycles to find sync byte */
    c = getonebyte();
    if (c < 0) {		/* byte available? no: */
      if (++try > 10 && byte <= 1) {
	PSDEBUG("%s\n", "!");
	byte = -1;
	try = 999;		/* give up if we've retried ~ 4ms */
      } else {
	PSDEBUG("%s", ".");
	flush = 0;		/* we've caught up with the serial FIFO! */
	usleep(400);		/* next arrive ~ 8bits/19200bits/s = .4167ms */
      }
    } else {			/* byte available? yes: */
      if (c > 0x7b) {		/* Synchronization Byte and/or WAITING! */
	PSDEBUG("\n%3d", byte);
	PSDEBUG(",%3d:", try);
	PSDEBUG("%02x  ", c);
	if ((maybe = c & PS_WAIT ? 0 : 1) != ps.wait) {
	  ps.wait = maybe;
	  cls = 1;
	}
	while ((c = getonebyte()) > 0x7b && ++try < 280) {
	}			/* suck all available sync/wait bytes */
	if (ps.wait || try >= 280) {
	  byte = -1;		/* return now if we're waiting or timed out */
	  try = 888;
	} else if (c > -1) {	/* we have byte 1 now */
	  byte = 1;
	} else {		/* byte 1 could be next */
	  byte = 0;
	}
      }
      if (byte >= 5 && byte <= 132) { /* Signal Bytes from 0 to 3F Hex */
	mem[25].data[byte - 4] = (c - 32) * 5;
      } else if (byte >= 134 && byte <= 136) { /* DVM Values, 100, 10, 1 */
	PSDEBUG("%d,", c);
	dvm += c * (byte == 134 ? 100 : byte == 135 ? 10 : 1);
	gotdvm = 1;
      } else if (byte == 1) {	/* Switch Setting Byte */
	PSDEBUG("1sw=%02x  ", c);
	if ((maybe = c & PS_100V ? 100 : c & PS_10V ? 10 : 1) != ps.volts) {
	  ps.volts = maybe;
	  mem[25].volts = maybe * 1000;
	  cls = 1;
	}
	ps.coupling = c & PS_AC ? "AC" : c & PS_DC ? "DC" : "GND";
      } else if (byte == 2) {	/* Timebase Definition Byte */
	PSDEBUG("2tb=%02x  ", c);
	if (c < 10 && (maybe = ps_rate[c]) != mem[25].rate) {
	  mem[25].rate = maybe;
	  cls = 1;
	}
      } else if (byte == 3) {	/* Trigger Definition Byte, true if set */
	PSDEBUG("3tr=%02x  ", c);
	if (c != ps.trigger) {
	  ps.trigger = c;
	  cls = 1;
	}
      } else if (byte == 4) {	/* Trigger Level Byte, true if set */
	PSDEBUG("4lv=%02x  ", c);
	if ((maybe = c & PS_TP5 ? 5 : c & PS_TP3 ? 3 : c & PS_TP1 ? 1
	     : c & PS_TM1 ? -1 : c & PS_TM3 ? -3 : -5) != ps.level) {
	  ps.level = maybe;
	  cls = 1;
	}
      } else if (byte == 133) { /* Undocumented Trigger Level (maybe?) */
	PSDEBUG("133TR=%02x  ", c);
      } else if (byte == 137) {	/* DVM Flags, true if set */
	PSDEBUG(" 137FL=%02x", c);
	ps.flags = c;
	if (c & PS_UNDERFLOW || c & PS_OVERFLOW)
	  clip = 3;
	if (c & PS_MINUS)
	  dvm *= -1;
      }
      if (byte > -1) byte++;	/* processed a valid byte */
      try++;
    }
  }
  mem[25].data[0] = mem[25].data[1];
  mem[25].num = 128;
  if (gotdvm) ps.dvm = dvm;
  if (cls) clear();		/* non-DVM text need changed? */
  if (flush) flush_serial();	/* catch up if we're getting behind */
}

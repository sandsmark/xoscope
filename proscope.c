/*
 * @(#)$Id: proscope.c,v 1.1 1997/05/24 23:22:22 twitham Exp $
 *
 * Copyright (C) 1997 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the ProbeScope protocol, as defined in its' hlp file.
 *
 */

#include <stdio.h>
#include <sys/time.h>
#include "proscope.h"
#include "oscope.h"
#include "display.h"

#ifdef PSDEBUG
#undef PSDEBUG
#define PSDEBUG(format, arg) printf(format, arg)
#else
#define PSDEBUG(format, arg) ;
#endif

unsigned int ps_rate[] = {20000000, 10000000, 2000000, 1000000,
			  200000, 100000, 20000, 10000, 2000, 1000};

/* sleep approximately this many microseconds  */
void
microsleep(unsigned long microsecs) {
  struct timeval timeout;

  timeout.tv_sec = microsecs/1000000L;
  timeout.tv_usec = microsecs - (timeout.tv_sec * 1000000L);
  select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &timeout);
}

/* get one set of bytes from the ProbeScope, if possible */
void
probescope()
{
  int c, byte = -1, try = 0;

  while (byte < 138 && try < 500) {
    c = getonebyte();
    if (c < 0) {		/* byte available? no: */
      if (byte < 0 && ++try > 25)
	try = 5000;		/* give up if we've retried ~ 10msec */
      else
	microsleep(400);	/* next arrive ~ 8bits/19200bits/s = .4167ms */
    } else {			/* byte available? yes: */
      if (c > 0x7b) {		/* Synchronization Byte and/or WAITING! */
	  PSDEBUG("\n%3d", byte);
	  PSDEBUG(",%3d:  ", try);
	  PSDEBUG("0=%02x  ", c);
	  byte = 0;		/* reset byte counter */
      }
      if (byte >= 5 && byte <= 132) { /* Signal Bytes from 0 to 3F Hex */
	mem[25].data[byte - 4] = (c - 32) * 5;
      } else if (byte >= 134 && byte <= 136) { /* DVM Values, 100, 10, 1 */
	PSDEBUG("%d.", c);
      } else if (byte == 1) {	/* Switch Setting Byte */
	PSDEBUG("1sw=%02x  ", c);
      } else if (byte == 2) {	/* Timebase Definition Byte */
	PSDEBUG("2tb=%02x  ", c);
	if (c < 10 && ps_rate[c] != mem[25].rate) {
	  mem[25].rate = ps_rate[c];
	  clear();
	}
      } else if (byte == 3) {	/* Trigger Definition Byte, true if set */
	PSDEBUG("3tr=%02x  ", c);
      } else if (byte == 4) {	/* Trigger Level Byte, true if set */
	PSDEBUG("4lv=%02x  ", c);
      } else if (byte == 133) { /* Undocumented Trigger Level */
	PSDEBUG("133TR=%02x  ", c);
      } else if (byte == 137) {	/* DVM Flags, true if set */
	if (c & PS_UNDERFLOW || c & PS_OVERFLOW)
	  clip = 3;
	PSDEBUG(" 137FL=%02x", c);
      }
      if (byte > -1) byte++;	/* processed a valid byte */
      try++;
    }
  }
  mem[25].data[0] = mem[25].data[1];
}

/*
 * @(#)$Id: sc_sb.c,v 1.7 1997/06/07 21:33:02 twitham Rel $
 *
 * Copyright (C) 1997 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the (mono only) DOS/DJGPP Sound Blaster interface
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <conio.h>
#include "sb.h"
#include "oscope.h"		/* program defaults */

extern int sb_dma_active;	/* in sb.c */

int snd = 0;			/* whether sound is on or off */

/* close the sound device */
void
close_sound_card()
{
  if (snd)
    sb_cleanup();
  snd = 0;
}

/* attempt to change sample rate and return actual sample rate set */
int
set_sound_card(int rate, int chan, int bits)
{
  if (!snd) return(rate);
  
  return(sb_set_sample_rate(rate));
}

/* [re]set the sound card, and return actual sample rate */
int
reset_sound_card(int rate, int chan, int bits)
{
  if (!snd) return(rate);

  return(set_sound_card(rate, chan, bits));
}

/* turn the sound device on */
void
open_sound_card(int dma)
{
  int i;

  if(!(i = sb_init())) {
    fprintf(stderr,"%s: can't init soundcard!\n", progname);
    snd = 0;
  } else {
    printf("using soundcard at 0x%x.\n", i);
    snd = 1;
    set_sound_card(44000, 2, 8);
  }
}

/* get data from sound card, return value is whether we triggered or not */
int
get_data()
{
  static unsigned char *prev, *buff, buffer[MAXWID * 2 + SAMPLESKIP];
  static int triggered;
  int i = 0;

  if (!snd) return(0);		/* device open? */

  if (sb_dma_active) return(triggered);

  sb_rec(buffer, h_points * 2);
  prev = buffer + SAMPLESKIP;
  buff = buffer + SAMPLESKIP + 1;
  if (scope.trige == 1) {
    while (((i++ < h_points)) && ((*(buff) < scope.trig) ||
				  (*(prev) >= scope.trig))) {
      prev += 1; buff += 1;
    }
  } else if (scope.trige == 2) {
    while (((i++ < h_points)) && ((*(buff) > scope.trig) ||
				  (*(prev) <= scope.trig))) {
      prev += 1; buff += 1;
    }
  }
  if (i > h_points)		/* haven't triggered within the screen */
    return(triggered = 0);	/* give up and keep previous samples */

  buff = prev;
  for(i=0; i < h_points; i++) {	/* move it into channel 1 and 2 */
    if (*buff == 0 || *buff == 255)
      clip = 1;
    mem[23].data[i] = mem[24].data[i] = (short)(*buff++) - 128;
  }
  return(triggered = 1);
}

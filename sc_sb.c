/*
 * @(#)$Id: sc_sb.c,v 1.4 1997/05/04 20:58:34 twitham Rel1_3 $
 *
 * Copyright (C) 1997 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file doesn't yet implement the DOS sound card interface
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <conio.h>
#include "sb_lib.h"
#include "sbdetect.h"
#include "dsp.h"
#include "dma.h"
#include "oscope.h"		/* program defaults */

void
close_sound_card()
{
  sb_uninstall_driver();
}

/* attempt to change sample rate and return actual sample rate set */
int
set_sound_card(int rate, int chan, int bits)
{
  sb_uninstall_driver();
  if(sb_install_driver(rate)!=SB_SUCCESS) {
    fprintf(stderr,"Driver error: %s",sb_driver_error);
    exit(1);
  }
  return(sb_sample_frequency);
}

void
open_sound_card(int dma)
{
  if(sb_install_driver(scope.rate)!=SB_SUCCESS) {
    fprintf(stderr,"Driver error: %s",sb_driver_error);
    exit(1);
  }
  set_sound_card(44000, 2, 8);
  clrscr();

  if(sb_info.dspVersion>=0x040C)
    cprintf("Sound-Blaster AWE32");
  else if(sb_info.dspVersion>=0x0400)
    cprintf("Sound-Blaster 16");
  else if(sb_info.dspVersion>=0x0300)
    cprintf("Sound-Blaster Pro");
  else if(sb_info.dspVersion>=0x0201)
    cprintf("Sound-Blaster 2.0");
  else
    cprintf("Sound-Blaster 1.0/1.5");

  cprintf(" detected at Address %xH, IRQ %d\n\r",sb_info.reset-6,sb_info.IRQ);
  cprintf("Using a sampling rate of %d",sb_sample_frequency);
}

/* [re]set the sound card, and return actual sample rate */
int
reset_sound_card(int rate, int chan, int bits)
{
  return(set_sound_card(rate, chan, bits));
}

/* get data from sound card, return value is whether we triggered or not */
int
get_data()
{
  static unsigned char *prev, *buff, buffer[MAXWID * 2];
  int i = 0;

  /* flush the sound card's buffer */

  sb_dma8bitReadSC((DWORD)buffer, MAXWID * 2);
  prev = buffer + SAMPLESKIP;
  buff = buffer + SAMPLESKIP + 2;
  if (scope.trige == 1) {
    while (((i++ < h_points)) && ((*(buff + scope.trigch) < scope.trig) ||
				  (*(prev + scope.trigch) >= scope.trig))) {
      prev += 2;
      buff += 2;
    }
  } else if (scope.trige == 2) {
    while (((i++ < h_points)) && ((*(buff + scope.trigch) > scope.trig) ||
				    (*(prev + scope.trigch) <= scope.trig))) {
      prev += 2;
      buff += 2;
    }
  }
  if (i > h_points)		/* haven't triggered within the screen */
    return(0);			/* give up and keep previous samples */

  clip = 0;
  buff = scope.trige ? prev : buffer;
  for(i=0; i < h_points; i++) {	/* move it into channel 1 and 2 */
    if (*buff == 0 || *buff == 255)
      clip = 1;
    mem[23].data[i] = (short)(*buff++) - 128;
    if (*buff == 0 || *buff == 255)
      clip = 2;
    mem[24].data[i] = (short)(*buff++) - 128;
  }
  return(1);
}

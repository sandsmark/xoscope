/*
 * @(#)$Id: sc_linux.c,v 1.5 1997/05/04 20:12:56 twitham Rel1_3 $
 *
 * Copyright (C) 1996 - 1997 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the Linux /dev/dsp sound card interface
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include "oscope.h"		/* program defaults */
#include "func.h"

int snd = 0;			/* file descriptor for sound device */

/* abort and show system error if given ioctl status is bad */
void
check_status(int status, int line)
{
  if (status < 0) {
    sprintf(error, "%s: error from sound device ioctl at line %d",
	    progname, line);
    perror(error);
    cleanup();
    exit(1);
  }
}

void
close_sound_card()
{
  close(snd);
}

/* attempt to change sample rate and return actual sample rate set */
int
set_sound_card(int rate)
{
  int actual = rate;

  check_status(ioctl(snd, SOUND_PCM_SYNC, 0), __LINE__);
  check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &actual), __LINE__);
  check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
  return(actual);
}

/* [re]set the sound card, and return actual sample rate */
int
reset_sound_card(int rate, int chan, int bits)
{
  int parm;

  parm = chan;			/* set mono/stereo */
  check_status(ioctl(snd, SOUND_PCM_WRITE_CHANNELS, &parm), __LINE__);

  parm = bits;			/* set 8-bit samples */
  check_status(ioctl(snd, SOUND_PCM_WRITE_BITS, &parm), __LINE__);

  /* set sampling rate */
  return(set_sound_card(rate));
}

void
open_sound_card(int dma)
{
  int parm, i = 5;

  /* we try a few times in case someone else is using device (FvwmAudio) */
  while ((snd = open("/dev/dsp", O_RDONLY)) < 0 && i > 0) {
    sprintf(error, "%s: cannot open /dev/dsp, retrying %d", progname, i--);
    perror(error);
    sleep(1);
  }
  if (snd < 0) {		/* open DSP device for read */
    sprintf(error, "%s: cannot open /dev/dsp", progname);
    perror(error);
    cleanup();
    exit(1);
  }
  reset_sound_card(44000, 2, 8);
  parm = dma;			/* set DMA buffer size */
  check_status(ioctl(snd, SOUND_PCM_SUBDIVIDE, &parm), __LINE__);
}

void
flush_sound_card()
{
  check_status(ioctl(snd, SNDCTL_DSP_RESET), __LINE__);
}

/* get data from sound card, return value is whether we triggered or not */
int
get_data()
{
  static unsigned char datum[2], prev[2], *buff;
  static char buffer[MAXWID * 2], junk[SAMPLESKIP];
  int i = 0;

  /* flush the sound card's buffer */
  check_status(ioctl(snd, SNDCTL_DSP_RESET), __LINE__);
  read(snd, junk, SAMPLESKIP);	/* toss some possibly invalid samples */
  if (scope.trige == 1) {
    read(snd, datum, 2);	/* look for rising edge */
    do {
      memcpy(prev, datum, 2);	/* remember previous, read channels */
      read(snd, datum, 2);
    } while (((i++ < h_points)) && ((datum[scope.trigch] < scope.trig) ||
				    (prev[scope.trigch] >= scope.trig)));
  } else if (scope.trige == 2) {
    read(snd, datum, 2);	/* look for falling edge */
    do {
      memcpy(prev, datum, 2);	/* remember previous, read channels */
      read(snd, datum, 2);
    } while (((i++ < h_points)) && ((datum[scope.trigch] > scope.trig) ||
				    (prev[scope.trigch] <= scope.trig)));
  }
  if (i > h_points)		/* haven't triggered within the screen */
    return(0);			/* give up and keep previous samples */

  clip = 0;
  memcpy(buffer, prev, 2);	/* now get the post-trigger data */
  memcpy(buffer + 2, datum, 2);
  read(snd, buffer + (scope.trige ? 4 : 0),
       h_points * 2 - (scope.trige ? 4 : 0));
  buff = buffer;
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

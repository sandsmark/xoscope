/*
 * @(#)$Id: sc_linux.c,v 1.8 1997/05/31 19:36:29 twitham Rel $
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

/* close the sound device */
void
close_sound_card()
{
  if (snd)
    close(snd);
  snd = 0;
}

/* show system error and close sound device if given ioctl status is bad */
void
check_status(int status, int line)
{
  if (status < 0) {
    sprintf(error, "%s: error from sound device ioctl at line %d",
	    progname, line);
    perror(error);
    close_sound_card();
  }
}

/* attempt to change sample rate and return actual sample rate set */
int
set_sound_card(int rate)
{
  int actual = rate;

  if (!snd) return(rate);
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

  if (!snd) return(rate);

  parm = chan;			/* set mono/stereo */
  check_status(ioctl(snd, SOUND_PCM_WRITE_CHANNELS, &parm), __LINE__);

  parm = bits;			/* set 8-bit samples */
  check_status(ioctl(snd, SOUND_PCM_WRITE_BITS, &parm), __LINE__);

  /* set sampling rate */
  return(set_sound_card(rate));
}

/* turn the sound device on */
void
open_sound_card(int dma)
{
  int parm, i = 5;

  /* we try a few times in case someone else is using device (FvwmAudio) */
  while ((snd = open("/dev/dsp", O_RDONLY)) < 0 && i > 0) {
    sprintf(error, "%s: can't open /dev/dsp, retrying %d", progname, i--);
    perror(error);
    sleep(1);
  }
  if (snd < 0) {		/* open DSP device for read */
    sprintf(error, "%s: can't open /dev/dsp", progname);
    perror(error);
    snd = 0;
    return;
  }
  parm = dma;			/* set DMA buffer size */
  check_status(ioctl(snd, SOUND_PCM_SUBDIVIDE, &parm), __LINE__);
}

/* get data from sound card, return value is whether we triggered or not */
int
get_data()
{
  static unsigned char datum[2], prev[2], *buff;
  static char buffer[MAXWID * 2], junk[SAMPLESKIP];
  int i = 0;

  if (!snd) return(0);		/* device open? */

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

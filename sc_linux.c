/*
 * @(#)$Id: sc_linux.c,v 1.10 1999/08/29 01:59:10 twitham Exp $
 *
 * Copyright (C) 1996 - 1999 Tim Witham <twitham@quiknet.com>
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

				/* quick macro */
/* show system error and close sound device if given ioctl status is bad */
#define check_status(status, line) \
{ \
  if (status < 0) { \
    sprintf(error, "%s: error from sound device ioctl at line %d", \
	    progname, line); \
    perror(error); \
    close_sound_card(); \
  } \
}

/* close the sound device */
void
close_sound_card()
{
  if (snd)
    close(snd);
  snd = 0;
}

/* attempt to change sample rate and return actual sample rate set */
int
set_sound_card(int rate)
{
  int actual = rate;
  static char junk[SAMPLESKIP];

  if (!snd) return(rate);
  check_status(ioctl(snd, SOUND_PCM_SYNC, 0), __LINE__);
  check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &actual), __LINE__);
  check_status(ioctl(snd, SNDCTL_DSP_RESET), __LINE__);
  check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
  read(snd, junk, SAMPLESKIP);
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
  static char buffer[MAXWID * 2], junk[DISCARDBUF];
  static int i;
  audio_buf_info info;

  if (!snd) return(0);		/* device open? */

  /* Discard excess samples so we can keep our time snapshot close to
     real-time and minimize sound recording overruns.  If we flush too
     much, then we have to wait for more to accumulate.  In 1.5 I kept
     only 2 screenfuls of samples and flushed the rest, but this was
     too few for slow sample rates.  So, let's just keep the buffer
     about 1/3 full, which should work better for all rates.  */

  check_status(ioctl(snd, SNDCTL_DSP_GETISPACE, &info), __LINE__);
#ifdef DEBUG
  printf("avail:%d\ttotal:%d\tsize:%d\tbytes:%d\n",
	 info.fragments, info.fragstotal, info.fragsize, info.bytes);
#endif
  if ((i = info.bytes - info.fragstotal * info.fragsize / 6 * 2) > 0)
    read(snd, junk, i < DISCARDBUF ? i : DISCARDBUF);
#ifdef DEBUG
  check_status(ioctl(snd, SNDCTL_DSP_GETISPACE, &info), __LINE__);
  printf("\tavail:%d\ttotal:%d\tsize:%d\tbytes:%d\n",
	 info.fragments, info.fragstotal, info.fragsize, info.bytes);
#endif
  i = 0;
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

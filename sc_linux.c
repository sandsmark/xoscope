/*
 * @(#)$Id: sc_linux.c,v 1.12 2000/02/26 08:19:10 twitham Exp $
 *
 * Copyright (C) 1996 - 1999 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the Linux /dev/dsp or ESD sound card interface
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef ESD
#define SOUNDDEVICE "ESounD"
#include <esd.h>
#else
#define SOUNDDEVICE "/dev/dsp"
#endif

#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include "oscope.h"		/* program defaults */
#include "func.h"

int snd = 0;			/* file descriptor for sound device */

#ifdef ESD
#define check_status(status, line) {}
#else
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
#endif

/* close the sound device */
void
close_sound_card()
{
  if (snd)
    close(snd);
  snd = 0;
}

/* turn the sound device on */
void
open_sound_card(int dma)
{
  int parm;
  char device[] = SOUNDDEVICE;
  int i = 3;

  if (snd) close(snd);

#ifdef ESD
  snd = esd_monitor_stream(ESD_BITS8|ESD_STEREO|ESD_STREAM|ESD_MONITOR,
			   ESD, NULL, progname);
  fcntl(snd, F_SETFL, O_NONBLOCK);
  i = 0;
#else
  /* we try a few times in case someone else is using device (FvwmAudio) */
  while ((snd = open(device, O_RDONLY)) < 0 && i > 0) {
    sprintf(error, "%s: can't open %s, retrying %d", progname, device, i--);
    perror(error);
    sleep(1);
  }
#endif

  if (snd < 0) {		/* open DSP device for read */
    sprintf(error, "%s: can't open %s", progname, device);
    perror(error);
    snd = 0;
    return;
  }
  parm = dma;			/* set DMA buffer size */
  check_status(ioctl(snd, SOUND_PCM_SUBDIVIDE, &parm), __LINE__);
}

/* [re]set the sound card, and return actual sample rate */
int
reset_sound_card(int rate, int chan, int bits)
{
  int parm;
  static char junk[SAMPLESKIP];

  if (snd) close(snd);
  open_sound_card(scope.dma);
  if (!snd) return(rate);

  parm = chan;			/* set mono/stereo */
  check_status(ioctl(snd, SOUND_PCM_WRITE_CHANNELS, &parm), __LINE__);

  parm = bits;			/* set 8-bit samples */
  check_status(ioctl(snd, SOUND_PCM_WRITE_BITS, &parm), __LINE__);

  parm = rate;			/* set sampling rate */
  check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &parm), __LINE__);
  check_status(ioctl(snd, SOUND_PCM_READ_RATE, &parm), __LINE__);

  read(snd, junk, SAMPLESKIP);

#ifdef ESD
  return(ESD);
#else
  return(parm);
#endif
}

/* get data from sound card, return value is whether we triggered or not */
int
get_data()
{
  static unsigned char datum[2], prev[2], *buff;
  static char buffer[MAXWID * 2], junk[DISCARDBUF];
  static int i;
  audio_buf_info info = {0, 0, 0, MAXWID * 2};

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
  if (read(snd, buffer + (scope.trige ? 4 : 0),
	   h_points * 2 - (scope.trige ? 4 : 0)) < 0)
    return(0);
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

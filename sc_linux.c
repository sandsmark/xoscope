/*
 * @(#)$Id: sc_linux.c,v 1.19 2000/07/06 20:12:08 twitham Exp $
 *
 * Copyright (C) 1996 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the Linux ESD & /dev/dsp sound card interface
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef ESD
#include <esd.h>
#else
#define ESD 0
#endif
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include "oscope.h"		/* program defaults */
#include "func.h"

#define ESDDEVICE "ESounD"
#define SOUNDDEVICE "/dev/dsp"

int snd = 0;			/* file descriptor for sound device */
int esd = 0;			/* using esd (1) or /dev/dsp (0) */

/* close the sound device */
void
close_sound_card()
{
  if (snd) close(snd);
  snd = esd = 0;
}

/* show system error and close sound device if given ioctl status is bad */
void
check_status_ioctl(int d, int request, void *argp, int line)
{
  if (!esd && ioctl(d, request, argp) < 0) {
    sprintf(error, "%s: error from sound device ioctl at line %d",
	    progname, line);
    perror(error);
    close_sound_card();
  }
}

/* turn the sound device (esd or /dev/dsp) on */
void
open_sound_card(int dma)
{
  int parm;
  int i = 3;

  close_sound_card();

#if ESD
  if ((snd = esd_monitor_stream(ESD_BITS8|ESD_STEREO|ESD_STREAM|ESD_MONITOR,
				ESD, NULL, progname)) <= 0) {
    sprintf(error, "%s: can't open %s", progname, ESDDEVICE);
    perror(error);
  } else {			/* we have esd connection! non-block it? */
    if (dma < 4)
      fcntl(snd, F_SETFL, O_NONBLOCK);
    esd = 1;
  }
#endif

  /* we try a few times in case someone else is using device (FvwmAudio) */
  while (!esd && (snd = open(SOUNDDEVICE, O_RDONLY | O_NDELAY)) < 0 && i > 0) {
    sprintf(error, "%s: can't open %s, retrying %d",
	    progname, SOUNDDEVICE, i--);
    perror(error);
    sleep(1);
  }

  if (snd < 0) {
    sprintf(error, "%s: can't open %s", progname,
	    esd ? ESDDEVICE : SOUNDDEVICE);
    perror(error);
    snd = 0;
    return;
  }
  parm = dma;			/* set DMA buffer size */
  check_status_ioctl(snd, SOUND_PCM_SUBDIVIDE, &parm, __LINE__);
}

/* [re]set the sound card, and return actual sample rate */
int
reset_sound_card(int rate, int chan, int bits)
{
  int parm;
  static char junk[SAMPLESKIP];

  close_sound_card();
  open_sound_card(scope.dma);
  if (!snd) return(rate);

  parm = chan;			/* set mono/stereo */
  check_status_ioctl(snd, SOUND_PCM_WRITE_CHANNELS, &parm, __LINE__);

  parm = bits;			/* set 8-bit samples */
  check_status_ioctl(snd, SOUND_PCM_WRITE_BITS, &parm, __LINE__);

  parm = rate;			/* set sampling rate */
  check_status_ioctl(snd, SOUND_PCM_WRITE_RATE, &parm, __LINE__);
  check_status_ioctl(snd, SOUND_PCM_READ_RATE, &parm, __LINE__);

  read(snd, junk, SAMPLESKIP);

  return(esd ? ESD : parm);
}

/* get data from sound card, return value is whether we triggered or not */
int
get_data()
{
  static unsigned char datum[2], prev[2], *buff;
  static unsigned char buffer[MAXWID * 2], junk[DISCARDBUF];
  static int i, j, k;
  audio_buf_info info = {0, 0, 0, 2048}; /* emulation values for ESD */

  if (!snd) return(0);		/* device open? */

  if (!in_progress) {
    /* Discard excess samples so we can keep our time snapshot close to
       real-time and minimize sound recording overruns.  */

    check_status_ioctl(snd, SNDCTL_DSP_GETISPACE, &info, __LINE__);
#ifdef DEBUG
    printf("avail:%d\ttotal:%d\tsize:%d\tbytes:%d\n",
	   info.fragments, info.fragstotal, info.fragsize, info.bytes);
#endif
    k = samples(scope.rate);	/* minimum samples needed */
    if ((i = info.bytes - k * 4) > 0)
      read(snd, junk, i < DISCARDBUF ? i : DISCARDBUF);
#ifdef DEBUG
    check_status_ioctl(snd, SNDCTL_DSP_GETISPACE, &info, __LINE__);
    printf("\tavail:%d\ttotal:%d\tsize:%d\tbytes:%d\n",
	   info.fragments, info.fragstotal, info.fragsize, info.bytes);
#endif
    i = 0;
    if (scope.trige == 1) {
      read(snd, datum, 2);	/* look for rising edge */
      do {
	memcpy(prev, datum, 2);	/* remember previous, read channels */
	read(snd, datum, 2);
      } while (((i++ < k)) && ((datum[scope.trigch] < scope.trig) ||
				      (prev[scope.trigch] >= scope.trig)));
    } else if (scope.trige == 2) {
      read(snd, datum, 2);	/* look for falling edge */
      do {
	memcpy(prev, datum, 2);	/* remember previous, read channels */
	read(snd, datum, 2);
      } while (((i++ < k)) && ((datum[scope.trigch] > scope.trig) ||
				      (prev[scope.trigch] <= scope.trig)));
    } else {
      read(snd, prev, 2);
      read(snd, datum, 2);
    }
    if (i > k)			/* haven't triggered within the screen */
      return(0);		/* give up and keep previous samples */

    mem[23].data[0] = prev[0] - 127;
    mem[24].data[0] = prev[1] - 127;
    mem[23].data[1] = datum[0] - 127;
    mem[24].data[1] = datum[1] - 127;
    in_progress = 2;
  }
  k = samples(scope.rate);
  if ((j = read(snd, buffer,  (k - in_progress) * 2)) > 0) {
    buff = buffer;		/* get the post-trigger data */
    i = 0;
    while (i < j) {
      if (in_progress >= MAXWID) break;
      if (*buff == 0 || *buff == 255)
	clip = i % 2 + 1;
      mem[23 + i % 2].data[in_progress] = (short)(*buff++) - 127;
      in_progress += i++ % 2;
    }
    mem[23].num = mem[24].num = in_progress < MAXWID ? in_progress : MAXWID;
  }
  if (in_progress >= k || in_progress >= MAXWID)
    in_progress = 0;
  return(1);
}

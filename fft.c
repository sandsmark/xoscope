/*
 * @(#)$Id: fft.c,v 1.1 1996/04/16 03:59:25 twitham Exp $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements an external fft function for oscope.
 *
 */

#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "oscope.h"
#include "fft.h"

/* must be shorter than minimum screen width and multiple of 2 */
#define FFTLEN	512

/* for the fft function: x position to bin number map, and data buffer */
int xmap[MAXWID];
short fftdata[MAXWID];
short indata[2][MAXWID];
short outdata[MAXWID];
int channel, samples;

/* Fast Fourier Transform of channel sig */
static inline void
fft()
{
  static int i, bri;
  static long re, im, root, mask, it;
  static short *a, *b;

  a = fftdata;
  b = indata[channel];
  for(i = 0 ; i < MAXWID ; i++) {
    *a++=((long)(*b++)  * 32767) >> 7;
  }
  RealFFT(fftdata);
  a = outdata;
  for (i = 0 ; i < samples ; i++) {
    if (xmap[i] > -1) {
      bri = BitReversed[xmap[i]];
      re = fftdata[bri];
      im = fftdata[bri + 1];
      it = (re * re + im * im);
      root = 32;
      do {			/* Square root code */
	mask = it / root;
	root=(root + mask) >> 1;
      } while (abs(root - mask) > 1);
      a[i] = root;
    } else {
      a[i] = a[i - 1];
    }
  }
}

int
main(int argc, char **argv)
{
  short buff[sizeof(short)];
  short *in[2], *out = outdata, *inmax;
  int i, j = 1;

  channel = 0;
  if (argc > 1)
    channel = ((j = atoi(argv[1])) == 2 ? 1 : 0);
  samples = 640;
  if (argc > 2)
    samples = atoi(argv[2]);

  for (i = 0 ; i < MAXWID ; i++) {
    fftdata[i] = 0;
    xmap[i]=floor((double)i / 440.0 * FFTLEN / 2.0 + 0.5);
    if(xmap[i] >= FFTLEN / 2)	/* don't display these uncomputed bins */
      xmap[i] = -1;
    if(xmap[i] == xmap[i-1])	/* duplicate bin */
      xmap[i] = -1;
  }
  InitializeFFT(FFTLEN);

  in[0] = indata[0];
  in[1] = indata[1];
  inmax = indata[0] + samples;

  while (1) {
    if ((i = read(0, buff, sizeof(short))) != sizeof(short))
      exit(i);
    *(in[0]) = (short)(*buff);
    if (j) {
      if ((i = read(0, buff, sizeof(short))) != sizeof(short))
	exit(i);
      *(in[1]) = (short)(*buff);
    }

    in[0]++;
    in[1]++;
    out++;
    if (in[0] >= inmax) {
      in[0] = indata[0];
      in[1] = indata[1];
      out = outdata;
      fft();
    }

    if ((i = write(1, out, sizeof(short))) != sizeof(short))
      exit(i);
  }
  EndFFT();
}

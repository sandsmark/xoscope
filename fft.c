/*
 * @(#)$Id: fft.c,v 1.2 1996/04/21 02:25:05 twitham Rel1_0 $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements internal & external FFT function for oscope.
 *
 */

#include <math.h>
#include "oscope.h"
#include "fft.h"

/* for the fft function: x position to bin number map, and data buffer */
int xmap[MAXWID];
short fftdata[MAXWID];

/* Fast Fourier Transform of in to out */
void
fft(short *in, short *out)
{
  static int i, bri;
  static long re, im, root, mask, it;
  static short *a, *b;

  a = fftdata;
  b = in;
  for(i = 0 ; i < MAXWID ; i++) {
    *a++=((long)(*b++)  * 32767) >> 7;
  }
  RealFFT(fftdata);
  a = out;
  for (i = 0 ; i < h_points ; i++) {
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

/* initialize global buffers for FFT */
void
init_fft()
{
  int i;

  for (i = 0 ; i < MAXWID ; i++) {
    fftdata[i] = 0;
    xmap[i]=floor((double)i / 440.0 * FFTLEN / 2.0 + 0.5);
    if(xmap[i] >= FFTLEN / 2)	/* don't display these uncomputed bins */
      xmap[i] = -1;
    if(xmap[i] == xmap[i-1])	/* duplicate bin */
      xmap[i] = -1;
  }
  InitializeFFT(FFTLEN);
}

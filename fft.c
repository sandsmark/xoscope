/* -*- mode: C++; fill-column: 100; c-basic-offset: 4; -*-
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements internal & external FFT function for xoscope.  It is mostly pieces cut and
 * pasted from freq's freq.c and setupsub.c for easier synchronization with that program.
 *
 */

#include <math.h>
#include <string.h>
#include "xoscope.h"
#include "fft.h"

/* these are all constants in oscope's context */
#define WINDOW_RIGHT			FFTLEN
#define WINDOW_LEFT			0

#define fftlen				FFTLEN
#define freq_scalefactor		1
#define freq_base			0
#define SampleRate			44100
#define shift				0

/* for the fft function: x position to bin number map, and data buffer */
int x[WINDOW_RIGHT-WINDOW_LEFT+1];     /* Array of bin #'s displayed */
int x2[WINDOW_RIGHT-WINDOW_LEFT+1];    /* Array of terminal bin #'s */
int *pX,*pX2;
short fftdata[FFTLEN];
short displayval[FFTLEN/2];
int *bri;
short *pDisplayval;
short *pOut;

/* Fast Fourier Transform of in to out */

void fft(short *in, short *out, int inLen)
{
    int i;
    long a2 = 0;	/* Variables for computing Sqrt/Log of Amplitude^2 */
    short *p1, *p2;	/* Various indexing pointers */

    p1=fftdata;
    p2=in;
    for(i = 0; i < inLen && i < fftlen; i++) {
	*p1++ = *p2++;
    }
      
    RealFFT(fftdata);

    /* Use sqrt(a2) in averaging mode and linear-amplitude mode */
    /* Use pointers for indexing to speed things up a bit. */
    bri = BitReversed;
    pDisplayval = displayval;
    for(i = 0; i < fftlen / 2; i++){
	/* Compute the magnitude */
	register long re = fftdata[*bri];
	register long im = fftdata[(*bri)+1];

	if((a2 = re*re + im*im) < 0) {		/* Watch for possible overflow */
	    a2 = 0;
	}
			
	if (a2 >= (1<<22)) {			/* avoid overflowing the short */
	    *pDisplayval  = (1<<15) - 1;	/* max short = 2^15 = sqrt(2^22)*16 */
	} else {
	    *pDisplayval = sqrt(a2)*16;
	}

	bri++;
	pDisplayval++;
    }
    /*fprintf(stderr, "num values in displayval: %ld\n", pDisplayval - displayval - 1);*/

    pX = x;
    pX2 = x2;
    pOut = out;

    memcpy(out, displayval, sizeof(short)*fftlen/2);
}

/* initialize global buffers for FFT */

void init_fft(void)
{
    InitializeFFT(fftlen);
}

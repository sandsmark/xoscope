/*
 * @(#)$Id: offt.c,v 2.0 2008/12/17 17:35:46 baccala Exp $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * [x]oscope external math filter command example in C.
 *
 * See operl for an example that can do arbitrary math at run-time.
 *
 * An external oscope command must continuously read two shorts from
 * stdin and write one short to stdout.  It must exit when stdin
 * closes.  The two shorts read in are the current input samples from
 * channel 1 & 2.  The short written out should be the desired math
 * function result.
 *
 * This example provides the FFT algorithm as an external command.
 *
 * usage: offt [ channel [ samples ] ]
 *
 * The first command-line argument should be a number (default = 1):
 *
 * 0: FFT *one* signal from stdin.  Use this only at the end of a pipe
 *    that has already split oscope's two signals down to one signal.
 *    For example: operl '$x * $y' | offt 0
 * 
 * 1: FFT of channel 1
 * 
 * 2: FFT of channel 2
 * 
 * Second command-line argument is the number of samples per screenful.
 * You may need to use this if you resize the window.  (default = 640)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "xoscope.h"		/* for MAXWID */
#include "fft.h"		/* for EndFFT */
#include "func.h"		/* for init_fft() and fft() */

int h_points;			/* number of samples per screen */

int
main(int argc, char **argv)	/* main program */
{
  short indata[2][MAXWID], outdata[MAXWID], buff[sizeof(short)];
  short *in[2], *out, *inmax;	/* signal I/O buffers and pointers */
  int i, two, channel;

  two = 1; channel = 0;		/* assume two inputs, using first channel */
  if (argc > 1)			/* first arg: channel to read or 0 for a pipe */
    channel = ((two = atoi(argv[1])) == 2 ? 1 : 0);
  h_points = 640;
  if (argc > 2)			/* second arg: samples per screen */
    h_points = atoi(argv[2]);

  init_fft();			/* initialize the FFT (in fft.c) */

  in[0] = indata[0];		/* pointers to the signal memories */
  in[1] = indata[1];
  inmax = indata[0] + h_points;	/* pointer to the end of a signal */
  out = outdata;

  while (1) {			/* while oscope sends us samples... */

    /* Read two shorts from stdin (channel 1 & 2), exiting if none to read. */
    if ((i = read(0, buff, sizeof(short))) != sizeof(short))
      exit(i);
    *(in[0]++) = (short)(*buff); /* get channel 1 sample, increment pointer */

    if (two) {			/* skip this read if argv[0] == 0 */
      if ((i = read(0, buff, sizeof(short))) != sizeof(short))
	exit(i);
      *(in[1]++) = (short)(*buff); /* get channel 2 sample, increment pointer */
    }
    out++;			/* increment output pointer */

    /* Do the signal math here. In the simplest case, this would be a
     * simple function of the current input samples.  But for FFT, we
     * must remember an entire screenful before we know the result.
     * So the output will actually be delayed by one screenful.  This
     * is not a problem as long as we output one sample for every two
     * inputs, including during the first screenful.  This first set
     * of data will therefore be invalid but will quickly be replaced
     * by the next valid screenful.
     */

    if (in[0] >= inmax) {	/* each time we have a screenful ... */
      in[0] = indata[0];
      in[1] = indata[1];	/* reset pointers to beginning of memory */
      out = outdata;
      fft(indata[channel], outdata); /* and perform FFT (in fft.c) */
    }

    /* Write one short to stdout as the result of our function. */
    if ((i = write(1, out, sizeof(short))) != sizeof(short))
      exit(i);			/* write a sample out */

  } /* loop back to read another 2 input samples, write 1, ... */

  EndFFT();			/* clean up */
}

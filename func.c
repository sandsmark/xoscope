/*
 * @(#)$Id: func.c,v 1.7 1996/03/02 07:09:22 twitham Exp $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the signal math and memory.
 * To add functions, search for !!! and add to those sections.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "oscope.h"
#include "fft.h"

/* must be shorter than minimum screen width and multiple of 2 */
#define FFTLEN	512

/* !!! The function names, the first three are special */
char *funcnames[] =
{
  "Left  Mix",
  "Right Mix",
  "Memory  ",
  "FFT. 1  ",
  "FFT. 2  ",
  "Sum  1+2",
  "Diff 1-2",
  "Avg. 1,2",
};

/* The total number of functions */
int funccount = sizeof(funcnames) / sizeof(char *);

/* The pointers to the signal memories */
short *mem[26] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL
};

/* the signal color that was recorded into the memory */
int memcolor[26];

/* for the fft function: x position to bin number map, and data buffer */
int xmap[MAXWID];
short fftdata[MAXWID];


/* store the currently selected signal to the given memory register */
void
save(char c)
{
  static int i;

  i = c - 'A';
  if (mem[i] == NULL)
    mem[i] = malloc(MAXWID);
  if (mem[i] != NULL) {
    memcpy(mem[i], ch[scope.select].data, MAXWID);
    memcolor[i] = ch[scope.select].color;
  }
}

/* recall given memory register to the currently selected signal */
void
recall(char c)
{
  static int i;

  i = c - 'a';
  if (mem[i] != NULL) {
    memcpy(ch[scope.select].data, mem[i], MAXWID);
    ch[scope.select].func = 2;
    ch[scope.select].mem = c;
  }
}


/* !!! The functions; they take one arg: the channel # to store results in */

/* The sum of the two channels */
void
sum(int num)
{
  static int i;
  static Signal *a, *b, *c;
  a = &ch[0];
  b = &ch[1];
  c = &ch[num];
  for (i = 0 ; i < h_points ; i++) {
    c->data[i] = a->data[i] + b->data[i];
  }
}

/* The difference of the two channels */
void
diff(int num)
{
  static int i;
  static Signal *a, *b, *c;
  a = &ch[0];
  b = &ch[1];
  c = &ch[num];
  for (i = 0 ; i < h_points ; i++) {
    c->data[i] = a->data[i] - b->data[i];
  }
}

/* The average of the two channels */
void
avg(int num)
{
  static int i;
  static Signal *a, *b, *c;
  a = &ch[0];
  b = &ch[1];
  c = &ch[num];
  for (i = 0 ; i < h_points ; i++) {
    c->data[i] = (a->data[i] + b->data[i]) / 2;
  }
}

/* Fast Fourier Transform of channel sig */
static inline void
fft(int num, int sig)
{
  static int i, bri;
  static long re, im, root, mask, it;
  static short *a, *b;

  a = fftdata;
  b = ch[sig].data;
  for(i = 0 ; i < MAXWID ; i++) {
    *a=((long)(*b)  * 32767) >> 7;
    a++;
    b++;
  }
  RealFFT(fftdata);
  a = ch[num].data;
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

void
fft1(int num)
{
  fft(num, 0);
}

void
fft2(int num)
{
  fft(num, 1);
}

/* !!! Array of the functions, the first three should be NULL */
void (*funcarray[])(int) =
{
  NULL,
  NULL,
  NULL,
  fft1,
  fft2,
  sum,
  diff,
  avg,
};

/* Initialize math, called once by main at startup */
void
init_math()
{
  static int i;

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

/* Perform any math on the software channels, called many times by main loop */
void
do_math()
{
  static int i;

  for (i = 2 ; i < CHANNELS ; i++) {
    if ((scope.select == i || ch[i].show) && (ch[i].func > 2))
      funcarray[ch[i].func](i);
  }
}

/* Perform any math cleanup, called once by cleanup at program exit */
void
cleanup_math()
{
  int i;

  for (i = 0 ; i < 26 ; i++) {
    free(mem[i]);
  }
  EndFFT();
}

/* auto-measure the given signal */
void
measure_data(Signal *sig) {
  static int i, j, prev;
  int first = 0, last = 0, count = 0, max = 0;

  sig->min = 0;
  sig->max = 0;
  prev = 1;
  for (i = 0 ; i < h_points ; i++) {
    j = sig->data[i];
    if (j < sig->min)		/* minimum */
      sig->min = j;
    if (j > sig->max) {		/* maximum */
      sig->max = j;
      max = i;
    }
    if (j > 0 && prev <= 0) {	/* locate and count rising edges */
      if (!first)
	first = i;
      last = i;
      count++;
    }
    prev = j;
  }
  if (sig->func == 3 || sig->func == 4) { /* frequency from peak FFT */
    if ((sig->freq = actual * max / 880) > 0)
      sig->time = 1000000 / sig->freq;
    else
      sig->time = 0;
  } else if (count > 1) {	/* wave: period = length / # periods */
    if ((sig->time = 1000000 * (last - first) / (count - 1) / actual) > 0)
      sig->freq = 1000000 / sig->time;
    else
      sig->freq = 0;
  } else {			/* couldn't measure */
    sig->time = 0;
    sig->freq = 0;
  }
}

/*
 * @(#)$Id: oscope.h,v 1.19 1996/02/22 06:23:21 twitham Exp $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see oscope.c and the file COPYING for more details)
 *
 * This file defines the programs global variables and structures
 *
 */

/* Use -DNOVGAMISC if you don't have libvgamisc from the g3fax kit */
#ifndef NOVGAMISC
#define HAVEVGAMISC
#endif

#include "config.h"

/* global program variables */
extern char *progname;
extern char error[256];
extern int quit_key_pressed;
extern int snd;
extern char buffer[MAXWID * 2];
extern char junk[SAMPLESKIP];
extern int v_points;
extern int h_points;
extern int offset;
extern int actual;
extern int clip;

typedef struct Scope {		/* The oscilloscope */
  int mode;
  int size;
  int dma;
  int run;
  int scale;
  int rate;
  int grat;
  int behind;
  int color;
  int select;
  int trigch;
  int trige;
  short trig;
} Scope;
extern Scope scope;

typedef struct Signal {		/* The signals (channels) */
  short data[MAXWID];
  short old[MAXWID];
  short min;
  short max;
  int time;
  int freq;
  int mult;
  int div;
  int pos;
  int color;
  int show;
  int func;
  char mem;
} Signal;
extern Signal ch[CHANNELS];

/* functions that are called by files other than oscope.c */
int
get_data();

void
handle_key(unsigned char);

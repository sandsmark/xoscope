/*
 * @(#)$Id: oscope.h,v 1.22 1996/04/21 02:28:48 twitham Rel1_1 $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file defines the program's global variables and structures
 *
 */

/* -DNOVGAMISC from the Makefile causes HAVEVGAMISC to be undefined */
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
extern char *filename;

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
void
usage();

int
get_data();

void
handle_key(unsigned char);

void
parse_args(int, char **);

char *
GetString(char *, char *);

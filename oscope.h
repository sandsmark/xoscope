/*
 * @(#)$Id: oscope.h,v 1.30 1997/05/31 17:25:11 twitham Rel $
 *
 * Copyright (C) 1996 - 1997 Tim Witham <twitham@pcocd2.intel.com>
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
extern char version[];
extern char error[256];
extern int quit_key_pressed;
extern int v_points;
extern int h_points;
extern int offset;
extern int clip;
extern char *filename;
extern int snd;

typedef struct Scope {		/* The oscilloscope */
  unsigned int mode;
  unsigned int size;
  unsigned int dma;
  unsigned int run;
  unsigned int scale;
  unsigned int div;
  unsigned int rate;
  unsigned int grat;
  unsigned int behind;
  unsigned int color;
  unsigned int select;
  unsigned int trigch;
  unsigned int trige;
  unsigned int verbose;
  short trig;
} Scope;
extern Scope scope;

typedef struct Signal {		/* The input/memory/math signals */
  short data[MAXWID];
  int rate;
  int color;
} Signal;
extern Signal mem[34];

typedef struct Channel {	/* The display channels */
  Signal *signal;
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
  char command[256];
  int pid;
} Channel;
extern Channel ch[CHANNELS];

/* functions that are called by files other than oscope.c */
void	usage();
int	get_data();
void	handle_key(unsigned char);
void	cleanup();
void	init_scope();
void	init_channels();

/*
 * @(#)$Id: oscope.h,v 1.40 2001/05/06 03:45:16 twitham Rel $
 *
 * Copyright (C) 1996 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file defines the program's global variables and structures
 *
 */

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
extern int in_progress;

typedef struct Scope {		/* The oscilloscope */
  int mode;
  int size;
  int dma;
  int run;
  int scale;
  int div;
  int rate;
  int grat;
  int behind;
  int color;
  int select;
  int trigch;
  int trige;
  int verbose;
  int trig;
  int curs;
  int cursa;
  int cursb;
} Scope;
extern Scope scope;

typedef struct Signal {		/* The input/memory/math signals */
  short data[MAXWID];
  int rate;
  int num;
  int volts;
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
  int bits;
} Channel;
extern Channel ch[CHANNELS];

/* functions that are called by files other than oscope.c */
void	usage();
int	get_data();
void	handle_key(unsigned char);
void	cleanup();
void	init_scope();
void	init_channels();
int	samples();
void	loadfile();
void	savefile();
void	startcommand();
void	resetsoundcard();

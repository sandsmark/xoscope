/*
 * @(#)$Id: oscope.h,v 1.16 1996/02/03 21:08:37 twitham Exp $
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see scope.c and the file COPYING for more details)
 *
 * This file simply sets the program's default options, see oscope.c
 * Tweaking these parameters will automagically fix the usage message
 *
 */

/* Use -DNOVGAMISC if you don't have libvgamisc from the g3fax kit */
#ifndef NOVGAMISC
#define HAVEVGAMISC
#endif

/* program defaults for the command-line options */
#define DEF_1 2
#define DEF_R 44000
#define DEF_S 1
#define DEF_T 128
#define DEF_C 4
#define DEF_M 0
#define DEF_D 4
#define DEF_F ""
#define DEF_FX "8x16"
#define DEF_P 2
#define DEF_G 1
#define DEF_B 0
#define DEF_V 0

/* maximum screen width, also the maximum number of samples in memory at once */
#define MAXWID	1600

/* The first few samples after a reset seem invalid.  If you see
   strange "glitches" at the left edge of the screen, increase this */
#define SAMPLESKIP	32

/* max number of channels */
#define CHANNELS 8

/* initial colors of the channels (see color definition in display.c) */
#define CHANNELCOLOR	{2,3,5,6,14,1,4,15}

/* text foreground color */
#define TEXT_FG		color[2]

/* key foreground color */
#define KEY_FG		color[5]

/* text background color */
#define TEXT_BG		color[0]

/* minimum number of milliseconds between refresh on X11 version */
#define MSECREFRESH	10

/* global program variables */
extern int verbose;
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
  short trig;
} Scope;
extern Scope scope;

typedef struct Signal {		/* The signals (channels) */
  short data[MAXWID];
  short old[MAXWID];
  short min;
  short max;
  int time;
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
get_data();

void
measure_data(Signal *);

void
handle_key(unsigned char);

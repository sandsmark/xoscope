/*
 * @(#)$Id: oscope.h,v 1.6 1996/01/28 08:12:24 twitham Exp $
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see scope.c and the file COPYING for more details)
 *
 * This file simply sets the program's default options, see scope.c
 * Tweaking these parameters will automagically fix the usage message
 *
 */

/* Use -DNOVGAMISC if you don't have libvgamisc from the g3fax kit */
#ifndef NOVGAMISC
#define HAVEVGAMISC
#endif

/* program defaults for the command-line options */
#define DEF_12 1		/* -1/-2: 1=single channel, 2=dual channels */
#define DEF_R 44000		/* -r: 5000 - 44100 works for SoundBlaster */
#define DEF_S 1			/* -s: 1, 2, 4, 8, 16 */
#define DEF_T 128		/* -t: 0 - 255 or -1 = disabled, 128 = center */
#define DEF_C 4			/* -c: channel 1 green like a real scope */
#define DEF_M 0			/* -m: predefined 640x480 */
#define DEF_D 4			/* -d: 1, 2, 4 */
#define DEF_F ""		/* empty-string is the default8x16 */
#define DEF_FX "8x16"		/* for X scope, an equivalent 8x16 */
#define DEF_PL 0		/* -p/-l: 0=line,                 1=point */
#define DEF_G 0			/* -g:    0=graticule off, 1=graticule on */
#define DEF_B 0			/* -b:    0=graticule in front, 1=in back */
#define DEF_V 0			/* -v:    0=verbose log off, 1=verbose on */

/* maximum screen width, also the maximum number of samples in memory at once */
#define MAXWID	1600

/* The first few samples after a reset seem invalid.  If you see
   strange "glitches" at the left edge of the screen, increase this */
#define SAMPLESKIP	32

/* max number of channels */
#define CHANNELS 4

/* global program variables */
extern int channels;
extern int sampling;
extern int trigger;
extern int colour;
extern int mode;
extern int dma;
extern int point_mode;
extern int graticule;
extern int behind;
extern int verbose;
extern char *progname;
extern char error[256];
extern char *def[];
extern int quit_key_pressed;
extern int running;
extern int snd;
extern unsigned char buffer[MAXWID * 2];
extern unsigned char junk[SAMPLESKIP];
extern int v_points;
extern int h_points;
extern int offset;
extern int actual;


typedef struct Scope {		/* The oscilloscope */
  int scale;
  int pos;
  int rate;
  unsigned char trigpos;
  unsigned char trigtype;
  unsigned char graticule;
  unsigned char run;
  unsigned char gcolor;
  unsigned char tcolor;
} Scope;
extern Scope scope;

typedef struct Signal {		/* Signals (channels) */
  unsigned char data[MAXWID];
  unsigned char old[MAXWID];
  int scale;
  int pos;
  int mode;
  int color;
} Signal;
extern Signal ch[CHANNELS];

/* functions that are called by files other than scope.c */
void
get_data();

void
handle_key(unsigned char c);

/*
 * @(#)$Id: config.h,v 1.9 1997/05/31 21:23:49 twitham Exp $
 *
 * Copyright (C) 1996 - 1997 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file simply sets the program's configurable options.
 * To reconfigure, simply edit them here, make clean, and make.
 * Original shipped values are in (parentheses)
 *
 */

/* program defaults for the command-line options (original values) */
#define DEF_A	1		/* 1-8 (1) */
#define DEF_R	44000		/* 8800,22000,44000 (44000) */
#define DEF_S	10		/* 1,2,5,10,20,50,100,200,500,1000 (10) */
#define DEF_T	"0:1:x"		/* -128-127:0,1,2:x,y ("0:1:x") */
#define DEF_C	4		/* 0-16 (4) */
#define DEF_M	0		/* 0,1,2 (0) */
#define DEF_D	4		/* 1,2,4 (4) */
#define DEF_F	""		/* console font, "" = default ("") */
#define DEF_FX	"8x16"		/* X11 font ("8x16") */
#define DEF_P	2		/* 0,1,2 (2) */
#define DEF_G	2		/* 0,1,2 (2) */
#define DEF_B	0		/* 0,1 (0) */
#define DEF_V	0		/* 0,1 (0) */
#define DEF_X	0		/* 0,1 (0) don't use sound card? */
#define DEF_Z	0		/* 0,1 (0) don't search for ProbeScope? */

/* maximum screen width, also max number of samples in memory at once (1600) */
#define MAXWID		1600

/* The first few samples after a reset seem invalid.  If you see
   strange "glitches" at the left edge of the screen, increase this (32) */
#define SAMPLESKIP	32

/* max number of channels, up to 8 (8) */
#define CHANNELS	8

/* colors of the channels; see colors in gr_sx.c ({2,3,5,6,14,1,4,15}) */
#define CHANNELCOLOR	{2,3,14,6,5,1,4,15}

/* text foreground color (color[15]) */
#define TEXT_FG		color[15]

/* key foreground color (color[5]) */
#define KEY_FG		color[5]

/* text background color (color[0]) */
#define TEXT_BG		color[0]

/* minimum number of milliseconds between refresh on X11 version (20) */
#define MSECREFRESH	20

/* bourne shell command for X11 Help ("man xoscope 2>&1") */
#define HELPCOMMAND	"man xoscope 2>&1"

/* default file name ("oscope.dat") */
#define FILENAME	"oscope.dat"

/* default external command pipe to run ("operl '$x + $y'") */
#define COMMAND		"operl '$x + $y'"

/* FFT length, shorter than minimum screen width and multiple of 2 (512) */
#define FFTLEN	512

/* extra places to look for ProbeScope (set to {""} to not search) */
/* ({"", "/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2", "/dev/ttyS3"}) */
#define PSDEVS {"", "/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2", "/dev/ttyS3"}

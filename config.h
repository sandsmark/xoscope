/*
 * @(#)$Id: config.h,v 1.3 1996/03/10 01:50:18 twitham Exp $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file simply sets the program's configurable options.
 * To reconfigure, simply edit them here, make clean, and make.
 * Original shipped values are in (parentheses)
 *
 */

/* program defaults for the command-line options */
#define DEF_A	1		/* 1-8 (1) */
#define DEF_R	44000		/* 8800,22000,44000 (44000) */
#define DEF_S	1		/* 1,2,5,10,20,50,100,200 (1) */
#define DEF_T	"0:1:1"		/* -128-127:0,1,2:1,2 ("0:1:1") */
#define DEF_C	4		/* 0-16 (4) */
#define DEF_M	0		/* 0,1,2 (0) */
#define DEF_D	4		/* 1,2,4 (4) */
#define DEF_F	""		/* console font, "" = default ("") */
#define DEF_FX	"8x16"		/* X11 font ("8x16") */
#define DEF_P	2		/* 0,1,2 (2) */
#define DEF_G	2		/* 0,1,2 (2) */
#define DEF_B	0		/* 0,1 (0) */

/* maximum screen width, also max number of samples in memory at once (1600) */
#define MAXWID		1600

/* The first few samples after a reset seem invalid.  If you see
   strange "glitches" at the left edge of the screen, increase this (32) */
#define SAMPLESKIP	32

/* max number of channels, up to 8 (8) */
#define CHANNELS	8

/* initial colors of the channels; see colors in x11.c ({2,3,5,6,14,1,4,15}) */
#define CHANNELCOLOR	{2,3,5,6,14,1,4,15}

/* text foreground color (color[2]) */
#define TEXT_FG		color[2]

/* key foreground color (color[5]) */
#define KEY_FG		color[5]

/* text background color (color[0]) */
#define TEXT_BG		color[0]

/* minimum number of milliseconds between refresh on X11 version (10) */
#define MSECREFRESH	10

/* bourne shell command for X11 Help ("man xoscope 2>&1") */
#define HELPCOMMAND	"man xoscope 2>&1"

/* default file name */
#define FILENAME	"oscope.osp"

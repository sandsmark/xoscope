/*
 * @(#)$Id: config.h,v 1.1 1996/02/22 06:19:18 twitham Exp $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see oscope.c and the file COPYING for more details)
 *
 * This file simply sets the program's configurable options.
 * Tweaking these parameters will automagically fix the usage message
 *
 */

/* program defaults for the command-line options */
#define DEF_1 2
#define DEF_R 44000
#define DEF_S 1
#define DEF_T 128
#define DEF_C 4
#define DEF_M 0
#define DEF_D 4
#define DEF_F ""		/* console font, "" = default */
#define DEF_FX "8x16"		/* X11 font */
#define DEF_P 2
#define DEF_G 2
#define DEF_B 0

/* maximum screen width, also the maximum number of samples in memory at once */
#define MAXWID	1600

/* The first few samples after a reset seem invalid.  If you see
   strange "glitches" at the left edge of the screen, increase this */
#define SAMPLESKIP	32

/* max number of channels, up to 8 */
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

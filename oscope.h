/* This file simply sets the program's default options, see scope.c */

/* Tweaking these parameters will automagically fix the usage message */

/* @(#)$Id: oscope.h,v 1.3 1996/01/02 06:26:02 twitham Exp $ */

/* maximum screen width, also the maximum number of samples in memory at once */
#define MAXWID	1280

/* program defaults for the command-line options */
#define DEF_12 1		/* -1/-2: 1=single channel, 2=dual channels */
#define DEF_R 8000		/* -r: 5000 - 44100 works for SoundBlaster */
#define DEF_S 4			/* -s: 1, 2, 4, 8, 16 */
#define DEF_T -1		/* -t: 0 - 255 or -1 = disabled, 128 = center */
#define DEF_C 2			/* -c: 0 - ?, depends on graphics mode */
#define DEF_M G640x480x16	/* -m: SVGA graphics mode as-in vga.h */
#define DEF_D 4			/* -d: 1, 2, 4 */
#define DEF_PL 0		/* -p/-l: 0=line,                 1=point */
#define DEF_G 0			/* -g:    0=graticule off, 1=graticule on */
#define DEF_B 0			/* -b:    0=graticule in front, 1=in back */
#define DEF_V 0			/* -v:    0=verbose log off, 1=verbose on */

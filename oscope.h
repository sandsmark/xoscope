/* This file simply sets the program's default options, see scope.c */

/* Tweaking these parameters will automagically fix the usage message */

/* @(#)$Id: oscope.h,v 1.4 1996/01/20 08:31:03 twitham Exp $ */

/* program defaults for the command-line options */
#define DEF_12 1		/* -1/-2: 1=single channel, 2=dual channels */
#define DEF_R 8000		/* -r: 5000 - 44100 works for SoundBlaster */
#define DEF_S 4			/* -s: 1, 2, 4, 8, 16 */
#define DEF_T -1		/* -t: 0 - 255 or -1 = disabled, 128 = center */
#define DEF_C 2			/* -c: 0 - ?, depends on graphics mode */
#define DEF_M G640x480x16	/* -m: SVGA graphics mode as-in vga.h */
#define DEF_D 4			/* -d: 1, 2, 4 */
#define DEF_F ""		/* empty-string is the default8x16 */
#define DEF_PL 0		/* -p/-l: 0=line,                 1=point */
#define DEF_G 0			/* -g:    0=graticule off, 1=graticule on */
#define DEF_B 0			/* -b:    0=graticule in front, 1=in back */
#define DEF_V 0			/* -v:    0=verbose log off, 1=verbose on */

/* maximum screen width, also the maximum number of samples in memory at once */
#define MAXWID	1280

/* get g3fax's libvgamisc and define this to get on-screen text */
#define HAVEVGAMISC	1

/* 16-color color definitions */
#define BLACK		0	/* dark colors */
#define BLUE		1
#define GREEN		2
#define CYAN		3
#define RED		4
#define MAGENTA		5
#define BROWN		6
#define LIGHTGRAY	7
#define DARKGRAY	8	/* light colors */
#define LIGHTBLUE	9
#define LIGHTGREEN	10
#define LIGHTCYAN	11
#define LIGHTRED	12
#define LIGHTMAGENTA	13
#define YELLOW		14
#define WHITE		15

/* text foreground color */
#define TEXT_FG		GREEN

/* text background color */
#define TEXT_BG		BLACK

/* how to convert text column (0-79) and row(-6-1,0-5) to graphics position */
#define COL(x)	(x * 8)
#define ROW(y)	(offset + y * 18 + 258 * (y >= 0))

/* The first few samples after a reset seem invalid.  If you see
   strange "glitches" at the left edge of the screen, increase this */
#define SAMPLESKIP	32

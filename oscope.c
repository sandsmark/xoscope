/*
 *                      /-----------------------\
 *                      | Software Oscilloscope |
 *                      \-----------------------/
 *
 * scope --- Use /dev/dsp (a sound card) as an oscilloscope under Linux
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * @(#)$Id: oscope.c,v 1.21 1996/01/19 07:12:34 twitham Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ********************************************************************
 *
 * See the man page for a description of what this program does and
 * what the requirements to run it are.  If you would prefer different
 * defaults, simply tweak scope.h and re-make.
 *
 * scope 0.1 (original by Jeff Tranter) was developed using:
 * - Linux kernel 1.0
 * - gcc 2.4.5
 * - svgalib version 1.05
 * - SoundBlaster Pro
 * - Trident VGA card
 * - 80386DX40 CPU with 8MB RAM
 *
 * scope 1.0 (enhancements by Tim Witham) was developed using:
 * - Linux kernel 1.2.10
 * - gcc 2.6.3
 * - svgalib 1.22
 * - SoundBlaster 16
 * - ATI Win Turbo 2MB VRAM
 * - 100 MHz Intel Pentium(R) Processor, 32MB RAM
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <vga.h>
#include <sys/soundcard.h>
#include "fontutils.h"
#include "scope.h"		/* program defaults */


/* a macro to always redraw the frame when we clear the screen */
#define CLEAR	vga_clear(); draw_frame()

/* global program defaults, defined in scope.h (see also) */
int channels = DEF_12;
int sampling = DEF_R;
int scale = DEF_S;
int trigger = DEF_T;
int colour = DEF_C;
int mode = DEF_M;
int dma = DEF_D;
char fontname[64] = DEF_F;
int point_mode = DEF_PL;
int graticule = DEF_G;
int behind = DEF_G;
int verbose = DEF_V;


/* extra global variables */
char *progname;			/* the program's name, autoset via argv[0] */
char error[256];		/* buffer for "one-line" error messages */
char *def[] = {
  "",				/* used by -1/-2/-p/-l in usage message */
  "(default)",
  "on",				/* used by -g/-v in usage message */
  "off",
  "graticule",			/* used by -b in usage message */
  "signal"
};
int quit_key_pressed;		/* set by handle_key() */
int snd;			/* file descriptor for sound device */
unsigned char buffer[MAXWID * 2]; /* buffer for sound data */
unsigned char old[MAXWID * 2];	/* previous buffer for sound data */
int v_points;			/* points in vertical axis */
int h_points;			/* points in horizontal axis */
int offset;			/* vertical offset */
int actual;			/* actual sampling rate */
font_t font;			/* pointer to the font structure */

/* display command usage on standard error and exit */
void
usage()
{
  fprintf(stderr, "usage: %s "
	  "[-1|-2] [-r<rate>] [-s<scale>] [-t<trigger>] [-c<colour>]
             [-m<mode>] [-d<dma divisor>] [-f font] [-p|-l] [-g] [-b] [-v]

Options          Runtime Keys   Description (defaults)
-1               1      toggle  Single channel, opposite of -2  %s
-2               2      toggle  Dual   channel, opposite of -1  %s
-r <rate>        R +10%% r -10%%  sampling Rate in Hz             (default=%d)
-s <scale>       S *2   s /2    time Scale or zoom: 1,2,4,8,16  (default=%d)
-t <trigger>     T +10  t -10   Trigger level:0-255,-1=disabled (default=%d)
-c <colour>      C +1   c -1    Channel 1 trace Colour          (default=%d)
-m <mode>        M +1   m -1    SVGA graphics Mode USE CAUTION  (default=%d)
-d <dma divisor> D *2   d /2    DMA buffer size divisor: 1,2,4  (default=%d)
-f <font name>                  The font name as-in /usr/lib/kbd/consolefonts/
-p               P  p   toggle  Point mode, opposite of -l      %s
-l               L  l   toggle  Line  mode, opposite of -p      %s
-g               G  g   toggle  turn %s Graticule of 5 msec major divisions
-b               B  b   toggle  %s Behind instead of in front of %s
-v               V  v   toggle  turn %s Verbose keypress option log to stdout
                 <space>        pause the display until another key is pressed
                 Q q            Quit %s
",
	  progname,
	  def[!(channels - 1)], def[channels - 1],
	  sampling, scale, trigger,
	  colour, mode, dma,
	  def[point_mode], def[!point_mode],
	  def[graticule + 2], def[behind + 4], def[!behind + 4],
	  def[verbose + 2],
	  progname);
  exit(1);
}

/* if verbose mode, show current parameter settings on standard out */
static inline void
show_info(unsigned char c) {
  if (verbose) {
    sprintf(error, "%1c %5dHz:  %2s  -r %5d  -s %2d  -t %3d  -c %2d  -m %2d  "
	    "-d %1d  %2s  %2s  %2s%s",
	    c, actual,
	    channels == 1 ? "-1" : "-2",
	    sampling, scale, trigger, colour, mode, dma,
	    point_mode ? "-p" : "-l",

	    /* reverse logic if these booleans are on by default in scope.h */
#if DEF_G
	    graticule ? "" : "-g",
#else
	    graticule ? "-g" : "",
#endif

#if DEF_B
	    behind ? "" : "-b",
#else
	    behind ? "-b" : "",
#endif

#if DEF_V
	    verbose ? "" : "  -v"
#else
	    verbose ? "  -v" : ""
#endif
	    );
    vga_write(error,  COL(0), ROW(5),  &font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
    printf("%s\n", error);
  }
}

/* handle command line options */
void
parse_args(int argc, char **argv)
{
  const char     *flags = "12r:s:t:c:m:d:f:plgbv";
  int             c;

  while ((c = getopt(argc, argv, flags)) != EOF) {
    switch (c) {
    case '1':			/* single channel (mono) */
      channels = 1;
      break;
    case '2':			/* dual channels (stereo) */
      channels = 2;
      break;
    case 'r':			/* sample rate */
      sampling = strtol(optarg, NULL, 0);
      break;
    case 's':			/* scale (zoom) */
      scale = strtol(optarg, NULL, 0);
      scale &= 0x000f;
      if (scale < 1)
	scale = 1;
      break;
    case 't':			/* trigger level */
      trigger = strtol(optarg, NULL, 0);
      trigger &= 0x00ff;
      break;
    case 'c':			/* channel 1 trace colour */
      colour = strtol(optarg, NULL, 0);
      break;
    case 'm':			/* video mode */
      mode = strtol(optarg, NULL, 0);
      break;
    case 'd':			/* dma divisor */
      dma = strtol(optarg, NULL, 0);
      dma &= 0x0007;
      break;
    case 'f':			/* font name */
      strcpy(fontname, optarg);
      break;
    case 'p':			/* point mode */
      point_mode = 1;
      break;
    case 'l':			/* line mode */
      point_mode = 0;
      break;
    case 'g':			/* graticule on/off */
      graticule = !graticule;
      break;
    case 'b':			/* behind/front */
      behind = !behind;
      break;
    case 'v':			/* verbose on/off */
      verbose = !verbose;
      break;
    case ':':			/* unknown option */
    case '?':
      usage();
      break;
    }
  }
}

/* cleanup: restore text mode and close sound device */
void
cleanup()
{
  vga_setmode(TEXT);		/* restore text screen */
  close(snd);			/* close sound device */
}

/* abort and show system error if given ioctl status is bad */
static inline void
check_status(int status, int line)
{
  if (status < 0) {
    sprintf(error, "%s: error from sound device ioctl at line %d",
	    progname, line);
    perror(error);
    cleanup();
    exit(1);
  }
}

/* draw a frame, done only when screen is cleared by CLEAR macro */
static inline void
draw_frame()
{
  vga_setcolor(colour+2);
  vga_drawline(0, offset-1, h_points-1, offset-1);
  vga_drawline(0, offset+256, h_points-1, offset+256);
  vga_drawline(0, offset-1, 0, offset+256);
  vga_drawline(h_points-1, offset, h_points-1, offset+256);
}

/* if graticule mode, draw graticule, done in main loop */
static inline void
draw_graticule()
{
  static int i, j;

  if (graticule) {

    /* horizontial line at mid-scale */
    vga_setcolor(colour+2);
    vga_drawline(0, offset+128, h_points-1, offset+128);

    /* 1 pixel dots at 0.1 msec intervals */
    for (i = 0 ; (j = i / 10000) < h_points ; i += (actual * scale)) {
      vga_drawpixel(j, offset);
      vga_drawline(j, offset+127, j, offset+129);
      vga_drawpixel(j, offset + 255);

      /* 5 pixel marks at 0.5 msec intervals */
      if ((j = i / 2000) < h_points) {
	vga_drawline(j, offset, j, offset+4);
	vga_drawline(j, offset+123, j, offset+133);
	vga_drawline(j, offset+251, j, offset+255);
      }
      /* 20 pixel marks at 1 msec intervals */
      if ((j = i / 1000) < h_points) {
	vga_drawline(j, offset, j, offset+20);
	vga_drawline(j, offset+107, j, offset+149);
	vga_drawline(j, offset+235, j, offset+255);
      }
      /* vertical major divs at 5 msec intervals */
      if ((j = i / 200) < h_points)
	vga_drawline(j, offset, j, offset+255);
    }
    /* a tick mark where the trigger level is */
    if (trigger > -1)
      vga_drawline(0, offset+trigger, 5, offset+trigger);

  }

}

/* initialize screen data to half scale */
void
init_data()
{
  int i;

  for (i = 0 ; i < MAXWID * 2 ; i++) {
    buffer[i] = 128;
    old[i] = 128;
  }
}

/* [re]initialize /dev/dsp */
void
init_sound_card(int firsttime)
{
  int parm;

  if (!firsttime)		/* resetting parameters? */
    close(snd);
  snd = open("/dev/dsp", O_RDONLY);
  if (snd < 0) {		/* open DSP device for read */
    sprintf(error, "%s: cannot open /dev/dsp", progname);
    perror(error);
    cleanup();
    exit(1);
  }

  parm = channels;		/* set mono/stereo */
  check_status(ioctl(snd, SOUND_PCM_WRITE_CHANNELS, &parm), __LINE__);

  parm = 8;			/* set 8-bit samples */
  check_status(ioctl(snd, SOUND_PCM_WRITE_BITS, &parm), __LINE__);

  /* set DMA buffer size */
  check_status(ioctl(snd, SOUND_PCM_SUBDIVIDE, &dma), __LINE__);

  /* set sampling rate */
  check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &sampling), __LINE__);
  check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
}

/* [re]initialize graphics screen */
void
init_screen(int firsttime)
{
  if (firsttime) {
/*     vga_disabledriverreport(); */
    vga_init();
    vga_initfont (fontname, &font, 1, 1);
  }
  vga_setmode(mode);
  v_points = vga_getydim();
  h_points = vga_getxdim();
  offset = v_points / 2 - 127;
  CLEAR;
  draw_graticule();
}

/* handle single key commands */
static inline unsigned char
handle_key()
{
  unsigned char c;

  switch (c = vga_getkey()) {
  case 0:
  case -1:			/* no key pressed */
    break;
  case '1':
  case '2':			/* single or dual channel mode */
    channels = channels == 1 ? 2 : 1;
    init_sound_card(0);
    CLEAR;
    break;
  case 'R':
    sampling += sampling / 10;	/* 10% sample rate increase */
    check_status(ioctl(snd, SOUND_PCM_SYNC, 0), __LINE__);
    check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &sampling), __LINE__);
    check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
    CLEAR;
    break;
  case 'r':
    sampling -= sampling / 10;	/* 10% sample rate decrease */
    check_status(ioctl(snd, SOUND_PCM_SYNC, 0), __LINE__);
    check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &sampling), __LINE__);
    check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
    CLEAR;
    break;
  case 'S':
    if (scale <= 8) {		/* double the scale (zoom) */
      scale <<= 1;
      CLEAR;
    }
    break;
  case 's':
    if (scale > 1) {		/* half the scale */
      scale >>= 1;
      CLEAR;
    }
    break;
  case 'T':
    if (trigger < 0)		/* enable the trigger at half scale */
      trigger = 118;
    trigger += 10;		/* increase trigger */
    if (trigger > 255)
      trigger = -1;		/* disable trigger when it leaves the scale */
    CLEAR;
    break;
  case 't':
    if (trigger < 0)		/* enable the trigger at half scale */
      trigger = 138;
    trigger -= 10;		/* decrease trigger */
    CLEAR;
    break;
  case 'C':
    colour++;			/* increase color */
    CLEAR;
    break;
  case 'c':
    if (colour > 0) {		/* decrease color */
      colour--;
      CLEAR;
    }
    break;
  case 'M':
    mode++;			/* increase video mode */
    init_screen(0);
    CLEAR;
    break;
  case 'm':
    if (mode > 0) {		/* decrease video mode */
      mode--;
      init_screen(0);
      CLEAR;
    }
    break;
  case 'D':
    if (dma < 3) {		/* double dma */
      dma <<= 1;
      init_sound_card(0);
    }
    break;
  case 'd':
    if (dma > 1) {		/* half dma */
      dma >>= 1;
      init_sound_card(0);
    }
    break;
  case 'L':
  case 'l':
  case 'P':
  case 'p':
    point_mode = !point_mode;	/* line/point mode */
    CLEAR;
    break;
  case 'G':
  case 'g':
    graticule = !graticule;	/* graticule on/off */
    CLEAR;
    break;
  case 'B':
  case 'b':
    behind = !behind;		/* graticule behind/in front of signal */
    break;
  case 'V':
  case 'v':
    verbose = !verbose;		/* verbose log on/off */
    break;
  case ' ':
    vga_write("STOP",  COL(0), ROW(0),  &font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
    while (vga_getkey() <= 0)	/* pause until key pressed */
      ;
    vga_write("RUN ",  COL(0), ROW(0),  &font, TEXT_FG, TEXT_BG, ALIGN_LEFT);
    break;
  case 'q':
  case 'Q':			/* quit */
    quit_key_pressed = 1;
    break;
  }

  if (c > 0)
    show_info(c);		/* show keypress and result on stdout */

  return(c);
}

/* get data from sound card, calling handle_key internally */
static inline void
get_data()
{
  static unsigned char datum[2], prev;

  if (trigger > -1) {		/* trigger enabled */
    if (trigger > 128) {
      datum[0] = 255;		/* positive trigger, look for rising edge */
      do {
	prev = datum[0];	/* remember prev. channel 1, read channel(s) */
	read(snd, datum , channels);
      } while ((handle_key() <= 0)
	       && ((datum[0] < trigger) || (prev > trigger)));
    } else {
      datum[0] = 0;		/* negative trigger, look for falling edge */
      do {
	prev = datum[0];	/* remember prev. channel 1, read channel(s) */
	read(snd, datum, channels);
      } while ((handle_key() <= 0)
	       && ((datum[0] > trigger) || (prev < trigger)));
    }
  } else {			/* not triggering */
    handle_key();
  }
  /* now get the real data */
  read(snd, buffer + channels, (h_points * channels / scale - 2));
  check_status(ioctl(snd, SNDCTL_DSP_RESET), __LINE__);
}

/* graph the data */
static inline void
graph_data()
{
  static int i, j, k;

  if (point_mode) {
    for (j = 0 ; j < channels ; j++) {
      for (i = 1 ; i < (h_points / scale - 1) ; i++) {
	k = i * channels + j;	/* calc this offset once for efficiency */
	vga_setcolor(0);	/* erase previous dot */
	vga_drawpixel(i * scale,
		      old[k] + offset);
	vga_setcolor(colour + j); /* draw dot */
	vga_drawpixel(i * scale,
		      buffer[k] + offset);
	old[k] = buffer[k];
      }
    }
  } else {			/* line mode, a little slower */
    for (j = 0 ; j < channels ; j++) {
      for (i = 1 ; i < (h_points / scale - 2) ; i++) {
	k = i * channels + j;	/* calc this offset once for efficiency */
	vga_setcolor(0);	/* erase previous line */
	vga_drawline(i * scale,
		     old[k] + offset,
		     i * scale + scale,
		     old[k + channels] + offset);
	vga_setcolor(colour + j); /* draw line */
	vga_drawline(i * scale,
		     buffer[k] + offset,
		     i * scale + scale,
		     buffer[k + channels] + offset);
	old[k] = buffer[k];
      }
      old[i * channels + j] = buffer[i * channels + j];
    }
  }
}

/* main program */
int
main(int argc, char **argv)
{
  progname = strrchr(argv[0], '/');
  if (progname == NULL)		/* who are we? */
    progname = argv[0];
  else
    progname++;

  parse_args(argc, argv);	/* what do you want? */
  init_data();
  init_sound_card(1);		/* get ready */
  init_screen(1);
  show_info(' ');

  while (!quit_key_pressed) {
    get_data();			/* keys are now handled in get_data */
    if (behind) {
      draw_graticule();		/* plot data on top of graticule */
      graph_data();
    } else {
      graph_data();		/* plot graticule on top of data */
      draw_graticule();
    }
  }

  cleanup();			/* close sound, back to text mode */
  exit(0);
}



/*
 *                      /-----------------------\
 *                      | Software Oscilloscope |
 *                      \-----------------------/
 *
 *
 * scope --- Use /dev/dsp (a sound card) as an oscilloscope under Linux
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * @(#)$Id: oscope.c,v 1.16 1996/01/02 00:31:54 twitham Exp $
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
 * See the man page for a description of what this program does and what
 * the requirements to run it are.  If you don't like the defaults,
 * tweak scope.h and re-make.
 *
 * scope 0.1 (Jeff Tranter) was developed using:
 * - Linux kernel 1.0
 * - gcc 2.4.5
 * - svgalib version 1.05
 * - SoundBlaster Pro
 * - Trident VGA card
 * - 80386DX40 CPU with 8MB RAM
 *
 * scope 1.1+ (Tim Witham) was developed using:
 * - Linux kernel 1.2.10
 * - gcc 2.6.3
 * - svgalib 1.22
 * - SoundBlaster Pro
 * - ATI Win Turbo 2MB VRAM
 * 100 MHz Intel Pentium(R) Processor, 32MB RAM
 *
 * So if your computer can't keep up with the latest,
 * you may want to try 0.1
 *
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
#include "scope.h"

/* always redraw the frame when we clear the screen */
#define CLEAR	vga_clear(); draw_frame()

/* program defaults, see scope.h for explanation */
int sampling = DEF_R;
int scale = DEF_S;
int trigger = DEF_T;
int colour = DEF_C;
int mode = DEF_M;
int dma = DEF_D;
int point_mode = DEF_P;
int graticule = DEF_G;
int behind = DEF_G;
int verbose = DEF_V;


/* extra global variables */
char *progname;			/* the program's name, autoset via argv[0] */
char error[256];		/* buffer for "one-line" error messages */
char *def[] = {		
  "on",				/* used by -g/-v in usage message */
  "off",
  "",				/* used by -p/-l in usage message */
  ", default",
  "graticule",			/* used by -b in usage message */
  "signal"
};
int quit_key_pressed;		/* set by handle_key() */
int snd;			/* file descriptor for sound device */
unsigned char buffer[MAXWID];	/* buffer for sound data */
unsigned char old[MAXWID];	/* previous buffer for sound data */
int v_points;			/* points in vertical axis */
int h_points;			/* points in horizontal axis */
int offset;			/* vertical offset */
int actual;			/* actual sampling rate */

/* display command usage on standard error and exit */
void
usage()
{
  fprintf(stderr, "usage: %s [-r<rate>] [-s<scale>] [-t<trigger>] [-c<colour>]
             [-m<mode>] [-d<dma divisor>] [-p|-l] [-g] [-b] [-v]

Options          Runtime Increase Decrease or Toggle Keys
-r <rate>        R +10%% r -10%%  sampling Rate in Hz             (default=%d)
-s <scale>       S *2   s /2    time Scale or zoom factor (1-32, default=%d)
-t <trigger>     T +10  t -10   Trigger level (0-255,-1=disabled,default=%d)
-c <colour>      C +1   c -1    trace Colour                    (default=%d)
-m <mode>        M +1   m -1    SVGA graphics Mode (USE CAUTION, default=%d)
-d <dma divisor> D *2   d /2    DMA buffer size divisor  (1,2,4, default=%d)
-p               P  p   toggle  Point mode (faster, opposite of -l%s)
-l               L  l   toggle  Line  mode (slower, opposite of -p%s)
-g               G  g   toggle  turn %s Graticule (5 msec major divisions)
-b               B  b   toggle  %s Behind instead of in front of %s
-v               V  v   toggle  turn %s Verbose keypress option log to stdout
                 <space>        pause the display until another key is pressed
                 Q q            Quit %s
",
	  progname,
	  sampling, scale, trigger,
	  colour, mode, dma,
	  def[point_mode + 2], def[!point_mode + 2],
	  def[graticule], def[behind + 4], def[!behind + 4],
	  def[verbose],
	  progname);
  exit(1);
}

/* if verbose mode, show current parameter settings on standard out */
static inline void
show_info(unsigned char c) {
  if (verbose) {
    printf("%1c %5dHz:  -r %5d  -s %2d  -t %3d  -c %2d  -m %2d  -d %1d  "
	   "%2s  %2s  %2s%s\n",
	   c, actual, sampling, scale, trigger, colour, mode, dma,

	   point_mode ? "-p" : "-l",
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
  }
}

/* handle command line options */
void
parse_args(int argc, char **argv)
{
  const char     *flags = "r:m:c:d:t:s:plgbv";
  int             c;

  while ((c = getopt(argc, argv, flags)) != EOF) {
    switch (c) {
    case 'r':
      sampling = strtol(optarg, NULL, 0);
      break;
    case 'm':
      mode = strtol(optarg, NULL, 0);
      break;
    case 'c':
      colour = strtol(optarg, NULL, 0);
      break;
    case 'd':
      dma = strtol(optarg, NULL, 0);
      break;
    case 't':
      trigger = strtol(optarg, NULL, 0);
      break;
    case 's':
      scale = strtol(optarg, NULL, 0);
      if (scale < 1)
	scale = 1;
      if (scale > 32)
	scale = 32;
      break;
    case 'p':
      point_mode = 1;
      break;
    case 'l':
      point_mode = 0;
      break;
    case 'g':
      graticule = !graticule;
      break;
    case 'b':
      behind = !behind;
      break;
    case 'v':
      verbose = !verbose;
      break;
    case ':':
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

/* draw a frame */
static inline void
draw_frame()
{
  vga_setcolor(colour+1);
  vga_drawline(0, offset-1, h_points-1, offset-1);
  vga_drawline(0, offset+256, h_points-1, offset+256);
  vga_drawline(0, offset-1, 0, offset+256);
  vga_drawline(h_points-1, offset, h_points-1, offset+256);

}

/* if graticule mode, draw graticule */
static inline void
draw_graticule()
{
  static int i, j;

  if (graticule) {
				/* horizontial line at mid-scale */
    vga_setcolor(colour+1);
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
    if (trigger != -1)
      vga_drawline(0, offset+trigger, 5, offset+trigger);

  }

}

/* initialize screen data to zero level */
void
init_data()
{
  int i;

  for (i = 0 ; i < 1024 ; i++) {
    buffer[i] = 128;
    old[i] = 128;
  }
}

/* initialize /dev/dsp */
void
init_sound_card(int firsttime)
{
  int parm;

  if (!firsttime)
    close(snd);
  snd = open("/dev/dsp", O_RDONLY);
  if (snd < 0) {		/* open DSP device for read */
    sprintf(error, "%s: cannot open /dev/dsp", progname);
    perror(error);
    cleanup();
    exit(1);
  }

  parm = 1;			/* set mono */
  check_status(ioctl(snd, SOUND_PCM_WRITE_CHANNELS, &parm), __LINE__);

  parm = 8;			/* set 8-bit samples */
  check_status(ioctl(snd, SOUND_PCM_WRITE_BITS, &parm), __LINE__);

  /* set DMA buffer size */
  check_status(ioctl(snd, SOUND_PCM_SUBDIVIDE, &dma), __LINE__);

  /* set sampling rate */
  check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &sampling), __LINE__);
  check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
}

/* initialize graphics screen */
void
init_screen(int firsttime)
{
  if (firsttime) {
    vga_disabledriverreport();
    vga_init();
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
  case 'R':
    sampling *= 1.1;		/* 10% sample rate increase */
    check_status(ioctl(snd, SOUND_PCM_SYNC, 0), __LINE__);
    check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &sampling), __LINE__);
    check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
    CLEAR;
    break;
  case 'r':
    sampling *= 0.9;		/* 10% sample rate decrease */
    check_status(ioctl(snd, SOUND_PCM_SYNC, 0), __LINE__);
    check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &sampling), __LINE__);
    check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
    CLEAR;
    break;
  case 'S':
    if (scale < 16) {		/* double the scale (zoom) */
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
    if (trigger == -1)		/* enable the trigger at half scale */
      trigger = 118;
    trigger += 10;		/* increase trigger */
    if (trigger > 255)
      trigger = -1;		/* disable trigger when it leaves the scale */
    CLEAR;
    break;
  case 't':
    if (trigger == -1)		/* enable the trigger at half scale */
      trigger = 138;
    trigger -= 10;		/* decrease trigger */
    if (trigger < 0)
      trigger = -1;		/* disable trigger when it leaves the scale */
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
    while (vga_getkey() <= 0)	/* pause until key pressed */
      ;
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
  static unsigned char datum, datem;
			
  if (trigger != -1) {		/* simple trigger function */
    if (trigger > 128) {
      datum = 255;		/* positive trigger, look for rising edge */
       do {		
	datem = datum;		/* remember previous sample */
	read(snd, &datum, 1);
      } while ((handle_key() <= 0)
	       && ((datum < trigger) || (datem > trigger)));
    } else {
      datum = 0;		/* negative trigger, look for falling edge */
      do {
	datem = datum;		/* remember previous sample */
	read(snd, &datum, 1);
      } while ((handle_key() <= 0)
	       && ((datum > trigger) || (datem < trigger)));
    }
  } else {			/* not triggering */
    handle_key();
  }
				/* now get the real data */
  read(snd, buffer + 1, (h_points / scale - 2));
}

/* graph the data */
static inline void
graph_data()
{
  static int i;

  if (point_mode) {
    for (i = 1 ; i < (h_points / scale - 1) ; i++) {
      vga_setcolor(0);		/* erase previous point */
      vga_drawpixel(i * scale, old[i] + offset);
      vga_setcolor(colour);	/* draw new point */
      vga_drawpixel(i * scale, buffer[i] + offset);
      old[i] = buffer[i];	/* this becomes the point to erase next time */
    }
  } else {			/* line mode */
    for (i = 1 ; i < (h_points / scale - 2) ; i++) {
      vga_setcolor(0);		/* erase previous line */
      vga_drawline(i*scale, old[i] + offset,
		   i*scale+scale, old[i+1] + offset);
      vga_setcolor(colour);	/* draw new line */
      vga_drawline(i*scale, buffer[i] + offset,
		   i*scale+scale, buffer[i+1] + offset);
      old[i] = buffer[i];	/* this becomes the point to erase next time */
    }
    old[i] = buffer[i];
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
  init_sound_card(1);
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

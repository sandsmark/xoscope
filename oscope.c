/*
 *                      /-----------------------\
 *                      | Software Oscilloscope |
 *                      \-----------------------/
 *
 * [x]scope --- Use Linux's /dev/dsp (a sound card) as an oscilloscope
 *
 * @(#)$Id: oscope.c,v 1.32 1996/01/30 06:45:27 twitham Exp $
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
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
 * [x]scope 1.0 (enhancements by Tim Witham) was developed using:
 * - Linux kernel 1.2.10
 * - gcc 2.6.3
 * - svgalib 1.22
 * - libsx 1.1
 * - SoundBlaster 16
 * - ATI Win Turbo 2MB VRAM
 * - 100 MHz Intel Pentium(R) Processor, 32MB RAM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include "scope.h"		/* program defaults */
#include "display.h"		/* display functions */

/* global program defaults, defined in scope.h (see also) */
Scope scope;
Signal ch[CHANNELS];
int channels = DEF_12;
int mode = DEF_M;
int dma = DEF_D;
int point_mode = DEF_PL;
int graticule = DEF_G;
int behind = DEF_G;
int verbose = DEF_V;

/* extra global variable definitions */
char *progname;			/* the program's name, autoset via argv[0] */
char error[256];		/* buffer for "one-line" error messages */
char *def[] = {
  "",				/* used by -1/-2/-p/-l in usage message */
  "(default)",
  "on",				/* used by -g/-v in usage message */
  "off",
  "graticule",			/* used by -b in usage message */
  "signal",
};
int quit_key_pressed = 0;	/* set by handle_key() */
int snd;			/* file descriptor for sound device */
char buffer[MAXWID * 2];	/* buffer for sound data */
char junk[SAMPLESKIP];		/* junk data buffer */
int v_points;			/* points in vertical axis */
int h_points;			/* points in horizontal axis */
int offset;			/* vertical offset */
int actual;			/* actual sampling rate */
int scaler[] = {1,2,5,10,20,50,100,200};
int *maxscaler = &scaler[7];	/* the last one */
int *pscaler = scaler;

/* display command usage on standard error and exit */
void
usage()
{
  fprintf(stderr, "usage: %s "
	  "[-1|-2|-3|-4] [-r<rate>] [-s<scale>] [-t<trigger>] [-c<colour>]
             [-m<mode>] [-d<dma divisor>] [-f font] [-p|-l] [-g] [-b] [-v]

Options          Runtime Keys   Description (defaults)
-1               `       cycle  single hardware channel , left  input  %s
-2               `       cycle  dual   hardware channels, right input  %s
-3               `       cycle  Three channel, 2 hardware + 1 software %s
-4               `       cycle  Four  channel, 2 hardware + 2 software %s
-r <rate>        S       auto   sampling Rate in Hz             (default=%d)
-s <scale>       S *2   s /2    time Scale or zoom: 1,2,4,8,16  (default=%d)
-t <trigger>     T +8   t -8    Trigger level:0-255,-1=disabled (default=%d)
-c <colour>      C +1   c -1    graticule Colour                (default=%d)
-m <mode>        M +1   m -1    video mode (size): 0,1,2,3,4,5,6(default=%d)
-d <dma divisor> D *2   d /2    DMA buffer size divisor: 1,2,4  (default=%d)
-f <font name>                  The font name as-in %s
-p               P  p   toggle  Point mode, opposite of -l      %s
-l               L  l   toggle  Line  mode, opposite of -p      %s
-g               G  g   toggle  turn %s Graticule of 5 msec major divisions
-b               B  b   toggle  %s Behind instead of in front of %s
-v               V  v   toggle  turn %s Verbose keypress option log to stdout
                 <space>toggle  Run / Stop
                 Q q            Quit %s
",
	  progname,
	  def[channels == 1],
	  def[channels == 2],
	  def[channels == 3],
	  def[channels == 4],
	  scope.rate, scope.scale, scope.trig,
	  scope.color, mode, dma,
	  fonts(),		/* the font method for the display */
	  def[point_mode], def[!point_mode],
	  def[scope.grat + 2], def[scope.behind + 4], def[!scope.behind + 4],
	  def[verbose + 2],
	  progname);
  exit(1);
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
    case '2':			/* dual channels (stereo) */
    case '3':
    case '4':
      channels = c - '0';
      break;
    case 'r':			/* sample rate */
      scope.rate = strtol(optarg, NULL, 0);
      break;
    case 's':			/* scale (zoom) */
      scope.scale = strtol(optarg, NULL, 0);
      scope.scale &= 0x000f;
      if (scope.scale < 1)
	scope.scale = 1;
      break;
    case 't':			/* trigger level */
      scope.trig = strtol(optarg, NULL, 0);
      scope.trig &= 0x00ff;
      break;
    case 'c':			/* channel 1 trace colour */
      scope.color = strtol(optarg, NULL, 0);
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
      scope.grat = !scope.grat;
      break;
    case 'b':			/* behind/front */
      scope.behind = !scope.behind;
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

/* initialize the scope and its' signals */
void
init_scope()
{
  int i;
  int channelcolor[] = CHANNELCOLOR;
  scope.scale = DEF_S;
  scope.rate = DEF_R;
  scope.trig = DEF_T;
  scope.grat = DEF_G;
  scope.behind = DEF_B;
  scope.run = 1;
  scope.color = DEF_C;
  for (i = 0 ; i < CHANNELS ; i++) {
    memset(ch[i].data, 0, MAXWID);
    memset(ch[i].old, 0, MAXWID);
    ch[i].scale = 1;
    ch[i].pos = 0;
    ch[i].color = color[channelcolor[i]];
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

  parm = (channels > 1) + 1;	/* set mono/stereo */
  check_status(ioctl(snd, SOUND_PCM_WRITE_CHANNELS, &parm), __LINE__);

  parm = 8;			/* set 8-bit samples */
  check_status(ioctl(snd, SOUND_PCM_WRITE_BITS, &parm), __LINE__);

  /* set DMA buffer size */
  check_status(ioctl(snd, SOUND_PCM_SUBDIVIDE, &dma), __LINE__);

  /* set sampling rate */
  check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &scope.rate), __LINE__);
  check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
}

/* handle single key commands */
void
handle_key(unsigned char c)
{
  switch (c) {
  case 0:
  case -1:			/* no key pressed */
    break;
  case '`':			/* single or dual channel mode */
    channels++;
    if (channels > 4)
      channels = 1;
    init_sound_card(0);
    clear();
    break;
  case '1':
  case '2':
  case '3':
  case '4':
    ch[c - '1'].pos += 8;
    clear();
    break;
  case '!':
    ch[0].pos -= 8;
    clear();
    break;
  case '@':
    ch[1].pos -= 8;
    clear();
    break;
  case '#':
    ch[2].pos -= 8;
    clear();
    break;
  case '$':
    ch[3].pos -= 8;
    clear();
    break;
  case 'q':
    ch[0].scale /= 2;
    clear();
    break;
  case 'Q':
    ch[0].scale *= 2;
    clear();
    break;
  case 'w':
    ch[1].scale /= 2;
    clear();
    break;
  case 'W':
    ch[1].scale *= 2;
    clear();
    break;
  case 'e':
    ch[2].scale /= 2;
    clear();
    break;
  case 'E':
    ch[2].scale *= 2;
    clear();
    break;
  case 'r':
    ch[3].scale /= 2;
    clear();
    break;
  case 'R':
    ch[3].scale *= 2;
    clear();
    break;
  case 'S':
    if (scope.rate == 8800) {
      scope.rate = 22000;
      check_status(ioctl(snd, SOUND_PCM_SYNC, 0), __LINE__);
      check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &scope.rate), __LINE__);
      check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
    } else if (scope.rate == 22000) {
      scope.rate = 44000;
      check_status(ioctl(snd, SOUND_PCM_SYNC, 0), __LINE__);
      check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &scope.rate), __LINE__);
      check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
    } else 
      pscaler++;
    if (pscaler > maxscaler)
      pscaler = maxscaler;
    scope.scale  = *pscaler;
    clear();
    break;
  case 's':
    if (scope.rate == 8800) {
				/* average samples into each pixel */
    } else if (scope.rate == 22000) {
      scope.rate = 8800;
      check_status(ioctl(snd, SOUND_PCM_SYNC, 0), __LINE__);
      check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &scope.rate), __LINE__);
      check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
    } else if (pscaler == scaler) {
      scope.rate = 22000;
      check_status(ioctl(snd, SOUND_PCM_SYNC, 0), __LINE__);
      check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &scope.rate), __LINE__);
      check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
    } else
      pscaler--;
    scope.scale  = *pscaler;
    clear();
    break;
  case 'T':
    if (scope.trig < 0)		/* enable the trigger at half scale */
      scope.trig = 120;
    scope.trig += 8;		/* increase trigger */
    if (scope.trig > 255)
      scope.trig = -1;		/* disable trigger when it leaves the scale */
    clear();
    break;
  case 't':
    if (scope.trig < 0)		/* enable the trigger at half scale */
      scope.trig = 136;
    scope.trig -= 8;		/* decrease trigger */
    clear();
    break;
  case 'C':
    scope.color++;			/* increase color */
    if (scope.color > 15)
      scope.color = 0;
    break;
  case 'c':
    scope.color--;
    if (scope.color < 0)		/* decrease color */
      scope.color = 15;
    break;
  case 'M':
    if (mode < 6) {
      mode++;			/* increase video mode */
      init_screen(0);
      clear();
    }
    break;
  case 'm':
    if (mode > 1) {		/* decrease video mode */
      mode--;
      init_screen(0);
      clear();
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
    point_mode++;		/* point, point accumulate, line, line acc. */
    if (point_mode > 3)
      point_mode = 0;
    clear();
    break;
  case 'G':
  case 'g':
    scope.grat = !scope.grat;	/* graticule on/off */
    clear();
    break;
  case 'B':
  case 'b':
    scope.behind = !scope.behind; /* graticule behind/in front of signal */
    break;
  case 'V':
  case 'v':
    verbose = !verbose;		/* verbose log on/off */
    draw_text();
    break;
  case ' ':
    scope.run = !scope.run;
    draw_text();
    c = 0;			/* suppress verbose log */
    break;
  case '\e':			/* quit */
    quit_key_pressed = 1;
    break;
  default:
    c = 0;			/* ignore unknown keys */
  }

  if (c > 0)
    show_info(c);		/* show keypress and result on stdout */
}

/* get data from sound card */
inline void
get_data()
{
  static unsigned char datum[2], prev, *buff;
  int c = 0;

				/* flush the sound card's buffer */
  check_status(ioctl(snd, SNDCTL_DSP_RESET), __LINE__);
  read(snd, junk, SAMPLESKIP);	/* toss some possibly invalid samples */
  if (scope.trig > -1) {		/* trigger enabled */
    if (scope.trig > 128) {
      datum[0] = 255;		/* positive trigger, look for rising edge */
      do {
	prev = datum[0];	/* remember prev. channel 1, read channel(s) */
	read(snd, datum , (channels > 1) + 1);
      } while (((c++ < h_points)) &&
	       ((datum[0] < scope.trig) || (prev > scope.trig)));
    } else {
      datum[0] = 0;		/* negative scope.trig, look for falling edge */
      do {
	prev = datum[0];	/* remember prev. channel 1, read channel(s) */
	read(snd, datum, (channels > 1) + 1);
      } while (((c++ < h_points)) &&
	       ((datum[0] > scope.trig) || (prev < scope.trig)));
    }
  }
  if (c > h_points)		/* haven't triggered within the screen */
    return;			/* give up and keep previous samples */

  /* now get the real data */
  read(snd, buffer, h_points * ((channels > 1) + 1));
  buff = buffer;
  for(c=0; c < h_points; c++) {
    ch[0].data[c] = (int)(*buff) - 128;
    buff += (channels > 1);
    ch[1].data[c] = (int)(*buff++) - 128;
    ch[2].data[c] = ch[0].data[c] + ch[1].data[c];
    ch[3].data[c] = ch[0].data[c] - ch[1].data[c];
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

  argc = opendisplay(argc, argv);
  parse_args(argc, argv);	/* what do you want? */
  init_sound_card(1);		/* get ready */
  scope.scale = 1;
  init_screen(1);
  init_scope();
  show_info(' ');
  mainloop();			/* from display.h */
  cleanup();			/* close sound, back to text mode */
  exit(0);
}

/*
 *                      /-----------------------\
 *                      | Software Oscilloscope |
 *                      \-----------------------/
 *
 * [x]oscope --- Use Linux's /dev/dsp (a sound card) as an oscilloscope
 *
 * @(#)$Id: oscope.c,v 1.39 1996/02/03 04:08:07 twitham Exp $
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
 * defaults, simply tweak oscope.h and re-make.
 *
 * scope 0.1 (original by Jeff Tranter) was developed using:
 * - Linux kernel 1.0
 * - gcc 2.4.5
 * - svgalib version 1.05
 * - SoundBlaster Pro
 * - Trident VGA card
 * - 80386DX40 CPU with 8MB RAM
 *
 * [x]oscope 1.0 (enhancements by Tim Witham) was developed using:
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
#include "oscope.h"		/* program defaults */
#include "display.h"		/* display routines */
#include "func.h"		/* signal math functions */

/* global program defaults, defined in oscope.h (see also) */
Scope scope;
Signal ch[CHANNELS];
int verbose = DEF_V;

/* extra global variable definitions */
char *progname;			/* the program's name, autoset via argv[0] */
char error[256];		/* buffer for "one-line" error messages */
int quit_key_pressed = 0;	/* set by handle_key() */
int snd;			/* file descriptor for sound device */
char buffer[MAXWID * 2];	/* buffer for sound data */
char junk[SAMPLESKIP];		/* junk data buffer */
int v_points;			/* points in vertical axis */
int h_points;			/* points in horizontal axis */
int offset;			/* vertical offset */
int actual;			/* actual sampling rate */

/* display command usage on standard error and exit */
void
usage()
{
  static char *def[] = {
    "on",				/* used by -g/-v in usage message */
    "off",
    "graticule",			/* used by -b in usage message */
    "signal",
  };

  fprintf(stderr, "usage: %s "
	  "[-r<rate>] [-s<scale>] [-t<trigger>] [-c<colour>]
[-m<mode>] [-d<dma divisor>] [-f font] [-p] [-g] [-b] [-v]

Startup Options  Description (defaults)
-r <rate>        sampling Rate in Hz             (%d)
-s <scale>       time Scale: 1,2,5,10,20,100,200 (%d)
-t <trigger>     Trigger level:0-255,-1=disabled (%d)
-c <colour>      graticule Color                 (%d)
-m <mode>        video mode (size): 0,1,2,3      (%d)
-d <dma divisor> DMA buffer size divisor: 1,2,4  (%d)
-f <font name>   The font name as-in %s
-p               Plot mode, 0,1,2,3              (%d)
-g               turn %s Graticule
-b               %s Behind instead of in front of %s
-v               turn %s Verbose keypress option log to stdout
",
	  progname,
	  DEF_R, DEF_S, DEF_T,
	  DEF_C, scope.size, scope.dma,
	  fonts(),		/* the font method for the display */
	  scope.mode,
	  def[scope.grat], def[scope.behind + 2], def[!scope.behind + 2],
	  def[verbose]);
  exit(1);
}

/* handle command line options */
void
parse_args(int argc, char **argv)
{
  const char     *flags = "1234r:s:t:c:m:d:f:p:gbv";
  int             c;

  while ((c = getopt(argc, argv, flags)) != EOF) {
    switch (c) {
    case '1':
    case '2':
    case '3':
    case '4':
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
    case 'c':			/* graticule colour */
      scope.color = strtol(optarg, NULL, 0);
      break;
    case 'm':			/* video mode */
      scope.size = strtol(optarg, NULL, 0);
      break;
    case 'd':			/* dma diviisor */
      scope.dma = strtol(optarg, NULL, 0);
      scope.dma &= 0x0007;
      break;
    case 'f':			/* font name */
      strcpy(fontname, optarg);
      break;
    case 'p':			/* point mode */
      scope.mode = strtol(optarg, NULL, 0);
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

/* initialize the scope */
void
init_scope()
{
  scope.size = DEF_M;
  scope.dma = DEF_D;
  scope.mode = DEF_P;
  scope.scale = DEF_S;
  scope.rate = DEF_R;
  scope.trig = DEF_T;
  scope.grat = DEF_G;
  scope.behind = DEF_B;
  scope.run = 1;
  scope.color = DEF_C;
  scope.select = 0;
}

/* initialize the signals */
void
init_channels()
{
  int i, channelcolor[] = CHANNELCOLOR;

  for (i = 0 ; i < CHANNELS ; i++) {
    memset(ch[i].data, 0, MAXWID);
    memset(ch[i].old, 0, MAXWID);
    ch[i].mult = 1;
    ch[i].div = 1;
    ch[i].pos = 0;
    ch[i].color = color[channelcolor[i]];
    ch[i].show = (i < 2);
    ch[i].func = i;
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

  parm = 2;			/* set mono/stereo */
  check_status(ioctl(snd, SOUND_PCM_WRITE_CHANNELS, &parm), __LINE__);

  parm = 8;			/* set 8-bit samples */
  check_status(ioctl(snd, SOUND_PCM_WRITE_BITS, &parm), __LINE__);

  /* set DMA buffer size */
  check_status(ioctl(snd, SOUND_PCM_SUBDIVIDE, &(scope.dma)), __LINE__);

  /* set sampling rate */
  check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &scope.rate), __LINE__);
  check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
}

/* handle single key commands */
void
handle_key(unsigned char c)
{
  static int scaler[] = {1,2,5,10,20,50,100,200};
  static int *maxscaler = &scaler[7];	/* the last one */
  static int *pscaler = scaler;

  switch (c) {
  case 0:
  case -1:			/* no key pressed */
    break;
  case '\t':
    ch[scope.select].show = !ch[scope.select].show;
    clear();
    break;
  case '1':
  case '2':
  case '3':
  case '4':
    scope.select = (c - '1');
    clear();
    break;
  case 'r':
    if (ch[scope.select].div > 1)
      ch[scope.select].div >>= 1;
    else
      ch[scope.select].mult <<= 1;
    clear();
    break;
  case 'f':
    if (ch[scope.select].mult > 1)
      ch[scope.select].mult >>= 1;
    else
      ch[scope.select].div <<= 1;
    clear();
    break;
  case 'u':
    ch[scope.select].pos -= 16;
    clear();
    break;
  case 'j':
    ch[scope.select].pos += 16;
    clear();
    break;
  case 't':
  case 'y':
    if (scope.select > 1) {
      ch[scope.select].func++;
      if (ch[scope.select].func >= funccount)
	ch[scope.select].func = 2;
      clear();
    }
    break;
  case '0':
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
  case '9':
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
  case 'o':
    if (scope.trig < 0)		/* enable the trigger at half scale */
      scope.trig = 120;
    scope.trig += 8;		/* increase trigger */
    if (scope.trig > 255)
      scope.trig = -1;		/* disable trigger when it leaves the scale */
    clear();
    break;
  case 'l':
    if (scope.trig < 0)		/* enable the trigger at half scale */
      scope.trig = 136;
    scope.trig -= 8;		/* decrease trigger */
    clear();
    break;
  case '=':
    scope.color++;		/* increase color */
    if (scope.color > 15)
      scope.color = 0;
    break;
  case '-':
    scope.color--;
    if (scope.color < 0)	/* decrease color */
      scope.color = 15;
    break;
  case '.':
    if (scope.dma < 3) {		/* double dma */
      scope.dma <<= 1;
      init_sound_card(0);
    }
    break;
  case ',':
    if (scope.dma > 1) {		/* half dma */
      scope.dma >>= 1;
      init_sound_card(0);
    }
    break;
  case 'p':
    scope.mode++;		/* point, point accumulate, line, line acc. */
    if (scope.mode > 3)
      scope.mode = 0;
    clear();
    break;
  case '[':
    scope.grat = !scope.grat;	/* graticule on/off */
    clear();
    break;
  case ']':
    scope.behind = !scope.behind; /* graticule behind/in front of signal */
    draw_text(1);
    break;
  case ' ':
    scope.run = !scope.run;
    draw_text(1);
    c = 0;			/* suppress verbose log */
    break;
  case '\r':
  case '\n':
    clear();			/* refresh screen */
    break;
  case '\b':		
    verbose = !verbose;		/* verbose on/off */
    break;
  case '\e':		
    quit_key_pressed = 1;	/* quit */
    break;
  default:
    c = 0;			/* ignore unknown keys */
  }

  if (c > 0)
    show_info(c);		/* show keypress and result on stdout */
}

/* auto-measurements */
void
measure_data(Signal *sig) {
  static int i, j, prev;
  int first = 0, last = 0, count = 0;

  sig->min = 0;
  sig->max = 0;
  sig->time = -1;
  prev = 0;
  for (i = 0 ; i < h_points ; i++) {
    j = sig->data[i];
    if (j < sig->min)
      sig->min = j;
    if (j > sig->max)
      sig->max = j;
    if ((j > 0 && prev <= 0) || (j < 0 && prev >= 0)) {
      if (!first)
	first = i;
      last = i;
      count++;
    }
    prev = j;
  }
  if (count > 2)
    sig->time = (last - first) / (count - 2) * 2;
}

/* get data from sound card */
inline void
get_data()
{
  static unsigned char datum[2], prev, *buff;
  int i = 0;

				/* flush the sound card's buffer */
  check_status(ioctl(snd, SNDCTL_DSP_RESET), __LINE__);
  read(snd, junk, SAMPLESKIP);	/* toss some possibly invalid samples */
  if (scope.trig > -1) {		/* trigger enabled */
    if (scope.trig > 128) {
      datum[0] = 255;		/* positive trigger, look for rising edge */
      do {
	prev = datum[0];	/* remember prev. channel 1, read channel(s) */
	read(snd, datum, 2);
      } while (((i++ < h_points)) &&
	       ((datum[0] < scope.trig) || (prev > scope.trig)));
    } else {
      datum[0] = 0;		/* negative scope.trig, look for falling edge */
      do {
	prev = datum[0];	/* remember prev. channel 1, read channel(s) */
	read(snd, datum, 2);
      } while (((i++ < h_points)) &&
	       ((datum[0] > scope.trig) || (prev < scope.trig)));
    }
  }
  if (i > h_points)		/* haven't triggered within the screen */
    return;			/* give up and keep previous samples */

  /* now get the real data */
  read(snd, buffer, h_points * 2);
  buff = buffer;
  for(i=0; i < h_points; i++) {
    ch[0].data[i] = (int)(*buff++) - 128;
    ch[1].data[i] = (int)(*buff++) - 128;
  }
  funcarray[ch[2].func](2);
  funcarray[ch[3].func](3);
  measure_data(&ch[scope.select]);
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
  init_scope();
  parse_args(argc, argv);
  init_screen(1);
  init_sound_card(1);
  init_channels();
  show_info(' ');
  mainloop();			/* to display.c */
  cleanup();
  exit(0);
}

/*
 * @(#)$Id: oscope.c,v 1.47 1996/03/02 06:59:16 twitham Exp $
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the basic oscilloscope internals
 *
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

/* global program variables */
Scope scope;
Signal ch[CHANNELS];

/* extra global variable definitions */
char *progname;			/* the program's name, autoset via argv[0] */
char error[256];		/* buffer for "one-line" error messages */
int quit_key_pressed = 0;	/* set by handle_key() */
int snd;			/* file descriptor for sound device */
char buffer[MAXWID * 2];	/* buffer for stereo sound data */
char junk[SAMPLESKIP];		/* junk data buffer */
int v_points;			/* pixels in vertical axis */
int h_points;			/* pixels in horizontal axis */
int offset;			/* vertical pixel offset to zero line */
int actual;			/* actual sampling rate */
int clip = 0;			/* whether we're maxed out or not */

/* display command usage on standard error and exit */
void
usage()
{
  static char *def[] = {
    "graticule",			/* used by -b in usage message */
    "signal",
  };

  fprintf(stderr, "usage: %s "
	  "[-h] [-#<code>] ... [-r<rate>] [-s<scale>] [-t<trigger>]
[-c<color>] [-m<mode>] [-d<dma divisor>] [-f<font>] [-p<type>] [-g<style>] [-b]

Startup Options  Description (defaults)
-h               this Help message and exit
-# <code>        #=channel (1-%d), code=position:[scale[:function#]] (0:1/1:0)
-r <rate>        sampling Rate in Hz: 8800,22000,44000      (%d)
-s <scale>       time Scale: 1,2,5,10,20,100,200            (%d)
-t <trigger>     Trigger level: -128 - 127, 255 = disabled  (%d)
-c <color>       graticule Color: 0-15                      (%d)
-m <mode>        video mode (size): 0,1,2,3                 (%d)
-d <dma divisor> DMA buffer size divisor: 1,2,4             (%d)
-f <font name>   The font name as-in %s
-p <type>        Plot mode: 0,1,2,3                         (%d)
-g <style>       Graticule: 0=none, 1=minor, 2=major        (%d)
-b               %s Behind instead of in front of %s
",
	  progname, CHANNELS,
	  DEF_R, DEF_S, DEF_T,
	  DEF_C, scope.size, scope.dma,
	  fonts(),		/* the font method for the display */
	  scope.mode,
	  scope.grat, def[scope.behind], def[!scope.behind]);
  exit(1);
}

/* handle command line options */
void
parse_args(int argc, char **argv)
{
  const char     *flags = "h"
    "1:2:3:4:5:6:7:8:"
    "r:s:t:c:m:d:f:p:g:b"
    "R:S:T:C:M:D:F:P:G:B";
  char *p, *q;
  int c;

  while ((c = getopt(argc, argv, flags)) != EOF) {
    switch (c) {
    case 'r':			/* sample rate */
    case 'R':
      scope.rate = strtol(optarg, NULL, 0);
      break;
    case 's':			/* scale (zoom) */
    case 'S':
      scope.scale = strtol(optarg, NULL, 0);
      if (scope.scale < 1)
	scope.scale = 1;
      break;
    case 't':			/* trigger level */
    case 'T':
      scope.trig = 128 + strtol(optarg, NULL, 0);
      if (scope.trig > 128) {
	scope.trig = 128;
	scope.trige = 0;
      }
      break;
    case 'c':			/* graticule color */
    case 'C':
      scope.color = strtol(optarg, NULL, 0);
      break;
    case 'm':			/* video mode */
    case 'M':
      scope.size = strtol(optarg, NULL, 0);
      break;
    case 'd':			/* dma diviisor */
    case 'D':
      scope.dma = strtol(optarg, NULL, 0);
      scope.dma &= 0x0007;
      break;
    case 'f':			/* font name */
    case 'F':
      strcpy(fontname, optarg);
      break;
    case 'p':			/* point mode */
    case 'P':
      scope.mode = strtol(optarg, NULL, 0);
      break;
    case 'g':			/* graticule on/off */
    case 'G':
      scope.grat = strtol(optarg, NULL, 0);
      break;
    case 'b':			/* behind/front */
    case 'B':
      scope.behind = !scope.behind;
      break;
    case ':':			/* help or unknown option */
    case '?':
    case 'h':
    case 'H':
      usage();
      break;
    default:
      if (c >= '1' && c <= '0' + CHANNELS) {
	p = optarg;
	ch[c - '1'].show = 1;
	ch[c - '1'].pos = -strtol(p, NULL, 0);
	if ((q = strchr(p, ':')) != NULL) {
	  ch[c - '1'].mult = strtol(++q, NULL, 0);
	  p = q;
	}
	if ((q = strchr(p, '/')) != NULL) {
	  ch[c - '1'].div = strtol(++q, NULL, 0);
	  p = q;
	}
	if (c > '1' && (q = strchr(p, ':')) != NULL) {
	  ch[c - '1'].func = strtol(++q, NULL, 0) + 2;
	  p = q;
	}
      }
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
  scope.trigch = 0;
  scope.trig = 128 + DEF_T;
  scope.trige = (DEF_T != -1);
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
  int i;

  for (i = 0 ; i < CHANNELS ; i++) {
    memset(ch[i].data, 0, MAXWID);
    memset(ch[i].old, 0, MAXWID);
    ch[i].mult = 1;
    ch[i].div = 1;
    ch[i].pos = 0;
    ch[i].show = (i < 2);
    ch[i].func = i < 2 ? i : 2;
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

/* scale num upward like a scope does, 1 to 200 */
void
scaleup(int *num)
{
  if (*num < 2)
    *num = 2;
  else if (*num < 5)
    *num = 5;
  else if (*num < 10)
    *num = 10;
  else if (*num < 20)
    *num = 20;
  else if (*num < 50)
    *num = 50;
  else if (*num < 100)
    *num = 100;
  else
    *num = 200;
}

/* scale num downward like a scope does, 200 to 1 */
void
scaledown(int *num)
{
  if (*num > 100)
    *num = 100;
  else if (*num > 50)
    *num = 50;
  else if (*num > 20)
    *num = 20;
  else if (*num > 10)
    *num = 10;
  else if (*num > 5)
    *num = 5;
  else if (*num > 2)
    *num = 2;
  else
    *num = 1;
}

/* handle single key commands */
void
handle_key(unsigned char c)
{
  static Signal *p;

  p = &ch[scope.select];
  if (c >= 'A' && c <= 'Z' && actual >= 44000) {
    save(c);
    draw_text(1);
    return;
  } else if (c >= 'a' && c <= 'z' && scope.select > 1) {
    recall(c);
    draw_text(1);
    return;
  } else if (c >= '1' && c <= '0' + CHANNELS) {
    scope.select = (c - '1');
    clear();
    return;
  }
  switch (c) {
  case 0:
  case -1:			/* no key pressed */
    break;
  case '\t':
    p->show = !p->show;
    clear();
    break;
  case ']':			/* increase scale */
    if (p->div > 1)
      scaledown(&p->div);
    else
      scaleup(&p->mult);
    clear();
    break;
  case '[':			/* decrease scale */
    if (p->mult > 1)
      scaledown(&p->mult);
    else
      scaleup(&p->div);
    clear();
    break;
  case '}':
    p->pos -= 16;		/* position up */
    clear();
    break;
  case '{':
    p->pos += 16;		/* position down */
    clear();
    break;
  case ';':
    if (scope.select > 1) {	/* next math function */
      p->func++;
      if (p->func >= funccount)
	p->func = 3;
      clear();
    }
    break;
  case ':':
    if (scope.select > 1) {	/* previous math function */
      p->func--;
      if (p->func <3)
	p->func = funccount - 1;
      clear();
    }
    break;
  case '0':
    if (scope.run)
      if (actual <= 8800) {
	scope.rate = 22000;
	check_status(ioctl(snd, SOUND_PCM_SYNC, 0), __LINE__);
	check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &scope.rate), __LINE__);
	check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
      } else if (actual <= 22000) {
	scope.rate = 44000;
	check_status(ioctl(snd, SOUND_PCM_SYNC, 0), __LINE__);
	check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &scope.rate), __LINE__);
	check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
      } else
	scaleup(&scope.scale);
    else if (actual >= 44000)
      scaleup(&scope.scale);
    clear();
    break;
  case '9':
    if (scope.run)
      if (actual <= 8800) {
	/* average several samples into each pixel */
      } else if (actual <= 22000) {
	scope.rate = 8800;
	check_status(ioctl(snd, SOUND_PCM_SYNC, 0), __LINE__);
	check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &scope.rate), __LINE__);
	check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
      } else if (scope.scale == 1) {
	scope.rate = 22000;
	check_status(ioctl(snd, SOUND_PCM_SYNC, 0), __LINE__);
	check_status(ioctl(snd, SOUND_PCM_WRITE_RATE, &scope.rate), __LINE__);
	check_status(ioctl(snd, SOUND_PCM_READ_RATE, &actual), __LINE__);
      } else
	scaledown(&scope.scale);
    else
      scaledown(&scope.scale);
    clear();
    break;
  case '=':
    scope.trig += 8;		/* increase trigger */
    if (scope.trig > 255)
      scope.trig = 0;
    clear();
    break;
  case '-':
    scope.trig -= 8;		/* decrease trigger */
    if (scope.trig < 0)
      scope.trig = 255;
    clear();
    break;
  case '_':
    scope.trigch = !scope.trigch;
    clear();
    break;
  case '+':
    scope.trige++;
    if (scope.trige > 2)
      scope.trige = 0;
    clear();
    break;
  case ')':
    scope.color++;		/* increase color */
    if (scope.color > 15)
      scope.color = 0;
    draw_text(1);
    break;
  case '(':
    scope.color--;		/* decrease color */
    if (scope.color < 0)
      scope.color = 15;
    draw_text(1);
    break;
  case '.':
    if (scope.dma < 3) {	/* double dma */
      scope.dma <<= 1;
      init_sound_card(0);
      draw_text(1);
    }
    break;
  case ',':
    if (scope.dma > 1) {	/* half dma */
      scope.dma >>= 1;
      init_sound_card(0);
      draw_text(1);
    }
    break;
  case '!':
    scope.mode++;		/* point, point accumulate, line, line acc. */
    if (scope.mode > 3)
      scope.mode = 0;
    clear();
    break;
  case '&':
    scope.grat++;		/* graticule off/on/more */
    if (scope.grat > 2)
      scope.grat = 0;
    clear();
    break;
  case '*':
    scope.behind = !scope.behind; /* graticule behind/in front of signal */
    draw_text(1);
    break;
  case ' ':
    scope.run = !scope.run;
    draw_text(1);
    break;
  case '\r':
  case '\n':
    clear();			/* refresh screen */
    break;
  case '\e':		
    quit_key_pressed = 1;	/* quit */
    break;
  default:
    c = 0;			/* ignore unknown keys */
  }
}

/* get data from sound card, return value is whether we triggered or not */
int
get_data()
{
  static unsigned char datum[2], prev, *buff;
  int i = 0;

  /* flush the sound card's buffer */
  check_status(ioctl(snd, SNDCTL_DSP_RESET), __LINE__);
  read(snd, junk, SAMPLESKIP);	/* toss some possibly invalid samples */
  if (scope.trige == 1) {
    datum[scope.trigch] = 255;	/* look for rising edge */
    do {
      prev = datum[scope.trigch]; /* remember previous, read channels */
      read(snd, datum, 2);
    } while (((i++ < h_points)) &&
	     ((datum[scope.trigch] < scope.trig) || (prev >= scope.trig)));
  } else if (scope.trige == 2) {
    datum[scope.trigch] = 0;	/* look for falling edge */
    do {
      prev = datum[scope.trigch]; /* remember previous, read channels */
      read(snd, datum, 2);
    } while (((i++ < h_points)) &&
	     ((datum[scope.trigch] > scope.trig) || (prev <= scope.trig)));
  }
  if (i > h_points)		/* haven't triggered within the screen */
    return(0);			/* give up and keep previous samples */

  clip = 0;			/* now get the real data */
  read(snd, buffer, h_points * 2);
  buff = buffer;
  for(i=0; i < h_points; i++) {
    if (*buff == 0 || *buff == 255)
      clip = 1;
    ch[0].data[i] = (short)(*buff++) - 128;
    if (*buff == 0 || *buff == 255)
      clip = 2;
    ch[1].data[i] = (short)(*buff++) - 128;
  }
  return(scope.trige);
}

/* main program */
int
main(int argc, char **argv)
{
  progname = strrchr(argv[0], '/');
  if (progname == NULL)
    progname = argv[0];
  else
    progname++;
  init_scope();
  init_channels();
  opendisplay(argc, argv);	/* in display.c, calls parse_args */
  init_screen();
  init_sound_card(1);
  init_math();
  mainloop();			/* to display.c */
  cleanup();
  exit(0);
}

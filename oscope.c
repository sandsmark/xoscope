/*
 * @(#)$Id: oscope.c,v 1.60 1997/05/04 20:03:00 twitham Rel1_3 $
 *
 * Copyright (C) 1996 - 1997 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the basic oscilloscope internals
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "oscope.h"		/* program defaults */
#include "display.h"		/* display routines */
#include "func.h"		/* signal math functions */
#include "file.h"		/* file I/O functions */

/* global program variables */
Scope scope;
Channel ch[CHANNELS];

/* extra global variable definitions */
char *progname;			/* the program's name, autoset via argv[0] */
char version[] = VER;		/* version of the program, from Makefile */
char error[256];		/* buffer for "one-line" error messages */
int quit_key_pressed = 0;	/* set by handle_key() */
char buffer[MAXWID * 2];	/* buffer for stereo sound data */
char junk[SAMPLESKIP];		/* junk data buffer */
int v_points;			/* pixels in vertical axis */
int h_points;			/* pixels in horizontal axis */
int verbose;
int offset;			/* vertical pixel offset to zero line */
int clip = 0;			/* whether we're maxed out or not */
char *filename;			/* default file name */

extern void open_sound_card();
extern void close_sound_card();
extern int set_sound_card();
extern int reset_sound_card();

/* display command usage on standard error and exit */
void
usage()
{
  static char *def[] = {"graticule", "signal"}, *onoff[] = {"on", "off"};

  fprintf(stderr, "usage: %s "
	  "[-h]
[-#<code>] ... [-a #] [-r<rate>] [-s<scale>] [-t<trigger>] [-c<color>]
[-m<mode>] [-d<dma>] [-f<font>] [-p<type>] [-g<style>] [-b] [-v] [file]

Startup Options  Description (defaults)               version %s
-h               this Help message and exit
-# <code>        #=1-%d, code=pos[:scale[:func#, mem letter, or cmd]] (0:1/1)
-a <channel>     set the Active channel: 1-%d                  (%d)
-r <rate>        sampling Rate in Hz: 8800,11000,22000,44000  (%d)
-s <scale>       time Scale: 1/100-100                        (%d/1)
-t <trigger>     Trigger level[:type[:channel]]               (%s)
-c <color>       graticule Color: 0-15                        (%d)
-m <mode>        video mode (size): 0,1,2,3                   (%d)
-d <dma divisor> DMA buffer size divisor: 1,2,4               (%d)
-f <font name>   The font name as-in %s
-p <type>        Plot mode: 0=point, 1=accum, 2=line, 3=accum (%d)
-g <style>       Graticule: 0=none,  1=minor, 2=major         (%d)
-b               %s Behind instead of in front of %s
-v               turn %s Verbose key help display
file             %s file to load to restore settings and memory
",
	  progname, version, CHANNELS, CHANNELS, DEF_A,
	  DEF_R, DEF_S, DEF_T,
	  DEF_C, scope.size, scope.dma,
	  fonts,		/* the font method for the display */
	  scope.mode,
	  scope.grat, def[DEF_B], def[!DEF_B],
	  onoff[DEF_V], progname);
  exit(1);
}

/* handle command line options */
void
parse_args(int argc, char **argv)
{
  const char     *flags = "Hh"
    "1:2:3:4:5:6:7:8:"
    "a:r:s:t:c:m:d:f:p:g:bv"
    "A:R:S:T:C:M:D:F:P:G:BV";
  int c;

  while ((c = getopt(argc, argv, flags)) != EOF) {
    handle_opt(c, optarg);
  }
}

/* cleanup before exiting due to error or program end */
void
cleanup()
{
  cleanup_math();
  cleanup_display();
  close_sound_card();
}

/* initialize the scope */
void
init_scope()
{
  scope.size = DEF_M;
  scope.dma = DEF_D;
  scope.mode = DEF_P;
  scope.scale = DEF_S;
  scope.div = 1;
  scope.rate = DEF_R;
  handle_opt('t', DEF_T);
  scope.grat = DEF_G;
  scope.behind = DEF_B;
  scope.run = 1;
  scope.color = DEF_C;
  scope.select = DEF_A - 1;
  scope.verbose = DEF_V;
}

/* initialize the signals */
void
init_channels()
{
  int i, j[] = {23, 24, 25, 0, 0, 0, 0, 0};

  for (i = 0 ; i < CHANNELS ; i++) {
    ch[i].signal = &mem[j[i]];
    memset(ch[i].old, 0, MAXWID * sizeof(short));
    ch[i].mult = 1;
    ch[i].div = 1;
    ch[i].pos = 0;
    ch[i].show = (i < 2);
    ch[i].func = i < 2 ? i : FUNCMEM;
    ch[i].mem = i < 2 ? 'x' + i : 0;
    strcpy(ch[i].command, COMMAND);
    ch[i].pid = 0;
  }
}

/* scale num upward like a scope does, 1 to 100 */
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
  else
    *num = 100;
}

/* scale num downward like a scope does, 100 to 1 */
void
scaledown(int *num)
{
  if (*num > 50)
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

/* internal only */
void
setsoundcard(int rate)
{
  scope.rate = set_sound_card(rate);
  mem[23].rate = mem[24].rate = scope.rate;
  do_math();			/* propogate new rate to any math */
}

/* internal only, change rate and propogate it everywhere */
void
resetsoundcard()
{
  scope.rate = reset_sound_card(scope.rate, 2, 8);
  mem[23].rate = mem[24].rate = scope.rate;
  do_math();			/* propogate new rate to any math */
  draw_text(1);
}

/* handle single key commands */
void
handle_key(unsigned char c)
{
  static Channel *p;
  char *s;

  p = &ch[scope.select];
  if (c >= 'A' && c <= 'Z') {
    save(c);			/* store channel */
    draw_text(1);
    return;
  } else if (c >= 'a' && c <= 'z') {
    recall(c);			/* recall signal */
    clear();
    return;
  } else if (c >= '1' && c <= '0' + CHANNELS) {
    scope.select = (c - '1');	/* select channel */
    clear();
    return;
  }
  switch (c) {
  case 0:
  case -1:			/* no key pressed */
    break;
  case '\t':
    p->show = !p->show;		/* show / hide channel */
    clear();
    break;
  case '}':
    if (p->div > 1)		/* increase scale */
      scaledown(&p->div);
    else
      scaleup(&p->mult);
    clear();
    break;
  case '{':
    if (p->mult > 1)		/* decrease scale */
      scaledown(&p->mult);
    else
      scaleup(&p->div);
    clear();
    break;
  case ']':
    p->pos -= 16;		/* position up */
    clear();
    break;
  case '[':
    p->pos += 16;		/* position down */
    clear();
    break;
  case ';':
    if (scope.select > 1) {	/* next math function */
      p->func++;
      if (p->func >= funccount || p->func < FUNC0)
	p->func = FUNC0;
      clear();
    }
    break;
  case ':':
    if (scope.select > 1) {	/* previous math function */
      p->func--;
      if (p->func < FUNC0)
	p->func = funccount - 1;
      clear();
    }
    break;
  case '0':
    if (scope.div > 1)		/* decrease time scale, zoom in */
      scaledown(&scope.div);
    else
      scaleup(&scope.scale);
    clear();
    break;
  case '9':
    if (scope.scale > 1)	/* increase time scale, zoom out */
      scaledown(&scope.scale);
    else
      scaleup(&scope.div);
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
  case '_':			/* change trigger channel */
    scope.trigch = !scope.trigch;
    clear();
    break;
  case '+':
    scope.trige++;		/* change trigger type */
    if (scope.trige > 2)
      scope.trige = 0;
    clear();
    break;
  case '(':
    if (scope.run)		/* decrease sample rate */
      if (scope.rate <= 11000)
	setsoundcard(8800);
      else if (scope.rate <= 22000)
	setsoundcard(11000);
      else
	setsoundcard(22000);
    clear();
    break;
  case ')':
    if (scope.run)		/* increase sample rate */
      if (scope.rate >= 22000)
	setsoundcard(44000);
      else if (scope.rate >= 11000)
	setsoundcard(22000);
      else
	setsoundcard(11000);
    clear();
    break;
  case '<':
    scope.color--;		/* decrease color */
    if (scope.color < 0)
      scope.color = 15;
    draw_text(1);
    break;
  case '>':
    scope.color++;		/* increase color */
    if (scope.color > 15)
      scope.color = 0;
    draw_text(1);
    break;
  case '@':			/* load file */
    if ((s = GetFile(NULL)) != NULL) {
      readfile(s);
      resetsoundcard();
    }
    break;
  case '#':			/* save file */
    if ((s = GetFile(NULL)) != NULL)
      writefile(s);
    break;
  case '$':			/* run external math */
    if (scope.select > 1) {
      if ((s = GetString("External command and args:",
			 ch[scope.select].command)) != NULL) {
	strcpy(ch[scope.select].command, s);
	ch[scope.select].func = FUNCEXT;
	ch[scope.select].mem = EXTSTART;
	clear();
      }
    }
    break;
  case '!':
    scope.mode++;		/* point, point accumulate, line, line acc. */
    if (scope.mode > 3)
      scope.mode = 0;
    clear();
    break;
  case ',':
    scope.grat++;		/* graticule off/on/more */
    if (scope.grat > 2)
      scope.grat = 0;
    clear();
    break;
  case '.':
    scope.behind = !scope.behind; /* graticule behind/in front of signal */
    draw_text(1);
    break;
  case '?':
  case '/':
    scope.verbose = !scope.verbose; /* on-screen help */
    clear();
    break;
  case ' ':
    scope.run++;		/* run / wait / stop */
    if (scope.run > 2)
      scope.run = 0;
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

/* main program */
int
main(int argc, char **argv)
{

  progname = strrchr(argv[0], '/');
  if (progname == NULL)
    progname = argv[0];		/* global for error messages, usage */
  else
    progname++;
  init_scope();
  init_channels();
  init_math();
  if ((argc = OpenDisplay(argc, argv)) == FALSE)
    exit(1);
  parse_args(argc, argv);
  init_widgets();
  init_screen();
  filename = FILENAME;
  if (optind < argc)
    if (argv[optind] != NULL) {
      filename = argv[optind];
      readfile(filename);
    }
  open_sound_card(scope.dma);
  resetsoundcard();
  mainloop();			/* to display.c */
  cleanup();
  exit(0);
}

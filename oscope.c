/*
 * @(#)$Id: oscope.c,v 1.89 2001/05/20 03:59:33 twitham Exp $
 *
 * Copyright (C) 1996 - 2001 Tim Witham <twitham@quiknet.com>
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
#include "oscope.h"		/* program defaults */
#include "display.h"		/* display routines */
#include "func.h"		/* signal math functions */
#include "file.h"		/* file I/O functions */
#include "proscope.h"		/* ProbeScope (serial) functions */
#include "bitscope.h"		/* BitScope (serial) functions */

/* global program structures */
Scope scope;
Channel ch[CHANNELS];

/* extra global variable definitions */
char *progname;			/* the program's name, autoset via argv[0] */
char version[] = VERSION;	/* version of the program, from Makefile */
char error[256];		/* buffer for "one-line" error messages */
int quit_key_pressed = 0;	/* set by handle_key() */
int v_points;			/* pixels in vertical axis */
int h_points;			/* pixels in horizontal axis */
int offset;			/* vertical pixel offset to zero line */
int clip = 0;			/* whether we're maxed out or not */
char *filename;			/* default file name */
int in_progress = 0;		/* parallel data collection in progress? */

extern void open_sound_card();
extern void close_sound_card();
extern int reset_sound_card();

/* display command usage on stdout or stderr and exit */
void
usage(int error)
{
  static char *def[] = {"graticule", "signal"}, *onoff[] = {"on", "off"};
  FILE *where;

  where = error ? stderr : stdout;

  fprintf(where, "usage: %s [options] [file]

Startup Options  Description (defaults)               version %s
-h               this Help message and exit
-# <code>        #=1-%d, code=pos[.bits][:scale[:func#, mem a-z or cmd]] (0:1/1)
-a <channel>     set the Active channel: 1-%d                  (%d)
-r <rate>        sampling Rate in Hz: 8000,11025,22050,44100  (%d)
-s <scale>       time Scale: 1/2000-1000/1 where 1=1ms/div    (%d/1)
-t <trigger>     Trigger level[:type[:channel]]               (%s)
-l <cursors>     cursor Line positions: first[:second[:on?]]  (%s)
-c <color>       graticule Color: 0-15                        (%d)
-d <dma divisor> sound card DMA buffer size divisor: 1,2,4    (%d)
-m <mode>        video Mode (size): 0,1,2,3                   (%d)
-f <font name>   the Font name as-in %s
-p <type>        Plot mode: 0/1=point, 2/3=line, 4/5=step     (%d)
-g <style>       Graticule: 0=none,  1=minor, 2=major         (%d)
-b               %s Behind instead of in front of %s
-v               turn Verbose key help display %s
-x               turn sound card (XY) input device %s
-z               turn SerialScope (Z) input device %s
file             %s file to load to restore settings and memory
",
	  progname, version, CHANNELS, CHANNELS, DEF_A,
	  DEF_R, DEF_S, DEF_T, DEF_L,
	  DEF_C, scope.dma, scope.size,
	  fonts,		/* the font method for the display */
	  scope.mode,
	  scope.grat, def[DEF_B], def[!DEF_B],
	  onoff[DEF_V], onoff[!DEF_X], onoff[!DEF_Z], progname);
  exit(error);
}

/* handle command line options */
void
parse_args(int argc, char **argv)
{
  const char     *flags = "Hh"
    "1:2:3:4:5:6:7:8:"
    "a:r:s:t:l:c:m:d:f:p:g:bvxyz"
    "A:R:S:T:L:C:M:D:F:P:G:BVXYZ";
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
  cleanup_serial();
}

/* initialize the scope */
void
init_scope()
{
  int i;

  scope.size = DEF_M;
  scope.dma = DEF_D;
  scope.mode = DEF_P;
  scope.scale = DEF_S;
  scope.div = 1;
  scope.rate = DEF_R;
  handle_opt('t', DEF_T);
  handle_opt('l', DEF_L);
  scope.grat = DEF_G;
  scope.behind = DEF_B;
  scope.run = 1;
  scope.color = DEF_C;
  scope.select = DEF_A - 1;
  scope.verbose = DEF_V;
  snd = DEF_X ? 0 : -1;
  ps.found = bs.found = !DEF_Z;
  for (i = 0; i < sizeof(bs.R) / sizeof(bs.R[0]); i++) {
    bs.R[i] = -1;
  }
}

/* initialize the signals */
void
init_channels()
{
  int i, j[] = {23, 24, 25, 0, 0, 0, 0, 0};
  static int first = 1;

  for (i = 0 ; i < CHANNELS ; i++) {
    if (ch[i].pid)
      ch[i].mem = EXTSTOP;
  }
  if (!first)
    do_math();			/* halt currently running commands */
  first = 0;
  for (i = 0 ; i < CHANNELS ; i++) {
    ch[i].signal = &mem[j[i]];
    memset(ch[i].old, 0, MAXWID * sizeof(short));
    ch[i].mult = 1;
    ch[i].div = 1;
    ch[i].pos = 0;
    ch[i].show = (i < FUNCMEM);
    ch[i].func = i < FUNCMEM ? i : FUNCMEM;
    ch[i].mem = i < FUNCMEM ? 'x' + i : 0;
    strcpy(ch[i].command, COMMAND);
    ch[i].pid = 0;
    ch[i].bits = 0;
  }
}

/* samples needed to draw a current display of RATE */
int
samples(int rate)
{
  static unsigned long int r;
  r = (rate * ((h_points - 200) / 44) * scope.div / scope.scale / 1000) + 2;
  if (r > MAXWID) r = MAXWID;
  return (r);
}

/* scale num upward like a scope does, to max */
int
scaleup(int num, int max)
{
  int i;

  i = num;
  while (!(i % 10)) {
    i /= 10;
  }
  if (i == 2) num = num * 5 / 2;
  else num *= 2;
  if (num > max) num = max;
  return(num);
}

/* scale num downward like a scope does */
int
scaledown(int num)
{
  int i;

  i = num;
  while (!(i % 10)) {
    i /= 10;
  }
  if (i == 5) num = num * 2 / 5;
  else num /= 2;
  if (num < 1) num = 1;
  return(num);
}

/* internal only, change rate and propogate it everywhere */
void
resetsoundcard(int rate)
{
  scope.rate = mem[23].rate = mem[24].rate = reset_sound_card(rate, 2, 8);
  mem[23].volts = mem[24].volts = 0;
  do_math();			/* propogate new rate to any math */
  draw_text(1);
}

/* gr_* UIs call this after selecting file and confirming overwrite */
void
savefile(char *file)
{
  writefile(filename = file);
}

/* gr_* UIs call this after selecting file to load */
void
loadfile(char *file)
{
  close_sound_card();
  readfile(filename = file);
  if (snd)
    resetsoundcard(scope.rate);
  if (ps.found || bs.found) {
    init_probescope();
    init_serial();
  }
}

/* gr_* UIs call this after prompting for command to run */
void
startcommand(char *command)
{
  if (scope.select > 1) {
    strcpy(ch[scope.select].command, command);
    ch[scope.select].func = FUNCEXT;
    ch[scope.select].mem = EXTSTART;
    clear();
  }
}

/* handle single key commands */
void
handle_key(unsigned char c)
{
  static Channel *p;
  static int s;

  p = &ch[scope.select];
  s = samples(p->signal->rate);
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
  case '\'':			/* toggle manual cursors */
    scope.curs = ! scope.curs;
    clear();
    break;
  case '"':			/* reset cursors to first sample */
    scope.cursa = scope.cursb = 1;
    clear();
    break;
  case 'q' - 96:		/* -96 is CTRL keys */
    if ((scope.cursa -= s / 20) < 1)
      scope.cursa = s - 1;
    break;
  case 'w' - 96:
    if ((scope.cursa += s / 20) >= s)
      scope.cursa = 1;
    break;
  case 'e' - 96:
    if (--scope.cursa < 1)
      scope.cursa = s - 1;
    break;
  case 'r' - 96:
    if (++scope.cursa >= s)
      scope.cursa = 1;
    break;
  case 'a' - 96:
    if ((scope.cursb -= s / 20) < 1)
      scope.cursb = s - 1;
    break;
  case 's' - 96:
    if ((scope.cursb += s / 20) >= s)
      scope.cursb = 1;
    break;
  case 'd' - 96:
    if (--scope.cursb < 1)
      scope.cursb = s - 1;
    break;
  case 'f' - 96:
    if (++scope.cursb >= s)
      scope.cursb = 1;
    break;
  case '\t':
    p->show = !p->show;		/* show / hide channel */
    clear();
    break;
  case '~':
    if ((p->bits += 2) > 16)
      p->bits = 0;
    clear();
    break;
  case '`':
    if ((p->bits -= 2) < 0)
      p->bits = 16;
    clear();
    break;
  case '}':
    if (p->div > 1)		/* increase scale */
      p->div = scaledown(p->div);
    else
      p->mult = scaleup(p->mult, 50);
    clear();
    break;
  case '{':
    if (p->mult > 1)		/* decrease scale */
      p->mult = scaledown(p->mult);
    else
      p->div = scaleup(p->div, 50);
    clear();
    break;
  case ']':
    p->pos -= 16;		/* position up */
    if (p->pos < -1 * v_points / 2)
      p->pos = v_points / 2;
    clear();
    break;
  case '[':
    p->pos += 16;		/* position down */
    if (p->pos > v_points / 2)
      p->pos = -1 * v_points / 2;
    clear();
    break;
  case ';':
    if (scope.select > 1) {	/* next math function */
      p->func++;
      if (p->func >= funccount || p->func < FUNC0)
	p->func = FUNC0;
      clear();
    } else
      message("Math can not run on Channel 1 or 2", KEY_FG);
    break;
  case ':':
    if (scope.select > 1) {	/* previous math function */
      p->func--;
      if (p->func < FUNC0)
	p->func = funccount - 1;
      clear();
    } else
      message("Math can not run on Channel 1 or 2", KEY_FG);
    break;
  case '0':
    if (scope.div > 1)		/* decrease time scale, zoom in */
      scope.div = scaledown(scope.div);
    else
      scope.scale = scaleup(scope.scale, 5000);
    clear();
    break;
  case '9':
    if (scope.scale > 1)	/* increase time scale, zoom out */
      scope.scale = scaledown(scope.scale);
    else
      scope.div = scaleup(scope.div, 2000);
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
      scope.trig = 248;
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
    if (bs.found)
      bs_changerate(bs.fd, -1);
    else if (scope.run) {	/* decrease sample rate */
      if (scope.rate < 16500)
	resetsoundcard(8000);
      else if (scope.rate < 33000)
	resetsoundcard(11025);
      else
	resetsoundcard(22050);
    }
    in_progress = 0;
    clear();
    break;
  case ')':
    if (bs.found)
      bs_changerate(bs.fd, 1);
    else if (scope.run) {	/* increase sample rate */
      if (scope.rate > 16500)
	resetsoundcard(44100);
      else if (scope.rate > 9500)
	resetsoundcard(22050);
      else
	resetsoundcard(11025);
    }
    in_progress = 0;
    clear();
    break;
  case '<':
    if (--scope.color < 0)	/* decrease color */
      scope.color = 15;
    draw_text(1);
    break;
  case '>':
    if (++scope.color > 15)	/* increase color */
      scope.color = 0;
    draw_text(1);
    break;
  case '@':			/* load file */
    LoadSaveFile(0);
    break;
  case '#':			/* save file */
    LoadSaveFile(1);
    break;
  case '$':			/* run external math */
    if (scope.select > 1)
      ExternCommand();
    else
      message("External commands can not run on Channel 1 or 2", KEY_FG);
    break;
  case '^':
    if (ps.found || bs.found) {	/* toggle Serial Scope on/off */
      ps.found = bs.found = 0;
      funcnames[0] = "Left  Mix";
      funcnames[1] = "Right Mix";
      funcnames[2] = "ProbeScope";
    } else {
      init_probescope();
      init_serial();
    }
    clear();
    break;
  case '&':
    if (snd)			/* toggle sound card on/off */
      close_sound_card();
    else
      resetsoundcard(scope.rate);
    clear();
    break;
  case '*':
    scope.dma >>= 1;
    if (scope.dma < 1)		/* DMA */
      scope.dma = 4;
    if (snd)
      resetsoundcard(scope.rate);
    clear();
    break;
  case '!':
    scope.mode++;		/* point, point accumulate, line, line acc. */
    if (scope.mode > 5)
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
  if (snd)
    resetsoundcard(scope.rate);
  if (ps.found || bs.found) {
    init_probescope();
    init_serial();
  }
  clear();
  mainloop();			/* to display.c */
  cleanup();
  exit(0);
}

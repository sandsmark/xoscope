/*
 * @(#)$Id: func.c,v 1.25 2000/07/10 23:36:51 twitham Exp $
 *
 * Copyright (C) 1996 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the signal math and memory.
 * To add math functions, search for !!! and add to those sections.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "oscope.h"
#include "fft.h"
#include "display.h"
#include "func.h"

/* !!! The function names, the first five are special */
char *funcnames[] =
{
  "Left  Mix",
  "Right Mix",
  "ProbeScope",
  "Memory  ",
  "External",
  "Inv. 1  ",
  "Inv. 2  ",
  "Sum  1+2",
  "Diff 1-2",
  "Avg. 1,2",
  "FFT. 1  ",
  "FFT. 2  ",
};

/* the total number of "functions" */
int funccount = sizeof(funcnames) / sizeof(char *);

int total_samples;

/* 0-22=A-W=memories, 23=X=left, 24=Y=right, 25=Z=ProbeScope, 26-33=functions */
Signal mem[34];

/* recall given memory register to the currently selected signal */
void
recall(char c)
{
  ch[scope.select].signal = &mem[c - 'a'];
  ch[scope.select].func = c >= 'x' ? c - 'x' : FUNCMEM;
  ch[scope.select].mem = c;
}

/* store the currently selected signal to the given memory register */
void
save(char c)
{
  static int i, j, k;

  i = c - 'A';
  if (c < 'X') {
    memcpy(mem[i].data, ch[scope.select].signal->data, MAXWID * sizeof(short));
    mem[i].rate = ch[scope.select].signal->rate;
    mem[i].num = ch[scope.select].signal->num;
    mem[i].color = ch[scope.select].color;
  }
  k = scope.select;
  for (j = 0 ; j < CHANNELS ; j++) { /* propogate to any display channels */
    scope.select = j;
    if (ch[j].mem - 'a' == i )
      recall(i + 'a');
  }
  scope.select = k;
}

/* Pipe through external process */
void
pipeto(int num)
{
  static short *a, *b, *c;
  static int i, j;
  static int to[CHANNELS][2], from[CHANNELS][2]; /* pipes to/from the child */
  static char *path, *oscopepath;

  if (ch[num].mem == EXTSTART || ch[num].mem == EXTSTOP) {
    if (ch[num].pid > 0) {	/* close previous command pipes */
      close(to[num][1]);
      close(from[num][0]);	/* and reap the child process */
      waitpid(ch[num].pid, NULL, 0);
      ch[num].pid = 0;
    }
    if (ch[num].mem == EXTSTOP)
      return;
    if (pipe(to[num]) || pipe(from[num])) { /* get a set of pipes */
      sprintf(error, "%s: can't create pipes", progname);
      perror(error);
      return;
    }
    signal(SIGPIPE, SIG_IGN);
    if ((ch[num].pid = fork()) > 0) { /* parent */
      close(to[num][0]);
      close(from[num][1]);
    } else if (ch[num].pid == 0) { /* child */
      close(to[num][1]);
      close(from[num][0]);
      close(0);
      close(1);			/* redirect stdin/out through pipes */
      dup2(to[num][0], 0);
      dup2(from[num][1], 1);
      close(to[num][0]);
      close(from[num][1]);

      if ((oscopepath = getenv("OSCOPEPATH")) == NULL)
	oscopepath = LIBPATH;	/* -D defined in Makefile */
      if ((path = malloc(strlen(oscopepath) + 6)) != NULL) {
	sprintf(path,"PATH=%s", oscopepath);
	putenv(path);
      }

      execlp("/bin/sh", "sh", "-c", ch[num].command, NULL);
      sprintf(error, "%s: child can't exec /bin/sh -c \"%s\"",
	      progname, ch[num].command);
      perror(error);
      exit(1);
    } else {			/* fork error */
      sprintf(error, "%s: can't fork", progname);
      perror(error);
      return;
    }
    message(ch[num].command, ch[num].color);
    ch[num].mem = EXTRUN;
  }
  if (ch[num].mem == EXTRUN) {	/* write to / read from child process */
    a = ch[0].signal->data;
    b = ch[1].signal->data;
    c = ch[num].signal->data;
    j = 0;
    for (i = 0 ; i < h_points ; i++) {
      if (write(to[num][1], a++, sizeof(short)) != sizeof(short))
	j++;
      if (write(to[num][1], b++, sizeof(short)) != sizeof(short))
	j++;
      if (read(from[num][0], c++, sizeof(short)) != sizeof(short))
	j++;
    }
    if (j) {
      sprintf(error, "%s: %d pipe r/w errors from \"%s\"",
	      progname, i, ch[num].command);
      perror(error);
      ch[num].mem = EXTSTOP;
    }
  }
}

/* !!! The functions; they take one arg: the channel # to store results in */

/* Invert */
void
inv(int num, int chan)
{
  static int i;
  static short *a, *b;

  a = ch[chan].signal->data;
  b = ch[num].signal->data;
  for (i = 0 ; i < total_samples ; i++) {
    *b++ = -1 * *a++;
  }
}

void
inv1(int num)
{
  inv(num, 0);
}

void
inv2(int num)
{
  inv(num, 1);
}

/* The sum of the two channels */
void
sum(int num)
{
  static int i;
  static short *a, *b, *c;

  a = ch[0].signal->data;
  b = ch[1].signal->data;
  c = ch[num].signal->data;
  for (i = 0 ; i < total_samples ; i++) {
    *c++ = *a++ + *b++;
  }
}

/* The difference of the two channels */
void
diff(int num)
{
  static int i;
  static short *a, *b, *c;

  a = ch[0].signal->data;
  b = ch[1].signal->data;
  c = ch[num].signal->data;
  for (i = 0 ; i < total_samples ; i++) {
    *c++ = *a++ - *b++;
  }
}

/* The average of the two channels */
void
avg(int num)
{
  static int i;
  static short *a, *b, *c;

  a = ch[0].signal->data;
  b = ch[1].signal->data;
  c = ch[num].signal->data;
  for (i = 0 ; i < total_samples ; i++) {
    *c++ = (*a++ + *b++) / 2;
  }
}

/* Fast Fourier Transform of channel sig */
void
fft1(int num)
{
  fft(ch[0].signal->data, ch[num].signal->data);
}

void
fft2(int num)
{
  fft(ch[1].signal->data, ch[num].signal->data);
}

/* !!! Array of the functions, the first five shouldn't be changed */
void (*funcarray[])(int) =
{
  NULL,				/* left */
  NULL,				/* right */
  NULL,				/* ProbeScope */
  NULL,				/* memory */
  pipeto,			/* external math commands */
  inv1,
  inv2,
  sum,
  diff,
  avg,
  fft1,
  fft2,
};

/* Initialize math, called once by main at startup */
void
init_math()
{
  static int i;

  for (i = 0 ; i < 34 ; i++) {
    mem[i].rate = 44100;
    memset(mem[i].data, 0, MAXWID * sizeof(short));
    mem[i].color = 0;
  }
  init_fft();
}

/* Perform any math on the software channels, called many times by main loop */
void
do_math()
{
  static int i, j;

  total_samples = samples(ch[0].signal->rate);
  for (i = 2 ; i < CHANNELS ; i++) {
    if (ch[i].pid && ch[i].func != FUNCEXT) { /* external needs halted */
      j = ch[i].mem;
      ch[i].mem = EXTSTOP;
      pipeto(i);
      ch[i].mem = j;
    }
    if ((ch[i].show || scope.select == i) && *funcarray[ch[i].func] != NULL) {
      ch[i].signal = &mem[26+i];
      ch[i].signal->rate = ch[0].signal->rate;
      ch[i].signal->num = ch[0].signal->num;
      funcarray[ch[i].func](i);
    }
  }
}

/* Perform any math cleanup, called once by cleanup at program exit */
void
cleanup_math()
{
  EndFFT();
}

/* measure the given channel */
void
measure_data(Channel *sig) {
  static int i, j, prev;
  int first = 0, last = 0, count = 0, max = 0;

  sig->min = 0;
  sig->max = 0;
  prev = 1;
  if (scope.curs) {		/* manual cursor measurements */
    if (scope.cursa < scope.cursb) {
      first = scope.cursa;
      last = scope.cursb;
    } else {
      first = scope.cursb;
      last = scope.cursa;
    }
    sig->min = sig->max = sig->signal->data[first];
    if ((j = sig->signal->data[last]) < sig->min)
      sig->min = j;
    else if (j > sig->max)
      sig->max = j;
    count = 2;
  } else {			/* automatic period measurements */
    for (i = 0 ; i < samples(sig->signal->rate) ; i++) {
      j = sig->signal->data[i];
      if (j < sig->min)		/* minimum */
	sig->min = j;
      if (j > sig->max) {	/* maximum */
	sig->max = j;
	max = i;
      }
      if (j > 0 && prev <= 0) {	/* locate and count rising edges */
	if (!first)
	  first = i;
	last = i;
	count++;
      }
      prev = j;
    }
  }
  if (funcarray[sig->func] == fft1 || funcarray[sig->func] == fft2) {
    if ((sig->freq = sig->signal->rate * max / 880) > 0) /* freq from peak FFT */
      sig->time = 1000000 / sig->freq;
    else
      sig->time = 0;
  } else if (count > 1) {	/* assume a wave: period = length / # periods */
    if ((sig->time = 1000000 * (last - first)
	 / (count - 1) / sig->signal->rate) > 0)
      sig->freq = 1000000 / sig->time;
    else
      sig->freq = 0;
  } else {			/* couldn't measure */
    sig->time = 0;
    sig->freq = 0;
  }
}

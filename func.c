/*
 * @(#)$Id: func.c,v 1.12 1996/08/06 04:56:42 twitham Exp $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
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
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "oscope.h"
#include "fft.h"
#include "display.h"
#include "func.h"

/* !!! The function names, the first four are special */
char *funcnames[] =
{
  "Left  Mix",
  "Right Mix",
  "External",
  "Memory  ",
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

/* the pointers to the signal memories */
short *mem[26] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL
};

/* the signal color that was recorded into the memory */
int memcolor[26];

/* for the fft function: x position to bin number map, and data buffer */
int xmap[MAXWID];
short fftdata[MAXWID];

/* the external commands and their PIDs */
char command[CHANNELS][256];
pid_t pid[] = {0, 0, 0, 0, 0, 0, 0, 0};

/* recall given memory register to the currently selected signal */
void
recall(char c)
{
  static int i;

  i = c - 'a';
  if (mem[i] != NULL) {
    memcpy(ch[scope.select].data, mem[i], MAXWID * sizeof(short));
    ch[scope.select].func = FUNCMEM;
    ch[scope.select].mem = c;
  }
}

/* store the currently selected signal to the given memory register */
void
save(char c)
{
  static int i, j, k;

  i = c - 'A';
  if (mem[i] == NULL)
    mem[i] = malloc(MAXWID * sizeof(short));
  if (mem[i] != NULL) {
    memcpy(mem[i], ch[scope.select].data, MAXWID * sizeof(short));
    memcolor[i] = ch[scope.select].color;
  }
  k = scope.select;
  for (j = 0 ; j < CHANNELS ; j++) {
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

  if (ch[num].mem == EXTSTART) {
    if (pid[num] > 0) {		/* close previous command pipes */
      close(to[num][1]);
      close(from[num][0]);	/* and reap the child process */
      waitpid(pid[num], NULL, 0);
    }
    if (pipe(to[num]) || pipe(from[num])) { /* get a set of pipes */
      sprintf(error, "%s: can't create pipes", progname);
      perror(error);
      return;
    }
    signal(SIGPIPE, SIG_IGN);
    if ((pid[num] = fork()) > 0) { /* parent */
	close(to[num][0]);
	close(from[num][1]);
    } else if (pid[num] == 0) {	/* child */
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

      execlp("/bin/sh", "sh", "-c", command[num], NULL);
      sprintf(error, "%s: child can't exec /bin/sh -c \"%s\"",
	      progname, command[num]);
      perror(error);
      exit(1);
    } else {			/* fork error */
      sprintf(error, "%s: can't fork", progname);
      perror(error);
      return;
    }
    message(command[num], ch[num].color);
    ch[num].mem = EXTRUN;
  }
  if (ch[num].mem == EXTRUN) {	/* write to / read from child process */
    a = ch[0].data;
    b = ch[1].data;
    c = ch[num].data;
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
	      progname, i, command[num]);
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

  a = ch[chan].data;
  b = ch[num].data;
  for (i = 0 ; i < h_points ; i++) {
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

  a = ch[0].data;
  b = ch[1].data;
  c = ch[num].data;
  for (i = 0 ; i < h_points ; i++) {
    *c++ = *a++ + *b++;
  }
}

/* The difference of the two channels */
void
diff(int num)
{
  static int i;
  static short *a, *b, *c;

  a = ch[0].data;
  b = ch[1].data;
  c = ch[num].data;
  for (i = 0 ; i < h_points ; i++) {
    *c++ = *a++ - *b++;
  }
}

/* The average of the two channels */
void
avg(int num)
{
  static int i;
  static short *a, *b, *c;

  a = ch[0].data;
  b = ch[1].data;
  c = ch[num].data;
  for (i = 0 ; i < h_points ; i++) {
    *c++ = (*a++ + *b++) / 2;
  }
}

/* Fast Fourier Transform of channel sig */
void
fft1(int num)
{
  fft(ch[0].data, ch[num].data);
}

void
fft2(int num)
{
  fft(ch[1].data, ch[num].data);
}

/* !!! Array of the functions, the first four shouldn't be changed */
void (*funcarray[])(int) =
{
  NULL,				/* left */
  NULL,				/* right */
  pipeto,			/* external math commands */
  NULL,				/* memory */
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

  for (i = 0 ; i < CHANNELS ; i++) {
    strcpy(command[i], COMMAND);
  }
  init_fft();
}

/* Perform any math on the software channels, called many times by main loop */
void
do_math()
{
  static int i;

  for (i = 2 ; i < CHANNELS ; i++) {
    if ((ch[i].show || scope.select == i) && *funcarray[ch[i].func] != NULL)
      funcarray[ch[i].func](i);
  }
}

/* Perform any math cleanup, called once by cleanup at program exit */
void
cleanup_math()
{
  int i;

  for (i = 0 ; i < 26 ; i++) {
    free(mem[i]);
  }
  EndFFT();
}

/* auto-measure the given signal */
void
measure_data(Signal *sig) {
  static int i, j, prev;
  int first = 0, last = 0, count = 0, max = 0;

  sig->min = 0;
  sig->max = 0;
  prev = 1;
  for (i = 0 ; i < h_points ; i++) {
    j = sig->data[i];
    if (j < sig->min)		/* minimum */
      sig->min = j;
    if (j > sig->max) {		/* maximum */
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
  if (funcarray[sig->func] == fft1 || funcarray[sig->func] == fft2) {
    if ((sig->freq = actual * max / 880) > 0) /* freq from peak FFT */
      sig->time = 1000000 / sig->freq;
    else
      sig->time = 0;
  } else if (count > 1) {	/* assume a wave: period = length / # periods */
    if ((sig->time = 1000000 * (last - first) / (count - 1) / actual) > 0)
      sig->freq = 1000000 / sig->time;
    else
      sig->freq = 0;
  } else {			/* couldn't measure */
    sig->time = 0;
    sig->freq = 0;
  }
}

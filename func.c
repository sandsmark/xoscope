/*
 * @(#)$Id: func.c,v 1.4 1996/02/03 21:08:48 twitham Exp $
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see oscope.c and the file COPYING for more details)
 *
 * This file implements the signal math and memory.
 * To add functions, search for !!! and add to those sections.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "oscope.h"

/* !!! The function names, the first three are special */
char *funcnames[] =
{
  "Left  In",
  "Right In",
  "Memory  ",
  "Sum  1+2",
  "Diff 1-2",
  "Avg. 1,2",
};

/* The total number of functions */
int funccount = sizeof(funcnames) / sizeof(char *);

/* The pointers to the signal memories */
short *mem[26] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL
};

/* The signal color that was recorded into the memory */
int memcolor[26];

/* store the currently selected signal to the given memory register */
void
save(char c)
{
  static int i;

  i = c - 'A';
  if (mem[i] == NULL)
    mem[i] = malloc(MAXWID);
  memcpy(mem[i], ch[scope.select].data, MAXWID);
  memcolor[i] = ch[scope.select].color;
}

/* recall given memory register to the currently selected signal */
void
recall(char c)
{
  static int i;

  i = c - 'a';
  if (mem[i] != NULL) {
    memcpy(ch[scope.select].data, mem[i], MAXWID);
    ch[scope.select].func = 2;
    ch[scope.select].mem = c;
  }
}

/* !!! The functions; they take one arg: the channel # to store results in */

/* The sum of the two channels */
void
sum(int num)
{
  static int i;
  static Signal *a, *b, *c;
  a = &ch[0];
  b = &ch[1];
  c = &ch[num];
  for (i = 0 ; i < h_points ; i++) {
    c->data[i] = a->data[i] + b->data[i];
  }
}

/* The difference of the two channels */
void
diff(int num)
{
  static int i;
  static Signal *a, *b, *c;
  a = &ch[0];
  b = &ch[1];
  c = &ch[num];
  for (i = 0 ; i < h_points ; i++) {
    c->data[i] = a->data[i] - b->data[i];
  }
}

/* The average of the two channels */
void
avg(int num)
{
  static int i;
  static Signal *a, *b, *c;
  a = &ch[0];
  b = &ch[1];
  c = &ch[num];
  for (i = 0 ; i < h_points ; i++) {
    c->data[i] = (a->data[i] + b->data[i]) / 2;
  }
}

/* !!! Array of the functions, the first three should be NULL */
void (*funcarray[])(int) =
{
  NULL,
  NULL,
  NULL,
  sum,
  diff,
  avg
};

/* Perform any math on the software channels */
void
do_math()
{
  static int i;

  for (i = 2 ; i < CHANNELS ; i++) {
    if (ch[i].func > 2)
      funcarray[ch[i].func](i);
  }
}

/*
 * @(#)$Id: func.c,v 1.1 1996/02/02 07:10:43 twitham Exp $
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see oscope.c and the file COPYING for more details)
 *
 * This file implements the internal math functions
 *
 */

#include <stdio.h>
#include "oscope.h"

/* The function names */
char *funcnames[] =
{
  "Left In",
  "Right In",
  "Sum  1+2",
  "Diff 1-2",
  "Avg. 1,2",
};

/* The total number of functions */
int funccount = sizeof(funcnames) / sizeof(char *);

/* The functions; they take one arg: the channel # to store results in */

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

/* Array of the functions */
void (*funcarray[])(int) =
{
  NULL,
  NULL,
  sum,
  diff,
  avg
};

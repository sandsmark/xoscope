/*
 * @(#)$Id: func.h,v 1.4 1996/02/03 21:08:54 twitham Exp $
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see oscope.c and the file COPYING for more details)
 *
 * oscope math function prototypes
 *
 */

extern int funccount;

extern char *funcnames[];

extern void (*funcarray[])(int);

extern short *mem[];

extern int memcolor[];

void
save(char);

void
recall(char);

void
do_math();

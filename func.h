/*
 * @(#)$Id: func.h,v 1.6 1996/02/24 04:30:11 twitham Exp $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see oscope.c and the file COPYING for more details)
 *
 * oscope math function prototypes
 *
 */

extern int funccount;

extern char *funcnames[];

extern short *mem[];

extern int memcolor[];

void
save(char);

void
recall(char);

void
init_math();

void
do_math();

void
cleanup_math();

void
measure_data(Signal *);

/*
 * @(#)$Id: func.h,v 1.11 1997/05/03 16:15:57 twitham Rel1_3 $
 *
 * Copyright (C) 1996 - 1997 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * oscope math function definitions and prototypes
 *
 */

#define FUNCLEFT  0
#define FUNCRIGHT 1
#define FUNCEXT	  2
#define FUNCMEM	  3
#define FUNC0	  4

#define EXTSTOP  0
#define EXTSTART 1
#define EXTRUN   2

extern int funccount;

extern char *funcnames[];

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
measure_data(Channel *);

void
init_fft();			/* in fft.c */

void
fft();				/* in fft.c */

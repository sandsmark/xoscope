/*
 * @(#)$Id: func.h,v 1.8 1996/04/21 02:31:28 twitham Rel1_0 $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * oscope math function definitions and prototypes
 *
 */

#define FUNCEXT  2
#define FUNCMEM  3
#define FUNCFFT1 4
#define FUNCFFT2 5

#define EXTSTOP  0
#define EXTSTART 1
#define EXTRUN   2

extern int funccount;

extern char *funcnames[];

extern short *mem[];

extern int memcolor[];

extern char command[CHANNELS][256];

extern int xmap[];		/* in oscopefft.c and func.c */

extern short fftdata[];		/* in oscopefft.c and func.c */

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

void
init_fft();			/* in fft.c */

void
fft();				/* in fft.c */

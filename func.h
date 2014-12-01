/* -*- mode: C++; fill-column: 100; c-basic-offset: 4; -*-
 *
 * @(#)$Id: func.h,v 2.2 2009/07/30 02:37:25 baccala Exp $
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
#define FUNCPS	  2
#define FUNCMEM	  3
#define FUNCEXT	  4
#define FUNC0	  5

#define EXTSTOP  0
#define EXTSTART 1
#define EXTRUN   2

struct signal_stats {
    short min;			/* Minimum signal value */
    short max;			/* Maximum signal value */
    int time;
    int freq;
};

void save(char);
void recall_on_channel(Signal *, Channel *);
void recall(Signal *);

void next_func(void);
void prev_func(void);
int function_bynum_on_channel(int, Channel *);

void start_command_on_channel(const char *, Channel *);
void startcommand(const char *);
void restart_command_on_channel(void);

void init_math();

int update_math_signals();

void do_math();

void cleanup_math();

void measure_data(Channel *, struct signal_stats *);

void init_fft(void);			    /* in fft.c */

void fft(short *, short *, int);	/* in fft.c */

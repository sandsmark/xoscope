/*
 * @(#)$Id: ser_dos.c,v 1.2 2000/07/03 18:18:14 twitham Exp $
 *
 * Copyright (C) 1997 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the DOS (DJGPP/SVAsync) serial interface to ProbeScope.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <pc.h>
#include <conio.h>
#include <unistd.h>
#include "svasync.h"
#include "oscope.h"
#include "proscope.h"
#include "bitscope.h"

/* stuff I need from svasync.c */
#define BufSize 32768                   /* Buffer Size */
extern unsigned volatile char RecBuffer[];
extern unsigned volatile int RecHead, RecTail;

int speed[] = {57600, 19200};
int flags[] = {BITS_8 | STOP_1 | NO_PARITY, BITS_7 | STOP_1 | NO_PARITY};

/* read one byte and return 1 or return -1 if none available */
/* SVAsyncIn won't work since 0x00 is indistinguishable from EOF */
int
readSVAsyncIn(unsigned char *ch) {

  if(RecTail == RecHead)
    return -1;

  disable();
  *ch = RecBuffer[RecTail++];
  if(RecTail >= BufSize)
    RecTail = 0;
  enable();
  return 1;
}

/* return a single byte from the serial device or return -1 if none avail. */
int
getonebyte()
{
  static unsigned char ch;

  if (readSVAsyncIn(&ch) == 1)
    return(ch);
  return(-1);
}

/* discard all input, clearing the serial FIFO queue to catch up */
void
flush_serial()
{
  while (getonebyte() > -1) {
  }
  SVAsyncClear();
}

/* serial cleanup routine called as the program exits */
void
cleanup_serial()
{
  SVAsyncStop();
}

/* check given serial device for BitScope (2), ProbeScope (1) or nothing (0) */
int
findscope(int dev)
{
  int i, r;

  for (i = 0; i < 2; i++) {	/* look for BitScope or ProbeScope */
    if(SVAsyncInit(dev)) {
      sprintf(error, "%s: can't open COM%d", progname, dev + 1);
      perror(error);
      return(0);
    }
    SVAsyncFifoInit();

    SVAsyncSet(speed[i], flags[i]);

    flush_serial();
    if ((r = idscope(i)))	/* serial port scope found! */
      return(r);
  }
  return(0);			/* nothing found. */
}

/* set [scope].found to non-zero if we find a scope on a serial port */
void
init_serial()
{
  int dev[] = {COM1, COM2, COM3, COM4};
  int i, num = sizeof(dev) / sizeof(int);

  for (i = 0; i < num; i++) {	/* look in all places specified in config.h */
    if (findscope(dev[i]))
      i = num;			/* done; exit the loop */
  }
}

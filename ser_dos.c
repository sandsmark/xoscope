/*
 * @(#)$Id: ser_dos.c,v 1.1 1997/06/07 21:30:40 twitham Rel $
 *
 * Copyright (C) 1997 Tim Witham <twitham@pcocd2.intel.com>
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

/* stuff I need from svasync.c */
#define BufSize 32768                   /* Buffer Size */
extern unsigned volatile char RecBuffer[];
extern unsigned volatile int RecHead, RecTail;

int speed=19200, bits=BITS_7, stopbits=STOP_1, parity=NO_PARITY;

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

/* return 1 if we find a ProbeScope on the given serial device */
int
findscope(int dev)
{
  int c, byte = 0, try = 0, n = 0;

  if(SVAsyncInit(dev)) {
    sprintf(error, "%s: can't open COM%d", progname, dev + 1);
    perror(error);
    return(0);
  }
  SVAsyncFifoInit();

  SVAsyncSet(speed, bits | stopbits | parity);

  flush_serial();
  while (byte < 300 && try < 25) { /* give up in 2.5ms */
    if ((c = getonebyte()) < 0) {
      microsleep(100);		/* try again in 0.1ms */
      try++;
    } else if (c == 0x7f && ++n > 1)
      return(1);		/* ProbeScope found! */
    else
      byte++;
  }
  return(0);			/* no ProbeScope found. */
}

/* set ps.found to non-zero if we find ProbeScope on a serial port */
void
init_serial()
{
  int dev[] = {COM1, COM2, COM3, COM4};
  int i, num = sizeof(dev) / sizeof(int);

  for (i = 0; i < num; i++) {	/* look in all places specified in config.h */
    if (findscope(dev[i])) {
      i = ps.found = num;	/* done; exit the loop */
    }
  }
}

/*
 * @(#)$Id: ser_dos.c,v 1.3 2000/07/07 02:39:13 twitham Rel $
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

int speed[] = {57600, 19200};
int flags[] = {BITS_8 | STOP_1 | NO_PARITY, BITS_7 | STOP_1 | NO_PARITY};

/* emulate a unix read(2) to the serial port (fd is ignored) */
ssize_t
serial_read(int fd, void *buf, size_t count)
{
  int i = 0;
  unsigned char *pos = buf;

  while (i < count) {
    if (SVAsyncInStat() >= 1) {
      *pos = SVAsyncIn();
      pos++;
      i++;
    } else
      break;
  }
  return(i);
}

/* emulate a unix write(2) to the serial port (fd is ignored) */
ssize_t
serial_write(int fd, void *buf, size_t count)
{
  int i = 0;
  unsigned char *pos = buf;

  while (i < count) {
    if (SVAsyncOutStat())
      return(-1);
    SVAsyncOut(*pos);
    pos++;
    i++;
  }
  return(i);
}

/* return a single byte from the serial device or return -1 if none avail. */
int
getonebyte()
{
  static unsigned char ch;

  if (SVAsyncInStat() >= 1)
    return(SVAsyncIn());
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

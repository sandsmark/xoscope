/*
 * @(#)$Id: ser_dos.c,v 2.0 2008/12/17 17:35:46 baccala Exp $
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
getonebyte(int fd)
{
  static unsigned char ch;

  if (SVAsyncInStat() >= 1)
    return(SVAsyncIn());
  return(-1);
}

/* discard all input, clearing the serial FIFO queue to catch up */
void
flush_serial(int fd)
{
  while (getonebyte(fd) > -1) {
  }
  SVAsyncClear();
}

/* serial cleanup routine called as the program exits */
void
cleanup_serial(int fd)
{
  SVAsyncStop();
}

/* check given serial device for BitScope (0) or ProbeScope (1) */
int
findscope(int dev, int i)
{

  if(SVAsyncInit(dev)) {
    sprintf(error, "%s: can't open COM%d", progname, dev + 1);
    perror(error);
    return(0);
  }
  SVAsyncFifoInit();

  SVAsyncSet(speed[i], flags[i]);

  flush_serial(0);
  if ((i && idprobescope(0)) || (!i && idbitscope(0))) {
    return (1);
  }

  return(0);			/* nothing found. */
}

int
init_serial_probescope()
{
  int dev[] = {COM1, COM2, COM3, COM4};
  int i, num = sizeof(dev) / sizeof(int);

  for (i = 0; i < num; i++) {	/* look in all places specified in config.h */
    if (findscope(dev[i], 1))
      return 1;
  }

  return 0;
}

int
init_serial_bitscope()
{
  int dev[] = {COM1, COM2, COM3, COM4};
  int i, num = sizeof(dev) / sizeof(int);

  for (i = 0; i < num; i++) {	/* look in all places specified in config.h */
    if (findscope(dev[i], 0))
      return 1;
  }

  return 0;
}

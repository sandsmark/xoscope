/*
 * @(#)$Id: ser_unix.c,v 1.5 2000/07/03 18:18:14 twitham Exp $
 *
 * Copyright (C) 1997 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the unix (Linux) serial interface to scopes.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "oscope.h"
#include "proscope.h"
#include "bitscope.h"

char device[512] = "";		/* Serial device */
int psfd = 0;			/* file descriptor */
int sflags[2];
struct termio stbuf[2], svbuf; /* termios: svbuf=saved, stbuf=set */

/* return a single byte from the serial device or return -1 if none avail. */
int
getonebyte()
{
  static unsigned char ch;

  if (read(psfd, &ch, 1) == 1)
    return(ch);
  return(-1);
}

/* return a single byte from the serial device or return -1 if none avail. */
int
GETONEBYTE()			/* we buffer here just to be safe */
{
  static unsigned char buff[256];
  static int count = 0, pos = 0;

  if (pos >= count) {
    if (psfd)
      if ((count = read(psfd, buff, 256 * sizeof(unsigned char))) < 1)
	return(-1);
    pos = 0;
  }
  return buff[pos++];
}

/* discard all input, clearing the serial FIFO queue to catch up */
void
flush_serial()
{
  while (getonebyte() > -1) {
  }
}

/* serial cleanup routine called as the program exits: restore settings */
void
cleanup_serial()
{
  if (psfd > 0) {
    if (ioctl(psfd, TCSETA, &svbuf) < 0) {
      sprintf(error, "%s: can't ioctl set device %s", progname, device);
      perror(error);
    }
    close(psfd);
  }
}

/* check given serial device for BitScope (2), ProbeScope (1) or nothing (0) */
int
findscope(char *dev)
{
  int i, r;

  for (i = 0; i < 2; i++) {	/* look for BitScope or ProbeScope */
    if ((psfd = bs.fd = open(dev, sflags[i])) < 0) {
      sprintf(error, "%s: can't open device %s", progname, dev);
      perror(error);
      return(0);
    }
    if (ioctl(psfd, TCGETA, &svbuf) < 0) { /* save settings */
      sprintf(error, "%s: can't ioctl get device %s", progname, dev);
      perror(error);
      close(psfd);
      return(0);
    }
    if (ioctl(psfd, TCSETA, &stbuf[i]) < 0) {
      sprintf(error, "%s: can't ioctl set device %s", progname, dev);
      perror(error);
      close(psfd);
      return(0);
    }
    if ((r = idscope(i)))	/* serial port scope found! */
      return(r);
    if (ioctl(psfd, TCSETA, &svbuf) < 0) { /* restore settings */
      sprintf(error, "%s: can't ioctl set device %s", progname, dev);
      perror(error);
      close(psfd);
      return(0);		/* nothing found. */
    }
  }
  return(0);			/* no scope found. */
}

/* set [scope].found to non-zero if we find a scope on a serial port */
void
init_serial()			/* prefer BitScope over ProbeScope */
{
  char *dev[] = PSDEVS, *p;
  int i, j = 0, num = sizeof(dev) / sizeof(char *);

  /* BitScope serial port settings */
  sflags[0] = O_RDWR | O_NDELAY | O_NOCTTY;
  memset(&stbuf[0], 0, sizeof(stbuf[0]));
  stbuf[0].c_cflag = B57600 | CS8 | CLOCAL | CREAD;
  stbuf[0].c_iflag = IGNPAR;	/* | ICRNL; (this would hose binary dumps) */

  /* ProbeScope serial port settings */
  sflags[1] = O_RDONLY | O_NDELAY;
  memset(&stbuf[1], 0, sizeof(stbuf[1]));
  stbuf[1].c_cflag = B19200 | CS7 | CLOCAL | CREAD;
  stbuf[1].c_iflag = ISTRIP;

  cleanup_serial();		/* close current device first, if any */
  if ((p = getenv("BITSCOPE")) == NULL) /* first place to look */
    p = BITSCOPE;		/* -D default defined in Makefile */
  dev[0] = p;
  if ((p = getenv("PROBESCOPE")) == NULL) /* second place to look */
    p = PROBESCOPE;		/* -D default defined in Makefile */
  dev[1] = p;
  for (i = 0; i < num; i++) {	/* look in all places specified in config.h */
    if ((j = findscope(dev[i]))) {
      strcpy(device, dev[i]);
      i = num;			/* done; exit the loop */
    }
  }
  if (!j) psfd = 0;
}

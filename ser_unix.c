/*
 * @(#)$Id: ser_unix.c,v 1.4 1999/08/29 02:14:05 twitham Rel $
 *
 * Copyright (C) 1997 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the unix (Linux) serial interface to ProbeScope.
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

int speed=B19200, bits=CS7, stopbits=0, parity=0;

char device[512] = "";		/* Serial device */
int psfd=0;			/* ProbeScope file descriptor */
struct termio stbuf, svbuf;	/* termios: svbuf=saved, stbuf=set */

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

/* serial cleanup routine called as the program exits */
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

/* return 1 if we find a ProbeScope on the given serial device */
int
findscope(char *dev)
{
  int c, byte = 0, try = 0;

  if ((psfd = open(dev, O_RDONLY|O_NDELAY)) < 0) {
    sprintf(error, "%s: can't open device %s", progname, dev);
    perror(error);
    return(0);
  }
  if (ioctl(psfd, TCGETA, &svbuf) < 0) {
    sprintf(error, "%s: can't ioctl get device %s", progname, dev);
    perror(error);
    close(psfd);
    return(0);
  }
  memcpy(&stbuf, &svbuf, sizeof(struct termio));
  stbuf.c_iflag = ISTRIP;
  stbuf.c_oflag = 0;
  stbuf.c_lflag = 0;
  stbuf.c_cflag = speed | bits | stopbits | parity | CLOCAL | CREAD;
  if (ioctl(psfd, TCSETA, &stbuf) < 0) {
    sprintf(error, "%s: can't ioctl set device %s", progname, dev);
    perror(error);
    close(psfd);
    return(0);
  }
  flush_serial();
  while (byte < 300 && try < 75) { /* give up in 7.5ms */
    if ((c = getonebyte()) < 0) {
      microsleep(100);		/* try again in 0.1ms */
      try++;
    } else if (c > 0x7b)
      return(1);		/* ProbeScope found! */
    else
      byte++;
#ifdef PSDEBUG
    printf("%d\t%d\n", try, byte);
#endif
  }
  if (ioctl(psfd, TCSETA, &svbuf) < 0) {
    sprintf(error, "%s: can't ioctl set device %s", progname, dev);
    perror(error);
    close(psfd);
    return(0);
  }
  return(0);			/* no ProbeScope found. */
}

/* set ps.found to non-zero if we find ProbeScope on a serial port */
void
init_serial()
{
  char *dev[] = PSDEVS, *p;
  int i, num = sizeof(dev) / sizeof(char *);

  cleanup_serial();		/* close current device first, if any */
  if ((p = getenv("PROBESCOPE")) == NULL) /* first place to look */
    p = PROBESCOPE;		/* -D default defined in Makefile */
  dev[0] = p;
  for (i = 0; i < num; i++) {	/* look in all places specified in config.h */
    if (findscope(dev[i])) {
      strcpy(device, dev[i]);
      i = ps.found = num;	/* done; exit the loop */
    }
  }
  if (!ps.found) psfd = 0;
}

/*
 * @(#)$Id: bitscope.h,v 2.3 2009/08/14 02:44:58 baccala Exp $
 *
 * Copyright (C) 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file defines BitScope serial bits and function prototypes.
 *
 */

#include <sys/socket.h>
#include <netinet/in.h>

/* BitScope serial data protocol bit definitions */

#define R13	(bs.r[13] ? bs.r[13] : 256)
#define R15	(bs.r[15] ? bs.r[15] : 256)

/* Spock Option Byte (R7) */

#define TRIGLEVEL	0
#define TRIGEDGE	0x20

#define TRIGEDGEF2T	0
#define TRIGEDGET2F	0x10

#define LOWER16BNC	0
#define UPPER16POD	0x08

#define TRIGDDBIT7	0
#define TRIGCOMPARE	2
#define TRIGEVENT1	4
#define TRIGEVENT2	6

#define TRIGDIGITAL	0
#define TRIGANALOG	1

/* Input / Attenuation Register (R14) */

#define PRIMARY(bits)	(bits)
#define SECONDARY(bits)	((bits) << 4)

#define RANGE130	0
#define RANGE600	1
#define RANGE1200	2
#define RANGE3160	3

#define CHANNELA	4
#define CHANNELB	0

#define	ZZCLK		8

/* Trace Modes (R8) */

#define SIMPLE		0
#define SIMPLECHOP	1
#define TIMEBASE	2
#define TIMEBASECHOP	3
#define SLOWCLOCK	4
#define FREQUENCY	8

#define SETWORD(r, w)	*r = w & 0xff; *(r + 1) = ((w & 0xff00) >> 8);

typedef struct BitScope {	/* The state of the BitScope */
  short probed;
  short found;
  short UDP_connected;
  struct sockaddr_in UDPaddr;
  char bcid[12];		/* ? output, minus <CR>s */
  int version;			/* numeric version equivalent */
  int clock_rate;		/* Hz */
  short analog_captures;	/* 0: none; 1: chan A; 2: chan B; 3: both*/
  short digital_captures;	/* 0 or 1 (single digital channel) */
  int capture_length;
  int fd;			/* file descriptor */
  unsigned char r[24];		/* registers */
  int R[24];			/* register overrides from file load */
  unsigned char buf[256 * 5 + 20]; /* serial input buffer */
} BitScope;
extern BitScope bs;

int idbitscope(int fd);

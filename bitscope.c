/*
 * @(#)$Id: bitscope.c,v 1.15 2000/07/18 06:01:55 twitham Exp $
 *
 * Copyright (C) 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the BitScope (www.bitscope.com) driver.
 * Developed on a 10 MHz "BC000100" BitScope with Motorola MC10319 ADC.
 * Some parts based on bitscope.c by Ingo Cyliax
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "oscope.h"
#include "proscope.h"		/* for PSDEBUG macro */
#include "bitscope.h"
#include "func.h"		/* to modify funcnames */

BitScope bs;			/* the BitScope structure */

/* use real read and write except on DOS where they are emulated */
#ifndef GO32
#define serial_read(a, b, c)	read(a, b, c)
#define serial_write(a, b, c)	write(a, b, c)
#endif

/* run CMD command string on bitscope FD or return 0 if unsuccessful */
int
bs_cmd(int fd, char *cmd)
{
  int i, j, k;
  char c;

/*    if (fd < 3) return(0); */
  in_progress = 0;
  j = strlen(cmd);
  PSDEBUG("bs_cmd: %s\n", cmd);
  for (i = 0; i < j; i++) {
    if (serial_write(fd, cmd + i, 1) == 1) {
      k = 10;
      while (serial_read(fd, &c, 1) < 1) {
	if (!k--) return(0);
	PSDEBUG("cmd sleeping %d\n", k);
	usleep(1);
      }
      if (cmd[i] != c)
	fprintf(stderr, "bs mismatch @ %d: sent:%c != recv:%c\n", i, cmd[i], c);
    } else {
      sprintf(error, "write failed to %d", fd);
      perror(error);
    }
  }
  return(1);
}

/* read N bytes from bitscope FD into BUF, or return 0 if unsuccessful */
int
bs_read(int fd, char *buf, int n)
{
  int i = 0, j, k = n + 10;

  in_progress = 0;
  buf[0] = '\0';
  while (n) {
    if ((j = serial_read(fd, buf + i, n)) < 1) {
      if (!k--) return(0);
      PSDEBUG("read sleeping %d\n", k);
      usleep(1);
    } else {
      n -= j;
      i += j;
    }
  }
  buf[i] = '\0';
  return(1);
}

/* asynchronously read N bytes from bitscope FD into BUF, return N when done */
int
bs_read_async(int fd, char *buf, int n, char c)
{
  static char *pos, *end;
  int i;

  if (in_progress) {
    if ((i = serial_read(fd, pos, end - pos)) > 0) {
      pos += i;
      if (pos >= end) {
	in_progress = 0;
	return(n);
      }
    }
    return(0);
  }
  pos = buf;
  *pos = '\0';
  end = pos + n;
  in_progress = c;
  return(0);
}

/* Run IN on bitscope FD and store results in OUT (not including echo) */
/* Only the last command in IN should produce output; else this won't work */
int
bs_io(int fd, char *in, char *out)
{

  out[0] = '\0';
  if (!in_progress)
    bs_cmd(fd, in);
  switch (in[strlen(in) - 1]) {
  case 'T':
    return bs_read_async(fd, out, 6, 'T');
  case 'M':
    if (bs.version >= 110)
      return bs_read_async(fd, out, R15 * 2, 'M');
  case 'A':
    if (bs.version >= 110)
      return bs_read_async(fd, out, R15, 'A');
  case 'S':			/* DDAA, per sample + newlines per 16 */
    return bs_read_async(fd, out, R15 * 5 + (R15 / 16) + 1, 'S');
  case 'P':
    if (bs.version >= 110)
      return bs_read(fd, out, 14);
    break;
  case 0:			/* least used last for efficiency */
  case '?':
    return bs_read(fd, out, 10);
  case 'p':
    return bs_read(fd, out, 4);
  case 'R':
  case 'W':
    if (bs.version >= 110)
      return bs_read(fd, out, 4);
  }
  in_progress = 0;
  return(1);
}

int
bs_getreg(int fd, int reg)
{
  unsigned char i[8], o[8];
  sprintf(i, "[%x]@p", reg);
  if (!bs_io(fd, i, o))
    return(-1);
  return strtol(o, NULL, 16);
}

int
bs_getregs(int fd, unsigned char *reg)
{
  unsigned char buf[8];
  int i;

  if (!bs_cmd(fd, "[3]@"))
    return(0);
  for (i = 3; i < 24; i++) {
    if (bs_io(fd, "p", buf))
      return(0);
    reg[i] = strtol(buf, NULL, 16);
    if (!bs_io(fd, "n", buf))
      return(0);
  }
  return(1);
}

int
bs_putregs(int fd, unsigned char *reg)
{
  unsigned char buf[8];
  int i;

  if (!bs_cmd(fd, "[3]@"))
    return(0);
  for (i = 3; i < 24; i++) {
    if (reg[i])
      sprintf(buf, "[%x]sn", reg[i]);
    else
      sprintf(buf, "[sn");
    if (!bs_cmd(fd, buf))
      return(0);
  }
  return(1);
}

void
bs_flush_serial()
{
  int c, byte = 0;

  c = 10;			/* clear serial FIFO */
  while ((byte < sizeof(bs.buf)) && c--) {
    byte += serial_read(bs.fd, bs.buf, sizeof(bs.buf) - byte);
    usleep(1);
  }
}

void
bs_changerate(int fd, int dir)
{
  unsigned char buf[10];

  bs_flush_serial();
  if (dir > 0)
    bs.r[13] = bs.r[13] ? bs.r[13] / 2 : 128; /* TB	time base */
  else if (dir < 0)
    bs.r[13] = bs.r[13] ? bs.r[13] * 2 : 1;
  mem[23].rate = mem[24].rate = mem[25].rate = 1250000 / (19 + 3 * (R13 + 1));
  sprintf(buf, "[d]@[%x]s", bs.r[13]);
  bs_cmd(fd, buf);
}

/* initialize previously identified BitScope FD */
void
bs_init(int fd)
{
  static int volts[] = {
     130 * 5 / 2,  600 * 5 / 2,  1200 * 5 / 2,  3160 * 5 / 2,
    1300 * 5 / 2, 6000 * 5 / 2, 12000 * 5 / 2, 31600 * 5 / 2,
     632 * 5 / 2, 2900 * 5 / 2,  5800 * 5 / 2, 15280 * 5 / 2,
  };
  char *labels[] = {
    "BNC B x1",		"BNC A x1",
    "BNC B x10",	"BNC A x10",
    "POD Ch. A",	"POD Ch. B",
    "Logic An.",
  };

  if (fd < 3) return;
  if (snd) handle_key('&');	/* turn off sound card and probescope */
  bs.found = 1;
  in_progress = 0;
  bs.version = strtol(bs.bcid + 2, NULL, 10);
  mem[23].rate = mem[24].rate = mem[25].rate = 25000000;

  bs_getregs(fd, bs.r);		/* get and reset registers */
  bs.r[3] = bs.r[4] = 0;
  bs.r[5] = 0;
  bs.r[6] = 127;		/*  scope.trig; ? */
//  bs.r[6] = 0xff;		/* don't care */
  bs.r[7] = TRIGEDGE | TRIGEDGEF2T | LOWER16BNC | TRIGCOMPARE | TRIGANALOG;
  bs.r[8] = 4;			/* trace mode, 0-4 */

  /* 0: 0, 2, 179 */
  /* 1: 0, 107, 1 */
  /* 2: 0, 80, 1 */
  /* 3: 0, 16319, 1 */

//  SETWORD(&bs.r[11], 2);	/* try to just fill the 16384 memory: */
//  bs.r[13] = 179;		/* 2,179 through mode 0 formula = 1644 */

  bs.r[20] = 1;			/* PRETD	pre trigger delay */
  SETWORD(&bs.r[11], 16319);	/* PTD		post trigger delay */
  bs.r[13] = 1;
  bs_changerate(fd, 0);

  bs.r[14] = PRIMARY(RANGE1200 | CHANNELA | ZZCLK)
    | SECONDARY(RANGE1200 | CHANNELB | ZZCLK);
  bs.r[15] = 0;			/* max samples per dump (256) */
  bs_putregs(fd, bs.r);
  funcnames[2] = labels[6];
  if (bs.r[7] & UPPER16POD) {
    mem[23].volts = volts[(bs.r[14] & RANGE3160) + 8];
    mem[24].volts = volts[((bs.r[14] >> 4) & RANGE3160) + 8];
    funcnames[0] = labels[4];
    funcnames[1] = labels[5];
  } else {
    bs.x10 = 0x11;
    mem[23].volts = volts[(bs.r[14] & RANGE3160)
			 + (bs.x10 & 0x01 ? 4 : 0)];
    mem[24].volts = volts[((bs.r[14] >> 4) & RANGE3160)
			 + (bs.x10 & 0x10 ? 4 : 0)];
    funcnames[0] = labels[((bs.r[14] & PRIMARY(CHANNELA)) ? 1 : 0)
			 + ((bs.x10 & 0x01) ? 2 : 0)];
    funcnames[1] = labels[((bs.r[14] & SECONDARY(CHANNELA)) ? 1 : 0)
			 + ((bs.x10 & 0x10) ? 2 : 0)];
  }
}

/* identify a BitScope (2), ProbeScope (1) or none (0) */
int
idscope(int probescope)
{
  int c, byte = 0, try = 0;

  flush_serial();
  ps.found = bs.found = 0;
  if (probescope) {
    while (byte < 300 && try < 75) { /* give up in 7.5ms */
      if ((c = getonebyte()) < 0) {
	usleep(100);		/* try again in 0.1ms */
	try++;
      } else if (c > 0x7b) {
	ps.found = 1;
	return(1);		/* ProbeScope found! */
      } else
	byte++;
      PSDEBUG("%d\t", try);
      PSDEBUG("%d\n", byte);
    }
  } else {			/* identify bitscope */
    bs_flush_serial();
    if (bs_io(bs.fd, "?", bs.buf) && bs.buf[1] == 'B' && bs.buf[2] == 'C') {
      strncpy(bs.bcid, bs.buf + 1, sizeof(bs.bcid));
      bs.bcid[8] = '\0';
      bs_init(bs.fd);
      return(2);		/* BitScope found! */
    }
  }
  return(0);
}

/* get pending available data from FD, or initiate new data collection */
int
bs_getdata(int fd)
{
  static unsigned char *buff;
  static int alt = 1, k, n;

  if (!fd) return(0);		/* device open? */
  if (in_progress == 'M') {	/* finish a get */
    if ((n = bs_io(fd, "M", bs.buf))) {
      buff = bs.buf;
      if (bs.version >= 110) { /* M mode, simple bytes */
	while (buff < bs.buf + n) {
	  if (k >= MAXWID)
	    break;
	  mem[25].data[k] = *buff++ - 128;
	  mem[23 + alt].data[k] = *buff++ - 128;
	  if (alt) k++;
	  alt = !alt;
	}
      } else {		/* S mode, hex ASCII */
	while (*buff != '\0') {
	  if (k >= MAXWID)
	    break;
	  if (*buff == '\r' || *buff == '\n')
	    buff++;
	  else {
	    n = strtol(buff, NULL, 16);
	    mem[23 + alt].data[k] = (n & 0xff) - 128;
	    mem[25].data[k] = ((n & 0xff00) >> 8) - 128;
	    buff += 5;
	    if (alt) k++;
	    alt = !alt;
	  }
	}
      }
      mem[23].num = mem[24].num = mem[25].num = k < MAXWID ? k : MAXWID;
      if (k >= samples(mem[23].rate) || k >= 8 * 1024) { /* all done */
	k = 0;
	in_progress = 0;
      } else {		/* still need more, start another */
	bs_io(fd, "M", bs.buf);
      }
    }
  } else if (in_progress == 'T') { /* finish a trigger */
    if (bs_io(fd, "T", bs.buf)) {
      fprintf(stderr, "%s", bs.buf);
      bs_io(fd, "M", bs.buf);	/* start a get */
      k = 0;
    } else
      return(0);
  } else {			/* start a trigger */
    return(bs_io(fd, ">T", bs.buf));
  }
  return(1);
}

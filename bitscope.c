/*
 * @(#)$Id: bitscope.c,v 1.1 2000/07/03 18:18:14 twitham Exp $
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

/* run CMD command string on bitscope FD or return 0 if unsuccessful */
int
bs_cmd(int fd, char *cmd)
{
  int i, j, k;
  char c;

  j = strlen(cmd);
  for (i = 0; i < j; i++) {
    if (write(fd, cmd + i, 1) == 1) {
      k = 10;
      while (read(fd, &c, 1) < 1) {
	if (!k--) return(0);
	PSDEBUG("cmd sleeping %d\n", k);
	usleep(1);
      }
      if (cmd[i] != c)
	fprintf(stderr, "bs mismatch @ %d: sent:%c != recv:%c\n", i, cmd[i], c);
    } else
      perror("write failed");
  }
  return(1);
}

/* read N bytes from bitscope FD into BUF, or return 0 if unsuccessful */
int
bs_read(int fd, char *buf, int n)
{
  int i = 0, j, k = n + 10;

  while (n) {
    if ((j = read(fd, buf + i, n)) < 1) {
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

/* Run IN on bitscope FD and store results in OUT (not including echo) */
/* Only the last command in IN should produce output; else this won't work */
int
bs_io(int fd, char *in, char *out)
{

  out[0] = '\0';
  if (!bs_cmd(fd, in))
    return(0);
  switch (in[strlen(in) - 1]) {
  case 'T':
    return bs_read(fd, out, 6);
  case 'S':			/* DDAA, per sample + newlines per 16 */
    return bs_read(fd, out, R15 * 5 + (R15 / 16) + 1);
  case 'M':
    if (bs.version >= 111)
      return bs_read(fd, out, R15 * 2);
    break;
  case 'A':
    if (bs.version >= 111)
      return bs_read(fd, out, R15);
    break;
  case 'P':
    if (bs.version >= 111)
      return bs_read(fd, out, 14);
    break;
  case 0:			/* least used last for efficiency */
  case '?':
    return bs_read(fd, out, 10);
  case 'p':
    return bs_read(fd, out, 4);
  case 'R':
    if (bs.version >= 111)
      return bs_read(fd, out, 4);
  }
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
bs_init(int fd)
{
  if (!fd) return;
  if (snd) handle_key('&');	/* turn off sound card and probescope */
  bs.found = 1; ps.found = 0;
  bs.version = strtol(bs.bcid + 2, NULL, 10);
/*    mem[23].rate = mem[24].rate = mem[25].rate = 25000000; */

  bs_getregs(fd, bs.r);		/* get and reset registers */
  bs.r[7] = TRIGEDGE | TRIGEDGEF2T | LOWER16BNC | TRIGCOMPARE | TRIGANALOG;
  bs.r[14] = PRIMARY(RANGE1200 | CHANNELA | ZZCLK)
    | SECONDARY(RANGE1200 | CHANNELB | ZZCLK);
  bs.volts = 2400;		/* !!! attenuation should be tweakable */
  bs.r[5] = 0;
  bs.r[6] = 127;		/*  scope.trig; ? */
  bs.r[3] = bs.r[4] = 0;
  bs.r[8] = SIMPLE;		/* TIMEBASE;  */
  SETWORD(&bs.r[11], 16383);
  bs.r[13] = 1;
  bs.r[15] = 0;			/* max samples per dump (256) */
  bs_putregs(fd, bs.r);
}

/* identify a BitScope (2), ProbeScope (1) or none (0) */
int
idscope(int probescope)
{
  int c, byte = 0, try = 0;

  flush_serial();
  funcnames[0] = "Left  Mix";
  funcnames[1] = "Right Mix";
  funcnames[2] = "ProbeScope";
  if (probescope) {
    while (byte < 300 && try < 75) { /* give up in 7.5ms */
      if ((c = getonebyte()) < 0) {
	microsleep(100);	/* try again in 0.1ms */
	try++;
      } else if (c > 0x7b) {
	ps.found = 1; bs.found = 0;
	return(1);		/* ProbeScope found! */
      } else
	byte++;
      PSDEBUG("%d\t", try);
      PSDEBUG("%d\n", byte);
    }
  } else {			/* identify bitscope */
    if (bs_io(bs.fd, "?", error) && error[1] == 'B' && error[2] == 'C') {
      strncpy(bs.bcid, error + 1, sizeof(bs.bcid));
      bs.bcid[8] = '\0';
      bs_init(bs.fd);
      funcnames[0] = "Channel A"; /* relabel */
      funcnames[1] = "Channel B";
      funcnames[2] = "Logic An.";
      return(2);		/* BitScope found! */
    }
  }
  return(0);
}

int
bs_getdata(int fd)
{
  static unsigned char buffer[MAXWID * 2], *buff;
  int i, j;

  if (!fd) return(0);		/* device open? */
  if (!bs_io(fd, ">T", buffer) < 0)
    return(0);
  fprintf(stderr, "%s", buffer);
  if (!bs_io(fd, "S", buffer) < 0)
    return(0);
  buff = buffer;
  i = 0;
  while (*buff != '\0') {
    if (*buff == '\r' || *buff == '\n')
      buff++;
    else {
      j = strtol(buff, NULL, 16);
      mem[23].data[i] = (j & 0xff) - 128;
      mem[25].data[i++] = ((j & 0xff00) >> 8) - 128;
      buff += 5;
    }
  }
  return(1);
}

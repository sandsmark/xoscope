/*
 * @(#)$Id: bitscope.c,v 2.13 2009/08/14 02:44:58 baccala Exp $
 *
 * Copyright (C) 2000 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the BitScope (www.bitscope.com) driver.
 * Developed on a 10 MHz "BC000112" BitScope with Motorola MC10319 ADC.
 * Some parts based on bitscope.c by Ingo Cyliax
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "oscope.h"
#include "proscope.h"		/* for PSDEBUG macro */
#include "bitscope.h"

char bitscope_device[80] = BITSCOPE;	/* UNIX device name */

BitScope bs;			/* the BitScope structure */

static Signal analogA_signal, analogB_signal, digital_signal;

/* This function is defined by Glade in callbacks.c */

extern void bitscope_dialog();

static int bs_rawread(unsigned char *buf, int n)
{
  static unsigned char network_buffer[4096];
  static int valid = 0;
  static int next = 0;

  /* XXX Assume that nothing calls us with a buffer bigger than 4096 */

  /* XXX ignores 4 byte header, including data sequence number */

  if (bs.UDP_connected) {
    if (!valid) {
      valid = recv(bs.fd, network_buffer, sizeof(network_buffer), 0);
      if (valid == -1) {
	perror("recv");
	valid = 0;
	return 0;
      }
      valid -= 4;
      next = 0;
      if (valid <= 0) {
	fprintf(stderr, "Too small packet received\n");
	valid = 0;
	return 0;
      }
    }
    if (valid <= n) {
      memcpy(buf, network_buffer+4+next, valid);
      n = valid;
      valid = 0;
      return n;
    } else {
      memcpy(buf, network_buffer+4+next, n);
      valid -= n;
      next += n;
      return n;
    }
  } else {
    return read(bs.fd, buf, n);
  }
}

static int bs_rawwrite(char *buf, int n)
{
  unsigned char network_buffer[4096];

  /* XXX Assume that nothing calls us with a buffer bigger than 4096 */

  if (bs.UDP_connected) {
    network_buffer[0] = 'a';	/* Command ID */
    memcpy(network_buffer+1, buf, n);
    return sendto(bs.fd, network_buffer, n+1, 0,
		  (struct sockaddr *) &bs.UDPaddr, sizeof(bs.UDPaddr)) - 1;
  } else {
    return write(bs.fd, buf, n);
  }
}

/* read N bytes from bitscope FD into BUF, or return 0 if unsuccessful
 *
 * block for up to 50+N milliseconds for the reply
 *
 * For comparison, the FT8U100AX USB serial port's standard buffering
 * latency (the maximum time it will buffer data for) is 40 ms.
 *
 * XXX this is the kind of thing we need threading for, because the
 * windowing system freezes while this routine blocks
 */

static int
bs_read(unsigned char *buf, int n)
{
  int i = 0, j, k = n + 50;

  in_progress = 0;
  buf[0] = '\0';
  while (n) {
    if ((j = bs_rawread(buf + i, n)) < 1) {
      if (!k--) {
	PSDEBUG("bs_read timeout\n", 0);
	return(0);
      }
      /* PSDEBUG("read sleeping %d\n", k); */
      usleep(1000);
    } else {
      n -= j;
      i += j;
    }
  }
  buf[i] = '\0';
  return(1);
}

/* asynchronously read N bytes from bitscope FD into BUF, return N when done
 *
 * if an asynchronous read isn't in progress, it will flag one as
 * started (using in_progress) and return 0
 *
 * otherwise, it attempts a single read (and might block) and if
 * doesn't get the number of bytes required will return 0, save some
 * static variables to indicate where we are in the transfer and leave
 * in_progress set to the running command code
 */

static int
bs_read_async(unsigned char *buf, int n, char c)
{
  static unsigned char *pos, *end;
  int i;

  if (in_progress) {
    if ((i = bs_rawread(pos, end - pos)) > 0) {
      pos += i;
      if (pos >= end) {
	in_progress = 0;
	buf[n] = '\0';
	return(n);
      }
    }
    return(0);
  }
  pos = buf;
  /* *pos = '\0'; */
  end = pos + n;
  in_progress = c;
  return(0);
}

/* run CMD command string on bitscope FD or return 0 if unsuccessful
 *
 * The command string must be a string of single-character bitscope
 * commands, each of which echos itself back (with nothing else)
 */
static int
bs_cmd(char *cmd)
{
  int i, j;
  unsigned char c[2];

  in_progress = 0;
  j = strlen(cmd);
  PSDEBUG("bs_cmd: %s\n", cmd);
  if (bs_rawwrite(cmd, j) != j) {
    sprintf(error, "write failed to %d", bs.fd);
    perror(error);
    return(0);
  }
  for (i = 0; i < j; i++) {
    bs_read(c, 1);
    if (cmd[i] != c[0]) {
      fprintf(stderr, "bs mismatch @ %d: sent:%c != recv:%c\n",
	      i, cmd[i], c[0]);
      return(0);
    }
  }
  return(1);
}

/* Run IN on bitscope FD and store results in OUT (not including echo)
 *
 * Only the last command in IN should produce output; else this won't work
 *
 * If the last command is "T", "M", "A", or "S" (i.e, start a trace or
 * one of the three commands that reads sampled data) the read is done
 * asynchronously and this function might return 0, in which case it
 * should be called again (and again) with the same tailing command
 * and OUT buffer until that commands completes.
 *
 * For any other command, this function will block waiting for it.
 */

static int
bs_io(char *in, unsigned char *out)
{
  char c;

  out[0] = '\0';
  if (!in_progress) {
    if (in[0] == 'M') {
      if (! bs_cmd(bs.version >= 110 ? "M" : "S")) return(0);
    } else {
      if (! bs_cmd(in)) return(0);
    }
  }
  switch (c = in[strlen(in) - 1]) {
  case 'T':
    return bs_read_async(out, 6, c);
  case 'M':
    if (bs.version >= 110)
      return bs_read_async(out, R15 * 2, c);
  case 'A':
    if (bs.version >= 110)
      return bs_read_async(out, R15, c);
  case 'S':			/* DDAA, per sample + newlines per 16 */
    return bs_read_async(out, R15 * 5 + (R15 / 16) + 1, c);
  case 'P':
    if (bs.version >= 110)
      return bs_read(out, 14);
    break;
  case 0:			/* least used last for efficiency */
  case '?':
    return bs_read(out, 10);
  case 'p':
    return bs_read(out, 4);
  case 'R':
  case 'W':
    if (bs.version >= 110)
      return bs_read(out, 4);
  }
  in_progress = 0;
  return(1);
}

#if 0

/* We don't use this function right now, because we never get
 * individual registers.  Might need it again in the future.
 */

static int
bs_getreg(int reg)
{
  char i[8], o[8];

  sprintf(i, "[%x]@p", reg);
  if (!bs_io(i, (unsigned char *)o))
    return(-1);
  return strtol(o, NULL, 16);
}

#endif

static int
bs_getregs(unsigned char *reg)
{
  unsigned char buf[8];
  int i;

  if (!bs_cmd("[3]@"))
    return(0);
  for (i = 3; i < 24; i++) {
    if (!bs_io("p", buf))
      return(0);
    reg[i] = strtol((char *)buf, NULL, 16);
//    printf("reg[%d]=%d\n", i, reg[i]);
    if (!bs_io("n", buf))
      return(0);
  }
  return(1);
}

static int
bs_putregs(unsigned char *reg)
{
  char buf[8];
  int i;

  if (!bs_cmd("[3]@"))
    return(0);
  for (i = 3; i < 24; i++) {
    if (reg[i])
      sprintf(buf, "[%x]sn", reg[i]);
    else
      sprintf(buf, "[sn");
    if (!bs_cmd(buf))
      return(0);
  }
  return(1);
}

/* Take 50 ms to clear the serial FIFO.  Though almost completely
 * arbitrary, this number at least meshes with the default 40 ms
 * latency on the FTDI FT8U100AX.
 */

static void
bs_flush_serial(void)
{
  int c, byte = 0;

  if (bs.UDP_connected) return;

  c = 50;
  while ((byte < sizeof(bs.buf)) && c--) {
    byte += read(bs.fd, bs.buf, sizeof(bs.buf) - byte);
    usleep(1000);
  }
}


static char *
save_option(int i)
{
  int regs[] = {5, 6, 7, 14}; /* regs to save */
  static char buf[32];

  if (i < sizeof(regs) / sizeof(regs[0])) {
    sprintf(buf, "%d:0x%02x\n", regs[i], bs.r[regs[i]]);
    return buf;
  } else {
    return NULL;
  }
}


static int
parse_option(char *optarg)
{
  char *p;
  int reg = 0, val = 0;

  reg = strtol(optarg, NULL, 0);
  if (reg >= 0 && reg < 24) {
    if ((p = strchr(optarg, ':')) != NULL) {
      val = strtol(++p, NULL, 0);
      bs.R[reg] = val;
//      printf("bs_option: %s b[%d] = %d\n", optarg, reg, val);
      return 1;
    }
  }
  return 0;
}


static int
bs_changerate(int dir)
{
  char buf[10];

  bs_flush_serial();
  if (dir > 0)
    bs.r[13] = bs.r[13] ? bs.r[13] / 2 : 128; /* TB	time base */
  else if (dir < 0)
    bs.r[13] = bs.r[13] ? bs.r[13] * 2 : 1;
  /* XXX need to set rate in Signal structures here - is this formula right? */
//  mem[23].rate = mem[24].rate = mem[25].rate = 1250000 / (19 + 3 * (R13 + 1));
  sprintf(buf, "[d]@[%x]s", bs.r[13]);
  bs_cmd(buf);
  return 1;
}


static void
bs_fixregs(void)
{
  int i;

  bs.r[3] = bs.r[4] = 0;
//  bs.r[5] = 0;
//  bs.r[6] = 127;		/*  scope.trig; ? */
  bs.r[6] = 0xff;		/* don't care */
  //  bs.r[7] = TRIGEDGE | TRIGEDGEF2T | LOWER16BNC | TRIGCOMPARE | TRIGANALOG;
  bs.r[7] = TRIGLEVEL | LOWER16BNC | TRIGCOMPARE | TRIGANALOG;

  // Trace mode 1 - Channel chop
  bs.r[8] = 1;			/* trace mode, 0-4 */

  /* 0: 0, 2, 179 */
  /* 1: 63, 53, 1 */
  /* 2: 0, 80, 1 */
  /* 3: 0, 16319, 1 */

  //  SETWORD(&bs.r[11], 2);	/* try to just fill the 16384 memory: */
  //  bs.r[13] = 179;		/* 2,179 through mode 0 formula = 1644 */

  bs.r[20] = 63;			/* PRETD	pre trigger delay */
  SETWORD(&bs.r[11], 53);	/* PTD		post trigger delay */
  bs.r[13] = 1;
  bs_changerate(0);

//  bs.r[14] = PRIMARY(RANGE1200 | CHANNELA | ZZCLK)
//    | SECONDARY(RANGE1200 | CHANNELB | ZZCLK);
  bs.r[15] = 0;			/* max samples per dump (256) */
//  bs.r[0] = 0x11;
  for (i = 0; i < 24; i++) {
    if (bs.R[i] > -1)
      bs.r[i] = bs.R[i];
  }
}

/* initialize previously identified BitScope FD */
static void
bs_init(void)
{
  static int volts[] = {
     130 * 5 / 2,  600 * 5 / 2,  1200 * 5 / 2,  3160 * 5 / 2,
    1300 * 5 / 2, 6000 * 5 / 2, 12000 * 5 / 2, 31600 * 5 / 2,
     632 * 5 / 2, 2900 * 5 / 2,  5800 * 5 / 2, 15280 * 5 / 2,
  };
  static char *labels[] = {
    "BNC B x1",		"BNC A x1",
    "BNC B x10",	"BNC A x10",
    "POD Ch. A",	"POD Ch. B",
    "Logic An.",
  };

  bs.found = 1;
  in_progress = 0;
  bs.version = strtol(bs.bcid + 2, NULL, 10);
  bs.clock_rate = 25000000;	/* Original BitScope clock rate - 25 MHz */

  /* We use trace mode 1, channel chop, so each analog channel is at
   * half rate
   */
  analogA_signal.rate = bs.clock_rate / 2;
  analogB_signal.rate = bs.clock_rate / 2;
  digital_signal.rate = bs.clock_rate;

  analogA_signal.width = MAXWID;
  analogB_signal.width = MAXWID;
  digital_signal.width = MAXWID;

  digital_signal.bits = 8;

  if (analogA_signal.data != NULL) free(analogA_signal.data);
  if (analogB_signal.data != NULL) free(analogB_signal.data);
  if (digital_signal.data != NULL) free(digital_signal.data);

  analogA_signal.data = malloc(MAXWID * sizeof(short));
  analogB_signal.data = malloc(MAXWID * sizeof(short));
  digital_signal.data = malloc(MAXWID * sizeof(short));

  bs_getregs(bs.r);
  bs_fixregs();
  bs_putregs(bs.r);

  strcpy(digital_signal.name, labels[6]);

  if (bs.r[7] & UPPER16POD) {
    analogA_signal.volts = volts[(bs.r[14] & RANGE3160) + 8];
    analogB_signal.volts = volts[((bs.r[14] >> 4) & RANGE3160) + 8];
    strcpy(analogA_signal.name, labels[4]);
    strcpy(analogB_signal.name, labels[5]);
  } else {
    analogA_signal.volts = volts[(bs.r[14] & RANGE3160)
				+ (bs.r[0] & 0x01 ? 4 : 0)];
    analogB_signal.volts = volts[((bs.r[14] >> 4) & RANGE3160)
				+ (bs.r[0] & 0x10 ? 4 : 0)];
#if 0
    strcpy(analogA_signal.name,
	   labels[((bs.r[14] & PRIMARY(CHANNELA)) ? 1 : 0)
		 + ((bs.r[0] & 0x01) ? 2 : 0)]);
    strcpy(analogB_signal.name,
	   labels[((bs.r[14] & SECONDARY(CHANNELA)) ? 1 : 0)
		 + ((bs.r[0] & 0x10) ? 2 : 0)]);
#else
    strcpy(analogA_signal.name, "BNC A");
    strcpy(analogB_signal.name, "BNC B");
#endif
  }
}

/* identify a BitScope; called from ser_*.c files */
int
idbitscope(int fd)
{
  bs.fd = fd;

  /* XXX shouldn't be tampering with a global var here */
  in_progress = 0;

  if (!bs.UDP_connected) {
    flush_serial(bs.fd);
    bs_flush_serial();
  }

  if (bs_io("?", bs.buf) && bs.buf[1] == 'B' && bs.buf[2] == 'C') {
    strncpy(bs.bcid, (char *)bs.buf + 1, sizeof(bs.bcid));
    bs.bcid[8] = '\0';
    bs_init();
    return(1);		/* BitScope found! */
  }

  bs.fd = -1;
  return(0);
}

static int open_bitscope(void)
{
  int i;
  static int once = 0;

  for (i = 0; i < sizeof(bs.R) / sizeof(bs.R[0]); i++) {
    bs.R[i] = -1;
  }

  bs.probed = 1;

  if (!once) {
    char *p;
    if ((p = getenv("BITSCOPE")) != NULL) /* first place to look */
      strncpy(bitscope_device, p, sizeof(bitscope_device));
    /* XXX hokey that this should be here. */
    bs.fd = -1;
    once = 1;
  }

  bs.UDP_connected = 1;

  if (bs.UDP_connected) {
    struct hostent *host;
    struct sockaddr_in local_addr;

    bs.UDPaddr.sin_family = AF_INET;
    bs.UDPaddr.sin_port = htons(16385);
    host = gethostbyname("sydney.bitscope.net");
    memcpy(&bs.UDPaddr.sin_addr, host->h_addr, host->h_length);

    bs.fd = socket(PF_INET, SOCK_DGRAM, 0);

    bzero(&local_addr, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY; 
    if (bind(bs.fd, (struct sockaddr *) &local_addr, sizeof(local_addr)) == -1) {
      perror("bind");
    }

    return idbitscope(bs.fd);
  } else {
    return init_serial_bitscope(bitscope_device);
  }
}

static int nchans(void)
{
  if (! bs.found && ! bs.probed) open_bitscope();

  return bs.found ? 3 : 0;
}

static int
serial_fd(void)
{
  return bs.found ? bs.fd : -1;
}

static Signal * bs_chan(int chan)
{
  switch (chan) {
  case 0:
    return &analogA_signal;
  case 1:
    return &analogB_signal;
  case 2:
    return &digital_signal;
  default:
    return NULL;
  }
}

static void reset(void)
{
  int count = 10;
  char in[2] = {in_progress, '\0'};

  /* Clear bs.probed, so that if we didn't find a bitscope on the
   * serial port, we'll probe again the next time nchans() is called.
   *
   * It's unclear to me just when we should re-attempt a probe, since
   * the user could plug the device in at any time.
   */

  bs.probed = 0;

  if (bs.fd == -1) return;

  if (in_progress) {
    bs_io(in, bs.buf);
    while (in_progress && count) {
      /* sleep up to 50 ms, 10 times, so we could wait a half second */
      usleep(50000);
      bs_io(in, bs.buf);
      count --;
    }
  }

  if (in_progress) {
    fprintf(stderr, "bs_reset: couldn't terminate in_progress transfer\n");
    in_progress = 0;
  }

  bs.analog_captures = 0;
  bs.digital_captures = 0;
  bs.capture_length = 0;

  if (analogA_signal.listeners > 0) {
    bs.analog_captures |= 1;
    analogA_signal.num = 0;
    analogA_signal.width = min(samples(analogA_signal.rate), 8192);
    analogA_signal.rate = bs.clock_rate;
    bs.capture_length = analogA_signal.width;
  }

  if (analogB_signal.listeners > 0) {
    bs.analog_captures |= 2;
    analogB_signal.num = 0;
    analogB_signal.width = min(samples(analogB_signal.rate), 8192);
    analogB_signal.rate = bs.clock_rate;
    bs.capture_length = analogB_signal.width;
  }

  /* If both signals are being captured, we're in channel chop mode,
   * so the capture length is twice whichever one is the maximum and
   * the capture rate is half the device's clock rate.
   */

  if (bs.analog_captures == 3) {
    bs.capture_length = 2 * max(analogA_signal.width, analogB_signal.width);
    analogA_signal.rate = bs.clock_rate / 2;
    analogB_signal.rate = bs.clock_rate / 2;
  }

  if (digital_signal.listeners > 0) {
    bs.digital_captures = 1;
    digital_signal.num = 0;
    digital_signal.width = min(samples(digital_signal.rate), 8192*2);
    bs.capture_length = max(bs.capture_length, digital_signal.width);
  }

  /* XXX actually set registers for correct analog capture */

  bs_io(">T", bs.buf);
}

/* get pending available data from FD, or initiate new data collection */
static int bs_getdata(void)
{
  static unsigned char *buff;
  static int k;
  long data;
  int n, digital_data, analog_data;

  if (bs.fd == -1) return(0);		/* device open? */

  if (in_progress == 'M') {	/* finish a get */
    if ((n = bs_io("M", bs.buf))) {
      buff = bs.buf;
      while (buff < bs.buf + n) {
	if (bs.version >= 110) {	/* M mode, simple D/A bytes */
	  digital_data = *buff++;
	  analog_data = *buff++ - 128;
	} else {			/* S mode, D/A hex ASCII */
	  if (*buff == '\r' || *buff == '\n') {
	    buff++;
	    continue;
	  }
	  /* next five bytes are four hex ASCII chars, then a comma */
	  data = strtol((char *) buff, NULL, 16);
	  digital_data = (data >> 8);
	  analog_data = (data & 0xff) - 128;
	  buff += 5;
	}

	if (k >= MAXWID)
	  break;

	if (bs.digital_captures) digital_signal.data[k] = digital_data;

	switch (bs.analog_captures) {
	case 1:
	  analogA_signal.data[k] = analog_data;
	  break;
	case 2:
	  analogB_signal.data[k] = analog_data;
	  break;
	case 3:
	  /* Both channels alternately captured in chop mode
	   *
	   * XXX this code is definitely wrong - default is to capture
	   * 50 samples at a time from each channel
	   */
	  if ((k % 2) == 0) {
	    analogA_signal.data[k/2] = analog_data;
	  } else {
	    analogB_signal.data[k/2] = analog_data;
	  }
	  break;
	}

	k++;
      }

      if (bs.digital_captures) {
	digital_signal.num = min(digital_signal.width, k);
      }

      switch (bs.analog_captures) {
      case 1:
	analogA_signal.num = min(analogA_signal.width, k);
	break;
      case 2:
	analogB_signal.num = min(analogB_signal.width, k);
	break;
      case 3:
	analogA_signal.num = min(analogA_signal.width, k/2);
	analogB_signal.num = min(analogB_signal.width, k/2);
	break;
      }

      if (k >= bs.capture_length) { /* all done */
	k = 0;
	in_progress = 0;
      } else {
	/* Haven't finished reading the current trace, so request
	 * another block of data.  XXX Should use "A" request if we
	 * don't need the digital data.
	 */
	bs_io("M", bs.buf);
      }
    }
  } else if (in_progress == 'T') { /* finish a trigger */
    if (bs_io("T", bs.buf)) {
      /* The current get logic starts reading at the current value of
       * the address counter.  This makes sense if we've completely
       * filled the capture RAM (and maybe wrapped).  On the other
       * hand, if we didn't fill the capture RAM, we should use ">M"
       * to reset the counter first.  Also, the best that I can figure
       * out the channel chop operation, 50 samples are taken from one
       * channel, followed by 50 from the other, and I don't see any
       * good way to figure out which samples came from where.
       */
      bs_io("M", bs.buf);	/* start a get */
      analogA_signal.num = 0;
      analogA_signal.frame ++;
      analogB_signal.num = 0;
      analogB_signal.frame ++;
      digital_signal.num = 0;
      digital_signal.frame ++;
      k = 0;
    } else
      return(0);
  } else {			/* start a trigger */
    return(bs_io(">T", bs.buf));
  }
  return(1);
}

static char * status_str(int i)
{
  switch (i) {
  case 0:
    if (bs.probed && !bs.found) {
      return split_field(serial_error, 0, 16);
    } else {
      return bs.bcid;
    }
  case 2:
    if (bs.probed && !bs.found) {
      return split_field(serial_error, 1, 16);
    } else {
      return NULL;
    }
  default:
    return NULL;
  }
}

DataSrc datasrc_bs = {
  "BitScope",
  nchans,
  bs_chan,
  NULL, /* set_trigger, */
  NULL, /* clear_trigger, */
  NULL, /* bs_changerate, */
  NULL,		/* set_width */
  reset,
  serial_fd,
  bs_getdata,
  status_str,
  NULL, /* option1, */
  NULL, /* option1str, */
  NULL, /* option2, */
  NULL, /* option2str, */
  parse_option,
  save_option,
  bitscope_dialog,
};

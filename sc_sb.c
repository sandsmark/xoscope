/*
 * @(#)$Id: sc_sb.c,v 1.1 1997/04/26 01:41:57 twitham Exp $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file doesn't yet implement the DOS sound card interface
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <conio.h>
#include "sb_lib.h"
#include "sbdetect.h"
#include "dsp.h"
/* #include "dma.h" */
#include "oscope.h"		/* program defaults */

/* int audio_port=0, audio_irq=0, audio_dma=0, audio_dma16=0; */
/* int SBok=0; */
/* struct dsp_device_caps *caps; */
/* int port, dma, irq,dma16; */

void
close_sound_card()
{
  sb_uninstall_driver();
/*   if (SBok) { */
/*     dsp_reset(); */
/*     dsp_close(); */
/*   } */
}

void
set_sound_card(unsigned int rate)
{
  sb_uninstall_driver();
  if(sb_install_driver(rate)!=SB_SUCCESS) {
    fprintf(stderr,"Driver error: %s",sb_driver_error);
    exit(1);
  }
  actual = sb_sample_frequency;
/*  dma_reset(SBdma); */		/* this needed? */
/*  dsp_reset(); */

/*   rate = min(rate, caps->max_8bit_rec_s); */
/*   rate = max(rate, caps->min_speed); */

/*    actual = dsp_set_record(rate, 1, 8, 0); */
}

/* [re]initialize /dev/dsp */
void
init_sound_card(int firsttime)
{
  if(sb_install_driver(44000)!=SB_SUCCESS) {
    fprintf(stderr,"Driver error: %s",sb_driver_error);
    exit(1);
  }
  clrscr();

  if(sb_info.dspVersion>=0x040C)
    cprintf("Sound-Blaster AWE32");
  else if(sb_info.dspVersion>=0x0400)
    cprintf("Sound-Blaster 16");
  else if(sb_info.dspVersion>=0x0300)
    cprintf("Sound-Blaster Pro");
  else if(sb_info.dspVersion>=0x0201)
    cprintf("Sound-Blaster 2.0");
  else
    cprintf("Sound-Blaster 1.0/1.5");

  cprintf(" detected at Address %xH, IRQ %d\n\r",sb_info.reset-6,sb_info.IRQ);
  cprintf("Using a sampling rate of %d",sb_sample_frequency);

  
  /*   int parm; */

  /*   if (!firsttime) */
  /*     close_sound_card(); */

  /*   sb_get_params(&port,&dma,&irq,&dma16); */

  /*   SBok = dsp_open(port, dma, irq, dma16, h_points * 2, 2); */

  /*   if (!SBok) { */
  /*     sprintf(error, "%s: Can't open sound device", progname); */
  /*     perror(error); */
  /*     cleanup(); */
  /*     exit(1); */
  /*   } */

  /*   caps = dsp_get_device_caps(); */

  set_sound_card(scope.rate);
}

/* get data from sound card, return value is whether we triggered or not */
int
get_data()
{
  static unsigned char *prev, *buff;
  int i = 0;
/*   static char *p; */

  /* flush the sound card's buffer */

/*   sb_dspReset(); */
/*   p = buffer; */
/*   while (p < buffer + 640) { */
/*     *p = (char)sb_dspRead; */
/*     p++; */
/*   } */
  sb_dma8bitReadSC(buffer, 640);
  prev = buffer + SAMPLESKIP;
  buff = buffer + SAMPLESKIP + 2;
  if (scope.trige == 1) {
    while (((i++ < h_points)) && ((*(buff + scope.trigch) < scope.trig) ||
				  (*(prev + scope.trigch) >= scope.trig))) {
      prev += 2;
      buff += 2;
    }
  } else if (scope.trige == 2) {
    while (((i++ < h_points)) && ((*(buff + scope.trigch) > scope.trig) ||
				    (*(prev + scope.trigch) <= scope.trig))) {
      prev += 2;
      buff += 2;
    }
  }
  if (i > h_points)
    return(0);

  clip = 0;
  buff = (scope.trige ? prev : buffer);
  for(i=0; i < h_points; i++) {
    if (*buff == 0 || *buff == 255)
      clip = 1;
    ch[0].data[i] = (short)(*buff++) - 128;
    if (*buff == 0 || *buff == 255)
      clip = 2;
    ch[1].data[i] = (short)(*buff++) - 128;
  }
  return(1);
}

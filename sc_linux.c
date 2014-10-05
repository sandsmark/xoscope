/*
 * @(#)$Id: sc_linux.c,v 2.7 2014/10/05 15:43:54 baccala Exp $
 *
 * Copyright (C) 1996 - 2001 Tim Witham <twitham@quiknet.com>
 * Copyright (C) 2014 Gerhard Schiller <gerhard.schiller@gmail.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the Linux ESD and sound card interfaces
 *
 * These two interfaces are very similar and share a lot of code.  In
 * shared routines, we can tell which one we're using by looking at
 * the 'snd' and 'esd' variables to see which one is a valid file
 * descriptor (!= -1).  Although it's possible for both to be open at
 * the same time (say, when sound card is open, user pushes '&' to
 * select next device, and xoscope tries to open ESD to see if it
 * works before closing the sound card), this really shouldn't happen
 * in any of the important routines.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>		/* for abs() */
#include <sys/ioctl.h>
/*GERHARD*/
#include <alsa/asoundlib.h>
#include <linux/soundcard.h>
/*GERHARD*/
#include "oscope.h"		/* program defaults */
#ifdef HAVE_LIBESD
#include <esd.h>
#endif

#define ESDDEVICE "ESounD"
/*GERHARD*/
#define DEFAULT_ALSADEVICE "plughw:0,0"
char	alsaDevice[32] = "\0";
char 	alsaDeviceName[64] = "\0";
double	alsa_volts = 0.0;
/*GERHARD*/

/*GERHARD*/
static snd_pcm_t *handle 	= NULL;
snd_pcm_format_t pcm_format = 0;
/*GERHARD*/
static int esd = -2;		/* file descriptor for ESD */

#ifdef HAVE_LIBESD
static int esdblock = 0;	/* 1 to block ESD; 0 to non-block */
static int esdrecord = 0;	/* 1 to use ESD record mode; 0 to use ESD monitor mode */
#endif

static int sc_chans = 0;
static int sound_card_rate = DEF_R;	/* sampling rate of sound card */

/* Signal structures we're capturing into */
static Signal left_sig = {"Left Mix", "a"};
static Signal right_sig = {"Right Mix", "b"};

static int trigmode = 0;
static int triglev;
static int trigch;

/*GERHARD*/
static const char * snd_errormsg1 = NULL;
static const char * snd_errormsg2 = NULL;
/*GERHARD*/

#ifdef HAVE_LIBESD
static char * esd_errormsg1 = NULL;
static char * esd_errormsg2 = NULL;
#endif

/* This function is defined as do-nothing and weak, meaning it can be
 * overridden by the linker without error.  It's used for the X
 * Windows GUI for this data source, and is defined in this way so
 * that this object file can be used either with or without GTK.  If
 * this causes compiler problems, just comment out the attribute lines
 * and leave the do-nothing functions.
 */

/*GERHARD*/
/*void sc_gtk_option_dialog() __attribute__ ((weak));*/
/*void sc_gtk_option_dialog() {}*/

void esd_gtk_option_dialog() __attribute__ ((weak));
/*void esdsc_gtk_option_dialog() {}*/

void alsa_gtk_option_dialog() __attribute__ ((weak));
/*GERHARD*/

/* close the sound device */
static void
close_sound_card()
{
/*GERHARD*/
  /* fprintf(stderr,"close_sound_card\n"); */
  if (handle != NULL) {
	snd_pcm_drop(handle);
	snd_pcm_hw_free(handle);
	snd_pcm_close(handle);

    handle = NULL;
  }
}

/*GERHARD*/
/* show system error and close sound device if given ioctl status is bad */
/*static void*/
/*check_status_ioctl(int d, int request, void *argp, int line)*/
/*{*/
/*  if (ioctl(d, request, argp) < 0) {*/
/*    snd_errormsg1 = "sound ioctl";*/
/*    snd_errormsg2 = strerror(errno);*/
/*    close_sound_card();*/
/*  }*/
/*}*/
/*GERHARD*/

#ifdef HAVE_LIBESD

/* close ESD */
static void
close_ESD()
{
  if (esd >= 0) {
    close(esd);
    esd = -2;
  }
}

/* turn ESD on */
static int
open_ESD(void)
{
  if (esd >= 0) return 1;

  esd_errormsg1 = NULL;
  esd_errormsg2 = NULL;

  if (esdrecord) {
    esd = esd_record_stream_fallback(ESD_BITS8|ESD_STEREO|ESD_STREAM|ESD_RECORD,
				     ESD_DEFAULT_RATE, NULL, progname);
  } else {
    esd = esd_monitor_stream(ESD_BITS8|ESD_STEREO|ESD_STREAM|ESD_MONITOR,
                             ESD_DEFAULT_RATE, NULL, progname);
  }

  if (esd <= 0) {
    esd_errormsg1 = "opening " ESDDEVICE;
    esd_errormsg2 = strerror(errno);
    return 0;
  }

  /* we have esd connection! non-block it? */
  if (!esdblock)
    fcntl(esd, F_SETFL, O_NONBLOCK);

  return 1;
}

#endif

/*GERHARD*/
static int
open_sound_card(void)
{
  unsigned int rate = sound_card_rate;
  unsigned int chan = 2;
  int bits = 8;
  int rc;
  snd_pcm_hw_params_t *params;
/*GERHARD*/
  int dir = 0;
/*GERHARD*/
  snd_pcm_uframes_t pcm_frames;
 
/*  fprintf(stderr, "open_sound_card() sound_card_rate=%d\n", sound_card_rate); */

  if (handle != NULL){
	fprintf(stderr, "open_sound_card() already open\n");
  	return 1;
  	}

  snd_errormsg1 = NULL;
  snd_errormsg2 = NULL;

  
  /* Open PCM device for recording (capture). */
  rc = snd_pcm_open(&handle, alsaDevice, SND_PCM_STREAM_CAPTURE,  SND_PCM_NONBLOCK);
  if (rc < 0) {
    snd_errormsg1 = "opening ";
    snd_errormsg2 = snd_strerror(rc);
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
   return 0;
  }
  
  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);

  /* Fill it in with default values. */
  rc = snd_pcm_hw_params_any(handle, params);
  if (rc < 0) {
    snd_errormsg1 = "snd_pcm_hw_params_any() failed ";
    snd_errormsg2 = snd_strerror(rc);
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
	return 0;
  }
  
  /* Set the desired hardware parameters. */

  /* Interleaved mode */
  rc = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  if (rc < 0) {
    snd_errormsg1 = "snd_pcm_hw_params_set_access() failed ";
    snd_errormsg2 = snd_strerror(rc);
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
	return 0;
  }

  /* Signed 16-bit little-endian format */
  rc = -1;
  if(bits == 8){
  	rc = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_U8);
  	pcm_format = SND_PCM_FORMAT_U8;
  }
  else if(bits == 16){
   	rc = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
  	pcm_format = SND_PCM_FORMAT_S16_LE;
  }
  else{
    snd_errormsg1 = "Wrong number of bits";
    snd_errormsg2 = "";
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
    return 0;
  }
  
  if (rc < 0) {
    snd_errormsg1 = "snd_pcm_hw_params_set_format() failed ";
    snd_errormsg2 = snd_strerror(rc);
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
	return 0;
  }
  
  rc = snd_pcm_hw_params_get_format(params, &pcm_format);
  if (rc < 0) {
    snd_errormsg1 = "snd_pcm_hw_params_get_format() failed ";
    snd_errormsg2 = snd_strerror(rc);
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
	return 0;
  }
 
  if(bits == 8 && pcm_format != SND_PCM_FORMAT_U8){
    snd_errormsg1 = "Can't set 8-bit format (SND_PCM_FORMAT_U8)";
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
	return 0;
  }
 
  if(bits == 16 && pcm_format != SND_PCM_FORMAT_S16_LE){
    snd_errormsg1 = "Can't set 16-bit format (SND_PCM_FORMAT_S16_LE)";
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
	return 0;
  }
  
  /* Two channels (stereo) */
  rc = snd_pcm_hw_params_set_channels(handle, params, chan);
  if (rc < 0) {
    snd_errormsg1 = "snd_pcm_hw_params_set_channels() failed ";
    snd_errormsg2 = snd_strerror(rc);
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
	return 0;
  }
  rc = snd_pcm_hw_params_get_channels(params, &chan);
  if (rc < 0) {
    snd_errormsg1 = "snd_pcm_hw_params_get_channels() failed ";
    snd_errormsg2 = snd_strerror(rc);
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
	return 0;
  }
  sc_chans = chan;
  
  rc = snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir);
  if (rc < 0) {
    snd_errormsg1 = "snd_pcm_hw_params_set_rate_near() failed ";
    snd_errormsg2 = snd_strerror(rc);
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
	return 0;
  }
  if(rate != sound_card_rate){
     snd_errormsg1 = "requested sample rate not available ";
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
 	return 0;
  }
  sound_card_rate = rate;

  /* Set period size. */
  pcm_frames = (sound_card_rate / 1000) * SND_QUERY_INTERVALL * 2;
  /*  pcm_frames = 32;*/
  rc = snd_pcm_hw_params_set_period_size_near(handle, params, &pcm_frames, &dir);
  if (rc < 0) {
    snd_errormsg1 = "snd_pcm_hw_params_set_period_size_near() failed ";
    snd_errormsg2 = snd_strerror(rc);
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
	return 0;
  }

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    snd_errormsg1 = "snd_pcm_hw_params() failed ";
    snd_errormsg2 = snd_strerror(rc);
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
    return 0;
  }
  /* fprintf(stderr,"open_sound_card\n"); */

//   snd_pcm_hw_params_free (params);

  if ((rc = snd_pcm_prepare (handle)) < 0) {
    snd_errormsg1 = "snd_pcm_prepare() failed ";
    snd_errormsg2 = snd_strerror(rc);
    fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
    return 0;
  }

  return 1;
}
/*GERHARD*/

static void
reset_sound_card(void)
{
/*GERHARD*/
#ifdef HAVE_LIBESD
	static char junk[SAMPLESKIP];
#else
  	static unsigned char *junk = NULL; 
  	
	/* fprintf(stderr,"reset_sound_card()\n"); */
	if(junk == NULL){
		if(!(junk =  malloc((SAMPLESKIP * snd_pcm_format_width(pcm_format) / 8) * 2))) {
    		snd_errormsg1 = "malloc() failed " ;
    		snd_errormsg2 = strerror(errno);
     		fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
   		return;
  		}
	}
#endif  
/*GERHARD*/

#ifdef HAVE_LIBESD
  if (esd >= 0) {

    close_ESD();
    open_ESD();

    if (esd < 0) return;

    read(esd, junk, SAMPLESKIP);

    return;
  }
#endif

/*GERHARD*/
  	if(handle != NULL){
    	close_sound_card();
   		open_sound_card();

 		if(handle == NULL)
 			return;
		snd_pcm_readi(handle, junk, SAMPLESKIP);
  	}
/*GERHARD*/
}

#ifdef HAVE_LIBESD

static int esd_nchans(void)
{
  /* ESD always has two channels, right? */

  if (esd == -2) open_ESD();

  return (esd >= 0) ? 2 : 0;
}

#endif

static int sc_nchans(void)
{
/*GERHARD*/
  if (handle == NULL) 
  	open_sound_card();
  return (handle != NULL) ? sc_chans : 0;
/*GERHARD*/
}

static int fd(void)
{
/*GERHARD*/
#ifdef HAVE_LIBESD
  	return esd;
#else
	return(-1);
#endif
/*GERHARD*/
}

static Signal *sc_chan(int chan)
{
  return (chan ? &right_sig : &left_sig);
}

/* Triggering - we save the trigger level in the raw, unsigned
 * byte values that we read from the sound card
 */

static int set_trigger(int chan, int *levelp, int mode)
{
  trigch = chan;
  trigmode = mode;
  triglev = 127 + *levelp;
  if (triglev > 255) {
    triglev = 255;
    *levelp = 128;
  }
  if (triglev < 0) {
    triglev = 0;
    *levelp = -128;
  }
  return 1;
}

static void clear_trigger(void)
{
  trigmode = 0;
}

static int change_rate(int dir)
{
  int newrate = sound_card_rate;

  if (esd >= 0) return 0;

  if (dir > 0) {
    if (sound_card_rate > 16500)
      newrate = 44100;
    else if (sound_card_rate > 9500)
      newrate = 22050;
    else
      newrate = 11025;
  } else {
    if (sound_card_rate < 16500)
      newrate = 8000;
    else if (sound_card_rate < 33000)
      newrate = 11025;
    else
      newrate = 22050;
  }

  if (newrate != sound_card_rate) {
    sound_card_rate = newrate;
    return 1;
  }
  return 0;
}

static void
reset(void)
{
  reset_sound_card();

#ifdef HAVE_LIBESD
  left_sig.rate = esd >= 0 ? ESD_DEFAULT_RATE : sound_card_rate;
  right_sig.rate = esd >= 0 ? ESD_DEFAULT_RATE : sound_card_rate;
#else
  left_sig.rate = sound_card_rate;
  right_sig.rate = sound_card_rate;
#endif

  left_sig.num = 0;
  left_sig.frame ++;

  right_sig.num = 0;
  right_sig.frame ++;
/*GERHARD*/
  left_sig.volts = alsa_volts;
  right_sig.volts = alsa_volts;

  in_progress = 0;
/*GERHARD*/
}

/* set_width(int)
 *
 * sets the frame width (number of samples captured per sweep) globally
 * for all the channels.
 */

static void set_width(int width)
{

  /* fprintf(stderr, "set_width(%d)\n", width); */
  left_sig.width = width;
  right_sig.width = width;

  if (left_sig.data != NULL) free(left_sig.data);
  if (right_sig.data != NULL) free(right_sig.data);

  	left_sig.data = malloc(width * sizeof(short));
/*GERHARD*/
	if(left_sig.data == NULL){
		fprintf(stderr, "set_width(), malloc failed, %s\n", strerror(errno));
		exit(0);
	}
  	right_sig.data = malloc(width * sizeof(short));
	if(right_sig.data == NULL){
		fprintf(stderr, "set_width(), malloc failed, %s\n", strerror(errno));
		exit(0);
	}
/*GERHARD*/
}

/*GERHARD*/
#ifdef HAVE_LIBESD 
/* get data from sound card, return value is whether we triggered or not */
static int
esd_get_data()
{
/*GERHARD*/

  static unsigned char buffer[MAXWID * 2];
  static int i, j, delay;
  int fd;

/*GERHARD*/
  if (esd >= 0) fd = esd;
  else return (0);
/*GERHARD*/

  if (!in_progress) {
    /* Discard excess samples so we can keep our time snapshot close
       to real-time and minimize sound recording overruns.  For ESD we
       don't know how many are available (do we?) so we discard them
       all to start with a fresh buffer that hopefully won't wrap
       around before we get it read. */

    /* read until we get something smaller than a full buffer */
    while ((j = read(fd, buffer, sizeof(buffer))) == sizeof(buffer));

  } else {

    /* XXX this ends up discarding everything after a complete read */
    j = read(fd, buffer, sizeof(buffer));

  }

  i = 0;

  if (!in_progress) {

    if (trigmode == 1) {
      i ++;
      while (((i+1)*2 <= j) && ((buffer[2*i + trigch] < triglev) ||
				(buffer[2*(i-1) + trigch] >= triglev))) i ++;
    } else if (trigmode == 2) {
      i ++;
      while (((i+1)*2 <= j) && ((buffer[2*i + trigch] > triglev) ||
				(buffer[2*(i-1) + trigch] <= triglev))) i ++;
    }

    if ((i+1)*2 > j)		/* haven't triggered within the screen */
      return(0);		/* give up and keep previous samples */

    delay = 0;

    if (trigmode) {
      int last = buffer[2*(i-1) + trigch] - 127;
      int current = buffer[2*i + trigch] - 127;
      if (last != current)
	delay = abs(10000 * (current - (triglev - 127)) / (current - last));
    }

    left_sig.data[0] = buffer[2*i] - 127;
    left_sig.delay = delay;
    left_sig.num = 1;
    left_sig.frame ++;

    right_sig.data[0] = buffer[2*i + 1] - 127;
    right_sig.delay = delay;
    right_sig.num = 1;
    right_sig.frame ++;

    i ++;
    in_progress = 1;
  }

  while ((i+1)*2 <= j) {
    if (in_progress >= left_sig.width) break;
#if 0
    if (*buff == 0 || *buff == 255)
      clip = i % 2 + 1;
#endif
    left_sig.data[in_progress] = buffer[2*i] - 127;
    right_sig.data[in_progress] = buffer[2*i + 1] - 127;

    in_progress ++;
    i ++;
  }

  left_sig.num = in_progress;
  right_sig.num = in_progress;

  if (in_progress >= left_sig.width) in_progress = 0;

  return(1);
}
/*GERHARD*/
#endif // HAVE_LIBESD
/*GERHARD*/

/*GERHARD*/
/* get data from ALSA sound system, */
/* return value is 0 when we wait for a trigger event or on error, otherwise 1 */
/* in_progress: 0 when we start a new plot, when a plot is in progress, number of samples read. */

static int
sc_get_data()
{
  	static unsigned char *buffer = NULL;
  	static int frameBufferSize;
  	static int i, rdCnt, delay;

   // fprintf(stderr, "sc_get_data(%d %d)\n", in_progress,(MAXWID * snd_pcm_format_width(pcm_format) / 8) * 2);
 
  	if(handle == NULL){
	   fprintf(stderr, "sc_get_data() handle == NULL\n");
  		return(0);
	}
	
	if(buffer == NULL){
		if(!(buffer =  malloc((MAXWID * snd_pcm_format_width(pcm_format) / 8) * 2))) {
    		snd_errormsg1 = "malloc() failed ";
    		snd_errormsg2 = strerror(errno);
    		fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
    		return 0;
  		}
	}

		frameBufferSize = (sound_card_rate / 1000) * SND_QUERY_INTERVALL * 2;

	if (!in_progress) {
	    /* Discard excess samples so we can keep our time snapshot close
	       to real-time and minimize sound recording overruns.  For ESD we
	       don't know how many are available (do we?) so we discard them
	       all to start with a fresh buffer that hopefully won't wrap
	       around before we get it read. */

	    /* read until we get something smaller than a full buffer */
    	while ((rdCnt = snd_pcm_readi(handle, buffer, MAXWID)) == frameBufferSize)
/*   			fprintf(stderr, "sc_get_data(): discarding\n");*/
   			;

  	} 
  	else {
		/* XXX this ends up discarding everything after a complete read */
		rdCnt = snd_pcm_readi(handle, buffer, MAXWID);
	}

   if (rdCnt == -EPIPE) {
      /* EPIPE means overrun */
/*      fprintf(stderr, "overrun occurred\n");*/
	    snd_pcm_recover(handle, rdCnt, TRUE);
/*		usleep(100); */
      	return(sc_get_data());

   } 
    else if (rdCnt < 0) {
/*    	fprintf(stderr,*/
/*       		"error from snd_pcm_readi(): %d %s\n", rdCnt, snd_strerror(rdCnt));*/
	    snd_pcm_recover(handle, rdCnt, TRUE);
		usleep(1000); 
    	return(0); 
/*      	return(sc_get_data());*/
    } 

	rdCnt *= 2; // 2 bytes per frame (needs change for 16-bit mode)!

	i = 0;
	if (!in_progress) {
		if (trigmode == 1) {
			i ++;
			while (((i+1)*2 <= rdCnt) && 
					((buffer[2*i + trigch] < triglev) || (buffer[2*(i-1) + trigch] >= triglev))) 
				i ++;
    	} 
    	else if (trigmode == 2) {
			i ++;
			while (((i+1)*2 <= rdCnt) && 
				((buffer[2*i + trigch] > triglev) || (buffer[2*(i-1) + trigch] <= triglev))) 
				i ++;
    	}

		if ((i+1)*2 > rdCnt){	/* haven't triggered within the screen */
/*  			fprintf(stderr, "sc_get_data returns at %d\n", __LINE__);*/
			return(0);		/* give up and keep previous samples */
		}
		delay = 0;

		if (trigmode) {
			int last = buffer[2*(i-1) + trigch] - 127;
			int current = buffer[2*i + trigch] - 127;
      		if (last != current)
				delay = abs(10000 * (current - (triglev - 127)) / (current - last));
    	}

		left_sig.data[0] = buffer[2*i] - 127;
		left_sig.delay = delay;
		left_sig.num = 1;
		left_sig.frame ++;

		right_sig.data[0] = buffer[2*i + 1] - 127;
		right_sig.delay = delay;
		right_sig.num = 1;
		right_sig.frame ++;

		i ++;
		in_progress = 1;
	}

	while ((i+1)*2 <= rdCnt) {
		if (in_progress >= left_sig.width) // enough samples for a screen
			break;
#if 0
    	if (*buff == 0 || *buff == 255)
      		clip = i % 2 + 1;
#endif
		left_sig.data[in_progress] = buffer[2*i] - 127;
		right_sig.data[in_progress] = buffer[2*i + 1] - 127;

		in_progress ++;
		i ++;
	}

	left_sig.num = in_progress;
	right_sig.num = in_progress;

	if (in_progress >= left_sig.width){ // enough samples for a screen
		in_progress = 0;
	}
/* 	fprintf(stderr, "sc_get_data returns at %d\n", __LINE__);*/
	return(1);
}
/*GERHARD*/


/*GERHARD*/
static const char * snd_status_str(int i)
{
#ifdef DEBUG
  static char string[16];
  sprintf(string, "status %d", i);
  return string;
#endif
	static char msg1[64];
  switch (i) {
  case 0:
    if (snd_errormsg1){
		strcpy(msg1, snd_errormsg1);
		strcat(msg1, alsaDevice);
      	return (const char*)msg1;
	}
    else return "";
    
  case 2:
    if (snd_errormsg2) return snd_errormsg2;
    else return "";
  }
  return NULL;
}
/*GERHARD*/

#ifdef HAVE_LIBESD

/*GERHARD*/
static const char * esd_status_str(int i)
/*GERHARD*/
{
  switch (i) {
  case 0:
    if (esd_errormsg1) return esd_errormsg1;
    else return "";
  case 2:
    if (esd_errormsg2) return esd_errormsg2;
    else return "";
  }
  return NULL;
}

/* ESD option key 1 (*) - toggle Record mode */

static int option1_esd(void)
{
  if(esdrecord)
    esdrecord = 0;
  else
    esdrecord = 1;

  return 1;
}

static char * option1str_esd(void)
{
  static char string[16];

  sprintf(string, "RECORD:%d", esdrecord);
  return string;
}

static int esd_set_option(char *option)
{
  if (sscanf(option, "esdrecord=%d", &esdrecord) == 1) {
    return 1;
  } else {
    return 0;
  }
}

/* The GUI interface in sc_linux_gtk.c depends on the esdrecord option
 * being number 1.
 */

static char * esd_save_option(int i)
{
  static char buf[32];

  switch (i) {
  case 1:
    snprintf(buf, sizeof(buf), "esdrecord=%d", esdrecord);
    return buf;

  default:
    return NULL;
  }
}

#endif

#ifdef DEBUG
static char * option1str_sc(void)
{
  static char string[16];

  sprintf(string, "opt1");
  return string;
}

static char * option2str_sc(void)
{
  static char string[16];

  sprintf(string, "opt2");
  return string;
}
#endif

static int sc_set_option(char *option)
{
  if (sscanf(option, "rate=%d", &sound_card_rate) == 1) {
    return 1;
  } else if (strcmp(option, "dma=") == 0) {
    /* a deprecated option, return 1 so we don't indicate error */
    return 1;
  } else {
    return 0;
  }
}

static char * sc_save_option(int i)
{
  static char buf[32];

  switch (i) {
  case 0:
    snprintf(buf, sizeof(buf), "rate=%d", sound_card_rate);
    return buf;

  default:
    return NULL;
  }
}

#ifdef HAVE_LIBESD
DataSrc datasrc_esd = {
  "ESD",
  esd_nchans,
  sc_chan,
  set_trigger,
  clear_trigger,
  change_rate,
  set_width,
  reset,
  fd,
/*GERHARD*/
  esd_get_data,
/*GERHARD*/
  esd_status_str,
  option1_esd,  /* option1 */
  option1str_esd,  /* option1str */
  NULL,  /* option2, */
  NULL,  /* option2str, */
  esd_set_option,  /* set_option */
  esd_save_option,  /* save_option */
  esd_gtk_option_dialog,  /* gtk_options */
};
#endif

DataSrc datasrc_sc = {
/*GERHARD*/
  alsaDeviceName,
/*GERHARD*/
  sc_nchans,
  sc_chan,
  set_trigger,
  clear_trigger,
  change_rate,
  set_width,
  reset,
/*GERHARD*/
  fd,	//should be REALY unused....
  sc_get_data,
/*GERHARD*/
  snd_status_str,
#ifdef DEBUG
  NULL,
  option1str_sc,
  NULL,
  option2str_sc,
#else
  NULL,  /* option1, */
  NULL,  /* option1str, */
  NULL,  /* option2, */
  NULL,  /* option2str, */
#endif
  sc_set_option,
  sc_save_option,
  alsa_gtk_option_dialog,  /* gtk_options */
};

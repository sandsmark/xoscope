/* -*- mode: C++; indent-tabs-mode: nil; fill-column: 100; c-basic-offset: 4; -*-
 *
 * Copyright (C) 1996 - 2001 Tim Witham <twitham@quiknet.com>
 * Copyright (C) 2014 Gerhard Schiller <gerhard.schiller@gmail.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the Linux ALSA sound card interface
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>             /* for abs() */
#include <sys/ioctl.h>
#include <alsa/asoundlib.h>
#include <linux/soundcard.h>
#include "xoscope.h"            /* program defaults */

#define DEFAULT_ALSADEVICE "plughw:0,0"
char    alsaDevice[32] = "\0";
char    alsaDeviceName[64] = "\0";
double  alsa_volts = 0.0;

static snd_pcm_t *handle        = NULL;
snd_pcm_format_t pcm_format = 0;

static int sc_chans = 0;
static int sound_card_rate = DEF_R;     /* sampling rate of sound card */

/* Signal structures we're capturing into */
static Signal left_sig = {"Left Mix", "a"};
static Signal right_sig = {"Right Mix", "b"};

static int trigmode = 0;
static int triglev;
static int trigch;

static const char * snd_errormsg1 = NULL;
static const char * snd_errormsg2 = NULL;

/* This function is defined as do-nothing and weak, meaning it can be overridden by the linker
 * without error.  It's used for the X Windows GUI for this data source, and is defined in this way
 * so that this object file can be used either with or without GTK.  If this causes compiler
 * problems, just comment out the attribute lines and leave the do-nothing functions.
 */
void alsa_gtk_option_dialog() __attribute__ ((weak));

/* close the sound device */
static void close_sound_card(void)
{
    /* fprintf(stderr,"close_sound_card\n"); */
    if (handle != NULL) {
        snd_pcm_drop(handle);
        snd_pcm_hw_free(handle);
        snd_pcm_close(handle);

        handle = NULL;
    }
}

static int open_sound_card(void)
{
    unsigned int rate = sound_card_rate;
    unsigned int chan = 2;
    int bits = 8;
    int rc;
    snd_pcm_hw_params_t *params;
    int dir = 0;
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
    if (bits == 8) {
        rc = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_U8);
        pcm_format = SND_PCM_FORMAT_U8;
    } else if (bits == 16) {
        rc = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
        pcm_format = SND_PCM_FORMAT_S16_LE;
    } else {
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

    if (bits == 8 && pcm_format != SND_PCM_FORMAT_U8) {
        snd_errormsg1 = "Can't set 8-bit format (SND_PCM_FORMAT_U8)";
        fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
        return 0;
    }

    if (bits == 16 && pcm_format != SND_PCM_FORMAT_S16_LE) {
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
    if (rate != sound_card_rate) {
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

    if ((rc = snd_pcm_prepare (handle)) < 0) {
        snd_errormsg1 = "snd_pcm_prepare() failed ";
        snd_errormsg2 = snd_strerror(rc);
        fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
        return 0;
    }

    return 1;
}

static void reset_sound_card(void)
{
    static unsigned char *junk = NULL;

    /* fprintf(stderr,"reset_sound_card()\n"); */
    if (junk == NULL) {
        if (!(junk =  malloc((SAMPLESKIP * snd_pcm_format_width(pcm_format) / 8) * 2))) {
            snd_errormsg1 = "malloc() failed " ;
            snd_errormsg2 = strerror(errno);
            fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
            return;
        }
    }

    if (handle != NULL) {
        close_sound_card();
        open_sound_card();

        if (handle == NULL) {
            return;
        }
        snd_pcm_readi(handle, junk, SAMPLESKIP);
    }
}

static int sc_nchans(void)
{
    if (handle == NULL) {
        open_sound_card();
    }
    return (handle != NULL) ? sc_chans : 0;
}

static int fd(void)
{
    return -1;
}

static Signal *sc_chan(int chan)
{
    return (chan ? &right_sig : &left_sig);
}

/* Triggering - we save the trigger level in the raw, unsigned byte values that we read from the
 * sound card
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

static void reset(void)
{
    reset_sound_card();

    left_sig.rate = sound_card_rate;
    right_sig.rate = sound_card_rate;

    left_sig.num = 0;
    left_sig.frame ++;

    right_sig.num = 0;
    right_sig.frame ++;

    left_sig.volts = alsa_volts;
    right_sig.volts = alsa_volts;

    in_progress = 0;
}

/* set_width(int)
 *
 * sets the frame width (number of samples captured per sweep) globally for all the channels.
 */

static void set_width(int width)
{
    /* fprintf(stderr, "set_width(%d)\n", width); */
    left_sig.width = width;
    right_sig.width = width;

    if (left_sig.data != NULL) 
        free(left_sig.data);
    if (right_sig.data != NULL) 
        free(right_sig.data);

    left_sig.data = malloc(width * sizeof(short));
    if (left_sig.data == NULL) {
        fprintf(stderr, "set_width(), malloc failed, %s\n", strerror(errno));
        exit(0);
    }
    bzero(left_sig.data, width * sizeof(short));
    
    right_sig.data = malloc(width * sizeof(short));
    if (right_sig.data == NULL) {
        fprintf(stderr, "set_width(), malloc failed, %s\n", strerror(errno));
        exit(0);
    }
    bzero(right_sig.data, width * sizeof(short));
}


/* get data from ALSA sound system, */
/* return value is 0 when we wait for a trigger event or on error, otherwise 1 */
/* in_progress: 0 when we start a new plot, when a plot is in progress, number of samples read. */

static int sc_get_data(void)
{
    static unsigned char *buffer = NULL;
    static int frameBufferSize;
    static int i, rdCnt, delay;

    // fprintf(stderr, "sc_get_data(%d %d)\n", in_progress,(MAXWID * snd_pcm_format_width(pcm_format) / 8) * 2);

    if (handle == NULL) {
        fprintf(stderr, "sc_get_data() handle == NULL\n");
        return 0;
    }

    if (buffer == NULL) {
        if (!(buffer =  malloc((MAXWID * snd_pcm_format_width(pcm_format) / 8) * 2))) {
            snd_errormsg1 = "malloc() failed ";
            snd_errormsg2 = strerror(errno);
            fprintf(stderr, "%s\n%s\n", snd_errormsg1, snd_errormsg2);
            return 0;
        }
    }

    frameBufferSize = (sound_card_rate / 1000) * SND_QUERY_INTERVALL * 2;

    if (!in_progress) {
        /* Discard excess samples so we can keep our time snapshot close to real-time and minimize
         * sound recording overruns.  For ESD we don't know how many are available (do we?) so we
         * discard them all to start with a fresh buffer that hopefully won't wrap around before we
         * get it read.
         */

        /* read until we get something smaller than a full buffer */
        while ((rdCnt = snd_pcm_readi(handle, buffer, MAXWID)) == frameBufferSize)
            /*                          fprintf(stderr, "sc_get_data(): discarding\n");*/
            ;

    } else {
        /* XXX this ends up discarding everything after a complete read */
        rdCnt = snd_pcm_readi(handle, buffer, MAXWID);
    }

    if (rdCnt == -EPIPE) {
        /* EPIPE means overrun */
        /*      fprintf(stderr, "overrun occurred\n");*/
        snd_pcm_recover(handle, rdCnt, TRUE);
        return sc_get_data();
    } else if (rdCnt < 0) {
        /*      fprintf(stderr,*/
        /*                      "error from snd_pcm_readi(): %d %s\n", rdCnt, snd_strerror(rdCnt));*/
        snd_pcm_recover(handle, rdCnt, TRUE);
        usleep(1000);
        return 0;
        /*              return(sc_get_data());*/
    }

    rdCnt *= 2; // 2 bytes per frame (needs change for 16-bit mode)!

    i = 0;
    if (!in_progress) {
        if (trigmode == 1) {
            i ++;
            while (((i+1)*2 <= rdCnt) &&
                   ((buffer[2*i + trigch] < triglev) || (buffer[2*(i-1) + trigch] >= triglev))) {
                i ++;
            }
        } else if (trigmode == 2) {
            i ++;
            while (((i+1)*2 <= rdCnt) &&
                   ((buffer[2*i + trigch] > triglev) || (buffer[2*(i-1) + trigch] <= triglev))) {
                i ++;
            }
        }

        if ((i+1)*2 > rdCnt) {  /* haven't triggered within the screen */
            /*                          fprintf(stderr, "sc_get_data returns at %d\n", __LINE__);*/
            return 0;           /* give up and keep previous samples */
        }
        delay = 0;

        if (trigmode) {
            int last = buffer[2*(i-1) + trigch] - 127;
            int current = buffer[2*i + trigch] - 127;
            if (last != current) {
                delay = abs(10000 * (current - (triglev - 127)) / (current - last));
            }
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
        if (in_progress >= left_sig.width) { // enough samples for a screen
            break;
        }
#if 0
        if (*buff == 0 || *buff == 255) {
            clip = i % 2 + 1;
        }
#endif
        left_sig.data[in_progress] = buffer[2*i] - 127;
        right_sig.data[in_progress] = buffer[2*i + 1] - 127;

        in_progress ++;
        i ++;
    }

    left_sig.num = in_progress;
    right_sig.num = in_progress;

    if (in_progress >= left_sig.width) { // enough samples for a screen
        in_progress = 0;
    }
    /*  fprintf(stderr, "sc_get_data returns at %d\n", __LINE__);*/
    return 1;
}

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
        } else {
            return "";
        }

    case 2:
        if (snd_errormsg2) {
            return snd_errormsg2;
        } else {
            return "";
        }
    }
    return NULL;
}

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

DataSrc datasrc_sc = {
    alsaDeviceName,
    sc_nchans,
    sc_chan,
    set_trigger,
    clear_trigger,
    change_rate,
    set_width,
    reset,
    fd, //should be REALY unused....
    sc_get_data,
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


/*
 * @(#)$Id: func.c,v 2.5 2009/07/30 02:37:25 baccala Exp $
 *
 * Copyright (C) 1996 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the signal math and memory.
 * To add math functions, search for !!! and add to those sections.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "oscope.h"
/* #include "fft.h" */
#include "display.h"
#include "func.h"
#include "com_gtk.h"

Signal mem[26];		/* 26 memories, corresponding to 26 letters */

/* recall given memory register to the currently selected signal */
void
recall_on_channel(Signal *signal, Channel *ch)
{
  if (ch->signal) ch->signal->listeners --;
  ch->signal = signal;
  if (signal) {
    signal->listeners ++;

    /* no guarantee that signal->bits will be correct yet */
    ch->bits = signal->bits;
  }
}

void
recall(Signal *signal)
{
  recall_on_channel(signal, &ch[scope.select]);
}

extern int in_progress;
/* store the currently selected signal to the given memory register */
void
save(char c)
{
  int i;

  i = c - 'A';

  if (ch[scope.select].signal == NULL) return;

  /* Don't want the name - leave that at 'Memory x'
   * Also, increment frame instead of setting it to signal->frame in
   * case signal->frame is the same as mem's old frame number!
   */
  if (mem[i].data != NULL) {
    free(mem[i].data);
  }
  mem[i].data = malloc(ch[scope.select].signal->width * sizeof(short));
  if(mem[i].data == NULL){
  	fprintf(stderr, "malloc failed in save()\n");
  	exit(0);
  } 
  memcpy(mem[i].data, ch[scope.select].signal->data, ch[scope.select].signal->width * sizeof(short));
  
  mem[i].rate = ch[scope.select].signal->rate;
  mem[i].num = ch[scope.select].signal->num;
  mem[i].width = ch[scope.select].signal->width;
  mem[i].frame ++;
  mem[i].volts = ch[scope.select].signal->volts;
}

/* !!! External process handling
 *
 * Could use some work... the original code (xoscope-1.8) always sent
 * the first h_points data points from the first two display channels
 * through to the external process, even if the scaling on those
 * channels was set up so that weren't h_points of valid data, or
 * set down so that what was displayed was way more than h_points.
 * In any event, I'm not a big fan of these math functions, so
 * I just left it alone like that.
 */

struct external {
  struct external *next;
  Signal signal;
  int pid;			/* Zero if we already closed it down */
  int to, from;			/* Pipes */
  int last_frame_ch0, last_frame_ch1;
  int last_num_ch0, last_num_ch1;
};

static struct external *externals = NULL;

/* startcommand() / start_command_on_channel()
 *
 * Start an external command running on the current display channel.
 *
 * gr_* UIs call this after prompting for command to run
 */

void
start_command_on_channel(const char *command, Channel *ch_select)
{
  struct external *ext;
  int pid;
  int from[2], to[2];
  static char *path, *oscopepath;

  if (pipe(to) || pipe(from)) { /* get a set of pipes */
    sprintf(error, "%s: can't create pipes", progname);
    perror(error);
    return;
  }

  signal(SIGPIPE, SIG_IGN);

  if ((pid = fork()) > 0) {		/* parent */
    close(to[0]);
    close(from[1]);
  } else if (pid == 0) {		/* child */
    close(to[1]);
    close(from[0]);
    close(0);
    close(1);				/* redirect stdin/out through pipes */
    dup2(to[0], 0);
    dup2(from[1], 1);
    close(to[0]);
    close(from[1]);

    /* XXX add additional environment vars here for sampling rate
     * and number of samples per frame
     */

    if ((oscopepath = getenv("OSCOPEPATH")) == NULL)
      oscopepath = PACKAGE_LIBEXEC_DIR;
    if ((path = malloc(strlen(oscopepath) + 6)) != NULL) {
	  if(path == NULL){
	  	fprintf(stderr, "malloc failed in start_command_on_channel() for path\n");
	  	exit(0);
	  } 
    
      sprintf(path,"PATH=%s", oscopepath);
      putenv(path);
      /* putenv() requires buffer to stick around, so no free(),
       * but we're in the child, and about to exec, so no big deal
       */
    }

    execlp("/bin/sh", "sh", "-c", command, NULL);
    sprintf(error, "%s: child can't exec /bin/sh -c \"%s\"",
	    progname, command);
    perror(error);
    exit(1);
  } else {			/* fork error */
    sprintf(error, "%s: can't fork", progname);
    perror(error);
    return;
  }

  ext = malloc(sizeof(struct external));
  if (ext == NULL) {
    fprintf(stderr, "malloc() struct external failed\n");
    return;
  }
  bzero(ext, sizeof(struct external));

  strncpy(ext->signal.savestr, command, sizeof(ext->signal.savestr));
  ext->pid = pid;
  ext->from = from[0];
  ext->to = to[1];

	ext->signal.data = malloc(ch[0].signal->width * sizeof(short));
    if(ext->signal.data == NULL){
	  	fprintf(stderr, "malloc failed in start_command_on_channel() for signal.data\n");
	  	exit(0);
	} 
	ext->signal.width = ch[0].signal->width;
	ext->signal.num = ch[0].signal->num;
	ext->signal.rate = ch[0].signal->rate;
	ext->signal.delay = ch[0].signal->delay;
	ext->signal.bits = ch[0].signal->bits;
/*	fprintf(stderr, "start_command_on_channel: width = %d\n", ext->signal.width);*/

  ext->next = externals;
  externals = ext;

  message(command);

  recall_on_channel(&ext->signal, ch_select);
	ch[scope.select].show = 1;
}

void
startcommand(const char *command)
{
  if (scope.select > 1) {
    start_command_on_channel(command, &ch[scope.select]);
    clear();
  }
}

void
restart_command_on_channel(void)
{
	struct external *ext;


	for (ext = externals; ext != NULL; ext = ext->next) {
/*		fprintf(stderr, "1. restart_command_on_channel: width = %d\n", ext->signal.width);*/
		if(ext->signal.data != NULL)
			free(ext->signal.data);
		ext->signal.data = malloc(ch[0].signal->width * sizeof(short));	
  		if(ext->signal.data == NULL){
	  		fprintf(stderr, "malloc failed in restart_command_on_channel() for signal.data\n");
	  		exit(0);
		} 
		ext->signal.width = ch[0].signal->width;
		ext->signal.num = ch[0].signal->num;
/*		fprintf(stderr, "2. restart_command_on_channel: width = %d\n", ext->signal.width);*/
	}
}


/* Check everything on the externals list; run what needs to be run,
 * and clean up anything left linguring behind.
 */

static void
run_externals(void)
{
  struct external *ext;

  short *a, *b, *c;
  int i, errors;

  for (ext = externals; ext != NULL; ext = ext->next) {

    if (ext->signal.listeners > 0) {

      if ((ext->pid > 0) && (ch[0].signal != NULL) && (ch[1].signal != NULL)) {

	/* There's a slight chance that if we change one of the channels,
	 * the new channel may have a frame number identical to the last
	 * one, but that shouldn't hurt us too bad.
	 */

	if ((ch[0].signal->frame != ext->last_frame_ch0) ||
	    (ch[1].signal->frame != ext->last_frame_ch1)) {

	  ext->last_frame_ch0 = ch[0].signal->frame;
	  ext->last_frame_ch1 = ch[1].signal->frame;
	  ext->signal.frame ++;
	  ext->signal.num = 0;
	}

	/* We may already have sent and received part of a frame, so
	 * start our pointers at whatever our last offset was, and
	 * keep going until we hit the limit of either channel 0
	 * or channel 1.   XXX explain this better
	 * XXX make sure this can't slow the program down!
	 */

	a = ch[0].signal->data + ext->signal.num;
	b = ch[1].signal->data + ext->signal.num;
	c = ext->signal.data + ext->signal.num;
	errors = 0;
	for (i = ext->signal.num;
	     (i < ch[0].signal->num) && (i < ch[1].signal->num); i++) {
	  if (write(ext->to, a++, sizeof(short)) != sizeof(short))
	    errors ++;
	  if (write(ext->to, b++, sizeof(short)) != sizeof(short))
	    errors ++;
	  if (read(ext->from, c++, sizeof(short)) != sizeof(short))
	    errors ++;
		
		if(errors){
			fprintf(stderr, "run_externals() r/w-error. ch[0].signal->num=%d, ext->signal.num=%d i=%d\n", ch[0].signal->num, ext->signal.num, i);
			return;
		}
	}
	ext->signal.num = i;

	if (errors) {
	  sprintf(error, "%s: %d pipe r/w errors from \"%s\"",
		  progname, errors, ext->signal.savestr);
	  perror(error);
	  /* XXX do something here other than perror to notify user */

	  close(ext->from);
	  close(ext->to);
	  waitpid(ext->pid, NULL, 0);
	  ext->pid = 0;
	}
      }

    } else {

      /* Nobody listening anymore; close down the pipes and wait for
       * process to exit.   Maybe we should timeout in case of a hang?
       */

      if (ext->pid) {
	close(ext->from);
	close(ext->to);
	waitpid(ext->pid, NULL, 0);
      }

      /* Delete ext from list and free() it */

    }
  }
}

/* !!! The functions; they take one arg: a Signal ptr to store results in */

/* Invert */
void
inv(Signal *dest, Signal *src)
{
  int i;
  short *a, *b;

  if (src == NULL) return;

  dest->rate = src->rate;
  dest->num = src->num;
  dest->volts = src->volts;
  dest->frame = src->frame;

  a = src->data;
  b = dest->data;
  for (i = 0 ; i < src->num; i++) {
    *b++ = -1 * *a++;
  }
}

void
inv1(Signal *sig)
{
  inv(sig, ch[0].signal);
}

void
inv2(Signal *sig)
{
  inv(sig, ch[1].signal);
}

/* The sum of the two channels */
void
sum(Signal *dest)
{
  int i;
  short *a, *b, *c;

  if ((ch[0].signal == NULL) || (ch[1].signal == NULL)) return;

  a = ch[0].signal->data;
  b = ch[1].signal->data;
  c = dest->data;

  dest->frame = ch[0].signal->frame + ch[1].signal->frame;
  dest->num = ch[0].signal->num;

  if (dest->num > ch[1].signal->num) dest->num = ch[1].signal->num;

  for (i = 0 ; i < dest->num ; i++) {
    *c++ = *a++ + *b++;
  }
}

/* The difference of the two channels */
void
diff(Signal *dest)
{
  int i;
  short *a, *b, *c;

  if ((ch[0].signal == NULL) || (ch[1].signal == NULL)) return;

  a = ch[0].signal->data;
  b = ch[1].signal->data;
  c = dest->data;

  dest->frame = ch[0].signal->frame + ch[1].signal->frame;
  dest->num = ch[0].signal->num;

  if (dest->num > ch[1].signal->num) dest->num = ch[1].signal->num;

  for (i = 0 ; i < dest->num ; i++) {
    *c++ = *a++ - *b++;
  }
}


/* The average of the two channels */
void
avg(Signal *dest)
{
  int i;
  short *a, *b, *c;

  if ((ch[0].signal == NULL) || (ch[1].signal == NULL)) return;

  a = ch[0].signal->data;
  b = ch[1].signal->data;
  c = dest->data;

  dest->frame = ch[0].signal->frame + ch[1].signal->frame;
  dest->num = ch[0].signal->num;

  if (dest->num > ch[1].signal->num) dest->num = ch[1].signal->num;

  for (i = 0 ; i < dest->num ; i++) {
    *c++ = (*a++ + *b++) / 2;
  }
}

/* Fast Fourier Transform of channels 0 and 1
 *
 * The point of the dest->frame calculation is that the value changes
 * whenever the data changes, but if the data is constant, it doesn't
 * change.  The display code only looks at changes in frame number to
 * decide when to redraw a signal; the actual value doesn't matter.
 */

void
fft1(Signal *dest)
{
	if (ch[0].signal == NULL) 
		return;
	if(ch[0].signal->width < FFTLEN){
		bzero(dest->data, sizeof(short) * FFTLEN);
		return;
	}
	
	fft(ch[0].signal->data, dest->data, ch[0].signal->width);
}

/*#define FFT_TEST*/
#ifndef FFT_TEST
void
fft2(Signal *dest)
{
	if (ch[1].signal == NULL) 
		return;
	if(ch[1].signal->width < FFTLEN){
		bzero(dest->data, sizeof(short) * FFTLEN);
		return;
	}
	
	fft(ch[1].signal->data, dest->data, ch[1].signal->width);
}

#else
void
make_sin(short * data, double freq, int rate)
{
	int i;
	for(i = 0; i < FFTLEN; i++){
		data[i] = sin( 360.0 * ((1.0/((double)rate/freq)) * i) * M_PI / 180.0) * 100;
	}
} 

void
fft2(Signal *dest)
{
	short testdata[FFTLEN];
	int i;

	make_sin(testdata, 2500.0, ch[1].signal->rate);
	fft(testdata, dest->data, FFTLEN);

	for(i = 0; i<FFTLEN; i+=51)
		dest->data[i] = -80;	
}
#endif

/* isvalid() functions for the various math functions.
 *
 * These functions also have the side effect of setting the volts/rate
 * fields in the Signal structure, something we count on happening
 * whenever we call update_math_signals(), which calls these
 * functions.
 *
 * These functions are also responsible for mallocing the data areas
 * in the math function's associated Signal structures.
 */

int ch1active(Signal *dest)
{
  dest->frame = 0;
  dest->num = 0;

  if (ch[0].signal == NULL) {
    dest->rate = 0;
    dest->volts = 0;
    return 0;
  }

  dest->rate = ch[0].signal->rate;
  dest->volts = ch[0].signal->volts;

  if (dest->width != ch[0].signal->width) {
    dest->width = ch[0].signal->width;
    if (dest->data != NULL) 
      free(dest->data);
      dest->data = malloc(ch[0].signal->width * sizeof(short));
      if(dest->data == NULL){
	  	fprintf(stderr, "malloc failed in ch1active()\n");
	  	exit(0);
	  } 
  }

  return 1;
}

int ch2active(Signal *dest)
{
  dest->frame = 0;
  dest->num = 0;

  if (ch[1].signal == NULL) {
    dest->rate = 0;
    dest->volts = 0;
    return 0;
  }

  dest->rate = ch[1].signal->rate;
  dest->volts = ch[1].signal->volts;

  if (dest->width != ch[1].signal->width) {
    dest->width = ch[1].signal->width;
    if (dest->data != NULL) 
      free(dest->data);
    dest->data = malloc(ch[1].signal->width * sizeof(short));
    if(dest->data == NULL){
	  	fprintf(stderr, "malloc failed in ch2active()\n");
	  	exit(0);
	} 
  }

  return 1;
}

int chs12active(Signal *dest)
{
  dest->frame = 0;
  dest->num = 0;

  if ((ch[0].signal == NULL) || (ch[1].signal == NULL)
      || (ch[0].signal->rate != ch[1].signal->rate)
      || (ch[0].signal->volts != ch[1].signal->volts)) {
    dest->rate = 0;
    dest->volts = 0;
    return 0;
  }

  dest->rate = ch[0].signal->rate;
  dest->volts = ch[0].signal->volts;

  /* All of the associated functions (sum, diff, avg) only use the
   * minimum of the samples on Channels 1 and 2, so we can safely base
   * the size of our data array on Channel 1 only... the worst that
   * can happen is that it is too big.
   */

  if (dest->width != ch[0].signal->width) {
    dest->width = ch[0].signal->width;
    if (dest->data != NULL) free(dest->data);
      dest->data = malloc(ch[0].signal->width * sizeof(short));
      if(dest->data == NULL){
	  	fprintf(stderr, "malloc failed in ch12active()\n");
	  	exit(0);
	  } 
  }

  return 1;
}

/* special isvalid() functions for FFT
 *
 * First, it allocates memory for the generated fft.
 *
 * Second, it sets the "rate", so that the increment 
 * from grid line to grid line is some "nice" value.
 * (a muliple of 500 Hz if increment is > 1kHz, otherwise
 * a multiple of 100 hZ.
 * 
 * Third: this value is stored in the "volts" member of
 * the dest signal structure. It is only displayed in the label.  
 */

int ch1FFTactive(Signal *dest)
{
	gfloat	left, right;
	int		HzDiv, HzDivAdj;

  	if (ch[0].signal == NULL) {
    	dest->rate = 0;
    	return 0;
  	} 

	if(ch[0].signal->width < FFTLEN){
		message("Too few samples to run FFT");
/*		fprintf(stderr,"ch1FFTactive(): Too few samples to run FFT\n");*/
    	return 0;
  	} 

	dest->width = 	FFTLEN / 2;
  	dest->num = 	FFTLEN / 2;
  	dest->frame = 	0;
	dest->bits = 	0;
	dest->delay = 	0;

	if(dest->data == NULL)
		dest->data = malloc(FFTLEN * sizeof(short));
    if(dest->data == NULL){
	    fprintf(stderr, "malloc failed in ch1FFTactive()\n");
	  	exit(0);
	} 
	bzero(dest->data, FFTLEN * sizeof(short));

	// (signal->rate / 2) = max FFT-freq
	HzDiv = ch[0].signal->rate / 2 / total_horizontal_divisions; 
	if(HzDiv > 1000)
		HzDivAdj = HzDiv - (HzDiv % 500) + 500;
	else
		HzDivAdj = HzDiv - (HzDiv % 100) + 100;

  	dest->volts = HzDivAdj;

	gtk_databox_get_visible_limits(GTK_DATABOX(databox), &left, &right, NULL, NULL);
    dest->rate = -FFTLEN/(right - left)/2; 
/* 	fprintf(stderr,"ch1FFTactive(): dest->rate=%d -> ", dest->rate);*/

	dest->rate *= (gfloat)HzDivAdj / (gfloat)HzDiv;
/*	fprintf(stderr,"%d\n", dest->rate);*/

/*	fprintf(stderr,"ch1FFTactive(): ch[0].signal->rate=%d\n", ch[0].signal->rate);*/
/*	fprintf(stderr,"ch1FFTactive(): left=%f, right=%f, total_horizontal_divisions=%d\n", 
                                                left, right, total_horizontal_divisions);*/
/*	fprintf(stderr,"ch1FFTactive(): Hz/div=%d -> %d\n", HzDiv, HzDivAdj);*/
/*	fprintf(stderr,"ch1FFTactive(): ch[1].signal->rate=%d, dest->rate=%d\n", 
                                                ch[1].signal->rate, dest->rate);*/
	return 1;
}

int ch2FFTactive(Signal *dest)
{
	gfloat	left, right;
	int		HzDiv, HzDivAdj;

  	if (ch[1].signal == NULL) {
    	dest->rate = 0;
    	return 0;
  	} 

	if(ch[1].signal->width < FFTLEN){
		message("Too few samples to run FFT");
    	return 0;
  	} 

	dest->width = 	FFTLEN / 2;
  	dest->num = 	FFTLEN / 2;
  	dest->frame = 	0;
	dest->bits = 	0;
	dest->delay = 	0;

	if(dest->data == NULL)
		dest->data = malloc(FFTLEN * sizeof(short));
    if(dest->data == NULL){
        fprintf(stderr, "malloc failed in ch2FFTactive()\n");
	  	exit(0);
    } 
	bzero(dest->data, FFTLEN * sizeof(short));

	// (signal->rate / 2) = max FFT-freq
	HzDiv = ch[1].signal->rate / 2 / total_horizontal_divisions; 
	if(HzDiv > 1000)
		HzDivAdj = HzDiv - (HzDiv % 500) + 500;
	else
		HzDivAdj = HzDiv - (HzDiv % 100) + 100;
  	dest->volts = HzDivAdj;

	gtk_databox_get_visible_limits(GTK_DATABOX(databox), &left, &right, NULL, NULL);
    dest->rate = -FFTLEN/(right - left)/2; 
/* 	fprintf(stderr,"ch2FFTactive(): dest->rate=%d -> ", dest->rate);*/

	dest->rate *= (gfloat)HzDivAdj / (gfloat)HzDiv;
/*	fprintf(stderr,"%d\n", dest->rate);*/

/*	fprintf(stderr,"ch2FFTactive(): ch[1].signal->rate=%d\n", ch[1].signal->rate);*/
/*	fprintf(stderr,"ch2FFTactive(): left=%f, right=%f, total_horizontal_divisions=%d\n", 
                                            left, right, total_horizontal_divisions);*/
/*	fprintf(stderr,"ch2FFTactive(): Hz/div=%d -> %d\n", HzDiv, HzDivAdj);*/
/*	fprintf(stderr,"ch2FFTactive(): ch[1].signal->rate=%d, dest->rate=%d\n", 
                                            ch[1].signal->rate, dest->rate);*/
	return 1;
}

struct func {
  void (*func)(Signal *);
  char *name;
  int (*isvalid)(Signal *);	/* returns TRUE if this function is valid */
  Signal signal;
};

struct func funcarray[] =
{
  {inv1, "Inv. 1  ", ch1active},
  {inv2, "Inv. 2  ", ch2active},
  {sum,  "Sum  1+2", chs12active},
  {diff, "Diff 1-2", chs12active},
  {avg,  "Avg. 1,2", chs12active},
  {fft1, "FFT. 1  ", ch1FFTactive}, 
  {fft2, "FFT. 2  ", ch2FFTactive}, 
};

/* the total number of "functions" */
int funccount = sizeof(funcarray) / sizeof(struct func);

/* Cycle current scope chan to next function, taking heavy advantage
 * of C incrementing pointers by the size of the thing they point to.
 * Start by finding the current function in the function array that
 * the channel is pointing to, and advancing to the next one,
 * selecting the first item in the array if either we're currently
 * pointing to the end of the array or pointing to something other
 * than a function.  Then keep going, looking for the first function
 * that returns TRUE to an isvalid() test, taking care that none of
 * the functions may currently be valid.
 */

void next_func(void)
{
  struct func *func, *func2;
  Channel *chan = &ch[scope.select];

  for (func = &funcarray[0]; func < &funcarray[funccount]; func++) {
    if (chan->signal == &func->signal) break;
  }

  if (func == &funcarray[funccount]) func = &funcarray[0];
  else if (func == &funcarray[funccount-1]) func = &funcarray[0];
  else func ++;

  /* At this point, func points to the candidate function structure.
   * See if it's valid, and keep going forward if it isn't
   */

  func2 = func;
  do {
    if (func->isvalid(&func->signal)) {
      recall(&func->signal);
      return;
    }
    func ++;
    if (func == &funcarray[funccount]) func = &funcarray[0];
  } while (func != func2);

  /* If we're here, it's because we went through all the functions
   * without finding one that returned valid.  No choice but to
   * clear the channel.
   */

  recall(NULL);
}

/* Basically the same deal, but moving backwards in the array. */

void prev_func(void)
{
  struct func *func, *func2;
  Channel *chan = &ch[scope.select];

  for (func = &funcarray[0]; func < &funcarray[funccount]; func++) {
    if (chan->signal == &func->signal) break;
  }

  if (func == &funcarray[funccount]) func = &funcarray[funccount-1];
  else if (func == &funcarray[0]) func = &funcarray[funccount-1];
  else func --;

  /* At this point, func points to the candidate function structure.
   * See if it's valid and go further backwards if it isn't
   */

  func2 = func;
  do {
    if (func->isvalid(&func->signal)) {
      recall(&func->signal);
      return;
    }
    func --;
    if (func < &funcarray[0]) func = &funcarray[funccount-1];
  } while (func != func2);

  /* If we're here, it's because we went through all the functions
   * without finding one that returned valid.  No choice but to
   * clear the channel.
   */

  recall(NULL);
}

int function_bynum_on_channel(int fnum, Channel *ch)
{
  if ((fnum >= 0) && (fnum < funccount)
      && funcarray[fnum].isvalid(&funcarray[fnum].signal)) {
    recall_on_channel(&funcarray[fnum].signal, ch);
    return TRUE;
  }
  return FALSE;
}

/* Initialize math, called once by main at startup, and again whenever
 * we read a file.
 */

void
init_math()
{
  static int i;
  static int once = 0;

  for (i = 0 ; i < 26 ; i++) {
    if (once==1 && mem[i].data != NULL) {
      free(mem[i].data);
    }
    mem[i].data = NULL;
    mem[i].num = mem[i].frame = mem[i].volts = 0;
    mem[i].listeners = 0;
    sprintf(mem[i].name, "Memory %c", 'a' + i);
    mem[i].savestr[0] = 'a' + i;
    mem[i].savestr[1] = '\0';
  }
  for (i = 0; i < funccount; i++) {
    strcpy(funcarray[i].signal.name, funcarray[i].name);
    funcarray[i].signal.savestr[0] = '0' + i;
    funcarray[i].signal.savestr[1] = '\0';
  }
  init_fft();
  once=1;
}

/* update_math_signals() is called whenever 'something' has changed in
 * the scope settings, and we may need to recompute voltage and rate
 * values for the generated math functions.  We do this by calling all
 * the isvalid() functions for those math functions that have
 * listeners, and return 0 if everything's OK, or -1 if some of them
 * are no longer valid.
 *
 * XXX I'm still not completely clear on just what we should
 * do with invalid channels that have listeners; don't want to
 * arbitrarily clear them (I don't think), because then a single
 * inadvertent keystroke on channel 0 or 1 might clear a bunch of
 * math.
 */

int
update_math_signals(void)
{
  int i;
  int retval = 0;

  for (i = 0; i < funccount; i++) {
    if (funcarray[i].signal.listeners > 0) {
      if (! funcarray[i].isvalid(&funcarray[i].signal)) retval = -1;
    }
  }

  return retval;
}

/* Perform any math on the software channels, called many times by main loop */
void
do_math()
{
  static int i;

  for (i = 0; i < funccount; i++) {
    if (funcarray[i].signal.listeners > 0) {
      funcarray[i].func(&funcarray[i].signal);
    }
  }

  run_externals();

}

/* Perform any math cleanup, called once by cleanup at program exit */
void
cleanup_math()
{
  EndFFT();
}

/* measure the given channel */
void
measure_data(Channel *sig, struct signal_stats *stats) {
  static long int i, j, prev;
  int min=0, max=0, midpoint=0;
  float first = 0, last = 0, count = 0, imax = 0;

  stats->min = 0;
  stats->max = 0;
  stats->time = 0;
  stats->freq = 0;
  if ((sig->signal == NULL) || (sig->signal->num == 0)) return;

  prev = 1;
  if (scope.curs) {		/* manual cursor measurements */
    if (scope.cursa < scope.cursb) {
      first = scope.cursa;
      last = scope.cursb;
    } else {
      first = scope.cursb;
      last = scope.cursa;
    }
    stats->min = stats->max = sig->signal->data[(int)first];
    if ((j = sig->signal->data[(int)last]) < stats->min)
      stats->min = j;
    else if (j > stats->max)
      stats->max = j;
    count = 2;
  } else {			/* automatic period measurements */
    min = max = sig->signal->data[0];
    for (i = 0 ; i < sig->signal->num ; i++) {
      j = sig->signal->data[i];
      if (j < min)
	min = j;
      if (j > max) {
	max = j;
	imax = i;
      }
    }

    /* locate and count rising edges
     * doesn't handle noise very well (noisy edges can get double counted)
     */

    midpoint = (min + max)/2;
    for (i = 0 ; i < sig->signal->num ; i++) {
      j = sig->signal->data[i];
      if (j > midpoint && prev <= midpoint) {
	if (!first)
	  first = i;
	last = i;
	count++;
      }
      prev = j;
    }
    stats->min = min;
    stats->max = max;
  }

  if (sig->signal->rate < 0) {

    /* Special case for FFT - signal rate will be < 0, the negative of
     * the frequency step for each point in the transform, times 10.
     * So multiply by the index of the maximum value to get frequency peak.
     */

    stats->freq = (- sig->signal->rate) * imax / 10;
    if (stats->freq > 0)
      stats->time = 1000000 / stats->freq;
  
  } else if ((sig->signal->rate > 0) && (count > 1)) {

    /* estimate frequency from rising edge count
     * assume a wave: period = length / # periods
     */

    stats->time = 1000000 * (last - first) / (count - 1) / sig->signal->rate;
    if (stats->time > 0)
      stats->freq = 1000000 / stats->time;

  }
}

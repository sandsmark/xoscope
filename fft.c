/* -*- mode: C++; indent-tabs-mode: nil; fill-column: 100; c-basic-offset: 4; -*-
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 * Copyright (C) 2014 Gerhard Schiller <gerhard.schiller@gmail.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the interface to the FFTW-library for xoscope.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fftw3.h>
#include "xoscope.h"
#include "fft.h"
#include "display.h"
#include "func.h"
#include "xoscope_gtk.h"

/*#define TIME_FFT*/
#ifdef TIME_FFT
#include <time.h>
#endif

double          *dp = NULL;
fftw_complex    *cp = NULL;
fftw_plan       plan = NULL;

/* fftLenIn: Length of input to fftW(). 
 * Equal to ch[0].signal->width if <= 16 384 
 * or else rounded down to a power of 2.
 */
int             fftLenIn = -1;
int             xLayOut[DISPLAY_LEN + 1];     /* Array of bin #'s displayed */

static int once = 1;

/* Fast Fourier Transform of in to out */
void fftW(short *in, short *out, int inLen)
{
    int     k;
#ifdef TIME_FFT
    clock_t begin, end;
    double time_spent;
#endif

/*fprintf(stderr, "fftW(): inLen:%d\n", inLen);*/

    for (k = 0; k < inLen && k < fftLenIn; k++) {
        dp[k] = (double)in[k];
    }

#ifdef TIME_FFT
    begin = clock();
#endif
    fftw_execute(plan);
    displayFFT(cp, out);
#ifdef TIME_FFT
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    fprintf(stderr, "Time to execute: %.3f ms\n", time_spent * 1000.0);
#endif
}

void InitializeFFTW(int inLen)
{
#ifdef TIME_FFT
    clock_t begin, end;
    double time_spent;
#endif

    if ((dp = (double *)malloc(sizeof (double) * inLen)) == NULL) {
        fprintf(stderr, "malloc failed in InitializeFFTW()\n");
        exit(0);
    }
    memset(dp, 0, sizeof (double) * inLen);
fprintf(stderr, "InitializeFFTW() allocated memory for dp. Samples:%d\n", inLen);

    if ((cp = (fftw_complex *)fftw_malloc(sizeof (fftw_complex) * ((inLen / 2) +1 ))) == NULL) {
        fprintf(stderr, "fftw_malloc failed in InitializeFFTW()\n");
        exit(0);
    }
    memset(cp, 0, sizeof (fftw_complex) * ((inLen / 2) +1 ));
fprintf(stderr, "InitializeFFTW() allocated memory for cp. Samples:%d\n", (inLen / 2) +1 );

    /* FFTW_MEASURE with huge (some 100.000) samples takes several seconds even for 
     * sizes that are a power of 2.
     * On the other hand, execution time stays well below 5 ms with huge samples
     * if we only do a FFTW_ESTIMATE.
     */ 
#ifdef TIME_FFT
    begin = clock();
#endif
    if ((plan = fftw_plan_dft_r2c_1d(inLen, dp, cp, FFTW_ESTIMATE)) == NULL) {
        fprintf(stderr, "fftw_plan failed in InitializeFFTW()\n");
        exit(0);
    }
#ifdef TIME_FFT
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    fprintf(stderr, "Time to plan: %.3f s\n", time_spent);
#endif
}


/* special isvalid() functions for FFT
 *
 * First, it allocates memory for the generated fft.
 *
 * Second, it sets the "rate", so that the increment from grid line to grid line is some "nice"
 * value.  (a muliple of 500 Hz if increment is > 1kHz, otherwise a multiple of 100 hZ.
 *
 * Third: this value is stored in the "volts" member of the dest signal structure. It is only
 * displayed in the label.
 */

int FFTactive(Signal *source, Signal *dest)
{
    int         lenIn, HzDiv, HzDivAdj;

fprintf(stderr, "FFTactive() starts.\n");

    if (source == NULL) {
        dest->rate = 0;
        return 0;
    }

    if(source->width < 128){
        message("Too few samples to run FFT");
fprintf(stderr,"FFTactive(): Too few samples to run FFT\n");
        EndFFTW();
        if(dest->data != NULL){
            bzero(dest->data, (DISPLAY_LEN) * sizeof(short));
        }
        return 0;
    }
    
 
    if(dest->data == NULL){
        fprintf(stderr, "allocating memory for dest->data\n");
        if((dest->data = malloc((DISPLAY_LEN) * sizeof(short))) == NULL){
            fprintf(stderr, "malloc failed in FFTactive()\n");
            exit(0);
        }
        bzero(dest->data, (DISPLAY_LEN) * sizeof(short));
        dest->width = DISPLAY_LEN;
        dest->frame = 0;
        dest->num = DISPLAY_LEN;

        // (signal->rate / 2) = max FFT-freq
        HzDiv = source->rate / 2 / total_horizontal_divisions;
        if(HzDiv > 1000)
            HzDivAdj = HzDiv - (HzDiv % 500) + 500;
        else
            HzDivAdj = HzDiv - (HzDiv % 100) + 100;

        dest->volts = HzDivAdj;

        dest->rate  = (((double)source->rate / (double)source->width) * (double)DISPLAY_LEN)+0.5; 
        dest->rate *= (gfloat)HzDivAdj / (gfloat)HzDiv;
        dest->rate *= -1;
fprintf(stderr,"**FFTactive(): HzDiv=%d, HzDivAdj=%d\n", HzDiv, HzDivAdj);
fprintf(stderr,"**FFTactive(): source->rate=%d source->width=%d (source->rate / source->width)=%d\n", source->rate, source->width, source->rate / source->width);
fprintf(stderr,"**FFTactive(): dest->rate=%d, source->rate=%d\n", dest->rate, source->rate);
    }

    /* if we have more than 16 384 samples, we round them down to a power of 2 */
    if(source->width < (2 << 14)){ 
        lenIn = source->width;
    }
    else if(source->width < (2 << 16)){
        lenIn = floor2(source->width);
    }
    else {
        lenIn = 2 << 16;
    }
 
fprintf(stderr, "FFTactive(): source->width:%d, lenIn:%d,  fftLenIn:%d\n", source->width, lenIn, fftLenIn);
    if(lenIn != fftLenIn){
        /* Either first call  fftLenIn == -1
         * or time base changed, so the number of samples changed too 
        */
fprintf(stderr, "FFTactive() first run or change of time base.\n");
        if (fftLenIn != -1) {
            EndFFTW();
        }
        
        InitializeFFTW(lenIn);
        fftLenIn = lenIn;
        initGraphX();
     
        dest->width = DISPLAY_LEN;
        dest->frame = 0;
        dest->num = DISPLAY_LEN;

        // (signal->rate / 2) = max FFT-freq
        HzDiv = source->rate / 2 / total_horizontal_divisions;
        if(HzDiv > 1000)
            HzDivAdj = HzDiv - (HzDiv % 500) + 500;
        else
            HzDivAdj = HzDiv - (HzDiv % 100) + 100;

        dest->volts = HzDivAdj;

        dest->rate  = (((double)source->rate / (double)source->width) * (double)DISPLAY_LEN)+0.5; 
        dest->rate *= (gfloat)HzDivAdj / (gfloat)HzDiv;
        dest->rate *= -1;
fprintf(stderr,"**FFTactive(): HzDiv=%d, HzDivAdj=%d\n", HzDiv, HzDivAdj);
fprintf(stderr,"**FFTactive(): source->rate=%d source->width=%d (source->rate / source->width)=%d\n", source->rate, source->width, source->rate / source->width);
fprintf(stderr,"**FFTactive(): dest->rate=%d, source->rate=%d\n", dest->rate, source->rate);
once = 1;
    }
    return 1;
}

void EndFFTW(void)
{
fprintf(stderr, "EndFFTW()\n");

    if (plan != NULL) {
        fftw_destroy_plan(plan);
        plan = NULL;
    }
    
    if (dp != NULL) {
        free(dp);
        dp = NULL;
    }
    
    if (cp != NULL) {
        fftw_free(cp);
        cp = NULL;
    }
    
    fftLenIn  = -1;
}

int floor2(int num)
{
    int num2 = 1;
    
    while(num2 < num){
        num2 <<= 1;
    } 
    num2 >>= 1;
    return(num2);
}

short calcDv(int FFTindex)
{
    double  re, im, mag;
    short dv;
    	        
    re = cp[FFTindex][0];
    im = cp[FFTindex][1];
    mag = sqrt((re * re) + (im * im)) / 256.0;
 
    if (mag >= (1<<(sizeof(short) * 8 - 1))) {      /* avoid overflowing the short */
        mag = (1<<(sizeof(short) * 8 - 1)) - 1;     /* max short = 2^15 */
    }
        
    /* TODO -255 puts the FFT at the botom of the screen.
     * But only, if the scaling ist set to "1".
     */
    dv = (short)(mag + 0.5);
    
    return(dv);
}


void displayFFT(fftw_complex *cp, short *out)
{
    long    y = 0, y2 = 0;
    int     DSPindex, FFTindex;
    short   *pOut = out;
    
once = 0;       
if(once)
fprintf(stderr, "OUT:\n"); 	        
    for(DSPindex = 0, FFTindex = xLayOut[0]; 
                        DSPindex < DISPLAY_LEN && FFTindex < (fftLenIn / 2); DSPindex++){
    	FFTindex = xLayOut[DSPindex];
if(once)
fprintf(stderr, "FFTindex: %3d,", FFTindex); 	        
        /*
    	 *  If this line is the same as the previous one,
    	 *  (FFTindex == -1) just use the previous y value.
    	 *  Else go ahead and compute the value.
    	 */
        if(FFTindex != -1){
            y = calcDv(FFTindex);
            for(; FFTindex < xLayOut[DSPindex+1]; FFTindex++){
if(once)
fprintf(stderr, "FFTindex %3d,", FFTindex); 	        
                y2 = calcDv(FFTindex);
                if(y2 > y){
                    y = y2;
                }
        	 }
        }
        *pOut++ = y;
if(once)
/*fprintf(stderr, "%.2f;%.2f=%d:%d ", cp[FFTindex][0], cp[FFTindex][1], DSPindex, y); 	        */
fprintf(stderr, "DSPindex=%3d y=%3ld\n", DSPindex, y); 	        
    }
if(once)
fprintf(stderr, "\n"); 	        
once = 0;        
}

void initGraphX()
{
    int DSPindex;
    int val;

    /*
     * xLayOut: an array that hold indicies to indacte which resutlts of the fft 
     * to to combine into one point of the graph.
     * In case we have fewer results from the fft than DISPLAY_LEN, we repeat
     * point. This is indicated by a "-1".
     */ 
    for(DSPindex = 0; DSPindex < (DISPLAY_LEN + 1); DSPindex++){
        val = floor(((DSPindex * (double)fftLenIn / 2.0) / (double)DISPLAY_LEN ) + 0.5);
/*        fprintf(stderr, "%.2f/%d ", DSPindex-0.45, val);*/

        if(val < 0) 
            val=0;
            
        if(val >= fftLenIn / 2) 
            val = fftLenIn / 2 - 1;
	 
        if(DSPindex <= DISPLAY_LEN)
	        xLayOut[DSPindex] = val + 1;   /* the +1 takes care of the DC-Value in the fft result */
    }
    fprintf(stderr, "\n");
    /*
     *  If lines are repeated on the screen, flag this so that we don't
     *  have to recompute the y values.
     */
    for(DSPindex = DISPLAY_LEN - 1; DSPindex > 0; DSPindex--){
        if(xLayOut[DSPindex] == xLayOut[DSPindex-1]){
	        xLayOut[DSPindex] = -1;
	    }
    }

/*fprintf(stderr, "DISPLAY_LEN:%d, fftLenIn/2:%d\nLayOut: ", DISPLAY_LEN, fftLenIn / 2); 	        */
/*for(DSPindex = 0; DSPindex < DISPLAY_LEN + 1; DSPindex++){*/
/*   fprintf(stderr, "%4.4d ", xLayOut[DSPindex]);*/
/*}*/
/*fprintf(stderr, "\n");*/
}






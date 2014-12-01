/* -*- mode: C++; indent-tabs-mode: nil; fill-column: 100; c-basic-offset: 4; -*-
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
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

double          *dp = NULL;
fftw_complex    *cp = NULL;
fftw_plan       plan;


/* Fast Fourier Transform of in to out */
void fftW(short *in, short *out, int inLen)
{
    double  r, i, mag;
    int     k, max;
    
    for(k = 0; k < inLen && k < FFTLEN; k++) {
            dp[k] = (double)in[k];
    }

        fftw_execute(plan);

        max = (FFTLEN / 2) + 1;
        for (k = 0; k < max; k++) {
                r = cp[k][0];
                i = cp[k][1];
                mag = sqrt((r * r) + (i * i));
/*              printf("%d %lf %lf %lf\n", k, r, i, mag);*/

        if (mag >= (1<<(sizeof(short) * 8 - 1))) {              /* avoid overflowing the short */
            mag = (1<<(sizeof(short) * 8 - 1)) - 1;     /* max short = 2^15 */
        } 
                out[k] = (short)mag;
        }
}

void InitializeFFTW(int fftlen)
{
        if(dp == NULL){
            if((dp = (double *)malloc(sizeof (double) * fftlen)) == NULL){
            fprintf(stderr, "malloc failed in InitializeFFTW()\n");
                exit(0);
            }
        } 
        memset(dp, 0, sizeof (double) * fftlen);
            
        if(cp == NULL){
        if((cp = (fftw_complex *)fftw_malloc(sizeof (fftw_complex) * fftlen)) == NULL){;
            fprintf(stderr, "malloc failed in InitializeFFTW()\n");
                exit(0);
            }
        }
        memset(cp, 0, sizeof (fftw_complex) * fftlen);

        plan = fftw_plan_dft_r2c_1d(fftlen, dp, cp, FFTW_MEASURE);
}

void EndFFTW(void)
{
        fftw_destroy_plan(plan);

    free(dp);
    dp = NULL;
    
    free(cp);
    cp = NULL;
}


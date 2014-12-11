/* -*- mode: C++; fill-column: 100; c-basic-offset: 4; -*-
 *
 *  Prototypes for the routines in fft.c
 *
 */
 
#include <fftw3.h>

#define FFT_DSP_LEN 440

extern int fftLenIn;   
extern int fftLenOut;
 
void InitializeFFTW(int fftlen);
void fftW(short *in, short *out, int inLen);
void EndFFTW(void);
int  floor2(int num);
int  FFTactive(Signal *source, Signal *dest);
void displayFFT(fftw_complex *cp, short *out);
void initGraphX(void);


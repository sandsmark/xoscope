/* -*- mode: C++; fill-column: 100; c-basic-offset: 4; -*-
 *
 *  Prototypes for the routines in fft.c
 *
 */
void InitializeFFTW(int fftlen);
void fftW(short *in, short *out, int inLen);
void EndFFTW(void);


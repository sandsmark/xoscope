/* -*- mode: C++; fill-column: 100; c-basic-offset: 4; -*-
 *
 *  Prototypes for the routines in realfft.c and realffta.asm
 *
 */

void InitializeFFT(int);
void EndFFT(void);
void RealFFT(short *);

extern int *BitReversed;


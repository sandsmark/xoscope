/*
 * @(#)$Id: proscope.h,v 1.1 1997/05/24 23:22:30 twitham Exp $
 *
 * Copyright (C) 1997 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file defines ProbeScope serial bits and function prototypes.
 *
 */

/* convenience/readability macros */
#define BIT_MASK(bitnum)	(1 << (bitnum))
#define BITS_CLR(byte, mask)	((~byte & mask) == mask)
#define BITS_SET(byte, mask)	((byte & mask) == mask)

/* Radio Shack ProbeScope serial data protocol bit definitions */

#define PS_WAIT		BIT_MASK(0) /* WAITING! for trigger? */

#define PS_10V		BIT_MASK(2) /* voltage switch */
#define PS_100V		BIT_MASK(3)
#define PS_1V		(PS_10V | PS_100V)
#define PS_DC		BIT_MASK(4) /* coupling switch */
#define PS_AC		BIT_MASK(5)
#define PS_GND		(PS_DC | PS_AC)

#define PS_SINGLE	BIT_MASK(0) /* trigger mode */
#define PS_MEXT		BIT_MASK(3) /* trigger type Plus/Minus Ext/Int */
#define PS_PEXT		BIT_MASK(4)
#define PS_MINT		BIT_MASK(5) /* (documentation was off-by-one here) */
#define PS_PINT		BIT_MASK(6)
#define PS_AUTO		0176	/* 1 111 110 = 0x7E */

#define PS_TP5		BIT_MASK(2) /* Trigger Plus/Minus 0.n */
#define PS_TP3		BIT_MASK(3)
#define PS_TP1		BIT_MASK(4)
#define PS_TM1		BIT_MASK(5)
#define PS_TM3		BIT_MASK(6)
#define PS_TM5		0

#define PS_UNDERFLOW	BIT_MASK(0)
#define PS_OVERFLOW	BIT_MASK(1)
#define PS_MINUS	BIT_MASK(3)

extern void probescope();
extern void microsleep();
extern void init_serial();
extern void cleanup_serial();
extern int getonebyte();

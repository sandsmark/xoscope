/*
 * @(#)$Id: func.h,v 1.1 1996/02/02 07:10:51 twitham Exp $
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see oscope.c and the file COPYING for more details)
 *
 * oscope math function prototypes
 *
 */

extern int funccount;

extern char *funcnames[];

extern void (*funcarray[])(int);

/*
 * @(#)$Id: x11.h,v 1.2 1996/03/10 01:49:11 twitham Rel1_1 $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * X11 specific display macros and routines
 *
 */

#include <libsx.h>

#define VGA_DRAWPIXEL	DrawPixel
#define VGA_DRAWLINE	DrawLine
#define VGA_SETCOLOR	SetColor
#define ALIGN_RIGHT	1
#define ALIGN_LEFT	2
#define ALIGN_CENTER	3

extern XFont font;

int
VGA_WRITE(char *s, short x, short y, XFont f, short fg, short bg, char p);

void
fix_widgets();

void
init_widgets();

void
cleanup_x11();

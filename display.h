/*
 * @(#)$Id: display.h,v 1.2 1996/01/28 08:11:09 twitham Exp $
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see scope.c and the file COPYING for more details)
 *
 * display routines available outside of display.c
 *
 */

/* libsx vs. svgalib drawing macros */
#ifdef XSCOPE

#include <libsx.h>
#define VGA_DRAWPIXEL	DrawPixel
#define VGA_DRAWLINE	DrawLine
#define VGA_SETCOLOR	SetColor
#define VGA_WRITE(s,x,y,f,fg,bg,p); SetColor(fg);DrawText(s,x,y+FontHeight(f))

#else

#include <vga.h>
#define VGA_DRAWPIXEL	vga_drawpixel
#define VGA_DRAWLINE	vga_drawline
#define VGA_SETCOLOR	vga_setcolor
#define VGA_WRITE(s,x,y,f,fg,bg,p)	;
#ifdef HAVEVGAMISC
#include <fontutils.h>
#undef VGA_WRITE
#define VGA_WRITE(s,x,y,f,fg,bg,p)	vga_write(s,x,y,&f,fg,bg,p)
#endif

#endif

/* text foreground color */
#define TEXT_FG		color[2]

/* text background color */
#define TEXT_BG		color[0]

extern char fontname[];
extern int color[];

/* functions that are called by files other than display.c */
void
draw_text();

void
show_info(unsigned char);

void
init_screen();

void
cleanup();

void
mainloop();

int
opendisplay();

void
clear();

char *
fonts();

/*
 * @(#)$Id: display.h,v 1.1 1996/01/23 07:59:31 twitham Exp $
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

/* 16-color color definitions */
#ifndef XSCOPE
#define BLACK		0	/* dark colors */
#define BLUE		1
#define GREEN		2
#define CYAN		3
#define RED		4
#define MAGENTA		5
#define BROWN		6
#define LIGHTGRAY	7
#define DARKGRAY	8	/* light colors */
#define LIGHTBLUE	9
#define LIGHTGREEN	10
#define LIGHTCYAN	11
#define LIGHTRED	12
#define LIGHTMAGENTA	13
#define YELLOW		14
#define WHITE		15
#endif

/* text foreground color */
#define TEXT_FG		GREEN

/* text background color */
#define TEXT_BG		BLACK

/* how to convert text column (0-79) and row(-6-1,0-5) to graphics position */
#define COL(x)	(x * 8)
#define ROW(y)	(offset + y * 18 + 258 * (y >= 0))

extern char fontname[];

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

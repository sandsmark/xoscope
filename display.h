/*
 * @(#)$Id: display.h,v 1.4 1996/01/31 07:40:37 twitham Exp $
 *
 * Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see oscope.c and the file COPYING for more details)
 *
 * display variables and routines available outside of display.c
 *
 */

extern char fontname[];
extern int color[];

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

/*
 * @(#)$Id: display.h,v 1.5 1996/02/22 06:25:53 twitham Exp $
 *
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

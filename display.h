/*
 * @(#)$Id: display.h,v 1.6 1996/03/02 06:59:47 twitham Exp $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * display variables and routines available outside of display.c
 *
 */

extern char fontname[];

void
draw_text();

void
init_screen();

void
cleanup();

void
mainloop();

void
opendisplay();

void
clear();

char *
fonts();

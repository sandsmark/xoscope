/*
 * @(#)$Id: display.h,v 1.7 1996/03/10 01:41:50 twitham Rel1_1 $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
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

void
opendisplay();

void
clear();

char *
fonts();

void
message(char *message, int clr);

char *
GetFile(char *path);

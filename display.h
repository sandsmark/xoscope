/*
 * @(#)$Id: display.h,v 1.8 1996/10/06 02:34:56 twitham Rel1_2 $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * display variables and routines available outside of display.c
 *
 */

#define ALIGN_RIGHT	1
#define ALIGN_LEFT	2
#define ALIGN_CENTER	3
#define TRUE	1
#define FALSE	0

extern char fontname[];
extern char fonts[];
extern int color[];

int	OpenDisplay();
void	init_screen();
void	mainloop();
void	draw_text();
void	clear();
void	message();
void	cleanup_display();
void	animate();
int	col();
char *	GetString();
char *	GetFile();
int	GetYesNo();

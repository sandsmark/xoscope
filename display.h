/*
 * @(#)$Id: display.h,v 1.11 2003/06/17 22:52:32 baccala Exp $
 *
 * Copyright (C) 1996 - 1999 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * display variables and routines available outside of display.c
 *
 */

#define ALIGN_RIGHT	1
#define ALIGN_LEFT	2
#define ALIGN_CENTER	3
#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

extern char fontname[];
extern char fonts[];
extern int color[];

void	init_screen();		/* exported from display.c */
void	init_widgets();
void	mainloop();
void	draw_text();
void	clear();
void	message();
void	cleanup_display();
int	animate();
int	col();

void	DrawPixel();		/* these are defined in */
void	DrawLine();		/* a display-specific file */
void	SetColor();		/* like gr_vga.c or gr_sx.c */
void	PolyPoint();
void	PolyLine();
void	SyncDisplay();
void	AddTimeOut();
void	LoadSaveFile();
void	ExternCommand();
char *	GetFile();
char *	GetString();
int	GetYesNo();
int	OpenDisplay();
void	ClearDrawArea();
void	MainLoop();

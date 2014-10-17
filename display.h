/*
 * @(#)$Id: display.h,v 2.6 2009/07/30 02:18:35 baccala Exp $
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

void	init_widgets()		/* exported from display.c */;
void	mainloop();
void	setup_help_text();
void	update_text();
void	show_data();
void	roundoff_multipliers(Channel *);
void	timebase_changed(void);
void	clear();
void	message(const char *);
void	cleanup_display();
void	animate();
int	col();

void	AddTimeOut();
void	LoadSaveFile(int);
void	ExternCommand();
char *	GetFile();
char *	GetString();
int	GetYesNo();
int	OpenDisplay(int, char **);

void	setup_help_text(GtkWidget *, gpointer);

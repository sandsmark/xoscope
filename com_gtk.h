/*
 * @(#)$Id: com_gtk.h,v 1.1 1999/08/27 04:24:47 twitham Rel $
 *
 * Copyright (C) 1996 - 1999 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * Some GTK widgets and event handlers common to xoscope and xy.
 *
 */

GdkColor gdkcolor[];
GdkGC *gc;
GdkPixmap *pixmap;
GdkRectangle update_rect;
GtkWidget *drawing_area;
GtkWidget *menubar;
GtkWidget *vbox;
GtkWidget *window;
char *colors[];
int color[];

gint expose_event();
gint key_press_event();
void delete_event();
void hit_key();

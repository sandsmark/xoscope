/*
 * @(#)$Id: com_gtk.h,v 1.2 2000/03/05 23:00:31 twitham Rel $
 *
 * Copyright (C) 1996 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * Some GTK widgets and event handlers common to xoscope and xy.
 *
 */

GdkColor gdkcolor[16];
GdkGC *gc;
GdkPixmap *pixmap;
GdkRectangle update_rect;
GtkWidget *drawing_area;
GtkWidget *menubar;
GtkWidget *vbox;
GtkWidget *window;
char *colors[16];
int color[16];

gint expose_event();
gint key_press_event();
void delete_event();
void hit_key();

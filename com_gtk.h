/*
 * @(#)$Id: com_gtk.h,v 2.1 2008/12/21 19:18:39 baccala Exp $
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
GdkRectangle update_rect;
GdkPixmap *pixmap;
GtkWidget *drawing_area;
GtkWidget *menubar;
GtkWidget *vbox;
GtkWidget *window;
GtkWidget *glade_window;
char *colors[16];
int color[16];

gint expose_event();
gint key_press_event();
void delete_event();
void hit_key();

#define LU(label)       lookup_widget(glade_window, label)

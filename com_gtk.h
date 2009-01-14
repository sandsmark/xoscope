/*
 * @(#)$Id: com_gtk.h,v 2.2 2009/01/14 18:21:06 baccala Exp $
 *
 * Copyright (C) 1996 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * Some GTK widgets and event handlers common to xoscope and xy.
 *
 */

GtkWidget *menubar;
GtkWidget *glade_window;

gint expose_event();
gint key_press_event();
void delete_event();
void hit_key();

#define LU(label)       lookup_widget(glade_window, label)

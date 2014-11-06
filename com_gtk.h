/*
 * @(#)$Id: com_gtk.h,v 2.3 2009/01/17 06:44:55 baccala Exp $
 *
 * Copyright (C) 1996 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * Some GTK widgets and event handlers common to xoscope and xy.
 *
 */

extern GtkWidget *menubar;
extern GtkWidget *glade_window;
extern GtkWidget *bitscope_options_dialog;
extern GtkWidget *comedi_options_dialog;
extern GtkWidget *alsa_options_dialog;
extern GtkWidget *databox;

gint expose_event();
gint key_press_event();
void delete_event();
void hit_key();

GtkWidget* lookup_widget(GtkWidget *widget, const gchar *widget_name);
#define LU(label)       lookup_widget(glade_window, label)

GtkWidget *create_comedi_dialog ();

void on_main_window_destroy (GtkObject *object, gpointer user_data);

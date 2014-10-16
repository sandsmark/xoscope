/*
 * @(#)$Id: com_gtk.c,v 2.4 2009/01/07 02:19:03 baccala Exp $
 *
 * Copyright (C) 1996 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file holds GTK code that is common to xoscope and xy.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "oscope.h"
#include "display.h"
#include "com_gtk.h"

#include "xoscope.rc.h"

GtkWidget *menubar = NULL;
GtkWidget *glade_window = NULL;
GtkWidget *bitscope_options_dialog = NULL;
GtkWidget *comedi_options_dialog = NULL;
GtkWidget *alsa_options_dialog = NULL;
GtkWidget *databox = NULL;

GdkPixmap *pixmap = NULL;
int fixing_widgets = 0;

/* emulate several libsx function in GTK */
int
OpenDisplay(int argc, char *argv[])
{
  gtk_init(&argc, &argv);
  return(argc);
}

/* set up some common event handlers */

void
delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gtk_main_quit ();
}

gint
key_press_event(GtkWidget *widget, GdkEventKey *event)
{
  if (event->keyval == GDK_Tab) {
    handle_key('\t');
  } else if (event->length == 1) {
    handle_key(event->string[0]);
  }

  return TRUE;
}

/* simple event callback that emulates the user hitting the given key */
void
hit_key(GtkWidget *w, guint data)
{
  if (fixing_widgets) return;
  handle_key(data);
}

int 
is_GtkWidget(GType Type) {
	GType GtkWidgetType = g_type_from_name("GtkWidget");

	while (Type) {
		if (Type == GtkWidgetType) 
				return 1;
			Type = g_type_parent(Type);
	}
	return 0;
}

/* gtk_builder sets the "name" property initially to the class-name, check with: gtk_widget_get_name()*/
/* the name defined by glade can be obtained with : gtk_buildable_get_name() */
/* we use gtk_buildable_set_buildable_property() to set the "name" property to what we defined in glade */
/* Without seting the "name" property, we can't set the styles from the rc-file ! */
void
set_name_property(GtkWidget *widget, GtkBuilder *builder)
{
	GValue	g_value = G_VALUE_INIT;

	g_value_init (&g_value, G_TYPE_STRING);
	g_value_set_string (&g_value, gtk_buildable_get_name(GTK_BUILDABLE (widget)));
	gtk_buildable_set_buildable_property(GTK_BUILDABLE (widget), builder, "name", &g_value);
}
 
#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full ( \
  	G_OBJECT (component), \
  	name, \
  	g_object_ref ((GtkWidget*)widget), \
  	(GDestroyNotify) g_object_unref \
  )

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

void
store_reference(GtkWidget* widget)
{
	GtkWidget *parent, *child;
	
	child = widget;
	while(TRUE){
		parent = (GtkWidget*)gtk_widget_get_parent(child);
		if(parent == NULL){
			parent = child;
			break;
		}
		child = parent;
	}

	if(parent == widget){
/*		fprintf(stderr, "TOP %s\n", gtk_widget_get_name(widget));*/
		GLADE_HOOKUP_OBJECT_NO_REF(widget, widget, gtk_widget_get_name(widget));
	}
	else{
/*		fprintf(stderr, "SUB %s %s\n", gtk_widget_get_name(parent), gtk_widget_get_name(widget));*/
		GLADE_HOOKUP_OBJECT(parent, widget, gtk_widget_get_name(widget));
	}
}

GtkWidget*
lookup_widget(GtkWidget *widget, const gchar  *widget_name)
{
  GtkWidget *parent, *found_widget;

  for (;;){
      if (GTK_IS_MENU (widget))
        parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
      else
        parent = widget->parent;
      if (!parent)
        parent = (GtkWidget*) g_object_get_data (G_OBJECT (widget), "GladeParentKey");
      if (parent == NULL)
        break;
      widget = parent;
    }

  found_widget = (GtkWidget*) g_object_get_data (G_OBJECT (widget),
                                                 widget_name);
  if (!found_widget)
    g_warning ("Widget not found: %s", widget_name);
  return found_widget;
}

GtkWidget *
create_main_window()
{
	GtkBuilder      *builder; 
	GError 			*err = NULL;
	GSList			*gslWidgets, *iterator = NULL;
	char 			**xoscope_rc_ptr = xoscope_rc;
	/* extern char		gladestring[]; */
	
	for (xoscope_rc_ptr=xoscope_rc; *xoscope_rc_ptr != NULL; xoscope_rc_ptr++) {
		gtk_rc_parse_string(*xoscope_rc_ptr);
	}
	
	builder = gtk_builder_new ();

/*	if(0 == gtk_builder_add_from_file (builder, "xoscope_new.glade", &err)){*/
/*		fprintf(stderr, "Error adding build from file. Error: %s\n", err->message);*/
/*		exit(1);*/
/*	}*/
	if(0 == gtk_builder_add_from_string(builder, gladestring, -1, &err)){
		fprintf(stderr, "Error adding build from string. Error: %s\n", err->message);
		exit(1);
	}

   	glade_window 			= GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
	comedi_options_dialog 	= GTK_WIDGET (gtk_builder_get_object (builder, "comedi_dialog"));
	bitscope_options_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "dialog2"));
	alsa_options_dialog 	= GTK_WIDGET (gtk_builder_get_object (builder, "alsa_options_dialog"));
	databox 				= GTK_WIDGET (gtk_builder_get_object (builder, "databox"));
 
	gslWidgets = gtk_builder_get_objects(builder);
 	for (iterator = gslWidgets; iterator; iterator = iterator->next) {
 		if(is_GtkWidget(G_TYPE_FROM_INSTANCE(iterator->data))){
			set_name_property((GtkWidget*)(iterator->data), builder);
			store_reference((GtkWidget*)(iterator->data));
 		}
 		else{
			continue;
 		}
 	}
	g_slist_free(gslWidgets);

    gtk_builder_connect_signals(builder, NULL);
    g_object_unref (G_OBJECT (builder));

    return(glade_window);
}

void
on_main_window_check_resize()
{
	fprintf(stderr, "on_main_window_frame_event()\n");
}

GtkWidget *
create_dialog2 ()
{
	return(bitscope_options_dialog);
}

GtkWidget *
create_comedi_dialog ()
{
	return(comedi_options_dialog);
}

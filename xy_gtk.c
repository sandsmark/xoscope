/*
 * @(#)$Id: xy_gtk.c,v 1.1 1999/08/25 03:04:34 twitham Rel $
 *
 * Copyright (C) 1996 - 1999 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the GTK specific UI for the xy command
 *
 */

#include <gtk/gtk.h>
#include "display.h"
#include "com_gtk.h"

extern int mode, quit_key_pressed;
extern void handle_key();

/* clear the drawing area and reset the menu check marks */
void
clear()
{
  ClearDrawArea();
}

/* quit button callback */
void
dismiss(GtkWidget *w, void *data)
{
  quit_key_pressed = 1;
}

/* plot mode menu callback */
void
plotmode(GtkWidget *w, void *data)
{
  char *c = (char *)data;

  mode = *c - '0';
  clear();
}

/* Create a new backing pixmap of the appropriate size */
static gint
configure_event (GtkWidget *widget, GdkEventConfigure *event)
{
  if (pixmap)
    gdk_pixmap_unref(pixmap);

  pixmap = gdk_pixmap_new(widget->window,
			  widget->allocation.width,
			  widget->allocation.height,
			  -1);
  ClearDrawArea();
  return TRUE;
}

void
get_main_menu(GtkWidget *window, GtkWidget ** menubar)
{
  static GtkMenuEntry menu_items[] =
  {
    {"<Main>/Quit", NULL, hit_key, "\e"},
    {"<Main>/Plot Mode/Point", NULL, plotmode, "0"},
    {"<Main>/Plot Mode/Point Accumulate", NULL, plotmode, "1"},
    {"<Main>/Plot Mode/Line", NULL, plotmode, "2"},
    {"<Main>/Plot Mode/Line Accumulate", NULL, plotmode, "3"},
  };
  int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

  GtkMenuFactory *factory;
  GtkMenuFactory *subfactory;

  factory = gtk_menu_factory_new(GTK_MENU_FACTORY_MENU_BAR);
  subfactory = gtk_menu_factory_new(GTK_MENU_FACTORY_MENU_BAR);

  gtk_menu_factory_add_subfactory(factory, subfactory, "<Main>");
  gtk_menu_factory_add_entries(factory, menu_items, nmenu_items);

  if (menubar)
    *menubar = subfactory->widget;
}

/* build the menubar and drawing area */
void
init_widgets()
{

  int i;
  GtkWidget *menubar;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT (window), "delete_event",
		     GTK_SIGNAL_FUNC (delete_event), NULL);
  gtk_signal_connect(GTK_OBJECT(window),"key_press_event",
		     (GtkSignalFunc) key_press_event, NULL);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  get_main_menu(window, &menubar);
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);
  gtk_widget_show(menubar);

  drawing_area = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), 256, 256);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event",
		     GTK_SIGNAL_FUNC(expose_event), NULL);
  gtk_signal_connect(GTK_OBJECT(drawing_area),"configure_event",
		     GTK_SIGNAL_FUNC(configure_event), NULL);

  gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);
  gtk_widget_show(drawing_area);
  gtk_widget_show(vbox);
  gtk_widget_show(window);

  gc = gdk_gc_new(drawing_area->window);
  for (i=0 ; i < 16 ; i++) {
    color[i] = i;
    gdk_color_parse(colors[i], &gdkcolor[i]);
    gdkcolor[i].pixel = (gulong)(gdkcolor[i].red * 65536 +
				 gdkcolor[i].green * 256 + gdkcolor[i].blue);
    gdk_color_alloc(gtk_widget_get_colormap(window), &gdkcolor[i]);
/*      printf("%d %s %ld:\tr:%d g:%d b:%d\n", */
/*  	   i, colors[i], gdkcolor[i].pixel, */
/*  	   gdkcolor[i].red, gdkcolor[i].green, gdkcolor[i].blue); */
  }
  gdk_gc_set_background(gc, &gdkcolor[0]);
  SetColor(15);
  ClearDrawArea();
}

void
MainLoop()
{
  animate(NULL);
  gtk_main();
}

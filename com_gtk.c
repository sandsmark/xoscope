/*
 * @(#)$Id: com_gtk.c,v 1.3 2000/03/03 23:30:40 twitham Rel $
 *
 * Copyright (C) 1996 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file holds GTK code that is common to xoscope and xy.
 *
 */

#include <gtk/gtk.h>
#include "oscope.h"

GdkRectangle update_rect;
GtkWidget *window;
GtkWidget *drawing_area;
GdkPixmap *pixmap = NULL;
GdkGC *gc;
GtkWidget *menubar;
GtkWidget *vbox;
GdkColor gdkcolor[16];
int color[16], fixing_widgets = 0;
char *colors[] = {		/* X colors similar to 16 console colors */
  "black",			/* 0 */
  "blue",
  "green",			/* 2 */
  "cyan",
  "red",			/* 4 */
  "magenta",
  "orange",			/* 6 */
  "gray66",
  "gray33",			/* 8 */
  "blue4",
  "green4",			/* 10 */
  "cyan4",
  "red4",			/* 12 */
  "magenta4",
  "yellow",			/* 14 */
  "white"
};

/* emulate several libsx function in GTK */
int
OpenDisplay(int argc, char *argv[])
{
  gtk_init(&argc, &argv);
  return(argc);
}

void
ClearDrawArea()
{
  gdk_draw_rectangle(pixmap,
		     drawing_area->style->black_gc,
		     TRUE,
		     0, 0,
		     drawing_area->allocation.width,
		     drawing_area->allocation.height);
}

void
SetColor(int c)
{
  gdk_gc_set_foreground(gc, &gdkcolor[c]);
}

void
DrawPixel(int x, int y)
{
  gdk_draw_point(pixmap, gc, x, y);
}

void
DrawLine(int x1, int y1, int x2, int y2)
{
  gdk_draw_line(pixmap, gc, x1, y1, x2, y2);
}

void
SyncDisplay()
{
  update_rect.x  = update_rect.y = 0;
  update_rect.width = drawing_area->allocation.width;
  update_rect.height = drawing_area->allocation.height;
  gtk_widget_draw(drawing_area, &update_rect);
}

void
AddTimeOut(unsigned long interval, int (*func)(), void *data) {
  static int done = 0;

  if (done) return;
  gtk_idle_add((GtkFunction)func, (gpointer)data);
  done = 1;
}

/* set up some common event handlers */
gint
expose_event(GtkWidget *widget, GdkEventExpose *event)
{
  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
		  pixmap,
		  event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}

void
delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gtk_main_quit ();
}

gint
key_press_event(GtkWidget *widget, GdkEventKey *event)
{
  handle_key(event->string[0]);
  return TRUE;
}

/* simple evnet callback that emulates the user hitting the given key */
void
hit_key(GtkWidget *w, gpointer data)
{
  if (fixing_widgets) return;
  handle_key(((char *)data)[0]);
}
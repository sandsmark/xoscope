/* GtkDatabox - An extension to the gtk+ library
 * Copyright (C) 1998 - 2005  Dr. Roland Bock
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <stdio.h>

#include <gtk/gtk.h>
#include <gtkdatabox.h>
#include <gtkdatabox_points.h>
#include <math.h>

#define POINTS 2000

/*----------------------------------------------------------------
 *  databox signals
 *----------------------------------------------------------------*/

const gchar *
get_name_of_current_signal (gpointer instance)
{
   GSignalInvocationHint * ihint;

   ihint = g_signal_get_invocation_hint (instance);

   return g_signal_name (ihint->signal_id);
}

/* 
 * Signal handlers 
 */
static gint
handle_signal_zoomed (GtkDatabox *box,
		      GtkDataboxValue *top_left,
		      GtkDataboxValue *bottom_right, void *unused)
{
   printf ("Name of the signal: %s\n", get_name_of_current_signal (box));
   printf ("It tells you that the GtkDatabox has zoomed to the following\n");
   printf
      ("rectangle (data coordindates only, pixels don't make sense here):\n");
   printf ("top_left (X,Y)=(%g, %g), bottom_right (X,Y)=(%g, %g)\n",
	   top_left->x, top_left->y, bottom_right->x, bottom_right->y);

   return 0;
}

static gint
handle_signal_selection_stopped (GtkDatabox *box,
				 GtkDataboxCoord *marked,
				 GtkDataboxCoord *select, void *unused)
{
   GtkDataboxValue value1;
   GtkDataboxValue value2;

   value1 = gtk_databox_value_from_coord (box, *marked);
   value2 = gtk_databox_value_from_coord (box, *select);

   printf ("Name of the signal: %s\n", get_name_of_current_signal (box));
   printf ("It tells you that the user has stopped changing the selection\n");
   printf ("box, i.e. the mouse button is released now.\n");
   printf ("Pixel: corner1 (X,Y)=(%d, %d), corner2 (X,Y)=(%d, %d)\n",
	   marked->x, marked->y, select->x, select->y);
   printf ("Data: corner1 (X,Y)=(%g, %g), corner2 (X,Y)=(%g, %g)\n",
	   value1.x, value1.y, value2.x, value2.y);

   return 0;
}

static gint
handle_signal_selection_started (GtkDatabox *box, void *unused)
{
   printf ("Name of the signal: gtk_databox_selection_started\n");
   printf ("It tells you that the user has started a the selection box\n");

   return 0;
}

static gint
handle_signal_selection_cancelled (GtkDatabox *box, void *unused)
{
   printf ("Name of the signal: %s\n", get_name_of_current_signal (box));
   printf ("It tells you that the user has dismissed the selection box\n");

   return 0;
}

enum {
  SHOW_BOX,
  SHOW_ACTUAL_X,
  SHOW_ACTUAL_Y,
  SHOW_MARKED_X,
  SHOW_MARKED_Y,
  SHOW_DELTA_X,
  SHOW_DELTA_Y,
  SHOW_NUM_ENTRIES
};


static GtkWidget *
show_entry (GtkWidget *hbox, gchar *text)
{
  GtkWidget *frame;
  GtkWidget *entry;

  frame = gtk_frame_new (text);
  gtk_container_add (GTK_CONTAINER(hbox), frame);
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 20, -1);
  gtk_editable_set_editable (GTK_EDITABLE(entry), FALSE);
  gtk_container_add (GTK_CONTAINER(frame), entry);

  return entry;
}

static gint
show_motion_notify_cb(GtkWidget *widget, GdkEventMotion *event, 
                      GtkWidget **entries)
{
  GtkDataboxCoord coord;
  GtkDataboxValue value;
  gchar *text;
  GtkDatabox *box = GTK_DATABOX (entries [SHOW_BOX] );

  coord.x = event->x;
  coord.y = event->y;

  value = gtk_databox_value_from_coord (box, coord);

  text=g_strdup_printf("%g", value.x);
  gtk_entry_set_text(GTK_ENTRY(entries[SHOW_ACTUAL_X]), text);
  g_free((gpointer) text);
  text=g_strdup_printf("%g", value.y);
  gtk_entry_set_text(GTK_ENTRY(entries[SHOW_ACTUAL_Y]), text);
  g_free((gpointer) text);

  return FALSE;
}

static gint
show_button_press_cb (GtkDatabox *box, GdkEventButton *event, 
                      GtkWidget **entries)
{
   GtkDataboxCoord coord;
   GtkDataboxValue value;
   gchar *text;
 
   if (!(event->button == 1 || event->button == 2))
      return FALSE;

   coord.x = event->x;
   coord.y = event->y;
     
   value = gtk_databox_value_from_coord (box, coord);
 
   text=g_strdup_printf("%g", value.x);
   gtk_entry_set_text(GTK_ENTRY(entries[SHOW_MARKED_X]), text);
   g_free((gpointer) text);
   text=g_strdup_printf("%g", value.y);
   gtk_entry_set_text(GTK_ENTRY(entries[SHOW_MARKED_Y]), text);
   g_free((gpointer) text);

   return FALSE;
}

static void
show_changed_cb(GtkDatabox *box, 
                GtkDataboxCoord *marked, 
                GtkDataboxCoord *select, GtkWidget **entries)
{
  GtkDataboxValue value1;
  GtkDataboxValue value2;
  gchar *text;

  value1 = gtk_databox_value_from_coord (box, *marked);
  value2 = gtk_databox_value_from_coord (box, *select);

  text=g_strdup_printf("%g", value2.x-value1.x);
  gtk_entry_set_text(GTK_ENTRY(entries[SHOW_DELTA_X]), text);
  g_free((gpointer) text);
  text=g_strdup_printf("%g", value2.y-value1.y);
  gtk_entry_set_text(GTK_ENTRY(entries[SHOW_DELTA_Y]), text);
  g_free((gpointer) text);

  text=g_strdup_printf("%g", value2.x);
  gtk_entry_set_text(GTK_ENTRY(entries[SHOW_ACTUAL_X]), text);
  g_free((gpointer) text);
  text=g_strdup_printf("%g", value2.y);
  gtk_entry_set_text(GTK_ENTRY(entries[SHOW_ACTUAL_Y]), text);
  g_free((gpointer) text);
}



static void
create_signals (void)
{
   GtkWidget *window = NULL;
   GtkWidget *box1;
   GtkWidget *box2;
   GtkWidget *close_button;
   GtkWidget *box;
   GtkWidget *table;
   GtkWidget *label;
   GtkWidget *separator;
   gfloat *X;
   gfloat *Y;
   GtkDataboxGraph *graph;
   GdkColor color;
   gint i;
   GtkWidget **entries;
   GtkWidget *hbox;

   entries=g_new0(GtkWidget *, SHOW_NUM_ENTRIES);

   window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   gtk_widget_set_size_request (window, 500, 500);

   g_signal_connect (G_OBJECT (window), "destroy",
		     G_CALLBACK (gtk_main_quit), NULL);

   gtk_window_set_title (GTK_WINDOW (window), "GtkDatabox: Signals Examples");
   gtk_container_set_border_width (GTK_CONTAINER (window), 0);

   box1 = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (window), box1);

   label =
      gtk_label_new
      ("The output on the shell and in the text boxes below\nshow you the information that you can get\n by using signals.\n\nSee basics for a usage of this window...");

   gtk_box_pack_start (GTK_BOX (box1), label, FALSE, FALSE, 0);
   separator = gtk_hseparator_new ();
   gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, FALSE, 0);

   hbox=gtk_hbox_new(TRUE,3);
   gtk_box_pack_start(GTK_BOX(box1), hbox, FALSE, TRUE, 0);

   entries[SHOW_ACTUAL_X] = show_entry (hbox, "Actual X");
   entries[SHOW_ACTUAL_Y] = show_entry (hbox, "Actual Y");
   entries[SHOW_MARKED_X] = show_entry (hbox, "Marked X");
   entries[SHOW_MARKED_Y] = show_entry (hbox, "Marked Y");
   entries[SHOW_DELTA_X]  = show_entry (hbox, "Delta X");
   entries[SHOW_DELTA_Y]  = show_entry (hbox, "Delta Y");

   /* Create a GtkDatabox widget along with scrollbars and rulers */
   gtk_databox_create_box_with_scrollbars_and_rulers (&box, &table, 
                                                      TRUE, TRUE,
                                                      TRUE, TRUE);

   gtk_box_pack_start (GTK_BOX (box1), table, TRUE, TRUE, 0);
                                      
   entries[SHOW_BOX]  = box;

   X = g_new0 (gfloat, POINTS);
   Y = g_new0 (gfloat, POINTS);

   for (i = 0; i < POINTS; i++)
   {
      X[i] = i;
      Y[i] = 100. * sin (i * 2 * G_PI / POINTS);
   }
   color.red = 0;
   color.green = 65535;
   color.blue = 0;

   graph = gtk_databox_points_new (POINTS, X, Y, &color, 1);
   gtk_databox_graph_add (GTK_DATABOX (box), graph);

   Y = g_new0 (gfloat, POINTS);

   for (i = 0; i < POINTS; i++)
   {
      Y[i] = 100. * cos (i * 2 * G_PI / POINTS);
   }
   color.red = 65535;
   color.green = 0;
   color.blue = 0;

   graph = gtk_databox_points_new (POINTS, X, Y, &color, 1);
   gtk_databox_graph_add (GTK_DATABOX (box), graph);

   gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);

   separator = gtk_hseparator_new ();
   gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

   box2 = gtk_vbox_new (FALSE, 10);
   gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
   gtk_box_pack_end (GTK_BOX (box1), box2, FALSE, TRUE, 0);
   close_button = gtk_button_new_with_label ("close");
   g_signal_connect_swapped (G_OBJECT (close_button), "clicked",
			     G_CALLBACK (gtk_main_quit),
			     G_OBJECT (box));
   gtk_box_pack_start (GTK_BOX (box2), close_button, TRUE, TRUE, 0);
   GTK_WIDGET_SET_FLAGS (close_button, GTK_CAN_DEFAULT);
   gtk_widget_grab_default (close_button);

   g_signal_connect (G_OBJECT (box), "databox_zoomed",
		     G_CALLBACK (handle_signal_zoomed), 
                     NULL);
   g_signal_connect (G_OBJECT (box), "databox_selection_started",
		     G_CALLBACK (handle_signal_selection_started), 
                     NULL);
   g_signal_connect (G_OBJECT (box), "databox_selection_stopped",
		     G_CALLBACK (handle_signal_selection_stopped), 
                     NULL);
   g_signal_connect (G_OBJECT (box), "databox_selection_cancelled",
		     G_CALLBACK (handle_signal_selection_cancelled),
		     NULL);
   g_signal_connect (G_OBJECT (box), 
                     "motion_notify_event",
                     G_CALLBACK (show_motion_notify_cb),
                     entries);
   g_signal_connect (G_OBJECT (box), "button_press_event",
                     G_CALLBACK (show_button_press_cb),
                     entries);
   g_signal_connect (G_OBJECT (box), "databox_selection_changed",
                            G_CALLBACK (show_changed_cb),
                            entries);

   gtk_widget_show_all (window);

}

gint
main (gint argc, char *argv[])
{
   gtk_init (&argc, &argv);

   create_signals ();
   gtk_main ();

   return 0;
}

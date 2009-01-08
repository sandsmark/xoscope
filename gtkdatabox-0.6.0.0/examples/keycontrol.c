/* GtkDatabox - An extension to the gtk+ library
 * Copyright (C) 1998 - 2006  Dr. Roland Bock
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
#include <gtkdatabox_lines.h>
#include <gtkdatabox_cross_simple.h>
#include <gtkdatabox_marker.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gprintf.h>
#include <math.h>

#define POINTS 10

/*----------------------------------------------------------------
 *  databox basics
 *----------------------------------------------------------------*/

gfloat *X;
gfloat *Y;
gfloat *markX;
gfloat *markY;
guint markIndex;
gchar markLabel [100];

static void
shift_marker (GtkDatabox *box, GtkDataboxMarker *marker)
{
   markX [0] = X [markIndex];
   markY [0] = Y [markIndex];

   g_sprintf (markLabel, "(%2.2f, %2.2f)", X[markIndex], Y[markIndex]);
   gtk_databox_marker_set_label (marker, 1, 
                                 GTK_DATABOX_TEXT_SW, markLabel, TRUE);
   gtk_databox_redraw (box);
}

static gint
key_press_cb (GtkDatabox *box,
	    GdkEventKey *event, GtkDataboxMarker *marker)
{
   if (event->type != GDK_KEY_PRESS)
     return FALSE;

    switch (event->keyval) {
     case GDK_KP_Right: 
        if (markIndex < POINTS - 1)
        {
           markIndex ++;
           shift_marker (box, marker);
        }
        break;
     case GDK_KP_Left: 
        if (markIndex > 0)
        {
           markIndex --;
           shift_marker (box, marker);
        }
        break;
      case GDK_c:  ;
         break;
      case GDK_n:  
         break;
      case GDK_p:  g_printerr("p");
         break;
      case GDK_h:  gtk_databox_zoom_home (box);
         break;
      case GDK_o:  gtk_databox_zoom_out (box);
         break;
      default: break;
    }

    return FALSE;
}


static void
create_basics (void)
{
   GtkWidget *window = NULL;
   GtkWidget *box1;
   GtkWidget *box2;
   GtkWidget *close_button;
   GtkWidget *box;
   GtkWidget *label;
   GtkWidget *separator;
   GtkWidget *table;
   GtkDataboxGraph *graph;
   GdkColor color;
   GtkDataboxValue top_left;
   GtkDataboxValue bottom_right;
   gint i;

   window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   gtk_widget_set_size_request (window, 400, 400);

   g_signal_connect (GTK_OBJECT (window), "destroy",
                     G_CALLBACK (gtk_main_quit), NULL);

   gtk_window_set_title (GTK_WINDOW (window), "GtkDatabox: Key Controls");
   gtk_container_set_border_width (GTK_CONTAINER (window), 0);

   box1 = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (window), box1);

   label =
      gtk_label_new
      ("As long as the GtkDatabox has the focus,\nthe following keys will work in this example:\nKeypad-Right: Move marker to next data point\nKeypad-Left: Move marker to previous data point\no: zoom out (only useful if you zoomed in previously)\nh: zoom_home (only useful if you zoomed in previously)");
   gtk_box_pack_start (GTK_BOX (box1), label, FALSE, FALSE, 0);
   separator = gtk_hseparator_new ();
   gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, FALSE, 0);

   gtk_databox_create_box_with_scrollbars_and_rulers (&box, &table, 
                                                      TRUE, TRUE,
                                                      TRUE, TRUE);
   gtk_widget_add_events (box, GDK_KEY_PRESS_MASK);
   GTK_WIDGET_SET_FLAGS (box, GTK_CAN_FOCUS | GTK_CAN_DEFAULT);
   
   gtk_box_pack_start (GTK_BOX (box1), table, TRUE, TRUE, 0);

   color.red = 16383;
   color.green = 16383;
   color.blue = 16383;
   gtk_widget_modify_bg (box, GTK_STATE_NORMAL, &color);

   X = g_new0 (gfloat, POINTS);
   Y = g_new0 (gfloat, POINTS);

   for (i = 0; i < POINTS; i++)
   {
      X[i] = i;
      Y[i] = 100. * sin (i * 2 * G_PI / (POINTS - 1));
   }
   color.red = 0;
   color.green = 65535;
   color.blue = 0;

   graph = gtk_databox_lines_new (POINTS, X, Y, &color, 1);
   gtk_databox_graph_add (GTK_DATABOX (box), graph);
   
   markX = g_new0 (gfloat, 2);
   markY = g_new0 (gfloat, 2);

   color.red = 65535;
   color.green = 0;
   color.blue = 0;

   graph = gtk_databox_marker_new (2, markX, markY, &color, 7, 
                                   GTK_DATABOX_MARKER_TRIANGLE);
   gtk_databox_marker_set_position (GTK_DATABOX_MARKER (graph), 0, 
                                    GTK_DATABOX_MARKER_S);
   gtk_databox_marker_set_position (GTK_DATABOX_MARKER (graph), 1, 
                                    GTK_DATABOX_MARKER_N);
   markIndex = 0;
   shift_marker (GTK_DATABOX (box), GTK_DATABOX_MARKER (graph));
   gtk_databox_graph_add (GTK_DATABOX (box), graph);
 
   g_signal_connect (GTK_OBJECT (box), "key_press_event",
                     GTK_SIGNAL_FUNC (key_press_cb), graph);
   
   gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);

   gtk_databox_get_canvas (GTK_DATABOX (box), &top_left, &bottom_right);

   markX [1] = bottom_right.x;
   markY [1] = top_left.y;

   separator = gtk_hseparator_new ();
   gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

   box2 = gtk_vbox_new (FALSE, 10);
   gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
   gtk_box_pack_end (GTK_BOX (box1), box2, FALSE, TRUE, 0);
   close_button = gtk_button_new_with_label ("close");
   g_signal_connect_swapped (GTK_OBJECT (close_button), "clicked",
                             G_CALLBACK (gtk_main_quit), GTK_OBJECT (box));
   gtk_box_pack_start (GTK_BOX (box2), close_button, TRUE, TRUE, 0);
   gtk_widget_grab_default (box);
   gtk_widget_grab_focus (box);

   gtk_widget_show_all (window);
   gdk_window_set_cursor(box->window, gdk_cursor_new (GDK_CROSS));
}

gint
main (gint argc, char *argv[])
{
   gtk_init (&argc, &argv);

   create_basics ();
   gtk_main ();

   return 0;
}

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
#include <gtkdatabox_points.h>
#include <gtkdatabox_lines.h>
#include <gtkdatabox_bars.h>
#include <gtkdatabox_grid.h>
#include <gtkdatabox_cross_simple.h>
#include <gtkdatabox_marker.h>
#include <math.h>

#define POINTS 2000
#define STEPS 50
#define BARS 25
#define MARKER 10

/*----------------------------------------------------------------
 *  databox basics
 *----------------------------------------------------------------*/

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
   GtkWidget *scrollbar;
   GtkWidget *ruler;
   GtkDataboxGraph *graph;
   GtkDataboxValue min;
   GtkDataboxValue max;
   gfloat *X;
   gfloat *Y;
   gfloat buffer;
   GdkColor color;
   gint i;

   window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   gtk_widget_set_size_request (window, 300, 300);

   g_signal_connect (GTK_OBJECT (window), "destroy",
                     G_CALLBACK (gtk_main_quit), NULL);

   gtk_window_set_title (GTK_WINDOW (window), "GtkDatabox: Basics");
   gtk_container_set_border_width (GTK_CONTAINER (window), 0);

   box1 = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (window), box1);

   label =
      gtk_label_new
      ("Draw a selection with the left button pressed,\nThan click into the selection.\nUse the right mouse button to zoom out.\nShift+ right mouse button zooms to default.");
   gtk_box_pack_start (GTK_BOX (box1), label, FALSE, FALSE, 0);
   separator = gtk_hseparator_new ();
   gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, FALSE, 0);

   /* Instead of the following stuff, you could also call 
    * gtk_databox_create_box_with_scrollbars_and_rulers
    * as it is done in the other examples.
    * Of course, you are more flexible in using scrollbars and rulers 
    * by doing it yourself. */
   table = gtk_table_new (3, 3, FALSE);
   box = gtk_databox_new ();
   gtk_table_attach (GTK_TABLE (table), box, 1, 2, 1, 2,
                     GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                     GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 0);

   /* You could associate a scrollbar with a GtkDatabox widget either 
    * this way ...*/
   scrollbar = gtk_hscrollbar_new (NULL);
   gtk_databox_set_hadjustment (GTK_DATABOX (box), 
                           gtk_range_get_adjustment (GTK_RANGE (scrollbar)));
   gtk_table_attach (GTK_TABLE (table), scrollbar, 1, 2, 2, 3,
                     GTK_FILL | GTK_EXPAND | GTK_SHRINK, GTK_FILL, 0, 0);
   /* or this way ... */
   scrollbar = gtk_vscrollbar_new 
                   (gtk_databox_get_vadjustment (GTK_DATABOX (box)));
   gtk_table_attach (GTK_TABLE (table), scrollbar, 2, 3, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 0);
   ruler = gtk_hruler_new ();
   gtk_table_attach (GTK_TABLE (table), ruler, 1, 2, 0, 1,
                     GTK_FILL | GTK_EXPAND | GTK_SHRINK, GTK_FILL, 0, 0);
   gtk_databox_set_hruler (GTK_DATABOX (box), GTK_RULER (ruler));
   ruler = gtk_vruler_new ();
   gtk_table_attach (GTK_TABLE (table), ruler, 0, 1, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 0);
   gtk_databox_set_vruler (GTK_DATABOX (box), GTK_RULER (ruler));
   /* end of gtk_databox_create_box_with_scrollbars_and_rulers */
                                      
   gtk_box_pack_start (GTK_BOX (box1), table, TRUE, TRUE, 0);

   color.red = 16383;
   color.green = 16383;
   color.blue = 16383;
   gtk_widget_modify_bg (box, GTK_STATE_NORMAL, &color);

  
   X = g_new0 (gfloat, POINTS);
   Y = g_new0 (gfloat, POINTS);

   for (i = 0; i < POINTS; i++)
   {
      X[i] = i * 100;
      Y[i] = 100. * sin (i * 2 * G_PI / POINTS);
   }
   color.red = 0;
   color.green = 65535;
   color.blue = 0;

   graph = gtk_databox_points_new (POINTS, X, Y, &color, 1);
   gtk_databox_graph_add (GTK_DATABOX (box), graph);
   
   X = g_new0 (gfloat, STEPS);
   Y = g_new0 (gfloat, STEPS);

   for (i = 0; i < STEPS / 2; i++)
   {
      X[i * 2] = X[i * 2 + 1] = 2 * (i + .5) * POINTS/STEPS;

      if (i > 0)
         Y[i * 2 - 1] = 100. * sin ((i * 2) * 2 * G_PI / STEPS);
      Y[i * 2] = 100. * sin ((i * 2) * 2 * G_PI / STEPS);
      if (i == STEPS / 2 - 1) 
         Y[i * 2 + 1] = 100. * sin (((i + 1) * 2) * 2 * G_PI / STEPS);
   }
   color.red = 65535;
   color.green = 0;
   color.blue = 0;

   graph = gtk_databox_lines_new (STEPS, X, Y, &color, 1);
   gtk_databox_graph_add (GTK_DATABOX (box), graph);
   
   X = g_new0 (gfloat, BARS);
   Y = g_new0 (gfloat, BARS);

   for (i = 0; i < BARS; i++)
   {
      X[i] = i * POINTS/BARS;
      Y[i] = 80. * sin (i * 2 * G_PI / BARS);
   }
   color.red = 0;
   color.green = 65535;
   color.blue = 65535;

   graph = gtk_databox_bars_new (BARS, X, Y, &color, 1);
   gtk_databox_graph_add (GTK_DATABOX (box), graph);
   
   color.red = 32768;
   color.green = 32768;
   color.blue = 32768;

   graph = gtk_databox_cross_simple_new (&color, 0);
   gtk_databox_graph_add (GTK_DATABOX (box), graph);

   /* Instead of doing stuff what you see below, you could call 
    * gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);
    * as is done in the other examples 
    * Of course, you are more flexible in adjusting the scaling by doing 
    * it yourself, though. */
   if (0 > gtk_databox_calculate_extrema (GTK_DATABOX (box), 
                                          &min, &max))
   {
      g_warning ("Calculating extrema failed. Resorting to default values");
      min.x = -100.;
      min.y = -100.;
      max.x = +100.;
      max.y = +100.;
   }
   else
   {
      gfloat width = max.x - min.x;
      gfloat height = max.y - min.y;
      gfloat border = 0.05;
      
      min.x -= border * width;
      min.y -= border * height;
      max.x += border * width;
      max.y += border * height;
   }

   /* We want larger y values to be higher on the screen */
   buffer = min.y;
   min.y = max.y;
   max.y = buffer;
   
   gtk_databox_set_canvas (GTK_DATABOX(box), min, max);
   /* end of gtk_databox_auto_rescale */

   separator = gtk_hseparator_new ();
   gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

   box2 = gtk_vbox_new (FALSE, 10);
   gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
   gtk_box_pack_end (GTK_BOX (box1), box2, FALSE, TRUE, 0);
   close_button = gtk_button_new_with_label ("close");
   g_signal_connect_swapped (GTK_OBJECT (close_button), "clicked",
                             G_CALLBACK (gtk_main_quit), GTK_OBJECT (box));
   gtk_box_pack_start (GTK_BOX (box2), close_button, TRUE, TRUE, 0);
   GTK_WIDGET_SET_FLAGS (close_button, GTK_CAN_DEFAULT);
   gtk_widget_grab_default (close_button);
   gtk_widget_grab_focus (close_button);

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

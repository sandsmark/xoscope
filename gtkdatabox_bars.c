/* $Id: gtkdatabox_bars.c 4 2008-06-22 09:19:11Z rbock $ */
/* GtkDatabox - An extension to the gtk+ library
 * Copyright (C) 1998 - 2008  Dr. Roland Bock
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

#include <gtkdatabox_bars.h>

G_DEFINE_TYPE(GtkDataboxBars, gtk_databox_bars,
	GTK_DATABOX_TYPE_XYC_GRAPH)

static void gtk_databox_bars_real_draw (GtkDataboxGraph * bars,
					GtkDatabox* box);
/**
 * GtkDataboxBarsPrivate
 *
 * A private data structure used by the #GtkDataboxBars. It shields all internal things
 * from developers who are just using the object.
 *
 **/
typedef struct _GtkDataboxBarsPrivate GtkDataboxBarsPrivate;

struct _GtkDataboxBarsPrivate
{
   GdkSegment *data;
};

static void
bars_finalize (GObject * object)
{
   GtkDataboxBars *bars = GTK_DATABOX_BARS (object);

   g_free (GTK_DATABOX_BARS_GET_PRIVATE(bars)->data);

   /* Chain up to the parent class */
   G_OBJECT_CLASS (gtk_databox_bars_parent_class)->finalize (object);
}

static void
gtk_databox_bars_class_init (GtkDataboxBarsClass *klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
   GtkDataboxGraphClass *graph_class = GTK_DATABOX_GRAPH_CLASS (klass);

   gobject_class->finalize = bars_finalize;

   graph_class->draw = gtk_databox_bars_real_draw;

	g_type_class_add_private(klass, sizeof(GtkDataboxBarsPrivate));
}

static void
gtk_databox_bars_complete (GtkDataboxBars * bars)
{
   GTK_DATABOX_BARS_GET_PRIVATE(bars)->data =
      g_new0 (GdkSegment,
	      gtk_databox_xyc_graph_get_length
	      (GTK_DATABOX_XYC_GRAPH (bars)));
}

static void
gtk_databox_bars_init (GtkDataboxBars *bars)
{
   g_signal_connect (bars, "notify::length",
		     G_CALLBACK (gtk_databox_bars_complete), NULL);
}

/**
 * gtk_databox_bars_new:
 * @len: length of @X and @Y
 * @X: array of horizontal position values of markers
 * @Y: array of vertical position values of markers
 * @color: color of the markers
 * @size: marker size or line width (depending on the @type)
 *
 * Creates a new #GtkDataboxBars object which can be added to a #GtkDatabox widget
 *
 * Return value: A new #GtkDataboxBars object
 **/
GtkDataboxGraph *
gtk_databox_bars_new (guint len, gfloat * X, gfloat * Y,
		      GdkColor * color, guint size)
{
   GtkDataboxBars *bars;
   g_return_val_if_fail (X, NULL);
   g_return_val_if_fail (Y, NULL);
   g_return_val_if_fail ((len > 0), NULL);

   bars = g_object_new (GTK_DATABOX_TYPE_BARS,
			"X-Values", X,
			"Y-Values", Y,
			"length", len, "color", color, "size", size, NULL);

   return GTK_DATABOX_GRAPH (bars);
}

static void
gtk_databox_bars_real_draw (GtkDataboxGraph * graph,
			    GtkDatabox* box)
{
   GtkDataboxBars *bars = GTK_DATABOX_BARS (graph);
   GdkSegment *data;
   guint i = 0;
   gfloat *X;
   gfloat *Y;
   guint len;
   gint size = 0;
   gint16 zero = 0;
   cairo_t *cr;

   g_return_if_fail (GTK_DATABOX_IS_BARS (bars));
   g_return_if_fail (GTK_IS_DATABOX (box));

   cr = gtk_databox_graph_create_gc (graph, box);

   if (gtk_databox_get_scale_type_y (box) == GTK_DATABOX_SCALE_LOG)
      g_warning
	 ("gtk_databox_bars do not work well with logarithmic scale in Y axis");

   len = gtk_databox_xyc_graph_get_length (GTK_DATABOX_XYC_GRAPH (graph));
   X = gtk_databox_xyc_graph_get_X (GTK_DATABOX_XYC_GRAPH (graph));
   Y = gtk_databox_xyc_graph_get_Y (GTK_DATABOX_XYC_GRAPH (graph));
   size = gtk_databox_graph_get_size (graph);

   data = GTK_DATABOX_BARS_GET_PRIVATE(bars)->data;

   zero = gtk_databox_value_to_pixel_y (box, 0);

   for (i = 0; i < len; i++, data++, X++, Y++)
   {
      data->x1 = data->x2 = gtk_databox_value_to_pixel_x (box, *X);
      data->y1 = zero;
      data->y2 = gtk_databox_value_to_pixel_y (box, *Y);

      cairo_move_to (cr, data->x1 + 0.5, data->y1 + 0.5);
      cairo_line_to (cr, data->x2 + 0.5, data->y2 + 0.5);
   }
   cairo_stroke(cr);
   cairo_destroy(cr);

   return;
}

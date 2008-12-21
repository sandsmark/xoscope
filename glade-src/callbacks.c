#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#if 0
GtkWidget*
create_databox (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{

}

#endif

void
on_open1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_save1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_device2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_device_options2_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


gboolean
on_window1_key_press_event             (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{

  if (event->keyval == GDK_Tab) {
    handle_key('\t');
  } else if (event->length == 1) {
    handle_key(event->string[0]);
  }

  return FALSE;
}

void
hit_key__1__                           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_2_activate                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_3_activate                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_4_activate                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_5_activate                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_6_activate                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_7_activate                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_8_activate                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


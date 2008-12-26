#include <gtk/gtk.h>


GtkWidget*
create_databox (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);

void
on_open1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_device2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_device_options2_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_window1_key_press_event             (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
delete_event                           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
hit_key__1__                           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_2_activate                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_3_activate                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_4_activate                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_5_activate                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_6_activate                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_7_activate                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_8_activate                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
key_press_event                        (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

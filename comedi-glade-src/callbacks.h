#include <gtk/gtk.h>

extern GtkWidget *window;
extern guchar dialog_r[];

void
comedi_on_ok                           (GtkButton       *button,
                                        gpointer         user_data);

void
comedi_on_apply                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_bufsize_custom_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_bufsize_default_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_device_entry_changed                (GtkEditable     *editable,
                                        gpointer         user_data);

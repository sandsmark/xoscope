#include <gtk/gtk.h>

extern GtkWidget *window;
extern guchar dialog_r[];

void
bitscope_dialog();

void
on_toggled                             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_value_changed                       (GtkAdjustment *adj,
                                        gpointer         user_data);

void
on_notebook1_switch_page               (GtkNotebook     *notebook,
                                        GtkNotebookPage *page,
                                        gint             page_num,
                                        gpointer         user_data);

void
on_ok                                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_apply                               (GtkButton       *button,
                                        gpointer         user_data);

void
on_entry1_changed                      (GtkEditable     *editable,
                                        gpointer         user_data);

gboolean
on_entry1_focusout                     (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

/*
 * @(#)$Id: callbacks.c,v 1.2 2001/05/22 05:11:03 twitham Exp $
 *
 * Copyright (C) 2000 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the Bitscope dialog window.
 *
 */

#include <stdio.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#define LU(label)	lookup_widget(GTK_WIDGET(window), label)

GtkWidget *window;

gdouble dialog_volts;

void
bitscope_dialog()
{
  GtkWidget *dialog2;
  GtkWidget *hscale;

  add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps");
  add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");

  /*
   * The following code was added by Glade to create one of each component
   * (except popup menus), just so that you see something after building
   * the project. Delete any components that you don't want shown initially.
   */
  dialog2 = create_dialog2 ();
  gtk_widget_show (dialog2);
  window = dialog2;

  hscale = lookup_widget(dialog2, "hscale1");
  gtk_signal_connect (GTK_OBJECT (GTK_RANGE (hscale)->adjustment),
                      "value_changed", GTK_SIGNAL_FUNC (on_value_changed),
                      "0");
  hscale = lookup_widget(dialog2, "hscale2");
  gtk_signal_connect (GTK_OBJECT (GTK_RANGE (hscale)->adjustment),
                      "value_changed", GTK_SIGNAL_FUNC (on_value_changed),
                      "1");

  /* here we need to initially set all the widgets from bs.r */

  /* force pages to update each other to current values */
  gtk_notebook_set_page(GTK_NOTEBOOK(lookup_widget(dialog2, "notebook1")), 1);
  gtk_notebook_set_page(GTK_NOTEBOOK(lookup_widget(dialog2, "notebook1")), 0);

}

void
propagate_changes()
{
  int x, y;
  gchar format[] = "\261%.4g V", buffer[128]; /* char 261: +/- */
  gdouble volts[] = {
    0.130, 0.600,  1.200,  3.160,
    1.300, 6.000, 12.000, 31.600,
    0.632, 2.900,  5.800, 15.280,
  };

  if (GTK_TOGGLE_BUTTON(LU("radiobutton24"))->active)
    x = y = 8;
  else {
    x = GTK_TOGGLE_BUTTON(LU("checkbutton2"))->active ? 4 : 0;
    y = GTK_TOGGLE_BUTTON(LU("checkbutton3"))->active ? 4 : 0;
  }
  sprintf(buffer, format, volts[x]);
  gtk_label_set_text(GTK_LABEL(GTK_BUTTON(LU("radiobutton20"))->child), buffer);

  sprintf(buffer, format, volts[x + 1]);
  gtk_label_set_text(GTK_LABEL(GTK_BUTTON(LU("radiobutton21"))->child), buffer);

  sprintf(buffer, format, volts[x + 2]);
  gtk_label_set_text(GTK_LABEL(GTK_BUTTON(LU("radiobutton22"))->child), buffer);

  sprintf(buffer, format, volts[x + 3]);
  gtk_label_set_text(GTK_LABEL(GTK_BUTTON(LU("radiobutton23"))->child), buffer);

  sprintf(buffer, format, volts[y]);
  gtk_label_set_text(GTK_LABEL(GTK_BUTTON(LU("radiobutton14"))->child), buffer);

  sprintf(buffer, format, volts[y + 1]);
  gtk_label_set_text(GTK_LABEL(GTK_BUTTON(LU("radiobutton15"))->child), buffer);

  sprintf(buffer, format, volts[y + 2]);
  gtk_label_set_text(GTK_LABEL(GTK_BUTTON(LU("radiobutton16"))->child), buffer);

  sprintf(buffer, format, volts[y + 3]);
  gtk_label_set_text(GTK_LABEL(GTK_BUTTON(LU("radiobutton17"))->child), buffer);

  if (GTK_TOGGLE_BUTTON(LU("radiobutton21"))->active) x += 1;
  if (GTK_TOGGLE_BUTTON(LU("radiobutton22"))->active) x += 2;
  if (GTK_TOGGLE_BUTTON(LU("radiobutton23"))->active) x += 3;

  dialog_volts = volts[x];

  gtk_signal_emit_by_name(GTK_OBJECT(GTK_RANGE(LU("hscale1"))->adjustment),
			  "value_changed");
/*    gtk_signal_emit_by_name(GTK_OBJECT(LU("entry1")), "changed"); */
}


void
on_toggled                             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  propagate_changes();
}


void
on_value_changed                       (GtkAdjustment *adj,
                                        gpointer         user_data)
{
  static gchar bits[] = "10000000";
  static gchar care[] = "00000111";
  static gchar both[] = "10000XXX";
  static gchar num[32], lvl[32];
  static int i, j, k, bit, level = 128;

  if (((gchar *)user_data)[0] == '1') {
    level = adj->value;
    for (i = 0; i < 8; i++) {
      bits[i] = (level & (1 << (7 - i))) ? '1' : '0';
    }
  } else if (adj->value > -1)
    for (i = 0; i < 8; i++) {
      care[i] = i >= adj->value ? '1' : '0';
    }
  bit = 128;
  j = 1;
  k = 0;
  for (i = 0; i < 8; i++) {
    both[i] = (care[i] == '1') ? (bits[i] == '0' ? 'X' : 'Y') : bits[i];
    if (care[i] != '1') {
      j *= 2;
      if (bits[i] == '1')
	k += bit;
    }
    bit >>= 1;
  }
//  bits[i] = '\0';
  printf("bits: %s\tcare: %s\tboth: %s\n", bits, care, both);
  gtk_entry_set_text(GTK_ENTRY(LU("entry1")), both);
  sprintf(num, "%d @ %.4g Volts", j, dialog_volts * 2 / j);
  gtk_label_set_text(GTK_LABEL(LU("label21")), num);
  sprintf(lvl, "%.4f to %.4f Volts (%02x=%d)",
	  dialog_volts * (k - 128) / 128,
	  dialog_volts * ((256 / j + k) - 128) / 128,
	  level, level);
  gtk_label_set_text(GTK_LABEL(LU("label19")), lvl);
}


void
on_notebook1_switch_page               (GtkNotebook     *notebook,
                                        GtkNotebookPage *page,
                                        gint             page_num,
                                        gpointer         user_data)
{
/*    printf("page num: %d\n", page_num); */
  propagate_changes();
}

void
on_ok                                  (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_signal_emit_by_name(GTK_OBJECT(LU("button2")), "clicked");
  printf("ok\n");
  gtk_widget_destroy(LU("dialog2"));
/*    gtk_main_quit(); */
}


void
on_apply                               (GtkButton       *button,
                                        gpointer         user_data)
{
  /* snag the state of all widgets into bs.r & reset BitScope */
  printf("apply\n");
}

void
on_entry1_changed                      (GtkEditable     *editable,
                                        gpointer         user_data)
{
  int bits = 0, care = 0, bit = 1, i;
  gchar s[] = "\0\0\0\0\0\0\0\0\0";

  strncpy(s, gtk_entry_get_text(GTK_ENTRY(editable)), 8);

  for (i = 7; i >= 0; i--) {
    if (s[i] == 'y') s[i] = 'Y';
    if (s[i] == 'x') s[i] = 'X';
    if (s[i] != '0' && s[i] != '1' && s[i] != 'X' && s[i] != 'Y') s[i] = 'X';
    if (s[i] == '1' || s[i] == 'Y') bits += bit;
    if (care >= 0 && s[i] && (s[i] == '0' || s[i] == '1')) care++;
    if (care > 0 && (s[i] == 'X' || s[i] == 'Y')) care = -1;
    bit <<= 1;
  }
  printf("text:%s\tcare=%d\tbits=%d\n", s, care, bits);
  gtk_adjustment_set_value(gtk_range_get_adjustment(GTK_RANGE(LU("hscale1"))),
			   care);
  gtk_adjustment_set_value(gtk_range_get_adjustment(GTK_RANGE(LU("hscale2"))),
			   bits);
  gtk_entry_set_text(GTK_ENTRY(editable), s);
}

gboolean
on_entry1_focusout                     (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
  gtk_signal_emit_by_name(GTK_OBJECT(LU("entry1")), "activate");
  return FALSE;
}

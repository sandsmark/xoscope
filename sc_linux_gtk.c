/*
 * @(#)$Id: sc_linux_gtk.c,v 1.2 2004/11/04 19:47:35 baccala Exp $
 *
 * Copyright (C) 1996 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the sound card GUI.  It's just setting the DMA
 * buffer divisor to 1, 2, or 4.  Rate setting can be done via the '('
 * and ')' keys.  The old DMA value is found via a save_option() call
 * (DMA value is assumed to be option #1).  We keep a temp_dma
 * variable in case the user closes or cancels the dialog without
 * clicking 'Accept'.  On 'Accept', we set the variable by forming an
 * ASCII option string and doing a set_option().
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "oscope.h"

static int temp_dma;
static int temp_rec;

static void
save_values(GtkWidget *w, gpointer data)
{
  char buf[16];

  snprintf(buf, sizeof(buf), "dma=%d", temp_dma);
  datasrc->set_option(buf);
  clear();
}

static void
esd_save_values(GtkWidget *w, gpointer data)
{
  char buf[16];

  snprintf(buf, sizeof(buf), "esdrecord=%d", temp_rec);
  datasrc->set_option(buf);
  clear();
}

static void
dma(GtkWidget *w, gpointer data)
{
  temp_dma = (int) data;
}

static void
esdrecord(GtkWidget *w, gpointer data)
{
  temp_rec = (int) data;
}

void
sc_gtk_option_dialog()
{
  GtkWidget *window, *label, *accept, *cancel;
  GtkWidget *dma1, *dma2, *dma4;
  char *option;

  window = gtk_dialog_new();
  accept = gtk_button_new_with_label(" Accept ");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area), accept,
		     TRUE, TRUE, 0);
  cancel = gtk_button_new_with_label("  Cancel  ");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area), cancel,
		     TRUE, TRUE, 0);

  label = gtk_label_new("  Sound Card DMA divisor:  ");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), label,
		     TRUE, TRUE, 5);

  dma1 = gtk_radio_button_new_with_label(NULL, "1");
  dma2 = gtk_radio_button_new_with_label
    (gtk_radio_button_group(GTK_RADIO_BUTTON(dma1)), "2");
  dma4 = gtk_radio_button_new_with_label
    (gtk_radio_button_group(GTK_RADIO_BUTTON(dma2)), "4");

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), dma1,
		     TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), dma2,
		     TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), dma4,
		     TRUE, TRUE, 0);

  option = datasrc->save_option(1);	/* format will be "dma=%d" */
  if (option) temp_dma = option[4] - '0';

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON(dma1), temp_dma == 1);
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON(dma2), temp_dma == 2);
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON(dma4), temp_dma == 4);

  gtk_signal_connect(GTK_OBJECT(dma1), "clicked",
		     GTK_SIGNAL_FUNC(dma), (gpointer) 1);
  gtk_signal_connect(GTK_OBJECT(dma2), "clicked",
		     GTK_SIGNAL_FUNC(dma), (gpointer) 2);
  gtk_signal_connect(GTK_OBJECT(dma4), "clicked",
		     GTK_SIGNAL_FUNC(dma), (gpointer) 4);

  gtk_signal_connect_object(GTK_OBJECT(window), "delete_event",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(window));
  gtk_signal_connect(GTK_OBJECT(accept), "clicked",
		     GTK_SIGNAL_FUNC(save_values), NULL);
  gtk_signal_connect_object_after(GTK_OBJECT(accept), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(window));
  gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(window));

  GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS(accept, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(accept);

  gtk_widget_show(dma1);
  gtk_widget_show(dma2);
  gtk_widget_show(dma4);
  gtk_widget_show(accept);
  gtk_widget_show(cancel);
  gtk_widget_show(label);
  gtk_widget_show(window);
}

void
esd_gtk_option_dialog()
{
  GtkWidget *window, *label, *accept, *cancel;
  GtkWidget *on, *off;
  char *option;

  window = gtk_dialog_new();
  accept = gtk_button_new_with_label(" Accept ");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area), accept,
		     TRUE, TRUE, 0);
  cancel = gtk_button_new_with_label("  Cancel  ");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area), cancel,
		     TRUE, TRUE, 0);

  label = gtk_label_new("  ESD Record Mode:  ");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), label,
		     TRUE, TRUE, 5);

  on = gtk_radio_button_new_with_label(NULL, "On");
  off = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(on)), "Off");

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), on,
		     TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), off,
		     TRUE, TRUE, 0);

  option = datasrc->save_option(1);	/* format will be "esdrecord=%d" */
  if (option) temp_rec = option[10] - '0';

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON(on), temp_rec == 1);
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON(off), temp_rec == 0);

  gtk_signal_connect(GTK_OBJECT(on), "clicked",
		     GTK_SIGNAL_FUNC(esdrecord), (gpointer) 1);
  gtk_signal_connect(GTK_OBJECT(off), "clicked",
		     GTK_SIGNAL_FUNC(esdrecord), (gpointer) 0);

  gtk_signal_connect_object(GTK_OBJECT(window), "delete_event",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(window));
  gtk_signal_connect(GTK_OBJECT(accept), "clicked",
		     GTK_SIGNAL_FUNC(esd_save_values), NULL);
  gtk_signal_connect_object_after(GTK_OBJECT(accept), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(window));
  gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(window));

  GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS(accept, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(accept);

  gtk_widget_show(on);
  gtk_widget_show(off);
  gtk_widget_show(accept);
  gtk_widget_show(cancel);
  gtk_widget_show(label);
  gtk_widget_show(window);
}

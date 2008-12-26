/*
 * @(#)$Id: display.c,v 2.9 2008/12/26 18:07:45 baccala Exp $
 *
 * Copyright (C) 1996 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the UI-independent display code
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "oscope.h"		/* program defaults */
#include "display.h"
#include "func.h"

#include "com_gtk.h"
#include <gtk/gtk.h>
#include <gtkdatabox.h>
#include <gtkdatabox_points.h>
#include <gtkdatabox_lines.h>
#include <gtkdatabox_grid.h>

extern GtkWidget *databox;

#define DEBUG 0

void	show_data();
void	init_widgets();
void	fix_widgets();

int	triggered = 0;		/* whether we've triggered or not */
void	*font;
int	math_warning = 0;	/* TRUE if math has a problem */

struct signal_stats stats;

/* how to convert text column (0-79) to graphics position */
int
col(int x)
{
  if (x < 13)			/* left side; absolute */
    return (x * 8);
  if (x > 67)			/* right side; absolute */
    return (h_points - ((80 - x) * 8));
  /* middle; spread it out proportionally */
  return ((x - 12) * (h_points - 200) / 55 + 100);
}

/* how to convert text row(0-29) to graphics position */
int
row(int y)
{
  if (y < 4)			/* top; absolute */
    return (y * 16);
  if (y == 4)
    return 62;
  if (y > 24)			/* bottom; absolute */
    return (v_points - ((30 - y) * 16));
  /* center; spread out proportionally */
  return ((y - 5) * (v_points - 160) / 20 + 80);
}

/* draw a temporary one-line message to center of screen */
void
message(char *message, int clr)
{
  /* XXX need something here */
}

void
format(char *buf, const char *fmt, float num)
{
  int power=0;

  /* Round off num to nearest 1% */

  while (num > 100) num /= 10, power ++;
  while ((num > 0) && (num < 10)) num *= 10, power --;
  num = rint(num);
  num *= pow(10.0, power);

  sprintf(buf, fmt, num >= 1000 ? num / 1000 : num, num >= 1000 ? "" : "m");
}

void
make_help_text_visible(GtkWidget *widget)
{
  if (GTK_IS_CONTAINER(widget)) {
    gtk_container_forall(widget, make_help_text_visible, NULL);
  } else {
    const gchar * name = gtk_widget_get_name(widget);
    if (name != NULL &&
	(!strcmp(name + strlen(name) - 11, "_help_label")
	 || !strcmp(name + strlen(name) - 10, "_key_label"))) {
      gtk_label_set_text(GTK_LABEL(widget), g_object_get_data(G_OBJECT(widget), "visible-text"));
    }
  }
}

void
make_help_text_invisible(GtkWidget *widget)
{
  if (GTK_IS_CONTAINER(widget)) {
    gtk_container_forall(widget, make_help_text_invisible, NULL);
  } else {
    const gchar * name = gtk_widget_get_name(widget);
    if (name != NULL &&
	(!strcmp(name + strlen(name) - 11, "_help_label")
	 || !strcmp(name + strlen(name) - 10, "_key_label"))) {
      gtk_label_set_markup(GTK_LABEL(widget), g_object_get_data(G_OBJECT(widget), "invisible-text"));
    }
  }
}

/* We have to copy the saved_text as well as the modified_text because
 * the text will be changed later (as we make it visible or invisible)
 * and that will invalidate the pointer returned from
 * gtk_label_get_label().
 */

void
setup_help_text(GtkWidget *widget)
{
  if (GTK_IS_CONTAINER(widget)) {
    gtk_container_forall(widget, setup_help_text, NULL);
  } else {
    const gchar * name = gtk_widget_get_name(widget);
    if (name != NULL &&
	(!strcmp(name + strlen(name) - 11, "_help_label")
	 || !strcmp(name + strlen(name) - 10, "_key_label"))) {
      gchar * text = gtk_label_get_label(GTK_LABEL(widget));
      gchar * saved_text = malloc(sizeof(gchar) * 80);
      gchar * modified_text = malloc(sizeof(gchar) * 80);

      sprintf(modified_text, "<span foreground=\"black\">%s</span>",
	      g_markup_escape_text(text, -1));
      strcpy(saved_text, text);
      g_object_set_data(G_OBJECT(widget), "visible-text", saved_text);
      g_object_set_data(G_OBJECT(widget), "invisible-text", modified_text);
    }
  }
}

/* draw just dynamic or all text to graphics screen */
void
draw_text(int all)
{
  static char string[81], widget[81];
  static char *s;
  static int i, j, k, frames = 0;
  static time_t sec, prev;
  static Channel *p;
  static char *strings[] = {
    "Point",
    "Point Accum.",
    "Line",
    "Line Accum.",
    "Step",
    "Step Accum.",
  };
  static char *trigs[] = {
    "Auto",
    "Rising",
    "Falling"
  };

  p = &ch[scope.select];

  if (all) {			/* everything */

    /* above graticule */
    if (scope.verbose) {

      /* progname and version dynamic */

      /* setting help text is special */
      gtk_label_set_text(GTK_LABEL(LU("graticule_position_help_label")),
			 scope.behind ? "Behind" : "In Front");
      setup_help_text(LU("graticule_position_help_label"));

    } else {			/* not verbose */

      /* XXX display (?) Help */
    }

    if (scope.trige) {
      Signal *trigsig = datasrc->chan(scope.trigch);

      if (trigsig->volts > 0) {
	char minibuf[256];
	format(minibuf, "%g %sV",
	       (scope.trig) * trigsig->volts / 320);
	sprintf(string, "%s Trigger @ %s", trigs[scope.trige], minibuf);
      } else {
	sprintf(string, "%s Trigger @ %d",
		trigs[scope.trige], scope.trig);
      }
      gtk_label_set_text(GTK_LABEL(LU("trigger_label")), string);
      gtk_label_set_text(GTK_LABEL(LU("trigger_source_label")), trigsig->name);
    } else {
      gtk_label_set_text(GTK_LABEL(LU("trigger_label")), "No Trigger");
      gtk_label_set_text(GTK_LABEL(LU("trigger_source_label")), "");
    }

    gtk_label_set_text(GTK_LABEL(LU("data_source_label")),
		       datasrc ? datasrc->name : "No data source");

    gtk_label_set_text(GTK_LABEL(LU("line_style_label")), strings[scope.mode]);

    if (datasrc && (datasrc->option1str != NULL)
	&& ((s = datasrc->option1str()) != NULL)) {
      gtk_label_set_text(GTK_LABEL(LU("data_source_opt1_label")), s);
    } else {
      gtk_label_set_text(GTK_LABEL(LU("data_source_opt1_label")), "");
    }


    /* sides of graticule */
    for (i = 0 ; i < CHANNELS ; i++) {

      j = (i % 4) * 5 + 5;
      k = ch[i].color;

      if ((scope.verbose || ch[i].show || scope.select == i) && ch[i].signal) {

	/* XXX here and other places, need to make sure we clear the field */
	if (!ch[i].bits && ch[i].signal->volts)
	  format(string, "%g %sV/div",
		 (float)ch[i].signal->volts * ch[i].div / ch[i].mult / 10);
	else
	  sprintf(string, "%d / %d", ch[i].mult, ch[i].div);
	sprintf(widget, "Ch%1d_scale_label", i+1);
	gtk_label_set_text(GTK_LABEL(LU(widget)), string);

	sprintf(string, "%d @ %d", ch[i].bits, -(ch[i].pos));
	sprintf(widget, "Ch%1d_position_label", i+1);
	gtk_label_set_text(GTK_LABEL(LU(widget)), string);

	sprintf(widget, "Ch%1d_source_label", i+1);
	gtk_label_set_text(GTK_LABEL(LU(widget)), ch[i].signal->name);

      } else {

	sprintf(widget, "Ch%1d_scale_label", i+1);
	gtk_label_set_text(GTK_LABEL(LU(widget)), "");
	sprintf(widget, "Ch%1d_position_label", i+1);
	gtk_label_set_text(GTK_LABEL(LU(widget)), "");
	sprintf(widget, "Ch%1d_source_label", i+1);
	gtk_label_set_text(GTK_LABEL(LU(widget)), "");

      }

      sprintf(widget, "Ch%1d_frame", i+1);
      if (scope.select == i) {
	gtk_frame_set_shadow_type(GTK_FRAME(LU(widget)), GTK_SHADOW_ETCHED_IN);
      } else {
	gtk_frame_set_shadow_type(GTK_FRAME(LU(widget)), GTK_SHADOW_NONE);
      }

    }

    /* below graticule */
    if (scope.verbose) {

      /* setting help text is special */
      gtk_label_set_text(GTK_LABEL(LU("tab_help_label")), p->show ? "Visible" : "HIDDEN");
      setup_help_text(LU("tab_help_label"));
#if 0
      if (scope.select > 1) {
	text_write("($)", 72, 25,
		  0, KEY_FG, TEXT_BG, ALIGN_RIGHT);
	text_write("Extern", 78, 25,
		  0, p->color, TEXT_BG, ALIGN_RIGHT);
	text_write("(:)    (;)", 79, 26,
		  0, KEY_FG, TEXT_BG, ALIGN_RIGHT);
	text_write("Math", 76, 26,
		  0, p->color, TEXT_BG, ALIGN_RIGHT);
      }
#endif

    }

    if (datasrc && (datasrc->option2str != NULL)
	&& ((s = datasrc->option2str()) != NULL)) {
      gtk_label_set_text(GTK_LABEL(LU("data_source_opt2_label")), s);
    } else {
      gtk_label_set_text(GTK_LABEL(LU("data_source_opt2_label")), "");
    }

    fix_widgets();
    prev = -1;
    show_data();
    return;			/* show_data will call again to do the rest */
  }

  /* always draw the dynamic text, if signal is analog (bits == 0) */
  if (p->signal && (p->signal->bits == 0)) {

    sprintf(string, "  Period of %6d us = %6d Hz  ", stats.time,  stats.freq);
    gtk_label_set_text(GTK_LABEL(LU("period_label")), string);

    if (p->signal->volts)
      sprintf(string, "   %7.5g - %7.5g = %7.5g mV   ",
	      (float)stats.max * p->signal->volts / 320,
	      (float)stats.min * p->signal->volts / 320,
	      ((float)stats.max - stats.min) * p->signal->volts / 320);
    else
      sprintf(string, " Max:%3d - Min:%4d = %3d Pk-Pk ",
	      stats.max, stats.min, stats.max - stats.min);
    gtk_label_set_text(GTK_LABEL(LU("min_max_label")), string);
  }

  if (math_warning) {
#if 0
#if 0
    sprintf(string, "WARNING: math(%d,%d) is bogus!",
	    ch[0].signal->rate, ch[1].signal->rate);
    text_write(string, 40, 4, 0, KEY_FG, TEXT_BG, ALIGN_CENTER);
#else
    text_write("WARNING: math is bogus!", 40, 4,
	       0, KEY_FG, TEXT_BG, ALIGN_CENTER);
#endif
#endif
  }

  if (datasrc && datasrc->status_str != NULL) {
    for (i=0; i<8; i++) {
      sprintf(widget, "status%d_label", i);
      if ((s = datasrc->status_str(i)) != NULL) {
	gtk_label_set_text(GTK_LABEL(LU(widget)), s);
      } else {
	gtk_label_set_text(GTK_LABEL(LU(widget)), "");
      }
    }
  } else {
    for (i=0; i<8; i++) {
      sprintf(widget, "status%d_label", i);
      gtk_label_set_text(GTK_LABEL(LU(widget)), "");
    }
  }

  time(&sec);
  if (sec != prev) {		/* fix "scribbled" text once a second */

    strcpy(string, scope.run ? (scope.run > 1 ? "WAIT" : " RUN") : "STOP");
    gtk_label_set_text(GTK_LABEL(LU("run_stop_label")), string);

    sprintf(string, "fps:%3d", frames);
    gtk_label_set_text(GTK_LABEL(LU("fps_label")), string);

    /* Use UTF-8 micro sign */
    i = 1000 * scope.div / scope.scale;
    sprintf(string, "%d %ss/div", i > 999 ? i / 1000: i, i > 999 ? "m" : "\302\265");
    gtk_label_set_text(GTK_LABEL(LU("timebase_label")), string);

    if (p->signal) {

      /* XXX what do we want here - frame samples, samples per screen? */

      /* I cut and changed this line a half dozen times trying to decide
       * what number I wanted displayed as the "Samples" - p->signal->num
       * would give us the actual number of samples in the signal, but
       * that changes during the course of a sweep.  Now I've got the
       * number of samples per sweep, which isn't quite acceptable if
       * there's no data on the screen, or if we're displaying a memory
       * channel with a fixed number of sample
       */

      /* sprintf(string, "%d Samples", p->signal->num); */
      /* sprintf(string, "%d Samples", samples(p->signal->rate)); */
      sprintf(string, "%d Samples/frame", p->signal->width);
      gtk_label_set_text(GTK_LABEL(LU("samples_per_frame_label")), string);

      if (p->signal->rate > 0) {

	sprintf(string, "%d S/s", p->signal->rate);
	gtk_label_set_text(GTK_LABEL(LU("sample_rate_label")), string);

      } else if (p->signal->rate < 0) {

	/* Special case for a Fourier Transform.  p->signal->rate is
	 * the negative of the frequency step for each point in the
	 * transform, times 10.  Since there are 44 x-coordinates in a
	 * division, after applying the scope's current time base
	 * multiplier (scope.div / scope.scale), we multiply by 44/10
	 * to get the number of Hz in a division.
	 */

	sprintf(string, "%d Hz/div FFT",
		(- p->signal->rate) * 44 * scope.div / scope.scale / 10);
	gtk_label_set_text(GTK_LABEL(LU("sample_rate_label")), string);
      }
    }

    for (i = 0 ; i < 26 ; i++) {
      if (datasrc && i < datasrc->nchans()) {
	/* XXX Maybe here we should show color by channel if sig displayed */
	string[i] = i + 'a';
      } else if (mem[i].num > 0) {
	/* XXX different color here for memory? */
	string[i] = i + 'a';
      } else {
	string[i ] = ' ';
      }
    }
    string[i] = '\0';
    gtk_label_set_text(GTK_LABEL(LU("registers_label")), string);

    /* setting help text is special */
    sprintf(string, "(%c-Z)", 'A' + (datasrc ? datasrc->nchans() : 0));
    gtk_label_set_text(GTK_LABEL(LU("store_key_label")), string);
    setup_help_text(LU("store_key_label"));

    /* setting help text is special */
    sprintf(string, "(%c-z)", 'a' + (datasrc ? datasrc->nchans() : 0));
    gtk_label_set_text(GTK_LABEL(LU("recall_key_label")), string);
    setup_help_text(LU("recall_key_label"));

    frames = 0;
    prev = sec;
  }

  if (scope.verbose) {
      make_help_text_visible(glade_window);
  } else {
      make_help_text_invisible(glade_window);
  }

  frames++;
}

/* roundoff_multipliers() - set mult/div, based on target mult/div if
 * channel is displaying a signal with a voltage scale, then round
 * the multipliers to something conventional (i.e, 10 mV/div instead
 * of 9.7 mV/div), otherwise leave them alone.
 *
 * The rounding is done by computing the base ten logarithm of what
 * the mV-per-division value would be if we just used the target
 * mult/div ratio.  Then we throw away the integer part, leaving
 * a number between 0 and 1 corresponding to a leading digit between
 * 1 and 10.  By comparing this to the logarithms of 7.5 (.875),
 * 3.5 (.544), and 1.5 (.176), we pick a target of 10 (1.0),
 * 5 (0.7), 2 (0.3), or 1 (0.0), and subtract out the corresponding
 * logarithm.  The difference is the power of ten we need to multiply
 * mult/div by to get to our target.  At this point, we'd like a nice
 * algorithm to find the closest rational fraction to a given real
 * number, but I don't know of one.  Instead, we just multiply
 * mult/div by 1000, then multiply the logarithm's power into
 * the largest of either mult or div.  It works within 1%, which
 * is the accuracy we display the voltage scale with.
 */

void
roundoff_multipliers(Channel *p)
{

  if (p->signal && p->signal->volts && !p->bits) {

    double mV_per_div;
    double logmV;

    mV_per_div = (double)p->signal->volts
      * p->target_div / p->target_mult / 10;

    logmV = log10(mV_per_div);
    logmV -= floor(logmV);

    if (logmV > .875) logmV = logmV - 1.0;
    else if (logmV > .544) logmV = logmV - 0.7;
    else if (logmV > .176) logmV = logmV - 0.3;

    p->mult = p->target_mult * 1000;
    p->div = p->target_div * 1000;
 
    if (p->mult > p->div) {
      p->mult *= pow(10.0, logmV);
    } else {
      p->div *= pow(10.0, -logmV);
    }

#if 0
    printf("roundoff_multipliers() %d/%d -> %d/%d\n",
	   p->target_mult, p->target_div, p->mult, p->div);
#endif

  } else {

    p->mult = p->target_mult;
    p->div = p->target_div;

  }

}

/* The Graticule - we create it as graphs within the databox, then add
 * them as necessary with calls to draw_graticule().  The reason we
 * remove and then re-add them is to make sure they appear either
 * above or below the data, which is controlled by the call ordering
 * to draw_data() and draw_graticule() from show_data().
 */

GtkDataboxGraph *graticule_majorx_graph = NULL;
GtkDataboxGraph *graticule_majory_graph = NULL;
GtkDataboxGraph *graticule_minor_graph = NULL;

int total_horizontal_divisions = 10;

int major_graticule_displayed = 0;
int minor_graticule_displayed = 0;

gfloat Xa[2], Ya[2], Xb[2], Yb[2];

void recompute_graticule(void)
{
  Xa[0] = Xa[1] = 5 * 0.001 * (gfloat) scope.div / scope.scale;
  Ya[0] = 80;
  Ya[1] = v_points - 80;

  Yb[0] = Yb[1] = (v_points - 160)/2 + 80;
  Xb[0] = 0;
  Xb[1] = 10 * 0.001 * (gfloat) scope.div / scope.scale;

  gtk_databox_grid_set_vlines(graticule_minor_graph,
			      total_horizontal_divisions - 1);
}

void
create_graticule()
{
  GtkStyle *style;
  GdkColor gcolor;

#if 0
  static int i, j;
  static int tilt[] = {
    0, -10, 10
  };

  /* a mark where the trigger level is, if the triggered channel is shown */
  if (scope.trige) {
    i = -1;
    for (j = 7 ; j >= 0 ; j--) {
      if (ch[j].show && ch[j].signal == datasrc->chan(scope.trigch))
	i = j;
    }
    if (i > -1) {
      j = offset + ch[i].pos - scope.trig * ch[i].mult / ch[i].div;
      SetColor(ch[i].color);
      DrawLine(90, j + tilt[scope.trige], 110, j - tilt[scope.trige]);
    }
  }
#endif

  /* Use the same color for the graticule that is used for the frame
   * around the databox, which is its BACKGROUND color.
   */

  style = gtk_widget_get_style(GTK_WIDGET(LU("databox_frame")));
  gcolor = style->bg[GTK_STATE_NORMAL];

  /* the minor graph - the scope display is divided into a 10x10 grid
   * with 9x9 lines
   */

  graticule_minor_graph = gtk_databox_grid_new (9, 9, &gcolor, 1);

  /* the major grid - 2x2 */

  recompute_graticule();

  graticule_majory_graph = gtk_databox_lines_new (2, Xa, Ya, &gcolor, 2);

  graticule_majorx_graph = gtk_databox_lines_new (2, Xb, Yb, &gcolor, 2);

}

/* clear_databox() - very similar to
 *    gtk_databox_graph_remove_all(GTK_DATABOX(databox))
 * except that we don't remove quite EVERYTHING (we leave the graticule
 * and the cursors), and we free all the associated data structures
 */

void free_signalline(SignalLine *sl)
{
  while (sl != NULL) {
    SignalLine *slnext = sl->next;

    if (sl->graph != NULL) {
      gtk_databox_graph_remove(GTK_DATABOX(databox), sl->graph);
      g_object_unref(G_OBJECT(sl->graph));
    }
    g_free(sl->X);
    g_free(sl->Y);

    free(sl);
    sl = slnext;
  }
}

void clear_databox(void)
{
  int j, bit;

  for (j = 0 ; j < CHANNELS ; j++) {
    Channel *p = &ch[j];
    for (bit = 0; bit < 16 ; bit++) {
      while (p->signalline[bit] != NULL) {
	free_signalline(p->signalline[bit]);
	p->signalline[bit] = NULL;
      }
    }
  }
}

/* configure_databox() - this function takes care of figuring out the
 * various settings needed on the databox to display a particular
 * timebase.  Since the floating point values plotted within the
 * databox are always stored in seconds, selecting a new timebase
 * means setting things on the databox more than anything else.
 */

void configure_databox(void)
{
   GtkDataboxValue topleft, bottomright;
   gfloat upper_time_limit;
   int j;

   /* The first thing we want to figure out is the maximum time span
    * of any of our displayed signals.  The scope's base rate of
    * (scope.div / scope.scale) is in ms/div; 10 (minor) divs are
    * visible on the screen at once, so the maximum time span (in
    * seconds) is at least...
    */

   upper_time_limit = 10 * 0.001 * (gfloat) scope.div / scope.scale;

   /* But it might be more, if we have stuff stored... */

   for (j = 0 ; j < CHANNELS ; j++) {
     Channel *p = &ch[j];

     /* XXX for an FFT channel, p->signal->rate will be negative */

     if (p->show && p->signal) {
       if ((gfloat) p->signal->num / p->signal->rate > upper_time_limit) {
	 upper_time_limit = (gfloat) p->signal->num / p->signal->rate;
       }
     }
   }

   /* Now figure how many total divisions wide we'll make the databox.
    * Since we sample a little past the end of the trace (to fill the
    * entire visible area), we ignore any trailing part of a trace
    * that takes less than half a division to display.  We start with
    * ten divisions, and jump up in increments of five because five
    * minor divisions make a major division and we want to stay on a
    * major division boundary to make our graticule grid nice and
    * neat.
    *
    * XXX If we have an enormous signal (relative to our timebase),
    * this calculation could overflow int total_horizontal_divisions.
    */

   for (total_horizontal_divisions = 10;
	upper_time_limit > (total_horizontal_divisions + 0.5)
	  * 0.001 * (gfloat) scope.div / scope.scale;
	total_horizontal_divisions += 5);

   /* Now set the total canvas size of the databox */

   topleft.x = 0;
   topleft.y = 80;

   bottomright.x = total_horizontal_divisions
     * 0.001 * (gfloat) scope.div / scope.scale;
   bottomright.y = v_points - 80;

   gtk_databox_set_canvas(GTK_DATABOX(databox), topleft, bottomright);

   /* A slight adjustment gets us our visible area.  Note that this
    * call also resets the databox viewport to its left most position.
    */

   bottomright.x = 10 * 0.001 * (gfloat) scope.div / scope.scale;
   gtk_databox_set_visible_canvas(GTK_DATABOX(databox), topleft, bottomright);

   /* Decide if we need a scrollbar or not */

   if (total_horizontal_divisions > 10) {
     gtk_widget_show(GTK_WIDGET(LU("databox_hscrollbar")));
   } else {
     gtk_widget_hide(GTK_WIDGET(LU("databox_hscrollbar")));
   }

   /* And recompute the graticule grids */

   recompute_graticule();
}

/* clear() - one of the most important functions in the program,
 * called whenever something 'changes'
 *
 * Clear the display, clear data history on all display
 * channels, and redraw all text.  Since this clears data history
 * (both on the screen and in memory), it should only be called when
 * needed.
 *
 * XXX These two goals are incompatible - to call this function
 * whenever something changes, and to call it only when needed - and
 * it shows.  We don't want to clear the screen's history unless
 * necessary.  In particular, we want to be able to change time bases
 * with a frozen trace on the screen.
 */

void
clear()
{
  int i;

  clear_databox();

  if (datasrc) {

    /* In oscope.h, I wrote "Only after reset() has been called are
     * the rate and volts fields in the Signal structures guaranteed
     * valid".  So... we reset() once to make sure the rate and volts
     * fields are valid, then use the rate field in the first active
     * channel to set the capture width to the number of samples
     * required to fill the screen at that rate, then reset() again to
     * (re)start the capture.
     *
     * XXX Probably reset() needs to be split into two functions -
     * say reset() and start_sweep(), so then our sequence is
     * reset(), set_width(), start_sweep()
     *
     * XXX Also seems a little hokey the way we run through the
     * channels.  Implicit here is the code's current design
     * that all the channels for a data source have the
     * same rate and frame width.
     */

    datasrc->reset();
    if (datasrc->set_width) {
      int i;
      for (i=0; i<datasrc->nchans(); i++) {
	if (datasrc->chan(i)->listeners > 0) {
	  datasrc->set_width(samples(datasrc->chan(i)->rate));
	  break;
	}
      }
      datasrc->reset();
    }
    setinputfd(datasrc->fd());
  }

  configure_databox();

  /* This also updates the 'volts' and 'rate' fields in the math
   * signals
   */

  math_warning = update_math_signals();

  for (i = 0; i < CHANNELS; i++) {
    ch[i].old_frame = 0;

    roundoff_multipliers(&ch[i]);
  }

  show_data();
  draw_text(1);
}

void
draw_graticule()
{
  if (graticule_minor_graph == NULL) {
    create_graticule();
  }

  if (major_graticule_displayed) {
    gtk_databox_graph_remove(GTK_DATABOX(databox), graticule_majorx_graph);
    gtk_databox_graph_remove(GTK_DATABOX(databox), graticule_majory_graph);
    major_graticule_displayed = 0;
  }

  if (minor_graticule_displayed) {
    gtk_databox_graph_remove(GTK_DATABOX(databox), graticule_minor_graph);
    minor_graticule_displayed = 0;
  }

  if (scope.grat) {
    gtk_databox_graph_add (GTK_DATABOX (databox), graticule_minor_graph);
    minor_graticule_displayed = 1;
  }

  if (scope.grat > 1) {
    gtk_databox_graph_add (GTK_DATABOX (databox), graticule_majorx_graph);
    gtk_databox_graph_add (GTK_DATABOX (databox), graticule_majory_graph);
    major_graticule_displayed = 1;
  }
}

/* draw_data()
 *
 * graph the data on the display, possibly erasing old data to make
 * room for the new.  To do this, we keep an array of data points,
 * half of which is the previous sweep that we're erasing as we go,
 * the other half of which is the new sweep that we're drawing as we
 * go.  At the end of a sweep, we flip pointers to the two halves.  If
 * we're in an accumulate mode, don't have to erase anything, because
 * we're just letting the traces accumulate on the screen.  Also draw
 * cursors here, and, near the end of the function, draw tick marks to
 * show zero levels.
 */

int max(int a, int b)
{
  return a > b ? a : b;
}

void
draw_data()
{
  static int i, j, y, mult, div, off, bit, start, end;
  int bitoff;
  gfloat num, l;
  static int preva = 100, prevb = 100;	/* previous cursor locations */
  Channel *p;
  SignalLine *sl;
  short *samp;
  gchar widget[80];
  GtkStyle *style;
  GdkColor gcolor;

  for (j = 0 ; j < CHANNELS ; j++) { /* plot each visible channel */
    p = &ch[j];

    if (p->show && p->signal) {

      /* Figure out color to use for this channel by fetching
       * foreground color of its label
       */

      sprintf(widget, "Ch%d_label", j+1);
      style = gtk_widget_get_style(GTK_WIDGET(LU(widget)));
      gcolor = style->fg[GTK_STATE_NORMAL];

      mult = p->mult;
      div = p->div;
      off = offset + p->pos;

      samp = p->signal->data;

      /* Compute num, the number of seconds per sample, based on the
       * signal's rate (in samples/sec). If the signal rate is zero
       * (unspecified) or negative (a special case for Fourier
       * Transforms, meaning the x scale is in Hz), we use a base rate
       * of one millisecond per sample.
       */

      if (p->signal->rate > 0) {
	num = (gfloat) 1 / p->signal->rate;
      } else {
	num = (gfloat) 1 / 1000;
      }

      /* Compute left offset based on delay specified by the signal
       * (which is in ten-thousandths of samples).
       */

      l = p->signal->delay * num / 10000;

      /* Erase and redraw the cursors, if they've moved.
       *
       * We always draw them, for much the same reason as we always
       * draw signal lines - because we've never quite sure if
       * a signal that coincides with them might have been erased,
       * and part of the cursor along with it.
       *
       * There's several things I don't like about the cursors.
       * First, the cursor positions are stored as offsets into the
       * data arrays, which means that if we change to a different
       * signal with a different sampling rate, the cursors move
       * around on the screen!  We'd have to worry about that in this
       * code, except that handle_key() always does a clear() when it
       * changes channels - not that I think a clear() is necessary
       * when we change channels, but at least it makes sure that we
       * don't get doubly drawn cursors if we move to a channel with a
       * different sampling rate.
       *
       * Also, this math might be a bit flakey, I'm not sure (I didn't
       * write it).  Multiplication and division with computer
       * integers is always a lot of trouble, and 'num' was designed
       * to convert from time to x-coords, not the other way around.
       *
       * And, of course, we're doing all this in one of the program's
       * most critical code sections, from a performance standpoint.
       * These cursors get redrawn every single time we refresh the
       * screen.  It really could be somewhat smarter.
       */

#if 0
      /* XXX update for new databox code */

      if (scope.curs && j == scope.select) {
	X = (scope.cursa - 1) * 10000 / num + l + 1;
	if (X != preva) {
	  /* XXX SetColor(color[0]); */
	  DrawLine(preva, 70, preva, v_points - 70);
	  preva = X;
	}
	if (X < h_points - 100) {
	  /* XXX SetColor(p->color); */
	  DrawLine(X, 70, X, v_points - 70);
	}

	X = (scope.cursb - 1) * 10000 / num + l + 1;
	if (X != prevb) {
	  /* XXX SetColor(color[0]); */
	  DrawLine(prevb, 70, prevb, v_points - 70);
	  prevb = X;
	}
	if (X < h_points - 100) {
	  /* XXX SetColor(p->color); */
	  DrawLine(X, 70, X, v_points - 70);
	}
      }
#endif

      /* XXX make sure that if we're displaying a digital signal,
       * we go into digital display mode.  Should be elsewhere.
       */
#if 0
      if (p->bits == 0 && p->signal->bits != 0) {
	p->bits = p->signal->bits;
      }
#endif

      if (!p->bits)		/* analog display mode: draw one line */
	start = end = -1;
      else {			/* logic analyzer mode: draw bits lines */
	start = 0;
	end = p->bits - 1;
      }

      for (bit = start ; bit <= end ; bit++) {

	/* Hardwired: 16 y-coords between bits in digital mode */
	bitoff = bit * 16 - end * 8 + 4;

	/* SignalLine structures contain all the stored information
	 * about the (x,y) coordinates we've drawn already and may
	 * need to erase
	 */
	sl = p->signalline[bit < 0 ? 0 : bit];

	if ((sl == NULL) ||
	    (p->signal->frame != p->old_frame) || (p->old_frame == 0)) {

	  /* New signal line, so we need a new SignalLine structure */

	  sl = (SignalLine *) malloc(sizeof(SignalLine));
	  if (sl == NULL) {
	    perror("xoscope: malloc(SignalLine)");
	    break;
	  }
	  bzero(sl, sizeof(SignalLine));

	  sl->next = p->signalline[bit < 0 ? 0 : bit];
	  p->signalline[bit < 0 ? 0 : bit] = sl;

	  /* There are two possibilities here.  Either this is a
	   * dynamic signal, in which can there will ultimately be
	   * samples(p->signal->rate) data points on the trace, or
	   * this is a stored signal, in which can there are
	   * p->signal->num data points and it won't change.  Oh, and
	   * if we're in step mode, we'll need twice as many display
	   * points as data points.
	   */

	  sl->X = g_new0(gfloat,
			 2*max(samples(p->signal->rate), p->signal->num));
	  sl->Y = g_new0(gfloat,
			 2*max(samples(p->signal->rate), p->signal->num));

	}


	/* If we're continuing a running sweep, remove the existing
	 * trace from the databox.  We'll put it back in later, with
	 * more data points.
	 */

	if (sl->graph != NULL) {
	  gtk_databox_graph_remove(GTK_DATABOX(databox), sl->graph);
	  g_object_unref(G_OBJECT(sl->graph));
	  sl->graph = NULL;
	}

	/* If we're not in an accumulate mode, erase anything
	 * lingering in the databox except the next to last trace,
	 * because we want to leave the trailing part of it drawn if
	 * we're in the middle of a sweep.  We do remove it from the
	 * databox, however.  We'll put it back in later, with fewer
	 * data points.
	 */

	if (scope.mode % 2 == 0) {

	  if (sl->next != NULL && sl->next->graph != NULL) {
	    gtk_databox_graph_remove(GTK_DATABOX(databox), sl->next->graph);
	    g_object_unref(G_OBJECT(sl->next->graph));
	    sl->next->graph = NULL;
	  }

	  if (sl->next != NULL && sl->next->next != NULL) {
	    free_signalline(sl->next->next);
	    sl->next->next = NULL;
	  }

	}

	/* Compute the points we want to draw on the current trace and
	 * write them into the SignalLine arrays.  'time' is an index
	 * into samp[] array, computed from the current x-coord.
	 * 'prev', the last 'time' we actually used to draw a point,
	 * is here to make sure that if a single sample is spread over
	 * several x-coords, we skip coords to draw one line across
	 * them all.
	 */

	/* XXX we'd really like to draw one point extra, so we're
	 * never left with a signal line that stops before the end of
	 * the screen.
	 */

	/* for (x = X; x <= h_points - 100 - l ; x++) { */
	/* for (x = X; 1; x++) { */
	for (i = sl->next_point; i < p->signal->num; i++) {

	  /* Hardwired: 8 y-coords is height of digital line */

	  y = off - (bit < 0 ? samp[i]
		       : (bitoff - (samp[i] & (1 << bit) ? 0 : 8)))
	    * mult / div;

	  if (scope.mode >= 4 && i > 0) {

	    /* Step mode.  Draw a horizontal line segment,
	     * then a vertical one (instead of a single line)
	     */

	    sl->X[sl->next_point] = l + i * num;
	    sl->Y[sl->next_point] = sl->Y[sl->next_point - 1];
	    sl->next_point ++;
	  }

	  sl->X[sl->next_point] = l + i * num;
	  sl->Y[sl->next_point] = y;
	  sl->next_point ++;

	}

	/* Add the current trace to the databox */

	if (sl->next_point > 0) {

	  if (scope.mode < 2)
	    sl->graph = gtk_databox_points_new (sl->next_point,
						sl->X, sl->Y, &gcolor, 1);
	  else
	    sl->graph = gtk_databox_lines_new (sl->next_point,
					       sl->X, sl->Y, &gcolor, 1);

	  gtk_databox_graph_add (GTK_DATABOX (databox), sl->graph);

	}

	/* If we're not in accumulate mode and there's a previous
	 * trace, draw the part of it to the right of the current
	 * trace.
	 */

	if ((scope.mode % 2 == 0) && (sl->next != NULL)
	    && (sl->next_point < sl->next->next_point)) {

	  if (scope.mode < 2)
	    sl->next->graph
	      = gtk_databox_points_new (sl->next->next_point - sl->next_point,
					sl->next->X + sl->next_point,
					sl->next->Y + sl->next_point,
					&gcolor, 1);
	  else
	    sl->next->graph
	      = gtk_databox_lines_new (sl->next->next_point - sl->next_point,
				       sl->next->X + sl->next_point,
				       sl->next->Y + sl->next_point,
				       &gcolor, 1);

	  gtk_databox_graph_add (GTK_DATABOX (databox), sl->next->graph);
	}

      }

      p->old_frame = p->signal->frame;

#if 0
      /* Draw tick marks on left and right sides of display showing zero pos */
      SetColor(p->color);
      DrawLine(90, off, 100, off);
      DrawLine(h_points - 100, off, h_points - 90, off);
#endif
    }
  }
}

/* calculate any math and plot the results and the graticule */
void
show_data(void)
{

  /* Run any math functions, then measure statistics to be displayed,
   * like min, max, frequency.  If the timebase is fast enough (less
   * than 100 ms/div) do this only at the end of a frame.
   */

  do_math();

  if ((scope.scale >= 100) || !in_progress)
    measure_data(&ch[scope.select], &stats);

  draw_text(0);
  if (scope.behind) {
    draw_graticule();		/* plot data on top of graticule */
    draw_data();
  } else {
    draw_data();		/* plot graticule on top of data */
    draw_graticule();
  }

  gtk_databox_redraw (GTK_DATABOX (databox));
}

/* animate() - get and plot some data
 *
 */

void
animate(void *data)
{
  static struct timeval current_time, prev_time;

  /* To avoid hammering the X server, don't do anything if it's been
   * less than scope.min_interval milliseconds (default 50) since the
   * last time we ran this function.  If we do skip processing, then
   * set a timeout to make sure we run again scope.min_interval
   * milliseconds from now.
   */

  gettimeofday(&current_time, NULL);

  if ((prev_time.tv_sec <= current_time.tv_sec)
      && (prev_time.tv_sec + 10 > current_time.tv_sec)
      && (1000000 * (current_time.tv_sec - prev_time.tv_sec)
	  + current_time.tv_usec - prev_time.tv_usec
	  < 1000 * scope.min_interval)) {
    settimeout(scope.min_interval);
    setinputfd(-1);
    return;
  }

  prev_time = current_time;
  if (datasrc) setinputfd(datasrc->fd());
  settimeout(0);

  clip = 0;
  if (datasrc) {
    if (scope.run) {
      triggered = datasrc->get_data();
      if (triggered && scope.run > 1) { /* auto-stop single-shot wait */
	scope.run = 0;
	draw_text(1);
      }
    } else if (in_progress) {
      datasrc->get_data();
    } else {
      //usleep(100000);		/* no need to suck all CPU cycles */
      setinputfd(-1);		/* scope not running, so why listen? */
    }
  }
  show_data();
}

/* [re]initialize graphics screen */
void
init_screen()
{
  int i, channelcolor[] = CHANNELCOLOR;

  for (i = 0 ; i < CHANNELS ; i++) {
    ch[i].color = color[channelcolor[i]];
  }
  fix_widgets();
  offset = v_points / 2;
  draw_text(1);
  clear();
}

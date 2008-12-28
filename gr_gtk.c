/*
 * @(#)$Id: gr_gtk.c,v 2.10 2008/12/28 04:31:33 baccala Exp $
 *
 * Copyright (C) 1996 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the GTK specific pieces of the display
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gtkdatabox.h>
#include <gtkdatabox_points.h>
#include <gtkdatabox_lines.h>
#include <math.h>

#include <string.h>
#include "oscope.h"		/* program defaults */
#include "display.h"
#include "func.h"
#include "file.h"
#include "com_gtk.h"

char my_filename[FILENAME_MAX] = "";
GdkFont *font;
char fontname[80] = DEF_FX;
char fonts[] = "xlsfonts";

GtkWidget *menubar;

GtkItemFactory *factory;
extern int fixing_widgets;	/* in com_gtk.c */

static gint
configure_event (GtkWidget *widget, GdkEventConfigure *event)
{
  static int h, v, once = 0;

  if (pixmap)
    gdk_pixmap_unref(pixmap);

  pixmap = gdk_pixmap_new(widget->window,
                          widget->allocation.width,
                          widget->allocation.height,
                          -1);
  ClearDrawArea();

  h = h_points;
  v = v_points;
  h_points = widget->allocation.width > 640
    ? widget->allocation.width - (widget->allocation.width - 200) % 44
    : 640;
  v_points = widget->allocation.height > 480
    ? widget->allocation.height - (widget->allocation.height - 160) % 64
    : 480;
  offset = v_points / 2;
  if (h != h_points || v != v_points)
    ClearDrawArea();
  if (once)
    draw_text();
  once = 1;
  return TRUE;
}

void
yes_sel(GtkWidget *w, GtkWidget *win)
{
  gtk_widget_destroy(GTK_WIDGET(win));
  savefile(my_filename);
}


void
loadfile_ok_sel(GtkWidget *w, GtkFileSelection *fs)
{
  strncpy(my_filename, gtk_file_selection_get_filename(fs), FILENAME_MAX);
  gtk_widget_destroy(GTK_WIDGET(fs));
  loadfile(my_filename);
}

void
savefile_ok_sel(GtkWidget *w, GtkFileSelection *fs)
{
  GtkWidget *window, *yes, *no, *label;
  struct stat buff;

  strncpy(my_filename, gtk_file_selection_get_filename(fs), FILENAME_MAX);
  gtk_widget_destroy(GTK_WIDGET(fs));

  if (!stat(my_filename, &buff)) {
    window = gtk_dialog_new();
    yes = gtk_button_new_with_label("Yes");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area), yes,
		       TRUE, TRUE, 0);
    no = gtk_button_new_with_label("No");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area), no,
		       TRUE, TRUE, 0);
    gtk_signal_connect_object(GTK_OBJECT(window), "delete_event",
			      GTK_SIGNAL_FUNC(gtk_widget_destroy),
			      GTK_OBJECT(window));
    gtk_signal_connect(GTK_OBJECT(yes), "clicked",
		       GTK_SIGNAL_FUNC(yes_sel),
		       GTK_OBJECT(window));
    gtk_signal_connect_object(GTK_OBJECT(no), "clicked",
			      GTK_SIGNAL_FUNC(gtk_widget_destroy),
			      GTK_OBJECT(window));
    gtk_widget_show(yes);
    gtk_widget_show(no);
    sprintf(error, "\n  Overwrite existing file %s?  \n", my_filename);
    label = gtk_label_new(error);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), label, TRUE, TRUE, 0);
    gtk_widget_show(label);
    gtk_widget_show(window);
    gtk_grab_add(window);
  } else
    savefile(my_filename);
}

/* get a file name for load (0) or save (1) */
void
LoadSaveFile(int save)
{
  GtkWidget *filew;

  filew = gtk_file_selection_new(save ? "Save File": "Load File");
  gtk_signal_connect_object(GTK_OBJECT(filew), "delete_event",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(filew));
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filew)->ok_button),
		     "clicked", save ? GTK_SIGNAL_FUNC(savefile_ok_sel)
		     : GTK_SIGNAL_FUNC(loadfile_ok_sel),
		     GTK_OBJECT(filew));
  gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(filew)
				       ->cancel_button), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(filew));
 if (my_filename[0] != '\0')
   gtk_file_selection_set_filename(GTK_FILE_SELECTION(filew),
				   (gchar *)my_filename);
  gtk_widget_show(filew);
}

void
run_sel(GtkWidget *w, GtkEntry *command)
{
  startcommand(gtk_entry_get_text(GTK_ENTRY(command)));
}

void
ExternCommand()
{
  GtkWidget *window, *label, *command, *run, *cancel;
  GList *glist = NULL;

  if (fixing_widgets) return;

  glist = g_list_append(glist, "xy");
  glist = g_list_append(glist, "ofreq");
  glist = g_list_append(glist, "operl 'abs($x)'");
  glist = g_list_append(glist, "operl '$x - $x[0]'");
  glist = g_list_append(glist, "operl '$x / ($y || 1)'");
  glist = g_list_append(glist, "operl '$x > $y ? $x : $y'");
  glist = g_list_append(glist, "operl '$t / (44100/2/250) % 2 ? 64 : -64'");
  glist = g_list_append(glist, "operl 'sin($t * $pi / (44100/2/250)) * 64'");
  glist = g_list_append(glist, "operl 'cos($t * $pi / (44100/2/250)) * 64'");

  window = gtk_dialog_new();
  run = gtk_button_new_with_label("  Run  ");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area), run,
		     TRUE, TRUE, 0);
  cancel = gtk_button_new_with_label("  Cancel  ");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area), cancel,
		     TRUE, TRUE, 0);
  label = gtk_label_new("\n  External command and args:  \n");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), label,
		     TRUE, TRUE, 0);
  command = gtk_combo_new();
  gtk_combo_set_popdown_strings(GTK_COMBO(command), glist);

  /* XXX recall previous command that was set here */
#if 0
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(command)->entry),
		     ch[scope.select].command);
#else
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(command)->entry),
		     "operl '$x + $y'");
#endif
  gtk_combo_set_value_in_list(GTK_COMBO(command), FALSE, FALSE);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), command,
		     TRUE, TRUE, 0);
  gtk_signal_connect_object(GTK_OBJECT(window), "delete_event",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(window));
  gtk_signal_connect(GTK_OBJECT(run), "clicked",
		     GTK_SIGNAL_FUNC(run_sel),
		     GTK_ENTRY(GTK_COMBO(command)->entry));
  gtk_signal_connect_object_after(GTK_OBJECT(run), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(window));
  gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(window));
  GTK_WIDGET_SET_FLAGS(run, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(run);
  gtk_widget_show(run);
  gtk_widget_show(cancel);
  gtk_widget_show(label);
  gtk_widget_show(command);
  gtk_widget_show(window);
  /*    gtk_grab_add(window); */
}

/* PolyPoint() and PolyLine()
 *
 * These functions are a bit special.  There are only used by code
 * drawing into the 'scope data display area (not the status area
 * surrounding it).  Therefore, we use special GCs that clip at the
 * edge of the data display area, because the code in draw_data() that
 * calls us here doesn't care about clipping, and we don't want our
 * signal lines scribbled all over our text.
 *
 * The GDK tutorial suggests doing all your graphic ops to a pixmap,
 * then copying the pixmap to the screen on an expose event.
 * Unfortunately, the pixmap copy is very slow on my X server (XFree86
 * 3.3.6 SVGA).  The display code doesn't work quite right with some
 * kind of pixmap; accumulate mode depends on having the pixmap to
 * keep a visual history of the signals.  So this functions write to
 * both the screen and an off-screen pixmap (the only code that does
 * this), in case we need to redraw the screen after an expose event.
 */

GtkWidget *databox;

void
PolyPoint(int color, Point *points, int count)
{
    GdkColor gcolor;
    GtkDataboxGraph *graph;
    gfloat *X = NULL;
    gfloat *Y = NULL;
    gint i;

    /* new widget code follows */
    /* XXX make sure we free the gfloat arrays at some point */

    X = g_new0(gfloat, count);
    Y = g_new0(gfloat, count);

    for (i = 0; i < count; i ++) {
	X[i] = points[i].x;
	Y[i] = points[i].y;
    }

    gcolor.red = 65535;
    gcolor.green = 65535;
    gcolor.blue = 0;

    graph = gtk_databox_points_new (count, X, Y, &gcolor, 1);
    gtk_databox_graph_add (GTK_DATABOX (databox), graph);
}

void
PolyLine(int color, Point *points, int count)
{
    GdkColor gcolor;
    GtkDataboxGraph *graph;
    gfloat *X = NULL;
    gfloat *Y = NULL;
    gint i;

    /* new widget code follows */
    /* XXX make sure we free the gfloat arrays at some point */

    X = g_new0(gfloat, count);
    Y = g_new0(gfloat, count);

    for (i = 0; i < count; i ++) {
	X[i] = points[i].x;
	Y[i] = points[i].y;
    }

    gcolor.red = 65535;
    gcolor.green = 65535;
    gcolor.blue = 0;

    graph = gtk_databox_lines_new (count, X, Y, &gcolor, 1);
    gtk_databox_graph_add (GTK_DATABOX (databox), graph);
}

/* callback to redisplay the drawing area; snap to a graticule division */
void
redisplay(GtkWidget w, int new_width, int new_height, void *data)
{
}

/* menu option callbacks */

void
datasource(GtkWidget *w, gpointer data)
{
  if (fixing_widgets) return;
  if (datasrc_byname(data)) {
    clear();
  }
}

void
option_dialog(GtkWidget *w, gpointer data)
{
  if (fixing_widgets) return;
  if (datasrc && datasrc->gtk_options) datasrc->gtk_options();
}

void
plotmode(GtkWidget *w, gpointer data)
{
  if (fixing_widgets) return;
  scope.mode = ((char *)data)[0] - '0';
  clear();
}

void
runmode(GtkWidget *w, gpointer data)
{
  scope.run = ((char *)data)[0] - '0';
  clear();
}

void
graticule(GtkWidget *w, gpointer data)
{
  int i;

  if (fixing_widgets) return;
  i = ((char *)data)[0] - '0';
  if (i < 2)
    scope.behind = i;
  else
    scope.grat = i - 2;
  clear();
}

void
change_trigger(int trigch, int trig, int trige)
{
  /* Triggering change.  Try the new trigger, and if it doesn't work,
   * try the old one settings again, if they don't work (!) leave
   * the trigger off.
   */

  if (trige == 0) {
    if (datasrc && datasrc->clear_trigger) datasrc->clear_trigger();
    scope.trige = 0;
  } else if (datasrc && datasrc->set_trigger
	     && datasrc->set_trigger(trigch, &trig, trige)) {
    scope.trigch = trigch;
    scope.trig = trig;
    scope.trige = trige;
  } else if (datasrc && datasrc->set_trigger && datasrc->clear_trigger
	     && !datasrc->set_trigger(scope.trigch, &scope.trig, scope.trige)){
    datasrc->clear_trigger();
    scope.trige = 0;
  }
}

void
trigger(GtkWidget *w, gpointer data)
{
  char c = ((char *)data)[0];

  if (fixing_widgets) return;

  if (c >= 'a' && c <= 'c') {
    change_trigger(scope.trigch, scope.trig, c - 'a');
  }

  if (c >= '1' && c <= '8') {
    change_trigger(c - '1', scope.trig, scope.trige);
  }

  clear();
}

/* Radio buttons cause 2 events: the deselect and the select.
   Selecting a built-in after external causes an extraneous command
   dialog but I can't figure out how to get rid of it.  */
void
mathselect(GtkWidget *w, gpointer data)
{
  if (fixing_widgets) return;
  if (scope.select > 1) {
    if (((char *)data)[0] == '$') {
/*        if (GTK_CHECK_MENU_ITEM */
/*  	 (gtk_item_factory_get_item */
/*  	  (factory, "/Channel/Math/External Command..."))->active) */
	handle_key('$');
    } else {
      function_bynum_on_channel(((char *)data)[0] - '0', &ch[scope.select]);
      ch[scope.select].show = 1;
    }
    clear();
  }
}

void
setbits(GtkWidget *w, gpointer data)
{
  if (fixing_widgets) return;
  ch[scope.select].bits = (int)data;
  clear();
}

void
setscale(GtkWidget *w, gpointer data)
{
  int scale[] = {0, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000};

  ch[scope.select].target_mult = scale[((char *)data)[0] - '0'];
  ch[scope.select].target_div = scale[((char *)data)[1] - '0'];
  clear();
}

void
setposition(GtkWidget *w, gpointer data)
{
  char c0 = ((char *)data)[0];
  char c1 = ((char *)data)[1];

  if (c0 == 'T') {
    change_trigger(scope.trigch, 256 - (c1 - 'a') * 8, scope.trige);
  } else if (c0 == 't') {
    change_trigger(scope.trigch, (c1 - 'a') * 8, scope.trige);
  } else {
    ch[scope.select].pos = (((char *)data)[0] == '-' ? 1 : -1)
      * (((char *)data)[1] - 'a') * 16;
  }
  clear();
}

/* slurp the manual page into a window */
void
help(GtkWidget *w, void *data)
{
  char c;
  char charbuffer[16];
  int charbuffer_len = 0;
  int running_tag = 0;
  FILE *p;

  GtkWidget *window;
  GtkWidget *box1;
  GtkWidget *box2;
  GtkWidget *button;
  GtkWidget *text;
  GtkWidget *scrolled_window;
  GtkTextBuffer *textbuffer;
  PangoFontDescription *font_desc;
  GtkTextIter iter, start;
  GtkTextTag *underline_tag, *bold_tag;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_usize (window, 640, 480);
  gtk_window_set_policy (GTK_WINDOW(window), TRUE, TRUE, FALSE);
  gtk_signal_connect_object(GTK_OBJECT (window), "destroy",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(window));
  gtk_window_set_title (GTK_WINDOW (window), "xoscope(1) man page");
  gtk_container_border_width (GTK_CONTAINER (window), 0);

  box1 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), box1);
  gtk_widget_show (box1);

  box2 = gtk_vbox_new (FALSE, 10);
  gtk_container_border_width (GTK_CONTAINER (box2), 10);
  gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);
  gtk_widget_show (box2);

  /* Create the GtkTextView widget */
  text = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_NONE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);

  /* Use a fixed width font throughout the widget */
  font_desc = pango_font_description_from_string ("Courier 10");
  gtk_widget_modify_font (text, font_desc);
  pango_font_description_free (font_desc);

  /* Add scrollbars (if needed) to the GtkTextView widget */
  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrolled_window), text);
  gtk_box_pack_start (GTK_BOX (box2), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show(scrolled_window);
  gtk_widget_show(text);


  /* Realizing a widget creates a window for it, ready to insert some text */
  gtk_widget_realize (text);

  textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));

  bold_tag = gtk_text_buffer_create_tag (textbuffer, NULL,
					 "weight", PANGO_WEIGHT_BOLD, NULL);  
  underline_tag = gtk_text_buffer_create_tag (textbuffer, NULL,
					      "underline", PANGO_UNDERLINE_SINGLE, NULL);  

  /* Now run 'man' and copy its output into the text buffer.  We use
   * an intermediate 'charbuffer' for two reasons: to handle
   * backspaces (for overstrikes or underlines) which get converted
   * into formatting tags, and to ensure that multibyte UTF-8
   * characters get inserted as a unit; otherwise GTK+ 2 complains.
   *
   * XXX this can hang the program if the 'man' runs slowly
   *
   * XXX we're counting on GNU man to handle the '-Tutf8' flag; other
   * versions of man might not do this.
   *
   * XXX the tags are attached individually to each character; it
   * would be better to attach them to entire words, as this could
   * conceivably affect formating, especially of underlines
   */

  gtk_text_buffer_get_end_iter(textbuffer, &iter);

  if ((p = popen(HELPCOMMAND, "r")) != NULL) {

    while ((c = fgetc(p)) != EOF) {
      if (c == '\b') {
	if (charbuffer[0] == '_') running_tag = 1;
	else running_tag = 2;
	charbuffer_len = 0;
	continue;
      }
      if (c < 0) {
	charbuffer[charbuffer_len ++] = c;
      } else {
	if (charbuffer_len > 0) {
	  gtk_text_buffer_insert(textbuffer, &iter,
				 charbuffer, charbuffer_len);
	  if (running_tag) {
	    start = iter;
	    gtk_text_iter_backward_char(&start);
	    if (running_tag == 1) {
	      gtk_text_buffer_apply_tag (textbuffer, underline_tag,
					 &start, &iter);
	    } else if (running_tag == 2) {
	      gtk_text_buffer_apply_tag (textbuffer, bold_tag,
					 &start, &iter);
	    }
	    running_tag = 0;
	  }
	}
	charbuffer[0] = c;
	charbuffer_len = 1;
      }
    }
    pclose(p);
  }

  box2 = gtk_vbox_new (FALSE, 10);
  gtk_container_border_width (GTK_CONTAINER (box2), 10);
  gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, TRUE, 0);
  gtk_widget_show (box2);

  button = gtk_button_new_with_label ("close");
  gtk_signal_connect_object(GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(window));
  gtk_box_pack_start (GTK_BOX (box2), button, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  gtk_widget_show (window);
}

void
cleanup_display()
{
}

/* WARNING: if the menu system is rearranged, the finditem-based logic
   in fix_widgets may need updated as well.  */

static GtkItemFactoryEntry menu_items[] =
{
  {"/File", NULL, NULL, 0, "<Branch>"},
  {"/File/tear", NULL, NULL, 0, "<Tearoff>"},
  /*     {"/File/New", "<control>N", print_hello, NULL, NULL}, */
  {"/File/Open...", NULL, hit_key, (int)"@", NULL},
  {"/File/Save...", NULL, hit_key, (int)"#", NULL},
  /*     {"/File/Save as", NULL, NULL, 0, NULL}, */
  {"/File/Device/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/File/Device Options...", NULL, option_dialog, 0, NULL},
  {"/File/sep", NULL, NULL, 0, "<Separator>"},
  {"/File/Quit", NULL, hit_key, (int)"\e", NULL},

  {"/Channel", NULL, NULL, 0, "<Branch>"},
  {"/Channel/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/Channel/Channel 1", NULL, hit_key, (int)"1", "<RadioItem>"},
  {"/Channel/Channel 2", NULL, hit_key, (int)"2", "/Channel/Channel 1"},
  {"/Channel/Channel 3", NULL, hit_key, (int)"3", "/Channel/Channel 2"},
  {"/Channel/Channel 4", NULL, hit_key, (int)"4", "/Channel/Channel 3"},
  {"/Channel/Channel 5", NULL, hit_key, (int)"5", "/Channel/Channel 4"},
  {"/Channel/Channel 6", NULL, hit_key, (int)"6", "/Channel/Channel 5"},
  {"/Channel/Channel 7", NULL, hit_key, (int)"7", "/Channel/Channel 6"},
  {"/Channel/Channel 8", NULL, hit_key, (int)"8", "/Channel/Channel 7"},
  {"/Channel/sep", NULL, NULL, 0, "<Separator>"},
  {"/Channel/Show", NULL, hit_key, (int)"\t", "<CheckItem>"},

  {"/Channel/Scale", NULL, NULL, 0, "<Branch>"},
  {"/Channel/Scale/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/Channel/Scale/up", NULL, hit_key, (int)"}", NULL},
  {"/Channel/Scale/down", NULL, hit_key, (int)"{", NULL},
  {"/Channel/Scale/sep", NULL, NULL, 0, "<Separator>"},
  {"/Channel/Scale/50", NULL, setscale, (int)"61", NULL},
  {"/Channel/Scale/20", NULL, setscale, (int)"51", NULL},
  {"/Channel/Scale/10", NULL, setscale, (int)"41", NULL},
  {"/Channel/Scale/5", NULL, setscale, (int)"31", NULL},
  {"/Channel/Scale/2", NULL, setscale, (int)"21", NULL},
  {"/Channel/Scale/1", NULL, setscale, (int)"11", NULL},
  /* How the ? do you put a / in a menu ? Just use \ until I figure it out. */
  {"/Channel/Scale/1\\2", NULL, setscale, (int)"12", NULL},
  {"/Channel/Scale/1\\5", NULL, setscale, (int)"13", NULL},
  {"/Channel/Scale/1\\10", NULL, setscale, (int)"14", NULL},
  {"/Channel/Scale/1\\20", NULL, setscale, (int)"15", NULL},
  {"/Channel/Scale/1\\50", NULL, setscale, (int)"16", NULL},

  {"/Channel/Position", NULL, NULL, 0, "<Branch>"},
  {"/Channel/Position/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/Channel/Position/up", "]", hit_key, (int)"]", NULL},
  {"/Channel/Position/down", "[", hit_key, (int)"[", NULL},
  {"/Channel/Position/sep", NULL, NULL, 0, "<Separator>"},
  {"/Channel/Position/160", NULL, setposition, (int)" k", NULL},
  {"/Channel/Position/144", NULL, setposition, (int)" j", NULL},
  {"/Channel/Position/128", NULL, setposition, (int)" i", NULL},
  {"/Channel/Position/112", NULL, setposition, (int)" h", NULL},
  {"/Channel/Position/96", NULL, setposition, (int)" g", NULL},
  {"/Channel/Position/80", NULL, setposition, (int)" f", NULL},
  {"/Channel/Position/64", NULL, setposition, (int)" e", NULL},
  {"/Channel/Position/48", NULL, setposition, (int)" d", NULL},
  {"/Channel/Position/32", NULL, setposition, (int)" c", NULL},
  {"/Channel/Position/16", NULL, setposition, (int)" b", NULL},
  {"/Channel/Position/0", NULL, setposition, (int)" a", NULL},
  {"/Channel/Position/-16", NULL, setposition, (int)"-b", NULL},
  {"/Channel/Position/-32", NULL, setposition, (int)"-c", NULL},
  {"/Channel/Position/-48", NULL, setposition, (int)"-d", NULL},
  {"/Channel/Position/-64", NULL, setposition, (int)"-e", NULL},
  {"/Channel/Position/-80", NULL, setposition, (int)"-f", NULL},
  {"/Channel/Position/-96", NULL, setposition, (int)"-g", NULL},
  {"/Channel/Position/-112", NULL, setposition, (int)"-h", NULL},
  {"/Channel/Position/-128", NULL, setposition, (int)"-i", NULL},
  {"/Channel/Position/-144", NULL, setposition, (int)"-j", NULL},
  {"/Channel/Position/-160", NULL, setposition, (int)"-k", NULL},

  {"/Channel/Bits", NULL, NULL, 0, "<Branch>"},
  {"/Channel/Bits/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/Channel/Bits/Analog", NULL, setbits, 0, "<RadioItem>"},
  {"/Channel/Bits/2", NULL, setbits, 2, "/Channel/Bits/Analog"},
  {"/Channel/Bits/4", NULL, setbits, 4, "/Channel/Bits/2"},
  {"/Channel/Bits/6", NULL, setbits, 6, "/Channel/Bits/4"},
  {"/Channel/Bits/8", NULL, setbits, 8, "/Channel/Bits/6"},
  {"/Channel/Bits/10", NULL, setbits, 10, "/Channel/Bits/8"},
  {"/Channel/Bits/12", NULL, setbits, 12, "/Channel/Bits/10"},
  {"/Channel/Bits/14", NULL, setbits, 14, "/Channel/Bits/12"},
  {"/Channel/Bits/16", NULL, setbits, 16, "/Channel/Bits/14"},

  {"/Channel/sep", NULL, NULL, 0, "<Separator>"},
  {"/Channel/Math", NULL, NULL, 0, "<Branch>"},
  {"/Channel/Math/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/Channel/Math/Prev Function", ":", hit_key, (int)":", NULL},
  {"/Channel/Math/Next Function", ";", hit_key, (int)";", NULL},
  {"/Channel/Math/sep", NULL, NULL, 0, "<Separator>"},

  /* this will need hacked if functions are added / changed in func.c */
  {"/Channel/Math/Other", NULL, NULL, 0, "<RadioItem>"},
  {"/Channel/Math/External Command...", "$", mathselect, (int)"$", "/Channel/Math/Other"},
  {"/Channel/Math/Inv. 1", NULL, mathselect, (int)"0", "/Channel/Math/External Command..."},
  {"/Channel/Math/Inv. 2", NULL, mathselect, (int)"1", "/Channel/Math/Inv. 1"},
  {"/Channel/Math/Sum  1+2", NULL, mathselect, (int)"2", "/Channel/Math/Inv. 2"},
  {"/Channel/Math/Diff 1-2", NULL, mathselect, (int)"3", "/Channel/Math/Sum  1+2"},
  {"/Channel/Math/Avg. 1,2", NULL, mathselect, (int)"4", "/Channel/Math/Diff 1-2"},
  {"/Channel/Math/FFT. 1", NULL, mathselect, (int)"5", "/Channel/Math/Avg. 1,2"},
  {"/Channel/Math/FFT. 2", NULL, mathselect, (int)"6", "/Channel/Math/FFT. 1"},

  {"/Channel/Store", NULL, NULL, 0, "<Branch>"},
  {"/Channel/Store/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/Channel/Store/Mem A", "A", hit_key, (int)"A", "<CheckItem>"},
  {"/Channel/Store/Mem B", "B", hit_key, (int)"B", "<CheckItem>"},
  {"/Channel/Store/Mem C", "C", hit_key, (int)"C", "<CheckItem>"},
  {"/Channel/Store/Mem D", "D", hit_key, (int)"D", "<CheckItem>"},
  {"/Channel/Store/Mem E", "E", hit_key, (int)"E", "<CheckItem>"},
  {"/Channel/Store/Mem F", "F", hit_key, (int)"F", "<CheckItem>"},
  {"/Channel/Store/Mem G", "G", hit_key, (int)"G", "<CheckItem>"},
  {"/Channel/Store/Mem H", "H", hit_key, (int)"H", "<CheckItem>"},
  {"/Channel/Store/Mem I", "I", hit_key, (int)"I", "<CheckItem>"},
  {"/Channel/Store/Mem J", "J", hit_key, (int)"J", "<CheckItem>"},
  {"/Channel/Store/Mem K", "K", hit_key, (int)"K", "<CheckItem>"},
  {"/Channel/Store/Mem L", "L", hit_key, (int)"L", "<CheckItem>"},
  {"/Channel/Store/Mem M", "M", hit_key, (int)"M", "<CheckItem>"},
  {"/Channel/Store/Mem N", "N", hit_key, (int)"N", "<CheckItem>"},
  {"/Channel/Store/Mem O", "O", hit_key, (int)"O", "<CheckItem>"},
  {"/Channel/Store/Mem P", "P", hit_key, (int)"P", "<CheckItem>"},
  {"/Channel/Store/Mem Q", "Q", hit_key, (int)"Q", "<CheckItem>"},
  {"/Channel/Store/Mem R", "R", hit_key, (int)"R", "<CheckItem>"},
  {"/Channel/Store/Mem S", "S", hit_key, (int)"S", "<CheckItem>"},
  {"/Channel/Store/Mem T", "T", hit_key, (int)"T", "<CheckItem>"},
  {"/Channel/Store/Mem U", "U", hit_key, (int)"U", "<CheckItem>"},
  {"/Channel/Store/Mem V", "V", hit_key, (int)"V", "<CheckItem>"},
  {"/Channel/Store/Mem W", "W", hit_key, (int)"W", "<CheckItem>"},
  {"/Channel/Store/Mem X", "X", hit_key, (int)"X", "<CheckItem>"},
  {"/Channel/Store/Mem Y", "Y", hit_key, (int)"Y", "<CheckItem>"},
  {"/Channel/Store/Mem Z", "Z", hit_key, (int)"Z", "<CheckItem>"},

  {"/Channel/Recall", NULL, NULL, 0, "<Branch>"},
  {"/Channel/Recall/tear", NULL, NULL, 0, "<Tearoff>"},
  //  {"/Channel/Recall/sep", NULL, NULL, 0, "<Separator>"},
  {"/Channel/Recall/Mem A", "a", hit_key, (int)"a", NULL},
  {"/Channel/Recall/Mem B", "b", hit_key, (int)"b", NULL},
  {"/Channel/Recall/Mem C", "c", hit_key, (int)"c", NULL},
  {"/Channel/Recall/Mem D", "d", hit_key, (int)"d", NULL},
  {"/Channel/Recall/Mem E", "e", hit_key, (int)"e", NULL},
  {"/Channel/Recall/Mem F", "f", hit_key, (int)"f", NULL},
  {"/Channel/Recall/Mem G", "g", hit_key, (int)"g", NULL},
  {"/Channel/Recall/Mem H", "h", hit_key, (int)"h", NULL},
  {"/Channel/Recall/Mem I", "i", hit_key, (int)"i", NULL},
  {"/Channel/Recall/Mem J", "j", hit_key, (int)"j", NULL},
  {"/Channel/Recall/Mem K", "k", hit_key, (int)"k", NULL},
  {"/Channel/Recall/Mem L", "l", hit_key, (int)"l", NULL},
  {"/Channel/Recall/Mem M", "m", hit_key, (int)"m", NULL},
  {"/Channel/Recall/Mem N", "n", hit_key, (int)"n", NULL},
  {"/Channel/Recall/Mem O", "o", hit_key, (int)"o", NULL},
  {"/Channel/Recall/Mem P", "p", hit_key, (int)"p", NULL},
  {"/Channel/Recall/Mem Q", "q", hit_key, (int)"q", NULL},
  {"/Channel/Recall/Mem R", "r", hit_key, (int)"r", NULL},
  {"/Channel/Recall/Mem S", "s", hit_key, (int)"s", NULL},
  {"/Channel/Recall/Mem T", "t", hit_key, (int)"t", NULL},
  {"/Channel/Recall/Mem U", "u", hit_key, (int)"u", NULL},
  {"/Channel/Recall/Mem V", "v", hit_key, (int)"v", NULL},
  {"/Channel/Recall/Mem W", "w", hit_key, (int)"w", NULL},
  {"/Channel/Recall/Mem X", "x", hit_key, (int)"x", NULL},
  {"/Channel/Recall/Mem Y", "y", hit_key, (int)"y", NULL},
  {"/Channel/Recall/Mem Z", "z", hit_key, (int)"z", NULL},

  {"/Trigger", NULL, NULL, 0, "<Branch>"},
  {"/Trigger/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/Trigger/Off", NULL, trigger, (int)"a", "<RadioItem>"},
  {"/Trigger/Rising", NULL, trigger, (int)"b", "/Trigger/Off"},
  {"/Trigger/Falling", NULL, trigger, (int)"c", "/Trigger/Rising"},
  {"/Trigger/sep", NULL, NULL, 0, "<Separator>"},
  {"/Trigger/Channel 1", NULL, trigger, (int)"1", "<RadioItem>"},
  {"/Trigger/Channel 2", NULL, trigger, (int)"2", "/Trigger/Channel 1"},
  {"/Trigger/Channel 3", NULL, trigger, (int)"3", "/Trigger/Channel 1"},
  {"/Trigger/Channel 4", NULL, trigger, (int)"4", "/Trigger/Channel 1"},
  {"/Trigger/Channel 5", NULL, trigger, (int)"5", "/Trigger/Channel 1"},
  {"/Trigger/Channel 6", NULL, trigger, (int)"6", "/Trigger/Channel 1"},
  {"/Trigger/Channel 7", NULL, trigger, (int)"7", "/Trigger/Channel 1"},
  {"/Trigger/Channel 8", NULL, trigger, (int)"8", "/Trigger/Channel 1"},
  {"/Trigger/sep", NULL, NULL, 0, "<Separator>"},
  {"/Trigger/Position up", "=", hit_key, (int)"=", NULL},
  {"/Trigger/Position down", "-", hit_key, (int)"-", NULL},
  {"/Trigger/Position Positive", NULL, NULL, 0, "<Branch>"},
  {"/Trigger/Position Positive/120", NULL, setposition, (int)"Tb", NULL},
  {"/Trigger/Position Positive/112", NULL, setposition, (int)"Tc", NULL},
  {"/Trigger/Position Positive/104", NULL, setposition, (int)"Td", NULL},
  {"/Trigger/Position Positive/96", NULL, setposition, (int)"Te", NULL},
  {"/Trigger/Position Positive/88", NULL, setposition, (int)"Tf", NULL},
  {"/Trigger/Position Positive/80", NULL, setposition, (int)"Tg", NULL},
  {"/Trigger/Position Positive/72", NULL, setposition, (int)"Th", NULL},
  {"/Trigger/Position Positive/64", NULL, setposition, (int)"Ti", NULL},
  {"/Trigger/Position Positive/56", NULL, setposition, (int)"Tj", NULL},
  {"/Trigger/Position Positive/48", NULL, setposition, (int)"Tk", NULL},
  {"/Trigger/Position Positive/40", NULL, setposition, (int)"Tl", NULL},
  {"/Trigger/Position Positive/32", NULL, setposition, (int)"Tm", NULL},
  {"/Trigger/Position Positive/24", NULL, setposition, (int)"Tn", NULL},
  {"/Trigger/Position Positive/16", NULL, setposition, (int)"To", NULL},
  {"/Trigger/Position Positive/8", NULL, setposition, (int)"Tp", NULL},
  {"/Trigger/Position Positive/0", NULL, setposition, (int)"Tq", NULL},
  {"/Trigger/Position Negative", NULL, NULL, 0, "<Branch>"},
  {"/Trigger/Position Negative/0", NULL, setposition, (int)"tq", NULL},
  {"/Trigger/Position Negative/-8", NULL, setposition, (int)"tp", NULL},
  {"/Trigger/Position Negative/-16", NULL, setposition, (int)"to", NULL},
  {"/Trigger/Position Negative/-24", NULL, setposition, (int)"tn", NULL},
  {"/Trigger/Position Negative/-32", NULL, setposition, (int)"tm", NULL},
  {"/Trigger/Position Negative/-40", NULL, setposition, (int)"tl", NULL},
  {"/Trigger/Position Negative/-48", NULL, setposition, (int)"tk", NULL},
  {"/Trigger/Position Negative/-56", NULL, setposition, (int)"tj", NULL},
  {"/Trigger/Position Negative/-64", NULL, setposition, (int)"ti", NULL},
  {"/Trigger/Position Negative/-72", NULL, setposition, (int)"th", NULL},
  {"/Trigger/Position Negative/-80", NULL, setposition, (int)"tg", NULL},
  {"/Trigger/Position Negative/-88", NULL, setposition, (int)"tf", NULL},
  {"/Trigger/Position Negative/-96", NULL, setposition, (int)"te", NULL},
  {"/Trigger/Position Negative/-104", NULL, setposition, (int)"td", NULL},
  {"/Trigger/Position Negative/-112", NULL, setposition, (int)"tc", NULL},
  {"/Trigger/Position Negative/-120", NULL, setposition, (int)"tb", NULL},
  {"/Trigger/Position Negative/-128", NULL, setposition, (int)"ta", NULL},

  {"/Scope", NULL, NULL, 0, "<Branch>"},
  {"/Scope/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/Scope/Refresh", NULL, hit_key, (int)"\n", NULL},
  {"/Scope/Plot Mode/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/Scope/Plot Mode/Point", NULL, plotmode, (int)"0", "<RadioItem>"},
  {"/Scope/Plot Mode/Point Accumulate", NULL, plotmode, (int)"1", "/Scope/Plot Mode/Point"},
  {"/Scope/Plot Mode/Line", NULL, plotmode, (int)"2", "/Scope/Plot Mode/Point Accumulate"},
  {"/Scope/Plot Mode/Line Accumulate", NULL, plotmode, (int)"3", "/Scope/Plot Mode/Line"},
  {"/Scope/Plot Mode/Step", NULL, plotmode, (int)"4", "/Scope/Plot Mode/Line Accumulate"},
  {"/Scope/Plot Mode/Step Accumulate", NULL, plotmode, (int)"5", "/Scope/Plot Mode/Step"},
  {"/Scope/Graticule/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/Scope/Graticule/In Front", NULL, graticule, (int)"0", "<RadioItem>"},
  {"/Scope/Graticule/Behind", NULL, graticule, (int)"1", "/Scope/Graticule/In Front"},
  {"/Scope/Graticule/sep", NULL, NULL, 0, "<Separator>"},
  {"/Scope/Graticule/None", NULL, graticule, (int)"2", "<RadioItem>"},
  {"/Scope/Graticule/Minor Divisions", NULL, graticule, (int)"3", "/Scope/Graticule/None"},
  {"/Scope/Graticule/Minor & Major", NULL, graticule, (int)"4", "/Scope/Graticule/Minor Divisions"},
  {"/Scope/Cursors", NULL, hit_key, (int)"'", "<CheckItem>"},

  {"/<<", NULL, hit_key, (int)"9", NULL},
  {"/<", NULL, hit_key, (int)"(", NULL},
  {"/>", NULL, hit_key, (int)")", NULL},
  {"/>> ", NULL, hit_key, (int)"0", NULL},

  {"/Run", NULL, runmode, (int)"1", NULL},
  {"/Wait", NULL, runmode, (int)"2", NULL},
  {"/Stop", NULL, runmode, (int)"0", NULL},

  {"/Help", NULL, NULL, 0, "<LastBranch>"},
  {"/Help/Keys&Info", NULL, hit_key, (int)"?", "<CheckItem>"},
  {"/Help/Manual", NULL, help, 0, NULL},

};
gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

/* return the first menu item containing str, or NULL if none */
GtkItemFactoryEntry *
finditem(char *str)
{
  int i;

  for (i = 0; i < nmenu_items; i++) {
    if (strstr(menu_items[i].path, str))
      return &menu_items[i];
  }
  return NULL;
}

/* set current state colors, labels, and check marks on widgets */
void
fix_widgets()
{
  GtkItemFactoryEntry *p, *q, *r;
  int i;

  fixing_widgets = 1;
  if ((p = finditem("/Channel/Channel 1"))) {
    p += scope.select;
    gtk_check_menu_item_set_active
      (GTK_CHECK_MENU_ITEM
       (gtk_item_factory_get_item(factory, p->path)), TRUE);
  }
  if ((p = finditem("/Channel/Bits/Analog"))) {
    p += ch[scope.select].bits / 2;
    gtk_check_menu_item_set_active
      (GTK_CHECK_MENU_ITEM
       (gtk_item_factory_get_item(factory, p->path)), TRUE);
  }
  gtk_check_menu_item_set_active
    (GTK_CHECK_MENU_ITEM
     (gtk_item_factory_get_item(factory, "/Channel/Show")),
     ch[scope.select].show);

  if ((p = finditem("/File/Device Options..."))) {
    gtk_widget_set_sensitive
      (GTK_WIDGET
       (gtk_item_factory_get_item(factory, p->path)),
       datasrc && datasrc->gtk_options != NULL);
  }

  if ((p = finditem("/Trigger/Off"))) {
    q = p + scope.trige;
    gtk_check_menu_item_set_active
      (GTK_CHECK_MENU_ITEM
       (gtk_item_factory_get_item(factory, q->path)), TRUE);
  }

  /* The trigger channels.  There are eight of them defined, but we
   * only show as many as datasrc->nchans() indicate are available.
   * (We assume datasrc->nchans() is <= 8).  Set their labels to the
   * names in the Signal arrays; make them all insensitive if
   * triggering is turned off, and select the one corresponding to the
   * triggered channel.
   */

  if ((p = finditem("/Trigger/Channel 1"))) {
    for (i=0; i<8; i++) {
      GtkWidget *widget;
      GtkLabel *label;

      q = p + i;
      widget = GTK_WIDGET(gtk_item_factory_get_item(factory, q->path));

      if (!datasrc || i >= datasrc->nchans()) {
	gtk_widget_hide(widget);
      } else {
	gtk_widget_show(widget);
	label = GTK_LABEL (GTK_BIN (widget)->child);
	gtk_label_set_text(label, datasrc->chan(i)->name);
	gtk_widget_set_sensitive(widget, scope.trige != 0);
      }
    }
    q = p + scope.trigch;
    gtk_check_menu_item_set_active
      (GTK_CHECK_MENU_ITEM
       (gtk_item_factory_get_item(factory, q->path)), TRUE);
  }

  /* The triggering options should only be sensitive if we have a
   * data source and it defines a set_trigger function.
   */

  if ((p = finditem("/Trigger/Rising"))) {
    gtk_widget_set_sensitive
      (GTK_WIDGET(gtk_item_factory_get_item(factory, p->path)),
       datasrc && datasrc->set_trigger != NULL);
  }
  if ((p = finditem("/Trigger/Falling"))) {
    gtk_widget_set_sensitive
      (GTK_WIDGET(gtk_item_factory_get_item(factory, p->path)),
       datasrc && datasrc->set_trigger != NULL);
  }

  /* The remaining items on the trigger menu should only be
   * sensitive if triggering is turned on
   */

  if ((p = finditem("/Trigger/Position up"))) {
    gtk_widget_set_sensitive
      (GTK_WIDGET(gtk_item_factory_get_item(factory, p->path)),
       scope.trige != 0);
  }
  if ((p = finditem("/Trigger/Position down"))) {
    gtk_widget_set_sensitive
      (GTK_WIDGET(gtk_item_factory_get_item(factory, p->path)),
       scope.trige != 0);
  }
  if ((p = finditem("/Trigger/Position Positive"))) {
    gtk_widget_set_sensitive
      (GTK_WIDGET(gtk_item_factory_get_item(factory, p->path)),
       scope.trige != 0);
  }
  if ((p = finditem("/Trigger/Position Negative"))) {
    gtk_widget_set_sensitive
      (GTK_WIDGET(gtk_item_factory_get_item(factory, p->path)),
       scope.trige != 0);
  }

  if ((p = finditem("/Scope/Plot Mode/Point"))) {
    p += scope.mode;
    gtk_check_menu_item_set_active
      (GTK_CHECK_MENU_ITEM
       (gtk_item_factory_get_item(factory, p->path)), TRUE);
  }
  if ((p = finditem("/Scope/Graticule/In Front"))) {
    q = p + scope.behind;
    gtk_check_menu_item_set_active
      (GTK_CHECK_MENU_ITEM
       (gtk_item_factory_get_item(factory, q->path)), TRUE);
    q = p + scope.grat + 3;
    gtk_check_menu_item_set_active
      (GTK_CHECK_MENU_ITEM
       (gtk_item_factory_get_item(factory, q->path)), TRUE);
  }
  gtk_check_menu_item_set_active
    (GTK_CHECK_MENU_ITEM
     (gtk_item_factory_get_item(factory, "/Scope/Cursors")), scope.curs);

  gtk_check_menu_item_set_active
    (GTK_CHECK_MENU_ITEM
     (gtk_item_factory_get_item(factory, "/Help/Keys&Info")), scope.verbose);

  if ((p = finditem("/Channel/Math")) &&
      (q = finditem("/Channel/Math/FFT. 2"))) {
    for (r = p; r <= q; r++) {
      /* XXX add a check to the function's isvalid() test and a better
       * way to figure out which (if any) function is active
       */
      gtk_widget_set_sensitive
	(GTK_WIDGET(gtk_item_factory_get_item(factory, r->path)),
	 scope.select > 1);

      /* Set 'other function' active when we hit it... */
      if (r == p + 5)
	gtk_check_menu_item_set_active
	  (GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(factory, r->path)),
	   TRUE);

      /* ...then look for function that actually is active (if it exists) */
      if (ch[scope.select].signal && (r > p + 5))
	gtk_check_menu_item_set_active
	  (GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(factory, r->path)),
	   ch[scope.select].signal->savestr[0] == '0' + r - p - 7);
    }
  }

  /* Now the store and recall channels - set the names for memory or
   * data source, and mark the channels active/sensitive if memory
   * is stored there.  Store menu only displays options for the memory
   * channels that are actually 'visible', i.e, not obscured by
   * data source channels.
   */

  if ((p = finditem("/Channel/Store/Mem A")) &&
      (q = finditem("/Channel/Recall/Mem A")))
    for (i = 0; i < 26; i++) {
      GtkWidget *widget;
      GtkLabel *label;
      char buf[32];

      r = p + i;
      if (datasrc && i < datasrc->nchans()) {
	gtk_widget_hide(GTK_WIDGET(gtk_item_factory_get_item(factory,
							     r->path)));
      } else {
	gtk_widget_show(GTK_WIDGET(gtk_item_factory_get_item(factory,
							     r->path)));
	gtk_check_menu_item_set_active
	  (GTK_CHECK_MENU_ITEM
	   (gtk_item_factory_get_item(factory, r->path)),
	   mem[i].num > 0);
      }
      r += (q - p);

      widget = GTK_WIDGET(gtk_item_factory_get_item(factory, r->path));
      label = GTK_LABEL (GTK_BIN (widget)->child);
      if (datasrc && i < datasrc->nchans()) {
	gtk_label_set_text(label, datasrc->chan(i)->name);
	gtk_widget_set_sensitive(widget, TRUE);
      } else {
	sprintf(buf, "Mem %c", 'A' + i);
	gtk_label_set_text(label, buf);
	gtk_widget_set_sensitive(widget, mem[i].num > 0);
      }
    }

#if 0
  if ((p = finditem("/Channel/Recall")) &&
      (q = finditem("/Channel/Recall/sep"))) {
    GtkWidget *widget;
    widget = gtk_hseparator_new();
    gtk_menu_append(GTK_MENU(gtk_item_factory_get_widget(factory,
							 p->path)),
		    widget);
    gtk_widget_show(widget);

  }
#endif

#if 0
  {
    GtkWidget *widget;
    GtkWidget *radioitem;
    GSList *group;

    radioitem = GTK_WIDGET(gtk_item_factory_get_item(factory, "/Trigger/Channel 1"));
    group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(radioitem));

    widget = gtk_radio_menu_item_new_with_label(group, "Hi!");
    gtk_menu_insert(GTK_MENU(gtk_item_factory_get_widget(factory,
						       "/Trigger")),
		    widget, 3);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), FALSE);
    gtk_widget_show(widget);
  }
#endif

  fixing_widgets = 0;
}

void
get_main_menu(GtkWidget *window, GtkWidget ** menubar)
{

  GtkWidget *menu;

/*    GtkAccelGroup *accel_group; */

/*    accel_group = gtk_accel_group_new (); */

  factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", NULL);
  gtk_item_factory_create_items(factory, nmenu_items, menu_items, NULL);

/*        gtk_accel_group_attach (accel_group, GTK_OBJECT (window)); */

  /* Dynamically add the device list to the File/Device menu item */

  menu = gtk_item_factory_get_widget(factory, "/File/Device");
  if (menu) {
    GtkWidget *p;
    int i;

    for (i = 0; i < ndatasrcs; i++) {
      p = gtk_menu_item_new_with_label (datasrcs[i]->name);
      gtk_menu_append (GTK_MENU (menu), p);
      gtk_signal_connect (GTK_OBJECT (p), "activate",
			  GTK_SIGNAL_FUNC (datasource),
			  (gpointer) (datasrcs[i]->name));
      gtk_widget_show (p);
    }
  }

#if 0
  menu = gtk_item_factory_get_widget(factory, "/Trigger");
  if (menu) {
    GtkWidget *p;

    p = gtk_menu_item_new_with_label("Mem Brent");
    gtk_menu_append (GTK_MENU (menu), p);
    gtk_widget_show(p);
  }
#endif

  if (menubar)
    *menubar = gtk_item_factory_get_widget(factory, "<main>");
}

/* these are the inverse of row and col in display.c, used by button_event */
int
buttoncol(int x)
{
  if (x < 100)
    return (x / 8);
  if (x > h_points - 100)
    return (79 - (h_points - x) / 8);
  return (10 * (x - 100) / (10 * (h_points - 200) / 55) + 12);
}

int
buttonrow(int y)
{
  if (y <= 80)
    return (y / 16);
  if (y >= v_points - 80)
    return (29 - (v_points - y) / 16);
  return (10 * (y - 80) / (10 * (v_points - 160) / 20) + 5);
}

gint
positioncursor(int x, int y, int b)
{
  static int z;

  if (x > 100 && x < h_points - 100 && y > 80 && y < v_points - 80
      && ch[scope.select].signal != NULL) {
    z = ((float)x - 100) * 100 * ch[scope.select].signal->rate * scope.div
      / scope.scale / 440 / 10000 + 1;
    if (b == 1) {
      scope.cursa = z;
      return TRUE;
    }
    if (b == 2) {
      scope.cursb = z;
      return TRUE;
    }
  }
  return FALSE;
}

gboolean
on_databox_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  gfloat num, l, x;
  int cursor;
  Channel *p = &ch[scope.select];

  /* XXX duplicates code in draw_data() */
  if (p->signal->rate > 0) {
    num = (gfloat) 1 / p->signal->rate;
  } else {
    num = (gfloat) 1 / 1000;
  }
  l = p->signal->delay * num / 10000;

  if (scope.curs) {
#if 1
    GtkDataboxCoord coord;
    GtkDataboxValue value;
    coord.x = event->x;
    coord.y = event->y;
    value = gtk_databox_value_from_coord (databox, coord);
    x = value.x;
#else
    x = gtk_databox_pixel_to_value_x (databox, event->x);
#endif
    cursor = rintf((x - l) / num) + 1;
#if 0
    if (event->state & GDK_BUTTON1_MASK) {
      scope.cursa = cursor;
    } else if (event->state & GDK_BUTTON2_MASK) {
      scope.cursb = cursor;
    }
#else
    if (event->button == 1) {
      scope.cursa = cursor;
    } else if (event->button == 3) {
      scope.cursb = cursor;
    }
#endif
  }

  return FALSE;
}

/* draggable cursor positioning */
gint
motion_event (GtkWidget *widget, GdkEventMotion *event)
{
  static int x, y;
  GdkModifierType state;

  if (event->is_hint)
    gdk_window_get_pointer (event->window, &x, &y, &state);
  else {
    x = event->x;
    y = event->y;
    state = event->state;
  }
  if (state & GDK_BUTTON1_MASK)
    return positioncursor(x, y, 1);
  if (state & GDK_BUTTON2_MASK)
    return positioncursor(x, y, 2);
  return TRUE;
}

/* context sensitive mouse click select, recall and pop-up menus */
gint
button_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  static int x, y, b;

  x = event->x;
  y = event->y;
  b = event->button;
  if (positioncursor(x, y, b))
    return TRUE;

  x = buttoncol(event->x);	/* convert graphic to text position */
  y = buttonrow(event->y);
  /*    printf("button: %d @ %f,%f -> %d,%d\n", b, event->x, event->y, x, y); */
  if (!y && x > 70)
    handle_key('?');
  else if (y == 28 && x >= 27 && x <= 53) {
    if (b > 1)
      gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
			      (factory, "/Channel/Store")),
		     NULL, NULL, NULL, NULL, event->button, event->time);
    else
      handle_key((x - 27) + 'a');
  } else if (y < 4) {
    if (b == 1) handle_key('-');
    else if (b == 2) handle_key('=');
    else gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
				 (factory, "/Trigger")),
			NULL, NULL, NULL, NULL, event->button, event->time);
  } else if (y == 4) {
    if (b < 3) handle_key('!');
    else gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
				 (factory, "/Scope/Plot Mode")),
			NULL, NULL, NULL, NULL, event->button, event->time);
  }  else if (y < 25 && (x < 11 || x > 69)) {
    handle_key((y - 5) / 5 + '1' + (x > 69 ? 4 : 0));
    if (!((y - 1) % 5)) {
      if (b == 1) handle_key('{');
      else if (b == 2) handle_key('}');
      else gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
				   (factory, "/Channel/Scale")),
			  NULL, NULL, NULL, NULL, event->button, event->time);
    } else if (!((y - 2) % 5)) {
      if (x < 4 || (x > 69 && x < 74)) {
	if (b == 1) handle_key('`');
	else if (b == 2) handle_key('~');
	else gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
				     (factory, "/Channel/Bits")),
			    NULL, NULL, NULL, NULL, event->button, event->time);
      } else {
	if (b == 1) handle_key('[');
	else if (b == 2) handle_key(']');
	else gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
				     (factory, "/Channel/Position")),
			    NULL, NULL, NULL, NULL, event->button, event->time);
      }
    } else if (b > 1)
      gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
			      (factory, "/Channel")),
		     NULL, NULL, NULL, NULL, event->button, event->time);
  } else if (b > 1)
    gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
			    (factory, "/Scope")),
		   NULL, NULL, NULL, NULL, event->button, event->time);
  return TRUE;
}

GtkWidget *
create_databox (void)
{
   /* This is a global var - our one, unique, databox */
   databox = gtk_databox_new();
                                      
   gtk_databox_set_enable_zoom(GTK_DATABOX(databox), FALSE);
   gtk_databox_set_enable_selection(GTK_DATABOX(databox), FALSE);

   return databox;
}

GtkWidget * create_main_window();

/* initialize all the widgets, called by init_screen in display.c */
void
init_widgets()
{
  GdkColor gcolor;
  int i;

  /* XXX need to remove all dependencies on this */
  h_points = 640;
  v_points = 480;

  gtk_rc_parse("xoscope.rc");

  glade_window = create_main_window();

  setup_help_text(glade_window);

#if 0
  gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		     GTK_SIGNAL_FUNC(delete_event), NULL);
#endif

  get_main_menu(glade_window, &menubar);
  gtk_box_pack_start(GTK_BOX(LU("vbox1")), menubar, FALSE, TRUE, 0);
  gtk_box_reorder_child(GTK_BOX(LU("vbox1")), menubar, 0);
  gtk_widget_show(menubar);

  gtk_databox_set_hadjustment (GTK_DATABOX (databox),
			       gtk_range_get_adjustment (GTK_RANGE (LU("databox_hscrollbar"))));

  gtk_widget_show(glade_window);

#if 0
  gtk_signal_connect(GTK_OBJECT(drawing_area), "motion_notify_event",
		     GTK_SIGNAL_FUNC(motion_event), NULL);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "button_press_event",
		     GTK_SIGNAL_FUNC(button_event), NULL);
  gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK
			 | GDK_LEAVE_NOTIFY_MASK
			 | GDK_BUTTON_PRESS_MASK
			 | GDK_POINTER_MOTION_MASK
			 | GDK_POINTER_MOTION_HINT_MASK);
#endif

}

void inputCallback(gpointer data, gint source, GdkInputCondition condition)
{
  animate(NULL);
}

static int input_fd = -1;
static int input_tag_valid = 0;
static gint input_tag;

static int timeout_tag_valid = 0;
static gint timeout_tag;

/* GTK documentation says to return 0 if you don't want your timeout
 * function to be called again, or 1 if you do.  In our case,
 * animate() will call show_data(), which will call settimeout(),
 * which will remove the old timeout and possibly set a new one, and
 * all before the callback returns.  It's a little unclear to me what
 * the return value should be in this case (or if it matters)...
 */

gint timeout_callback(gpointer data)
{
  animate(NULL);
  return 0;
}

void settimeout(int ms)
{
  if (timeout_tag_valid) {
    gtk_timeout_remove(timeout_tag);
    timeout_tag_valid = 0;
  }

  if (ms == 0) return;

  timeout_tag = gtk_timeout_add(ms, timeout_callback, 0);
  timeout_tag_valid = 1;
}

void setinputfd(int fd)
{
  if (input_fd != fd) {

    if (input_tag_valid) {
      gdk_input_remove(input_tag);
      input_tag_valid = 0;
    }

    if (fd != -1) {
      input_tag = gdk_input_add(fd, GDK_INPUT_READ, inputCallback, NULL);
      input_tag_valid = 1;
    }

    input_fd = fd;
  }
}

/* loop until finished */
void
mainloop()
{
  draw_text(1);
  animate(NULL);
  gtk_main();
}

/*
 * @(#)$Id: gr_gtk.c,v 1.20 2000/07/03 18:18:14 twitham Exp $
 *
 * Copyright (C) 1996 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the GTK specific pieces of the display
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <string.h>
#include "oscope.h"		/* program defaults */
#include "display.h"
#include "func.h"
#include "file.h"
#include "proscope.h"
#include "bitscope.h"
#include "com_gtk.h"

int XX[] = {640,800,1024,1280};
int XY[] = {480,600, 768,1024};
char my_filename[FILENAME_MAX] = "";
GdkFont *font;
char fontname[80] = DEF_FX;
char fonts[] = "xlsfonts";

GtkItemFactory *factory;
extern int fixing_widgets;	/* in com_gtk.c */

void
clear_display()
{
  ClearDrawArea();
}

/* Create a new backing pixmap of the appropriate size */
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
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(command)->entry),
		     ch[scope.select].command);
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

/* a GTK text writer similar to libvgamisc's vga_write */
int
vga_write(char *s, short x, short y, void *f, short fg, short bg, char p)
{
  static int w;

  SetColor(fg);
  w = gdk_string_width(f, s);
  if (p == ALIGN_CENTER)
    x -= w / 2;
  else if (p == ALIGN_RIGHT)
    x -= w;
  gdk_draw_rectangle(pixmap,
		     drawing_area->style->black_gc,
		     TRUE,
		     x, y + 2, w, 16);

  gdk_draw_string(pixmap, font, gc, x, y + 16, s);
  return(1);
}

/* callback to redisplay the drawing area; snap to a graticule division */
void
redisplay(GtkWidget w, int new_width, int new_height, void *data)
{
}

/* menu option callbacks */
void
plotmode(GtkWidget *w, gpointer data)
{
  if (fixing_widgets) return;
  scope.mode = ((char *)data)[0] - '0';
  clear();
}

void
dma(GtkWidget *w, gpointer data)
{
  if (fixing_widgets) return;
  scope.dma = ((char *)data)[0] - '0';
  if (snd)
    resetsoundcard(scope.rate);
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
setcolor(GtkWidget *w, gpointer data)
{
  scope.color = ((char *)data)[0] - 'a';
  draw_text(1);
}

void
trigger(GtkWidget *w, gpointer data)
{
  int i;

  if (fixing_widgets) return;
  if ((i = ((char *)data)[0] - '0') > 2)
    scope.trigch = i - 3;
  else
    scope.trige = i;
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
    } else
      ch[scope.select].func = ((char *)data)[0] - '0' + FUNC0;
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

  ch[scope.select].mult = scale[((char *)data)[0] - '0'];
  ch[scope.select].div = scale[((char *)data)[1] - '0'];
  clear();
}

void
setposition(GtkWidget *w, gpointer data)
{
  if (((char *)data)[0] == 'T') {
    scope.trig = 256 - (((char *)data)[1] - 'a') * 8;
  } else if (((char *)data)[0] == 't') {
    scope.trig = (((char *)data)[1] - 'a') * 8;
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
  char c, prev = '\0', pprev = '\0';
  FILE *p;
  int i;

  GtkWidget *window;
  GtkWidget *box1;
  GtkWidget *box2;
  GtkWidget *button;
  GtkWidget *table;
  GtkWidget *vscrollbar;
  GtkWidget *text;
  GdkFont *fixed_font;

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

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
  gtk_box_pack_start (GTK_BOX (box2), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  /* Create the GtkText widget */
  text = gtk_text_new(NULL, NULL);
  gtk_text_set_editable(GTK_TEXT(text), FALSE);
  gtk_text_set_word_wrap(GTK_TEXT(text), TRUE);
  gtk_table_attach(GTK_TABLE(table), text, 0, 1, 0, 1,
		   GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		   GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show(text);

  /* Add a vertical scrollbar to the GtkText widget */
  vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
  gtk_table_attach (GTK_TABLE (table), vscrollbar, 1, 2, 0, 1,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (vscrollbar);

  /* Load a fixed font */
  fixed_font = gdk_font_load ("-misc-fixed-medium-r-*-*-*-120-*-*-*-*-*-*");

  /* Realizing a widget creates a window for it, ready to insert some text */
  gtk_widget_realize (text);

  /* Freeze the text widget, ready for multiple updates */
  gtk_text_freeze (GTK_TEXT (text));

  if ((p = popen(HELPCOMMAND, "r")) != NULL) {
    while ((c = fgetc(p)) != EOF) {
      i = 0;
      if (c == '\b') {
	c = fgetc(p);
	i = prev == '_' ? 1 : 4;
	gtk_text_backward_delete(GTK_TEXT(text), 1);
      }
      if (!(c == '\n' && prev == '\n' && pprev == '\n'))
	gtk_text_insert(GTK_TEXT(text), fixed_font, &gdkcolor[i],
			NULL, &c, 1);
      pprev = prev;
      prev = c;
    }
    pclose(p);
  }

  /* Thaw the text widget, allowing the updates to become visible */
  gtk_text_thaw (GTK_TEXT (text));

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

  {"/Channel/Recall", NULL, NULL, 0, "<Branch>"},
  {"/Channel/Recall/tear", NULL, NULL, 0, "<Tearoff>"},
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
  {"/Channel/Recall/sep", NULL, NULL, 0, "<Separator>"},
  {"/Channel/Recall/Left Mix", "x", hit_key, (int)"x", NULL},
  {"/Channel/Recall/Right Mix", "y", hit_key, (int)"y", NULL},
  {"/Channel/Recall/Serial Scope", "z", hit_key, (int)"z", NULL},

  {"/Trigger", NULL, NULL, 0, "<Branch>"},
  {"/Trigger/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/Trigger/Channel 1", NULL, trigger, (int)"3", "<RadioItem>"},
  {"/Trigger/Channel 2", NULL, trigger, (int)"4", "/Trigger/Channel 1"},
  {"/Trigger/sep", NULL, NULL, 0, "<Separator>"},
  {"/Trigger/Auto", NULL, trigger, (int)"0", "<RadioItem>"},
  {"/Trigger/Rising", NULL, trigger, (int)"1", "/Trigger/Auto"},
  {"/Trigger/Falling", NULL, trigger, (int)"2", "/Trigger/Rising"},
  {"/Trigger/sep", NULL, NULL, 0, "<Separator>"},
  {"/Trigger/Position up", "=", hit_key, (int)"=", NULL},
  {"/Trigger/Position down", "-", hit_key, (int)"-", NULL},
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
  {"/Scope/Graticule/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/Scope/Graticule/Color/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/Scope/Graticule/Color/black", NULL, setcolor, (int)"a", NULL},
  {"/Scope/Graticule/Color/blue", NULL, setcolor, (int)"b", NULL},
  {"/Scope/Graticule/Color/green", NULL, setcolor, (int)"c", NULL},
  {"/Scope/Graticule/Color/cyan", NULL, setcolor, (int)"d", NULL},
  {"/Scope/Graticule/Color/red", NULL, setcolor, (int)"e", NULL},
  {"/Scope/Graticule/Color/magenta", NULL, setcolor, (int)"f", NULL},
  {"/Scope/Graticule/Color/orange", NULL, setcolor, (int)"g", NULL},
  {"/Scope/Graticule/Color/gray66", NULL, setcolor, (int)"h", NULL},
  {"/Scope/Graticule/Color/gray33", NULL, setcolor, (int)"i", NULL},
  {"/Scope/Graticule/Color/blue4", NULL, setcolor, (int)"j", NULL},
  {"/Scope/Graticule/Color/green4", NULL, setcolor, (int)"k", NULL},
  {"/Scope/Graticule/Color/cyan4", NULL, setcolor, (int)"l", NULL},
  {"/Scope/Graticule/Color/red4", NULL, setcolor, (int)"m", NULL},
  {"/Scope/Graticule/Color/magenta4", NULL, setcolor, (int)"n", NULL},
  {"/Scope/Graticule/Color/yellow", NULL, setcolor, (int)"o", NULL},
  {"/Scope/Graticule/Color/white", NULL, setcolor, (int)"p", NULL},
  {"/Scope/Graticule/sep", NULL, NULL, 0, "<Separator>"},
  {"/Scope/Graticule/In Front", NULL, graticule, (int)"0", "<RadioItem>"},
  {"/Scope/Graticule/Behind", NULL, graticule, (int)"1", "/Scope/Graticule/In Front"},
  {"/Scope/Graticule/sep", NULL, NULL, 0, "<Separator>"},
  {"/Scope/Graticule/None", NULL, graticule, (int)"2", "<RadioItem>"},
  {"/Scope/Graticule/Minor Divisions", NULL, graticule, (int)"3", "/Scope/Graticule/None"},
  {"/Scope/Graticule/Minor & Major", NULL, graticule, (int)"4", "/Scope/Graticule/Minor Divisions"},
  {"/Scope/Cursors", NULL, hit_key, (int)"'", "<CheckItem>"},
  {"/Scope/sep", NULL, NULL, 0, "<Separator>"},
  {"/Scope/Serial Scope", NULL, hit_key, (int)"^", "<CheckItem>"},
  {"/Scope/SoundCard", NULL, hit_key, (int)"&", "<CheckItem>"},

  {"/Scope/DMA", NULL, NULL, 0, "<Branch>"},
  {"/Scope/DMA/tear", NULL, NULL, 0, "<Tearoff>"},
  {"/Scope/DMA/4 (block)", NULL, dma, (int)"4", "<RadioItem>"},
  {"/Scope/DMA/2 (nonblock)", NULL, dma, (int)"2", "/Scope/DMA/4 (block)"},
  {"/Scope/DMA/1 (nonblock)", NULL, dma, (int)"1", "/Scope/DMA/2 (nonblock)"},

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

  if ((p = finditem("/Trigger/Channel 1"))) {
    q = p + scope.trigch;
    gtk_check_menu_item_set_active
      (GTK_CHECK_MENU_ITEM
       (gtk_item_factory_get_item(factory, q->path)), TRUE);
    q = p + scope.trige + 3;
    gtk_check_menu_item_set_active
      (GTK_CHECK_MENU_ITEM
       (gtk_item_factory_get_item(factory, q->path)), TRUE);
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
     (gtk_item_factory_get_item(factory, "/Scope/SoundCard")), snd);
  gtk_check_menu_item_set_active
    (GTK_CHECK_MENU_ITEM
     (gtk_item_factory_get_item(factory, "/Scope/Serial Scope")),
     ps.found || bs.found);
  if ((p = finditem("/Scope/DMA"))) {
    p += 2;
    if (scope.dma < 4) p++;
    if (scope.dma < 2) p++;
    gtk_check_menu_item_set_active
      (GTK_CHECK_MENU_ITEM
       (gtk_item_factory_get_item(factory, p->path)), TRUE);
  }

  gtk_check_menu_item_set_active
    (GTK_CHECK_MENU_ITEM
     (gtk_item_factory_get_item(factory, "/Help/Keys&Info")), scope.verbose);

  if ((p = finditem("/Channel/Math")) &&
      (q = finditem("/Channel/Math/FFT. 2"))) {
    for (r = p; r <= q; r++) {
      gtk_widget_set_sensitive
	(GTK_WIDGET(gtk_item_factory_get_item(factory, r->path)),
	 scope.select > 1);
    }
    r = p + 5;
    gtk_widget_set_sensitive
      (GTK_WIDGET(gtk_item_factory_get_item(factory, r->path)), FALSE);
    r = scope.select < 2 ? p + 5 : p + 5 + ch[scope.select].func - FUNCMEM;
    if (r < p + 5) r = p + 5;
    gtk_check_menu_item_set_active
      (GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(factory, r->path)), TRUE);
  }
  if ((p = finditem("/Channel/Store/Mem A")) &&
      (q = finditem("/Channel/Recall/Mem A")))
    for (i = 0; i < 23; i++) {
      r = p + i;
      gtk_check_menu_item_set_active
	(GTK_CHECK_MENU_ITEM
	 (gtk_item_factory_get_item(factory, r->path)),
	 mem[i].color);
      r += (q - p);
      gtk_widget_set_sensitive
	(GTK_WIDGET(gtk_item_factory_get_item(factory, r->path)),
	 mem[i].color);
    }
  fixing_widgets = 0;
}

void
get_main_menu(GtkWidget *window, GtkWidget ** menubar)
{

/*    GtkAccelGroup *accel_group; */

/*    accel_group = gtk_accel_group_new (); */

  factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", NULL);
  gtk_item_factory_create_items(factory, nmenu_items, menu_items, NULL);

/*        gtk_accel_group_attach (accel_group, GTK_OBJECT (window)); */

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

/* context sensitive mouse click select, recall and pop-up menus */
gint
button_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  static int x, y, z, b;

  x = event->x;
  y = event->y;
  b = event->button;
  if (x > 100 && x < h_points - 100 && y > 80 && y < v_points - 80) {
    z = (event->x - 100) * 100 * ch[scope.select].signal->rate * scope.div
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
  } else if (y < 4)
    gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
			    (factory, "/Trigger")),
		   NULL, NULL, NULL, NULL, event->button, event->time);
  else if (y == 4)
    gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
			    (factory, "/Scope/Plot Mode")),
		   NULL, NULL, NULL, NULL, event->button, event->time);
  else if (y < 25 && (x < 11 || x > 69)) {
    handle_key((y - 5) / 5 + '1' + (x > 69 ? 4 : 0));
    if (b > 1) {
      if (!((y - 1) % 5))
	gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
				(factory, "/Channel/Scale")),
		       NULL, NULL, NULL, NULL, event->button, event->time);
      else if (!((y - 2) % 5)) {
	if (x < 4 || (x > 69 && x < 74))
	  gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
				  (factory, "/Channel/Bits")),
			 NULL, NULL, NULL, NULL, event->button, event->time);
	else
	  gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
				  (factory, "/Channel/Position")),
			 NULL, NULL, NULL, NULL, event->button, event->time);
      } else
	gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
				(factory, "/Channel")),
		       NULL, NULL, NULL, NULL, event->button, event->time);
    }
  } else if (b > 1)
    gtk_menu_popup(GTK_MENU(gtk_item_factory_get_widget
			    (factory, "/Scope")),
		   NULL, NULL, NULL, NULL, event->button, event->time);
  return TRUE;
}

/* initialize all the widgets, called by init_screen in display.c */
void
init_widgets()
{
  int i;
  GtkWidget *menubar;

  h_points = XX[scope.size];
  v_points = XY[scope.size];

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		     GTK_SIGNAL_FUNC(delete_event), NULL);
  gtk_signal_connect(GTK_OBJECT(window),"key_press_event",
		     GTK_SIGNAL_FUNC(key_press_event), NULL);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  get_main_menu(window, &menubar);
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);
  gtk_widget_show(menubar);

  drawing_area = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), h_points, v_points);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event",
		     GTK_SIGNAL_FUNC(expose_event), NULL);
  gtk_signal_connect(GTK_OBJECT(drawing_area),"configure_event",
		     GTK_SIGNAL_FUNC(configure_event), NULL);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "button_press_event",
		     GTK_SIGNAL_FUNC(button_event), NULL);
  gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK
			 | GDK_BUTTON_PRESS_MASK);

  gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);
  gtk_widget_show(drawing_area);
  gtk_widget_show(vbox);
  gtk_widget_show(window);

  gc = gdk_gc_new(drawing_area->window);
  for (i=0 ; i < 16 ; i++) {
    color[i] = i;
    gdk_color_parse(colors[i], &gdkcolor[i]);
    gdkcolor[i].pixel = (gulong)(gdkcolor[i].red * 65536 +
				 gdkcolor[i].green * 256 + gdkcolor[i].blue);
    gdk_color_alloc(gtk_widget_get_colormap(window), &gdkcolor[i]);
  }
  gdk_gc_set_background(gc, &gdkcolor[0]);
  SetColor(15);
  font = gdk_font_load(fontname);
  ClearDrawArea();
}

/* loop until finished */
void
mainloop()
{
  draw_text(1);
  animate(NULL);
  gtk_main();
}

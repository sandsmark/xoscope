/*
 * @(#)$Id: gr_gtk.c,v 1.9 1999/08/24 03:03:22 twitham Exp $
 *
 * Copyright (C) 1996 - 1999 Tim Witham <twitham@quiknet.com>
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

GtkWidget *window;
GtkWidget *drawing_area;	/* scope drawing area */
GdkPixmap *pixmap = NULL;
GdkRectangle update_rect;
GdkGC *gc;
GtkWidget *menubar;
/*  GtkWidget *filemenu; */
GtkWidget *vbox;
/*  GtkWidget *hbox; */
/*  GtkWidget *table; */
/*  GtkWidget *table2; */
/*  GtkStyle *mystyle; */

/*  GtkWidget colormenu[17]; */
/*  GtkWidget xwidg[11]; */
/*  GtkWidget *mwidg[57]; */
/*  GtkWidget *cwidg[CHANNELS]; */
/*  GtkWidget *ywidg[15]; */
/*  GtkWidget **math; */
/*  int **intarray; */
int XX[] = {640,800,1024,1280};
int XY[] = {480,600, 768,1024};
char my_filename[FILENAME_MAX] = "";
GdkFont *font;
char fontname[80] = DEF_FX;
char fonts[] = "xlsfonts";
GdkColor gdkcolor[16];
int color[16];
char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
char *colors[] = {		/* X colors similar to 16 console colors */
  "black",			/* 0 */
  "blue",
  "green",			/* 2 */
  "cyan",
  "red",			/* 4 */
  "magenta",
  "orange",			/* 6 */
  "gray66",
  "gray33",			/* 8 */
  "blue4",
  "green4",			/* 10 */
  "cyan4",
  "red4",			/* 12 */
  "magenta4",
  "yellow",			/* 14 */
  "white"
};

void
ClearDrawArea ()
{
  gdk_draw_rectangle(pixmap,
		     drawing_area->style->black_gc,
		     TRUE,
		     0, 0,
		     drawing_area->allocation.width,
		     drawing_area->allocation.height);
}

void
clear_display()
{
  ClearDrawArea();
}

void
AddTimeOut(unsigned long interval, int (*func)(), void *data) {
  gtk_timeout_add((guint32)interval,
		  (GtkFunction)func,
		  (gpointer)data);
}

void
delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gtk_main_quit ();
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

/* Refill the screen from the backing pixmap */
static gint
expose_event(GtkWidget *widget, GdkEventExpose *event)
{
  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
		  pixmap,
		  event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}

static gint
key_press_event(GtkWidget *widget, GdkEventKey *event)
{
  handle_key(event->string[0]);
  return TRUE;
}

int
OpenDisplay(int argc, char *argv[])
{
  gtk_init(&argc, &argv);
  return(argc);
}

void
DrawPixel(int x, int y)
{
  gdk_draw_point(pixmap, gc, x, y);
}

void
DrawLine(int x1, int y1, int x2, int y2)
{
  gdk_draw_line(pixmap, gc, x1, y1, x2, y2);
}

void
SetColor(int c)
{
  gdk_gc_set_foreground(gc, &gdkcolor[c]);
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
  command = gtk_entry_new_with_max_length(256);
  gtk_entry_set_text(GTK_ENTRY(command), ch[scope.select].command);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), command,
		     TRUE, TRUE, 0);
  gtk_signal_connect_object(GTK_OBJECT(window), "delete_event",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(window));
  gtk_signal_connect(GTK_OBJECT(run), "clicked",
		     GTK_SIGNAL_FUNC(run_sel),
		     GTK_ENTRY(command));
  gtk_signal_connect_object_after(GTK_OBJECT(run), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(window));
  gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(window));
  gtk_widget_show(run);
  gtk_widget_show(cancel);
  gtk_widget_show(label);
  gtk_widget_show(command);
  gtk_widget_show(window);
  /*    gtk_grab_add(window); */
}

void SyncDisplay() {
  update_rect.x  = update_rect.y = 0;
  update_rect.width = drawing_area->allocation.width;
  update_rect.height = drawing_area->allocation.height;
  gtk_widget_draw(drawing_area, &update_rect);
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
  /* there's probably a better way to blank the area with GC attribs? */
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
mathselect(GtkWidget *w, gpointer data)
{
  if (scope.select > 1) {
    ch[scope.select].func = ((char *)data)[0] - '0' + FUNC0;
    clear();
  }
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

/* close the window */
void
dismiss(GtkWidget *w, gpointer data)
{
/*   CloseWindow(); */
}

/* run an external command */
void
runextern(GtkWidget *w, gpointer data)
{
  strcpy(ch[scope.select].command, (char *)data);
  ch[scope.select].func = FUNCEXT;
  ch[scope.select].mem = EXTSTART;
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
  fixed_font = gdk_font_load ("-misc-fixed-medium-r-*-*-*-140-*-*-*-*-*-*");

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

/* simple button callback that just hits the given key */
void
hit_key(GtkWidget *w, gpointer data)
{
  handle_key(((char *)data)[0]);
}

void
cleanup_display()
{
}

/* set current state colors, labels, and check marks on widgets */
void
fix_widgets()
{
/* I suppose someday I could figure out how to check mark the menus */
}

void
get_main_menu(GtkWidget *window, GtkWidget ** menubar)
{
  static GtkMenuEntry menu_items[] =
  {
/*     {"<Main>/File/New", "<control>N", print_hello, NULL}, */
    {"<Main>/File/Open...", "@", hit_key, "@"},
    {"<Main>/File/Save...", "#", hit_key, "#"},
/*     {"<Main>/File/Save as", NULL, NULL, NULL}, */
    {"<Main>/File/<separator>", NULL, NULL, NULL},
    {"<Main>/File/Quit", NULL, hit_key, "\e"},

    {"<Main>/Channel/Hide Show", "\t", hit_key, "\t"},
    {"<Main>/Channel/Select/Channel 1", "1", hit_key, "1"},
    {"<Main>/Channel/Select/Channel 2", "2", hit_key, "2"},
    {"<Main>/Channel/Select/Channel 3", "3", hit_key, "3"},
    {"<Main>/Channel/Select/Channel 4", "4", hit_key, "4"},
    {"<Main>/Channel/Select/Channel 5", "5", hit_key, "5"},
    {"<Main>/Channel/Select/Channel 6", "6", hit_key, "6"},
    {"<Main>/Channel/Select/Channel 7", "7", hit_key, "7"},
    {"<Main>/Channel/Select/Channel 8", "8", hit_key, "8"},
    {"<Main>/Channel/<separator>", NULL, NULL, NULL},

    {"<Main>/Channel/Scale/up", "}", hit_key, "}"},
    {"<Main>/Channel/Scale/down", "{", hit_key, "{"},
    {"<Main>/Channel/Scale/<separator>", NULL, NULL, NULL},
    {"<Main>/Channel/Scale/50", NULL, setscale, "61"},
    {"<Main>/Channel/Scale/20", NULL, setscale, "51"},
    {"<Main>/Channel/Scale/10", NULL, setscale, "41"},
    {"<Main>/Channel/Scale/5", NULL, setscale, "31"},
    {"<Main>/Channel/Scale/2", NULL, setscale, "21"},
    {"<Main>/Channel/Scale/1", NULL, setscale, "11"},
/* How the ? do you put a / in a menu ? Just use \ until I figure it out. */
    {"<Main>/Channel/Scale/1\\2", NULL, setscale, "12"},
    {"<Main>/Channel/Scale/1\\5", NULL, setscale, "13"},
    {"<Main>/Channel/Scale/1\\10", NULL, setscale, "14"},
    {"<Main>/Channel/Scale/1\\20", NULL, setscale, "15"},
    {"<Main>/Channel/Scale/1\\50", NULL, setscale, "16"},

    {"<Main>/Channel/Position/up", "]", hit_key, "]"},
    {"<Main>/Channel/Position/down", "[", hit_key, "["},
    {"<Main>/Channel/Position/<separator>", NULL, NULL, NULL},
    {"<Main>/Channel/Position/160", NULL, setposition, " k"},
    {"<Main>/Channel/Position/144", NULL, setposition, " j"},
    {"<Main>/Channel/Position/128", NULL, setposition, " i"},
    {"<Main>/Channel/Position/112", NULL, setposition, " h"},
    {"<Main>/Channel/Position/96", NULL, setposition, " g"},
    {"<Main>/Channel/Position/80", NULL, setposition, " f"},
    {"<Main>/Channel/Position/64", NULL, setposition, " e"},
    {"<Main>/Channel/Position/48", NULL, setposition, " d"},
    {"<Main>/Channel/Position/32", NULL, setposition, " c"},
    {"<Main>/Channel/Position/16", NULL, setposition, " b"},
    {"<Main>/Channel/Position/0", NULL, setposition, " a"},
    {"<Main>/Channel/Position/-16", NULL, setposition, "-b"},
    {"<Main>/Channel/Position/-32", NULL, setposition, "-c"},
    {"<Main>/Channel/Position/-48", NULL, setposition, "-d"},
    {"<Main>/Channel/Position/-64", NULL, setposition, "-e"},
    {"<Main>/Channel/Position/-80", NULL, setposition, "-f"},
    {"<Main>/Channel/Position/-96", NULL, setposition, "-g"},
    {"<Main>/Channel/Position/-112", NULL, setposition, "-h"},
    {"<Main>/Channel/Position/-128", NULL, setposition, "-i"},
    {"<Main>/Channel/Position/-144", NULL, setposition, "-j"},
    {"<Main>/Channel/Position/-160", NULL, setposition, "-k"},

    {"<Main>/Channel/<separator>", NULL, NULL, NULL},

    {"<Main>/Channel/Math/Next Function", ";", hit_key, ";"},
    {"<Main>/Channel/Math/Prev Function", ":", hit_key, ":"},
    {"<Main>/Channel/Math/<separator>", NULL, NULL, NULL},
/* this will need hacked if functions are added / changed in func.c */
    {"<Main>/Channel/Math/Inv. 1", NULL, mathselect, "0"},
    {"<Main>/Channel/Math/Inv. 2", NULL, mathselect, "1"},
    {"<Main>/Channel/Math/Sum  1+2", NULL, mathselect, "2"},
    {"<Main>/Channel/Math/Diff 1-2", NULL, mathselect, "3"},
    {"<Main>/Channel/Math/Avg. 1,2", NULL, mathselect, "4"},
    {"<Main>/Channel/Math/FFT. 1", NULL, mathselect, "5"},
    {"<Main>/Channel/Math/FFT. 2", NULL, mathselect, "6"},

    {"<Main>/Channel/External Command...", "$", hit_key, "$"},
    {"<Main>/Channel/<separator>", NULL, NULL, NULL},

    {"<Main>/Channel/Store/Mem A", "A", hit_key, "A"},
    {"<Main>/Channel/Store/Mem B", "B", hit_key, "B"},
    {"<Main>/Channel/Store/Mem C", "C", hit_key, "C"},
    {"<Main>/Channel/Store/Mem D", "D", hit_key, "D"},
    {"<Main>/Channel/Store/Mem E", "E", hit_key, "E"},
    {"<Main>/Channel/Store/Mem F", "F", hit_key, "F"},
    {"<Main>/Channel/Store/Mem G", "G", hit_key, "G"},
    {"<Main>/Channel/Store/Mem H", "H", hit_key, "H"},
    {"<Main>/Channel/Store/Mem I", "I", hit_key, "I"},
    {"<Main>/Channel/Store/Mem J", "J", hit_key, "J"},
    {"<Main>/Channel/Store/Mem K", "K", hit_key, "K"},
    {"<Main>/Channel/Store/Mem L", "L", hit_key, "L"},
    {"<Main>/Channel/Store/Mem M", "M", hit_key, "M"},
    {"<Main>/Channel/Store/Mem N", "N", hit_key, "N"},
    {"<Main>/Channel/Store/Mem O", "O", hit_key, "O"},
    {"<Main>/Channel/Store/Mem P", "P", hit_key, "P"},
    {"<Main>/Channel/Store/Mem Q", "Q", hit_key, "Q"},
    {"<Main>/Channel/Store/Mem R", "R", hit_key, "R"},
    {"<Main>/Channel/Store/Mem S", "S", hit_key, "S"},
    {"<Main>/Channel/Store/Mem T", "T", hit_key, "T"},
    {"<Main>/Channel/Store/Mem U", "U", hit_key, "U"},
    {"<Main>/Channel/Store/Mem V", "V", hit_key, "V"},
    {"<Main>/Channel/Store/Mem W", "W", hit_key, "W"},
    {"<Main>/Channel/Recall/Mem A", "a", hit_key, "a"},
    {"<Main>/Channel/Recall/Mem B", "b", hit_key, "b"},
    {"<Main>/Channel/Recall/Mem C", "c", hit_key, "c"},
    {"<Main>/Channel/Recall/Mem D", "d", hit_key, "d"},
    {"<Main>/Channel/Recall/Mem E", "e", hit_key, "e"},
    {"<Main>/Channel/Recall/Mem F", "f", hit_key, "f"},
    {"<Main>/Channel/Recall/Mem G", "g", hit_key, "g"},
    {"<Main>/Channel/Recall/Mem H", "h", hit_key, "h"},
    {"<Main>/Channel/Recall/Mem I", "i", hit_key, "i"},
    {"<Main>/Channel/Recall/Mem J", "j", hit_key, "j"},
    {"<Main>/Channel/Recall/Mem K", "k", hit_key, "k"},
    {"<Main>/Channel/Recall/Mem L", "l", hit_key, "l"},
    {"<Main>/Channel/Recall/Mem M", "m", hit_key, "m"},
    {"<Main>/Channel/Recall/Mem N", "n", hit_key, "n"},
    {"<Main>/Channel/Recall/Mem O", "o", hit_key, "o"},
    {"<Main>/Channel/Recall/Mem P", "p", hit_key, "p"},
    {"<Main>/Channel/Recall/Mem Q", "q", hit_key, "q"},
    {"<Main>/Channel/Recall/Mem R", "r", hit_key, "r"},
    {"<Main>/Channel/Recall/Mem S", "s", hit_key, "s"},
    {"<Main>/Channel/Recall/Mem T", "t", hit_key, "t"},
    {"<Main>/Channel/Recall/Mem U", "u", hit_key, "u"},
    {"<Main>/Channel/Recall/Mem V", "v", hit_key, "v"},
    {"<Main>/Channel/Recall/Mem W", "w", hit_key, "w"},
    {"<Main>/Channel/Recall/Left Mix", "x", hit_key, "x"},
    {"<Main>/Channel/Recall/Right Mix", "y", hit_key, "y"},
    {"<Main>/Channel/Recall/ProbeScope", "z", hit_key, "z"},

    {"<Main>/Trigger/Channel 1,2", "_", hit_key, "_"},
    {"<Main>/Trigger/Auto,Rising,Falling", "+", hit_key, "+"},
    {"<Main>/Trigger/<separator>", NULL, NULL, NULL},
    {"<Main>/Trigger/Position up", "=", hit_key, "="},
    {"<Main>/Trigger/Position down", "-", hit_key, "-"},
    {"<Main>/Trigger/Position Positive/120", NULL, setposition, "Tb"},
    {"<Main>/Trigger/Position Positive/112", NULL, setposition, "Tc"},
    {"<Main>/Trigger/Position Positive/104", NULL, setposition, "Td"},
    {"<Main>/Trigger/Position Positive/96", NULL, setposition, "Te"},
    {"<Main>/Trigger/Position Positive/88", NULL, setposition, "Tf"},
    {"<Main>/Trigger/Position Positive/80", NULL, setposition, "Tg"},
    {"<Main>/Trigger/Position Positive/72", NULL, setposition, "Th"},
    {"<Main>/Trigger/Position Positive/64", NULL, setposition, "Ti"},
    {"<Main>/Trigger/Position Positive/56", NULL, setposition, "Tj"},
    {"<Main>/Trigger/Position Positive/48", NULL, setposition, "Tk"},
    {"<Main>/Trigger/Position Positive/40", NULL, setposition, "Tl"},
    {"<Main>/Trigger/Position Positive/32", NULL, setposition, "Tm"},
    {"<Main>/Trigger/Position Positive/24", NULL, setposition, "Tn"},
    {"<Main>/Trigger/Position Positive/16", NULL, setposition, "To"},
    {"<Main>/Trigger/Position Positive/8", NULL, setposition, "Tp"},
    {"<Main>/Trigger/Position Positive/0", NULL, setposition, "Tq"},
    {"<Main>/Trigger/Position Negative/0", NULL, setposition, "tq"},
    {"<Main>/Trigger/Position Negative/-8", NULL, setposition, "tp"},
    {"<Main>/Trigger/Position Negative/-16", NULL, setposition, "to"},
    {"<Main>/Trigger/Position Negative/-24", NULL, setposition, "tn"},
    {"<Main>/Trigger/Position Negative/-32", NULL, setposition, "tm"},
    {"<Main>/Trigger/Position Negative/-40", NULL, setposition, "tl"},
    {"<Main>/Trigger/Position Negative/-48", NULL, setposition, "tk"},
    {"<Main>/Trigger/Position Negative/-56", NULL, setposition, "tj"},
    {"<Main>/Trigger/Position Negative/-64", NULL, setposition, "ti"},
    {"<Main>/Trigger/Position Negative/-72", NULL, setposition, "th"},
    {"<Main>/Trigger/Position Negative/-80", NULL, setposition, "tg"},
    {"<Main>/Trigger/Position Negative/-88", NULL, setposition, "tf"},
    {"<Main>/Trigger/Position Negative/-96", NULL, setposition, "te"},
    {"<Main>/Trigger/Position Negative/-104", NULL, setposition, "td"},
    {"<Main>/Trigger/Position Negative/-112", NULL, setposition, "tc"},
    {"<Main>/Trigger/Position Negative/-120", NULL, setposition, "tb"},
    {"<Main>/Trigger/Position Negative/-128", NULL, setposition, "ta"},

    {"<Main>/Scope/Refresh", NULL, hit_key, "\n"},
    {"<Main>/Scope/Plot Mode/Point", NULL, plotmode, "0"},
    {"<Main>/Scope/Plot Mode/Point Accumulate", NULL, plotmode, "1"},
    {"<Main>/Scope/Plot Mode/Line", NULL, plotmode, "2"},
    {"<Main>/Scope/Plot Mode/Line Accumulate", NULL, plotmode, "3"},
    {"<Main>/Scope/Graticule/Color/black", NULL, setcolor, "a"},
    {"<Main>/Scope/Graticule/Color/blue", NULL, setcolor, "b"},
    {"<Main>/Scope/Graticule/Color/green", NULL, setcolor, "c"},
    {"<Main>/Scope/Graticule/Color/cyan", NULL, setcolor, "d"},
    {"<Main>/Scope/Graticule/Color/red", NULL, setcolor, "e"},
    {"<Main>/Scope/Graticule/Color/magenta", NULL, setcolor, "f"},
    {"<Main>/Scope/Graticule/Color/orange", NULL, setcolor, "g"},
    {"<Main>/Scope/Graticule/Color/gray66", NULL, setcolor, "h"},
    {"<Main>/Scope/Graticule/Color/gray33", NULL, setcolor, "i"},
    {"<Main>/Scope/Graticule/Color/blue4", NULL, setcolor, "j"},
    {"<Main>/Scope/Graticule/Color/green4", NULL, setcolor, "k"},
    {"<Main>/Scope/Graticule/Color/cyan4", NULL, setcolor, "l"},
    {"<Main>/Scope/Graticule/Color/red4", NULL, setcolor, "m"},
    {"<Main>/Scope/Graticule/Color/magenta4", NULL, setcolor, "n"},
    {"<Main>/Scope/Graticule/Color/yellow", NULL, setcolor, "o"},
    {"<Main>/Scope/Graticule/Color/white", NULL, setcolor, "p"},
    {"<Main>/Scope/Graticule/<separator>", NULL, NULL, NULL},
    {"<Main>/Scope/Graticule/In Front", NULL, graticule, "0"},
    {"<Main>/Scope/Graticule/Behind", NULL, graticule, "1"},
    {"<Main>/Scope/Graticule/<separator>", NULL, NULL, NULL},
    {"<Main>/Scope/Graticule/None", NULL, graticule, "2"},
    {"<Main>/Scope/Graticule/Minor Divisions", NULL, graticule, "3"},
    {"<Main>/Scope/Graticule/Minor & Major", NULL, graticule, "4"},

    {"<Main>/<<", NULL, hit_key, "9"},
    {"<Main>/<", NULL, hit_key, "("},
    {"<Main>/>", NULL, hit_key, ")"},
    {"<Main>/>> ", NULL, hit_key, "0"},

    {"<Main>/SC", NULL, hit_key, "&"},
    {"<Main>/PS", NULL, hit_key, "^"},

    {"<Main>/Run", NULL, runmode, "1"},
    {"<Main>/Wait", NULL, runmode, "2"},
    {"<Main>/Stop", NULL, runmode, "0"},

    {"<Main>/?", NULL, hit_key, "?"},
    {"<Main>/Help", NULL, help, NULL},
  };
  int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

  GtkMenuFactory *factory;
  GtkMenuFactory *subfactory;
  GtkMenuPath *menu_path;

  factory = gtk_menu_factory_new(GTK_MENU_FACTORY_MENU_BAR);
  subfactory = gtk_menu_factory_new(GTK_MENU_FACTORY_MENU_BAR);

  gtk_menu_factory_add_subfactory(factory, subfactory, "<Main>");
  gtk_menu_factory_add_entries(factory, menu_items, nmenu_items);
/*    gtk_window_add_accelerator_table(GTK_WINDOW(window), subfactory->table); */

  menu_path = gtk_menu_factory_find(factory,  "<Main>/Help");
  gtk_menu_item_right_justify(GTK_MENU_ITEM(menu_path->widget));

  if (menubar)
    *menubar = subfactory->widget;
}

/* initialize all the widgets, called by init_screen in display.c */
void
init_widgets()
{
  int i;
  GtkWidget *menubar;
/*    static char *s[] = {"1", "2", "3", "4", "5", "6", "7", "8"}; */

  h_points = XX[scope.size];
  v_points = XY[scope.size];

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT (window), "delete_event",
		     GTK_SIGNAL_FUNC (delete_event), NULL);
  gtk_signal_connect(GTK_OBJECT(window),"key_press_event",
		     (GtkSignalFunc) key_press_event, NULL);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  get_main_menu(window, &menubar);
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);
  gtk_widget_show(menubar);

  drawing_area = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), h_points, v_points);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event",
		     (GtkSignalFunc) expose_event, NULL);
  gtk_signal_connect(GTK_OBJECT(drawing_area),"configure_event",
		     (GtkSignalFunc) configure_event, NULL);

/* This stuff was an attempt to be like the libsx interface.  It
   almost works, but because I can't figure out how to color code the
   widgets and because they steal keyboard focus, I've opted to put
   everything in the menubar instead.  -twitham, 1999/08/21 */

/*    table2 = gtk_table_new(21, 2, FALSE); */

/*    ywidg[1] = gtk_label_new("Trig"); */
/*    gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[1], 0, 2, 0, 1); */

/*    ywidg[2] = gtk_button_new_with_label("/\\"); */
/*    gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[2], 0, 2, 1, 2); */
/*    gtk_signal_connect(GTK_OBJECT(ywidg[2]), "clicked", */
/*  		     GTK_SIGNAL_FUNC(hit_key), "="); */

/*    ywidg[3] = gtk_button_new_with_label("<"); */
/*    gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[3], 0, 1, 2, 3); */
/*    gtk_signal_connect(GTK_OBJECT(ywidg[3]), "clicked", */
/*  		     GTK_SIGNAL_FUNC(hit_key), "_"); */

/*    ywidg[4] = gtk_button_new_with_label(">"); */
/*    gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[4], 1, 2, 2, 3); */
/*    gtk_signal_connect(GTK_OBJECT(ywidg[4]), "clicked", */
/*  		     GTK_SIGNAL_FUNC(hit_key), "+"); */

/*    ywidg[5] = gtk_button_new_with_label("\\/"); */
/*    gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[5], 0, 2, 3, 4); */
/*    gtk_signal_connect(GTK_OBJECT(ywidg[5]), "clicked", */
/*  		     GTK_SIGNAL_FUNC(hit_key), "-"); */

/*    ywidg[6] = gtk_label_new("Scal"); */
/*    gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[6], 0, 2, 4, 5); */

/*    ywidg[7] = gtk_button_new_with_label("/\\"); */
/*    gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[7], 0, 2, 5, 6); */
/*    gtk_signal_connect(GTK_OBJECT(ywidg[7]), "clicked", */
/*  		     GTK_SIGNAL_FUNC(hit_key), "}"); */

/*    ywidg[8] = gtk_button_new_with_label("\\/"); */
/*    gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[8], 0, 2, 6, 7); */
/*    gtk_signal_connect(GTK_OBJECT(ywidg[8]), "clicked", */
/*  		     GTK_SIGNAL_FUNC(hit_key), "{"); */

/*    ywidg[9] = gtk_label_new("Chan"); */
/*    gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[9], 0, 2, 7, 8); */

/*    for (i = 0 ; i < CHANNELS ; i++) { */
/*      cwidg[i] = gtk_button_new_with_label(&s[i][0]); */
/*      gtk_table_attach_defaults(GTK_TABLE(table2), cwidg[i], 0, 2, i + 8, i + 9); */
/*      gtk_widget_show(cwidg[i]); */
/*      gtk_signal_connect(GTK_OBJECT(cwidg[i]), "clicked", */
/*  		       GTK_SIGNAL_FUNC(hit_key), &s[i][0]); */
/*    } */

/*    ywidg[10] = gtk_label_new("Pos."); */
/*    gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[10], 0, 2, 16, 17); */

/*    ywidg[11] = gtk_button_new_with_label("/\\"); */
/*    gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[11], 0, 2, 17, 18); */
/*    gtk_signal_connect(GTK_OBJECT(ywidg[11]), "clicked", */
/*  		     GTK_SIGNAL_FUNC(hit_key), "]"); */

/*    ywidg[12] = gtk_button_new_with_label("\\/"); */
/*    gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[12], 0, 2, 18, 19); */
/*    gtk_signal_connect(GTK_OBJECT(ywidg[12]), "clicked", */
/*  		     GTK_SIGNAL_FUNC(hit_key), "["); */

/*    ywidg[13] = gtk_label_new(""); */
/*    gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[13], 0, 2, 19, 20); */

/*    ywidg[14] = gtk_button_new_with_label("Hide"); */
/*    gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[14], 0, 2, 20, 21); */
/*    gtk_signal_connect(GTK_OBJECT(ywidg[14]), "clicked", */
/*  		     GTK_SIGNAL_FUNC(hit_key), "\t"); */

/*    for (i = 1 ; i < 15 ; i++) { */
/*      gtk_widget_show(ywidg[i]); */
/*    } */

/*    hbox = gtk_hbox_new(FALSE, 0); */
/*    gtk_box_pack_start(GTK_BOX(hbox), drawing_area, TRUE, TRUE, 0); */
/*    gtk_box_pack_start(GTK_BOX(hbox), table2, FALSE, TRUE, 0); */
/*    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0); */
/*    gtk_widget_show(drawing_area); */
/*    gtk_widget_show(table2); */
/*    gtk_widget_show(hbox); */

/*    table = gtk_table_new(2, 29, FALSE); */
/*    mwidg[0]  = gtk_label_new(" Store "); */
/*    mwidg[29]  = gtk_label_new(" Recall"); */
/*    gtk_table_attach_defaults(GTK_TABLE(table), mwidg[0], 0, 1, 0, 1); */
/*    gtk_widget_show(mwidg[0]); */
/*    gtk_table_attach_defaults(GTK_TABLE(table), mwidg[29], 0, 1, 1, 2); */
/*    gtk_widget_show(mwidg[29]); */
/*    gtk_widget_show(table); */

/*    for (i = 0 ; i < 26 ; i++) { */
/*      sprintf(error, "%c", i + 'A'); */
/*      mwidg[i + 1] = gtk_button_new_with_label(error); */
/*      gtk_table_attach_defaults(GTK_TABLE(table), mwidg[i + 1], */
/*  			      i + 1, i + 2, 0, 1); */
/*      gtk_signal_connect(GTK_OBJECT(mwidg[i + 1]), "clicked", */
/*  		       GTK_SIGNAL_FUNC(hit_key), &alphabet[i]); */
/*      gtk_widget_show(mwidg[i + 1]); */
/*      sprintf(error, "%c", i + 'a'); */
/*      mwidg[i + 30] = gtk_button_new_with_label(error); */
/*      gtk_table_attach_defaults(GTK_TABLE(table), mwidg[i + 30], */
/*  			      i + 1, i + 2, 1, 2); */
/*      gtk_signal_connect(GTK_OBJECT(mwidg[i + 30]), "clicked", */
/*  		       GTK_SIGNAL_FUNC(hit_key), &alphabet[i + 26]); */
/*      gtk_widget_show(mwidg[i + 30]); */
/*    } */

/*    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0); */

				/* end of libsx-like interface */

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
/*      printf("%d %s %ld:\tr:%d g:%d b:%d\n", */
/*  	   i, colors[i], gdkcolor[i].pixel, */
/*  	   gdkcolor[i].red, gdkcolor[i].green, gdkcolor[i].blue); */
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

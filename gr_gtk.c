/*
 * @(#)$Id: gr_gtk.c,v 1.6 1999/08/20 05:43:38 twitham Exp $
 *
 * Copyright (C) 1996 - 1998 Tim Witham <twitham@pcocd2.intel.com>
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
GtkWidget *filemenu;
GtkWidget *vbox;
GtkWidget *hbox;
GtkWidget *table;
GtkWidget *table2;
GtkStyle *mystyle;

GtkWidget colormenu[17];	/* color menu */
GtkWidget xwidg[11];		/* extra horizontal widgets */
GtkWidget *mwidg[57];		/* memory / math widgets */
GtkWidget *cwidg[CHANNELS];	/* channel button widgets */
GtkWidget *ywidg[15];		/* vertical widgets */
GtkWidget **math;		/* math menu */
int **intarray;			/* indexes of math functions */
int XX[] = {640,800,1024,1280};
int XY[] = {480,600, 768,1024};
char my_filename[FILENAME_MAX];
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
/*   static int i = 2; */
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

/* prompt for a string */
char *
GetString(char *msg, char *def)
{
/* #ifdef HAVEVGAMISC */
/*   char *s; */

/*   s = vga_prompt(0, v_points / 2, */
/* 		 80 * 8, 8 + FONT.font_height, msg, */
/* 		 &FONT, &FONT, TEXT_FG, KEY_FG, TEXT_BG, PROMPT_SCROLLABLE); */
/*   if (s[0] == '\e' || s[0] == '\0') */
/*     return(NULL); */
/*   return(s); */
/* #else */
  return(def);
/* #endif */
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
    sprintf(error, "\n\n%s exists,\n\nOVERWRITE?\n\n", my_filename);
    label = gtk_label_new(error);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), label, TRUE, TRUE, 0);
    gtk_widget_show(label);
    gtk_widget_show(window);
    gtk_grab_add(window);
  } else
    savefile(my_filename);
}

/* get a file name */
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
/*   if (path) */
/*     gtk_file_selection_set_filename(GTK_FILE_SELECTION(filew), */
/* 				    (gchar *)path); */
  gtk_widget_show(filew);
}

void SyncDisplay() {
  update_rect.x  = update_rect.y = 0;
  update_rect.width = drawing_area->allocation.width;
  update_rect.height = drawing_area->allocation.height;
  gtk_widget_draw(drawing_area, &update_rect);
}

/* die on malloc error */
void
nomalloc(char *file, int line)
{
  sprintf(error, "%s: out of memory at %s line %d", progname, file, line);
  perror(error);
  cleanup();
  exit(1);
}

/* a libsx text writer similar to libvgamisc's vga_write */
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
  /*   DrawText(s, x, y + FontHeight(f)); */
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

/* callback for keypress events on the scope drawing area */
void
keys(GtkWidget w, char *input, int up_or_down, void *data)
{
/*   if (!up_or_down)		/* 0 press, 1 release */
/*     return; */
/*   if (input[1] == '\0')		/* single character to handle */
/*     handle_key(input[0]); */
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
  int *c = (int *)data;

  scope.color = *c;
  draw_text(1);
}

void
mathselect(GtkWidget w, void *data)
{
  int *c = (int *)data;

  if (scope.select > 1) {
    ch[scope.select].func = *c + FUNC0;
    clear();
  }
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
help(GtkWidget w, void *data)
{
/*   GtkWidget x[2]; */
/*   char c, prev = '\0', pprev = '\0', *tmp, *s = NULL; */
/*   FILE *p; */
/*   int i = 0; */

/*   if ((p = popen(HELPCOMMAND, "r")) != NULL) { */
/*     while ((c = fgetc(p)) != EOF) { */
/*       if (c == '\b') */
/* 	fgetc(p); */
/*       else if (!(c == '\n' && prev == '\n' && pprev == '\n')) { */
/* 	tmp = realloc(s, i + 1); */
/* 	s = tmp; */
/* 	s[i] = c; */
/* 	i++; */
/*       } */
/*       pprev = prev; */
/*       prev = c; */
/*     } */
/*     tmp = realloc(s, i + 1); */
/*     s = tmp; */
/*     s[i] = '\0'; */
/*     pclose(p); */
/*     MakeWindow("Help", SAME_DISPLAY, NONEXCLUSIVE_WINDOW); */
/*     x[0] = MakeButton("Dismiss", dismiss, NULL); */
/*     x[1] = MakeTextWidget(s, FALSE, FALSE, 500, 500); */
/*     free(s); */
/*     SetWidgetPos(x[1], PLACE_UNDER, x[0], NO_CARE, NULL); */
/*     ShowDisplay(); */
/*   } */
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
  int i;

  for (i = 0 ; i < (funccount > 16 + FUNC0 ? funccount - FUNC0 : 16) ; i++) {
    if (i < funccount - FUNC0)
      if (math && math[i]) free(math[i]);
    if (intarray && intarray[i]) free(intarray[i]);
  }
  if (math) free(math);
  if (intarray) free(intarray);
/*   if (font) FreeFont(font); */
}

/* set current state colors, labels, and check marks on widgets */
void
fix_widgets()
{
/*   mystyle = gtk_widget_get_style(cwidg[0]); */

  mystyle = gtk_style_new();
  gtk_widget_set_name(cwidg[0], "foo");
  mystyle->fg_gc[0] = gc;
  mystyle->fg_gc[1] = gc;
  mystyle->fg_gc[2] = gc;
  mystyle->fg_gc[3] = gc;
  mystyle->fg_gc[4] = gc;
  gtk_widget_set_style(cwidg[0], mystyle);
  gtk_widget_queue_draw(cwidg[0]);

/*   int i; */
/*   char *tilt[] = {"-", "/", "\\"}; */

/*   for (i = 0 ; i < 4 ; i++) { */
/*     SetMenuItemChecked(plot[i + 1], scope.mode == i); */
/*   } */
/*   SetMenuItemChecked(grat[1], !scope.behind); */
/*   SetMenuItemChecked(grat[2], scope.behind); */
/*   for (i = 0 ; i < 3 ; i++) { */
/*     SetMenuItemChecked(grat[i + 3], scope.grat == i); */
/*   } */
/*   for (i = 0 ; i < 16 ; i++) { */
/*     SetMenuItemChecked(colormenu[i + 1], scope.color == i); */
/*   } */
/*   for (i = 0 ; i < funccount - FUNC0 ; i++) { */
/*     SetMenuItemChecked(*math[i], ch[scope.select].func == i + FUNC0); */
/*   } */

/*   SetBgColor(mwidg[0], ch[scope.select].color); */
/*   SetBgColor(mwidg[27], ch[scope.select].color); */
/*   SetBgColor(mwidg[28], ch[scope.select].color); */
/*   SetBgColor(mwidg[29], ch[scope.select].color); */
/*   SetBgColor(mwidg[56], ch[scope.select].color); */
/*   SetWidgetState(mwidg[27], scope.select > 1); */
/*   SetWidgetState(mwidg[28], scope.select > 1); */
/*   SetWidgetState(mwidg[56], scope.select > 1); */
/*   for (i = 0 ; i < 26 ; i++) { */
/*     SetBgColor(mwidg[i + 30], mem[i].color); */
/*     if (i > 22) { */
/*       SetBgColor(mwidg[i + 1], mem[i].color); */
/*       SetWidgetState(mwidg[i + 1], 0); */
/*     } */
/*   } */

/*   SetWidgetState(xwidg[7], scope.run != 1); */
/*   SetWidgetState(xwidg[8], scope.run != 2); */
/*   SetWidgetState(xwidg[9], scope.run != 0); */

/*   SetBgColor(ywidg[2], ch[scope.trigch].color); */
/*   SetBgColor(ywidg[3], ch[scope.trigch].color); */
/*   SetBgColor(ywidg[4], ch[scope.trigch].color); */
/*   SetBgColor(ywidg[5], ch[scope.trigch].color); */
/*   SetBgColor(ywidg[7], ch[scope.select].color); */
/*   SetBgColor(ywidg[8], ch[scope.select].color); */
/*   SetBgColor(ywidg[11], ch[scope.select].color); */
/*   SetBgColor(ywidg[12], ch[scope.select].color); */
/*   SetBgColor(ywidg[14], ch[scope.select].color); */

/*   SetLabel(ywidg[3], scope.trigch ? "Y" : "X"); */
/*   SetLabel(ywidg[4], tilt[scope.trige]); */
/*   SetLabel(ywidg[14], ch[scope.select].show ? "Hide" : "Show"); */

/*   for (i = 0 ; i < CHANNELS ; i++) { */
/*     SetFgColor(cwidg[i], ch[i].show ? color[0] : ch[i].color); */
/*     SetBgColor(cwidg[i], ch[i].show ? ch[i].color : color[0]); */
/*   } */
}

void
get_main_menu(GtkWidget *window, GtkWidget ** menubar)
{
  static GtkMenuEntry menu_items[] =
  {
/*     {"<Main>/File/New", "<control>N", print_hello, NULL}, */
    {"<Main>/File/Open...", "<control>O", hit_key, "@"},
    {"<Main>/File/Save...", "<control>S", hit_key, "#"},
/*     {"<Main>/File/Save as", NULL, NULL, NULL}, */
    {"<Main>/File/<separator>", NULL, NULL, NULL},
    {"<Main>/File/Quit", "<control>Q", hit_key, "\e"},
    {"<Main>/Plot Mode/Point", NULL, plotmode, "0"},
    {"<Main>/Plot Mode/Point Accumulate", NULL, plotmode, "1"},
    {"<Main>/Plot Mode/Line", NULL, plotmode, "2"},
    {"<Main>/Plot Mode/Line Accumulate", NULL, plotmode, "3"},
    {"<Main>/Graticule/In Front", NULL, graticule, "0"},
    {"<Main>/Graticule/Behind", NULL, graticule, "1"},
    {"<Main>/Graticule/<separator>", NULL, NULL, NULL},
    {"<Main>/Graticule/None", NULL, graticule, "2"},
    {"<Main>/Graticule/Minor Divisions", NULL, graticule, "3"},
    {"<Main>/Graticule/Minor & Major", NULL, graticule, "4"},
    {"<Main>/Refresh", NULL, hit_key, "\n"},
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
    {"<Main>/Help", NULL, hit_key, "?"},
  };
  int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

  GtkMenuFactory *factory;
  GtkMenuFactory *subfactory;
  GtkMenuPath *menu_path;

  factory = gtk_menu_factory_new(GTK_MENU_FACTORY_MENU_BAR);
  subfactory = gtk_menu_factory_new(GTK_MENU_FACTORY_MENU_BAR);

  gtk_menu_factory_add_subfactory(factory, subfactory, "<Main>");
  gtk_menu_factory_add_entries(factory, menu_items, nmenu_items);
  gtk_window_add_accelerator_table(GTK_WINDOW(window), subfactory->table);

  menu_path = gtk_menu_factory_find(factory,  "<Main>/Help");
  gtk_menu_item_right_justify(menu_path->widget);

  if (menubar)
    *menubar = subfactory->widget;
}

/* initialize all the widgets, called by init_screen in display.c */
void
init_widgets()
{
  int i;
  GtkWidget *menubar;
  static char *s[] = {"1", "2", "3", "4", "5", "6", "7", "8"};

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

  table2 = gtk_table_new(21, 2, FALSE);

  ywidg[1] = gtk_label_new("Trig");
  gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[1], 0, 2, 0, 1);

  ywidg[2] = gtk_button_new_with_label("/\\");
  gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[2], 0, 2, 1, 2);
  gtk_signal_connect(GTK_OBJECT(ywidg[2]), "clicked",
		     GTK_SIGNAL_FUNC(hit_key), "=");

  ywidg[3] = gtk_button_new_with_label("<");
  gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[3], 0, 1, 2, 3);
  gtk_signal_connect(GTK_OBJECT(ywidg[3]), "clicked",
		     GTK_SIGNAL_FUNC(hit_key), "_");

  ywidg[4] = gtk_button_new_with_label(">");
  gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[4], 1, 2, 2, 3);
  gtk_signal_connect(GTK_OBJECT(ywidg[4]), "clicked",
		     GTK_SIGNAL_FUNC(hit_key), "+");

  ywidg[5] = gtk_button_new_with_label("\\/");
  gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[5], 0, 2, 3, 4);
  gtk_signal_connect(GTK_OBJECT(ywidg[5]), "clicked",
		     GTK_SIGNAL_FUNC(hit_key), "-");

  ywidg[6] = gtk_label_new("Scal");
  gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[6], 0, 2, 4, 5);

  ywidg[7] = gtk_button_new_with_label("/\\");
  gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[7], 0, 2, 5, 6);
  gtk_signal_connect(GTK_OBJECT(ywidg[7]), "clicked",
		     GTK_SIGNAL_FUNC(hit_key), "}");

  ywidg[8] = gtk_button_new_with_label("\\/");
  gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[8], 0, 2, 6, 7);
  gtk_signal_connect(GTK_OBJECT(ywidg[8]), "clicked",
		     GTK_SIGNAL_FUNC(hit_key), "{");

  ywidg[9] = gtk_label_new("Chan");
  gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[9], 0, 2, 7, 8);

  for (i = 0 ; i < CHANNELS ; i++) {
    cwidg[i] = gtk_button_new_with_label(&s[i][0]);
    gtk_table_attach_defaults(GTK_TABLE(table2), cwidg[i], 0, 2, i + 8, i + 9);
    gtk_widget_show(cwidg[i]);
    gtk_signal_connect(GTK_OBJECT(cwidg[i]), "clicked",
		       GTK_SIGNAL_FUNC(hit_key), &s[i][0]);
  }

  ywidg[10] = gtk_label_new("Pos.");
  gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[10], 0, 2, 16, 17);

  ywidg[11] = gtk_button_new_with_label("/\\");
  gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[11], 0, 2, 17, 18);
  gtk_signal_connect(GTK_OBJECT(ywidg[11]), "clicked",
		     GTK_SIGNAL_FUNC(hit_key), "]");

  ywidg[12] = gtk_button_new_with_label("\\/");
  gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[12], 0, 2, 18, 19);
  gtk_signal_connect(GTK_OBJECT(ywidg[12]), "clicked",
		     GTK_SIGNAL_FUNC(hit_key), "[");

  ywidg[13] = gtk_label_new("");
  gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[13], 0, 2, 19, 20);

  ywidg[14] = gtk_button_new_with_label("Hide");
  gtk_table_attach_defaults(GTK_TABLE(table2), ywidg[14], 0, 2, 20, 21);
  gtk_signal_connect(GTK_OBJECT(ywidg[14]), "clicked",
		     GTK_SIGNAL_FUNC(hit_key), "\t");

  for (i = 1 ; i < 15 ; i++) {
    gtk_widget_show(ywidg[i]);
  }

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), drawing_area, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), table2, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(drawing_area);
  gtk_widget_show(table2);
  gtk_widget_show(hbox);

  table = gtk_table_new(2, 29, FALSE);
  mwidg[0]  = gtk_label_new(" Store ");
  mwidg[29]  = gtk_label_new(" Recall");
  gtk_table_attach_defaults(GTK_TABLE(table), mwidg[0], 0, 1, 0, 1);
  gtk_widget_show(mwidg[0]);
  gtk_table_attach_defaults(GTK_TABLE(table), mwidg[29], 0, 1, 1, 2);
  gtk_widget_show(mwidg[29]);
  gtk_widget_show(table);

  for (i = 0 ; i < 26 ; i++) {
    sprintf(error, "%c", i + 'A');
    mwidg[i + 1] = gtk_button_new_with_label(error);
    gtk_table_attach_defaults(GTK_TABLE(table), mwidg[i + 1],
			      i + 1, i + 2, 0, 1);
    gtk_signal_connect(GTK_OBJECT(mwidg[i + 1]), "clicked",
		       GTK_SIGNAL_FUNC(hit_key), &alphabet[i]);
    gtk_widget_show(mwidg[i + 1]);
    sprintf(error, "%c", i + 'a');
    mwidg[i + 30] = gtk_button_new_with_label(error);
    gtk_table_attach_defaults(GTK_TABLE(table), mwidg[i + 30],
			      i + 1, i + 2, 1, 2);
    gtk_signal_connect(GTK_OBJECT(mwidg[i + 30]), "clicked",
		       GTK_SIGNAL_FUNC(hit_key), &alphabet[i + 26]);
    gtk_widget_show(mwidg[i + 30]);
  }

  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);
  gtk_widget_show(vbox);
  gtk_widget_show(window);

  gc = gdk_gc_new(drawing_area->window);
  for (i=0 ; i < 16 ; i++) {
    color[i] = i;
    gdk_color_parse(colors[i], &gdkcolor[i]);
    gdkcolor[i].pixel = (gulong)(gdkcolor[i].red * 65536 +
				 gdkcolor[i].green * 256 + gdkcolor[i].blue);
    gdk_color_alloc(gtk_widget_get_colormap(window), &gdkcolor[i]);
    printf("%d %s %ld:\tr:%d g:%d b:%d\n",
	   i, colors[i], gdkcolor[i].pixel,
	   gdkcolor[i].red, gdkcolor[i].green, gdkcolor[i].blue);
  }
  gdk_gc_set_background(gc, &gdkcolor[0]);
  SetColor(15);

/*   mystyle = gtk_style_copy(cwidg[0]->style); */
/*   mystyle = gtk_widget_get_style(cwidg[1]); */
/*   mystyle->fg_gc[0] = gc; */
/*   mystyle->fg_gc[1] = gc; */
/*   mystyle->fg_gc[2] = gc; */
/*   mystyle->fg_gc[3] = gc; */
/*   mystyle->fg_gc[4] = gc; */
/*   gtk_widget_set_style(cwidg[0], mystyle); */
/*   gtk_widget_queue_resize(cwidg[0]); */
/*   gtk_widget_queue_draw(cwidg[0]); */

/*   colormenu[0] = MakeMenu("Color"); */

/*   /* bottom rows of widgets */

/*   j = funccount - FUNC0; */
/*   if ((math = malloc(sizeof(Widget *) * j)) == NULL) */
/*     nomalloc(__FILE__, __LINE__); */
/*   if ((intarray = malloc(sizeof(int *) * (j > 16 ? j : 16))) == NULL) */
/*     nomalloc(__FILE__, __LINE__); */
/*   for (i = 0 ; i < (j > 16 ? j : 16) ; i++) { */
/*     if ((intarray[i] = malloc(sizeof(int))) == NULL) */
/*       nomalloc(__FILE__, __LINE__); */
/*     *intarray[i] = i; */
/*     if (i < j) { */
/*       if ((math[i] = malloc(sizeof(GtkWidget))) == NULL) */
/* 	nomalloc(__FILE__, __LINE__); */
/*       *math[i] = MakeMenuItem(mwidg[27], funcnames[FUNC0 + i], */
/* 			      mathselect, intarray[i]); */
/*     } */
/*   } */

/*   ShowDisplay(); */

/*   for (i = 0 ; i < 16 ; i++) { */
/*     color[i] = GetNamedColor(colors[i]); */
/*     colormenu[i + 1] = MakeMenuItem(colormenu[0], colors[i], */
/* 				    setcolor, intarray[i]); */
/*   } */

/*   for (i = 0 ; i < 26 ; i++) { */
/*     SetBgColor(mwidg[i + 30], color[0]); */
/*   } */

/*   SetBgColor(drawing_area, color[0]); */
/*   font = GetFont(fontname); */
  font = gdk_font_load(fontname);
/*   SetWidgetFont(drawing_area, font); */
  ClearDrawArea();
}

/* loop until finished */
void
mainloop()
{
  draw_text(1);
  animate(NULL);
/*   AddTimeOut(MSECREFRESH, animate, NULL); */
/*   MainLoop(); */
  gtk_main();
}

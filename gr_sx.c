/*
 * @(#)$Id: gr_sx.c,v 1.16 1997/06/11 01:08:07 twitham Rel $
 *
 * Copyright (C) 1996 - 1997 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the X11 (libsx) specific pieces of the display
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <libsx.h>
#include "oscope.h"		/* program defaults */
#include "display.h"
#include "func.h"
#include "file.h"

Widget draw_widget;		/* scope drawing area */
Widget file[4];			/* file menu */
Widget plot[5];			/* plot menu */
Widget grat[6];			/* graticule menu */
Widget colormenu[17];		/* color menu */
Widget xwidg[11];		/* extra horizontal widgets */
Widget mwidg[57];		/* memory / math widgets */
Widget cwidg[CHANNELS];		/* channel button widgets */
Widget ywidg[15];		/* vertical widgets */
Widget **math;			/* math menu */
int **intarray;			/* indexes of math functions */
int XX[] = {640,800,1024,1280};
int XY[] = {480,600, 768,1024};
XFont font;
char fontname[80] = DEF_FX;
char fonts[] = "xlsfonts";
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
  SetColor(fg);
  if (p == ALIGN_CENTER)
    x -= (TextWidth(f, s) / 2);
  else if (p == ALIGN_RIGHT)
    x -= (TextWidth(f, s));
  DrawText(s, x, y + FontHeight(f));
  return(1);
}

/* callback to redisplay the drawing area; snap to a graticule division */
void
redisplay(Widget w, int new_width, int new_height, void *data) {
  static int i = 2, h, v;

  h = h_points;
  v = v_points;
  h_points = new_width > 640
    ? new_width - (new_width - 200) % 44
    : 640;
  v_points = new_height > 480
    ? new_height - (new_height - 160) % 64
    : 480;
  offset = v_points / 2;
  if (h != h_points || v != v_points)
    ClearDrawArea();
  /* redo text later after widget redraws have settled down */
  AddTimeOut(200, draw_text, &i);
}

/* callback for keypress events on the scope drawing area */
void
keys(Widget w, char *input, int up_or_down, void *data)
{
  if (!up_or_down)		/* 0 press, 1 release */
    return;
  if (input[1] == '\0')		/* single character to handle */
    handle_key(input[0]);
}

/* menu option callbacks */
void
plotmode(Widget w, void *data)
{
  char *c = (char *)data;

  scope.mode = *c - '0';
  clear();
}

void
runmode(Widget w, void *data)
{
  char *c = (char *)data;

  scope.run = *c - '0';
  clear();
}

void
graticule(Widget w, void *data)
{
  char *c = (char *)data;
  int i;

  i = *c - '0';
  if (i < 2)
    scope.behind = i;
  else
    scope.grat = i - 2;
  clear();
}

void
setcolor(Widget w, void *data)
{
  int *c = (int *)data;

  scope.color = *c;
  draw_text(1);
}

void
mathselect(Widget w, void *data)
{
  int *c = (int *)data;

  if (scope.select > 1) {
    ch[scope.select].func = *c + FUNC0;
    clear();
  }
}

/* close the window */
void
dismiss(Widget w, void *data)
{
  CloseWindow();
}

/* run an external command */
void
runextern(Widget w, void *data)
{
  strcpy(ch[scope.select].command, (char *)data);
  ch[scope.select].func = FUNCEXT;
  ch[scope.select].mem = EXTSTART;
  clear();
}

/* slurp the manual page into a window */
void
help(Widget w, void *data)
{
  Widget x[2];
  char c, prev = '\0', pprev = '\0', *tmp, *s = NULL;
  FILE *p;
  int i = 0;

  if ((p = popen(HELPCOMMAND, "r")) != NULL) {
    while ((c = fgetc(p)) != EOF) {
      if (c == '\b')
	fgetc(p);
      else if (!(c == '\n' && prev == '\n' && pprev == '\n')) {
	tmp = realloc(s, i + 1);
	s = tmp;
	s[i] = c;
	i++;
      }
      pprev = prev;
      prev = c;
    }
    tmp = realloc(s, i + 1);
    s = tmp;
    s[i] = '\0';
    pclose(p);
    MakeWindow("Help", SAME_DISPLAY, NONEXCLUSIVE_WINDOW);
    x[0] = MakeButton("Dismiss", dismiss, NULL);
    x[1] = MakeTextWidget(s, FALSE, FALSE, 500, 500);
    free(s);
    SetWidgetPos(x[1], PLACE_UNDER, x[0], NO_CARE, NULL);
    ShowDisplay();
  }
}

/* simple button callback that just hits the given key */
void
hit_key(Widget w, void *data)
{
  char *c = (char *)data;

  handle_key(*c);
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
  if (font) FreeFont(font);
}

/* set current state colors, labels, and check marks on widgets */
void
fix_widgets()
{
  int i;
  char *tilt[] = {"-", "/", "\\"};

  for (i = 0 ; i < 4 ; i++) {
    SetMenuItemChecked(plot[i + 1], scope.mode == i);
  }
  SetMenuItemChecked(grat[1], !scope.behind);
  SetMenuItemChecked(grat[2], scope.behind);
  for (i = 0 ; i < 3 ; i++) {
    SetMenuItemChecked(grat[i + 3], scope.grat == i);
  }
  for (i = 0 ; i < 16 ; i++) {
    SetMenuItemChecked(colormenu[i + 1], scope.color == i);
  }
  for (i = 0 ; i < funccount - FUNC0 ; i++) {
    SetMenuItemChecked(*math[i], ch[scope.select].func == i + FUNC0);
  }

  SetBgColor(mwidg[0], ch[scope.select].color);
  SetBgColor(mwidg[27], ch[scope.select].color);
  SetBgColor(mwidg[28], ch[scope.select].color);
  SetBgColor(mwidg[29], ch[scope.select].color);
  SetBgColor(mwidg[56], ch[scope.select].color);
  SetWidgetState(mwidg[27], scope.select > 1);
  SetWidgetState(mwidg[28], scope.select > 1);
  SetWidgetState(mwidg[56], scope.select > 1);
  for (i = 0 ; i < 26 ; i++) {
    SetBgColor(mwidg[i + 30], mem[i].color);
    if (i > 22) {
      SetBgColor(mwidg[i + 1], mem[i].color);
      SetWidgetState(mwidg[i + 1], 0);
    }
  }

  SetWidgetState(xwidg[7], scope.run != 1);
  SetWidgetState(xwidg[8], scope.run != 2);
  SetWidgetState(xwidg[9], scope.run != 0);

  SetBgColor(ywidg[2], ch[scope.trigch].color);
  SetBgColor(ywidg[3], ch[scope.trigch].color);
  SetBgColor(ywidg[4], ch[scope.trigch].color);
  SetBgColor(ywidg[5], ch[scope.trigch].color);
  SetBgColor(ywidg[7], ch[scope.select].color);
  SetBgColor(ywidg[8], ch[scope.select].color);
  SetBgColor(ywidg[11], ch[scope.select].color);
  SetBgColor(ywidg[12], ch[scope.select].color);
  SetBgColor(ywidg[14], ch[scope.select].color);

  SetLabel(ywidg[3], scope.trigch ? "Y" : "X");
  SetLabel(ywidg[4], tilt[scope.trige]);
  SetLabel(ywidg[14], ch[scope.select].show ? "Hide" : "Show");

  for (i = 0 ; i < CHANNELS ; i++) {
    SetFgColor(cwidg[i], ch[i].show ? color[0] : ch[i].color);
    SetBgColor(cwidg[i], ch[i].show ? ch[i].color : color[0]);
  }
}

/* initialize all the widgets, called by init_screen in display.c */
void
init_widgets()
{
  static char *s[] = {" 1  ", " 2  ", " 3  ", " 4  ",
		      " 5  ", " 6  ", " 7  ", " 8  "};
  int i, j;

  /* top row of widgets */
  file[0] = MakeMenu("File");
  file[1] = MakeMenuItem(file[0], "Load...", hit_key, "@");
  file[2] = MakeMenuItem(file[0], "Save...", hit_key, "#");
  file[3] = MakeMenuItem(file[0], "Quit", hit_key, "\e");

  plot[0] = MakeMenu("Plot Mode");
  plot[1] = MakeMenuItem(plot[0], "Point", plotmode, "0");
  plot[2] = MakeMenuItem(plot[0], "Point Accumulate", plotmode, "1");
  plot[3] = MakeMenuItem(plot[0], "Line", plotmode, "2");
  plot[4] = MakeMenuItem(plot[0], "Line Accumulate", plotmode, "3");

  colormenu[0] = MakeMenu("Color");

  grat[0] = MakeMenu("Graticule");
  grat[1] = MakeMenuItem(grat[0], "In Front", graticule, "0");
  grat[2] = MakeMenuItem(grat[0], "Behind", graticule, "1");
  grat[3] = MakeMenuItem(grat[0], "None", graticule, "2");
  grat[4] = MakeMenuItem(grat[0], "Minor Divisions", graticule, "3");
  grat[5] = MakeMenuItem(grat[0], "Minor & Major", graticule, "4");

  xwidg[0] = MakeButton("Refresh", hit_key, "\n");
  xwidg[1] = MakeButton(" < ", hit_key, "9");
  xwidg[2] = MakeButton("<", hit_key, "(");
  xwidg[3] = MakeButton(">", hit_key, ")");
  xwidg[4] = MakeButton(" > ", hit_key, "0");
  xwidg[5] = MakeButton("SC", hit_key, "&");
  xwidg[6] = MakeButton("PS", hit_key, "^");
  xwidg[7] = MakeButton("Run", runmode, "1");
  xwidg[8] = MakeButton("Wait", runmode, "2");
  xwidg[9] = MakeButton("Stop", runmode, "0");
  xwidg[10] = MakeButton(" ? ", hit_key, "?");

  SetWidgetPos(plot[0], PLACE_RIGHT, file[0], NO_CARE, NULL);
  SetWidgetPos(grat[0], PLACE_RIGHT, plot[0], NO_CARE, NULL);
  SetWidgetPos(colormenu[0], PLACE_RIGHT, grat[0], NO_CARE, NULL);
  SetWidgetPos(xwidg[0], PLACE_RIGHT, colormenu[0], NO_CARE, NULL);
  SetWidgetPos(xwidg[1], PLACE_RIGHT, xwidg[0], NO_CARE, NULL);
  SetWidgetPos(xwidg[2], PLACE_RIGHT, xwidg[1], NO_CARE, NULL);
  SetWidgetPos(xwidg[3], PLACE_RIGHT, xwidg[2], NO_CARE, NULL);
  SetWidgetPos(xwidg[4], PLACE_RIGHT, xwidg[3], NO_CARE, NULL);
  SetWidgetPos(xwidg[5], PLACE_RIGHT, xwidg[4], NO_CARE, NULL);
  SetWidgetPos(xwidg[6], PLACE_RIGHT, xwidg[5], NO_CARE, NULL);
  SetWidgetPos(xwidg[7], PLACE_RIGHT, xwidg[6], NO_CARE, NULL);
  SetWidgetPos(xwidg[8], PLACE_RIGHT, xwidg[7], NO_CARE, NULL);
  SetWidgetPos(xwidg[9], PLACE_RIGHT, xwidg[8], NO_CARE, NULL);
  SetWidgetPos(xwidg[10], PLACE_RIGHT, xwidg[9], NO_CARE, NULL);

  /* the drawing area for the scope */
  h_points = XX[scope.size];
  v_points = XY[scope.size];
  draw_widget = MakeDrawArea(h_points, v_points, redisplay, NULL);
  SetKeypressCB(draw_widget, keys);
  SetWidgetPos(draw_widget, PLACE_UNDER, file[0], NO_CARE, NULL);

  /* right column of widgets */
  ywidg[0] = MakeButton("Help", help, NULL);

  ywidg[1] = MakeLabel("Trig");
  ywidg[2] = MakeButton(" /\\ ", hit_key, "=");
  ywidg[3] = MakeButton("<", hit_key, "_");
  ywidg[4] = MakeButton(">", hit_key, "+");
  ywidg[5] = MakeButton(" \\/ ", hit_key, "-");

  ywidg[6] = MakeLabel("Scal");
  ywidg[7] = MakeButton(" /\\ ", hit_key, "}");
  ywidg[8] = MakeButton(" \\/ ", hit_key, "{");

  ywidg[9] = MakeLabel("Chan");

  SetWidgetPos(ywidg[0],  PLACE_RIGHT, draw_widget, NO_CARE, NULL);
  SetWidgetPos(ywidg[1],  PLACE_RIGHT, draw_widget, PLACE_UNDER, ywidg[0]);
  SetWidgetPos(ywidg[2],  PLACE_RIGHT, draw_widget, PLACE_UNDER, ywidg[1]);
  SetWidgetPos(ywidg[3],  PLACE_RIGHT, draw_widget, PLACE_UNDER, ywidg[2]);
  SetWidgetPos(ywidg[4],  PLACE_RIGHT, ywidg[3], PLACE_UNDER, ywidg[2]);
  SetWidgetPos(ywidg[5],  PLACE_RIGHT, draw_widget, PLACE_UNDER, ywidg[4]);
  SetWidgetPos(ywidg[6],  PLACE_RIGHT, draw_widget, PLACE_UNDER, ywidg[5]);
  SetWidgetPos(ywidg[7],  PLACE_RIGHT, draw_widget, PLACE_UNDER, ywidg[6]);
  SetWidgetPos(ywidg[8],  PLACE_RIGHT, draw_widget, PLACE_UNDER, ywidg[7]);
  SetWidgetPos(ywidg[9],  PLACE_RIGHT, draw_widget, PLACE_UNDER, ywidg[8]);
  
  for (i = 0 ; i < CHANNELS ; i++) {
    cwidg[i] = MakeButton(s[i], hit_key, &s[i][1]);
    SetWidgetPos(cwidg[i],  PLACE_RIGHT, draw_widget,
		 PLACE_UNDER, i ? cwidg[i - 1] : ywidg[9]);
  }

  ywidg[10] = MakeLabel("Pos.");
  ywidg[11] = MakeButton(" /\\ ", hit_key, "]");
  ywidg[12] = MakeButton(" \\/ ", hit_key, "[");
  ywidg[13] = MakeLabel("");
  ywidg[14] = MakeButton("Hide", hit_key, "\t");

  SetWidgetPos(ywidg[10],  PLACE_RIGHT, draw_widget,
	       PLACE_UNDER, cwidg[CHANNELS - 1]);
  SetWidgetPos(ywidg[11],  PLACE_RIGHT, draw_widget, PLACE_UNDER, ywidg[10]);
  SetWidgetPos(ywidg[12],  PLACE_RIGHT, draw_widget, PLACE_UNDER, ywidg[11]);
  SetWidgetPos(ywidg[13],  PLACE_RIGHT, draw_widget, PLACE_UNDER, ywidg[12]);
  SetWidgetPos(ywidg[14],  PLACE_RIGHT, draw_widget, PLACE_UNDER, ywidg[13]);

  /* bottom rows of widgets */
  mwidg[0]  = MakeLabel(" Store ");
  mwidg[29] = MakeLabel(" Recall");
  SetWidgetPos(mwidg[0],  PLACE_UNDER, draw_widget, NO_CARE, NULL);
  SetWidgetPos(mwidg[29],  PLACE_UNDER, mwidg[0], NO_CARE, NULL);
  for (i = 0 ; i < 26 ; i++) {
    sprintf(error, "%c", i + 'A');
    mwidg[i + 1] = MakeButton(error, hit_key, &alphabet[i]);
    SetWidgetPos(mwidg[i + 1], PLACE_UNDER, draw_widget,
		 PLACE_RIGHT, mwidg[i]);
    sprintf(error, "%c", i + 'a');
    mwidg[i + 30] = MakeButton(error, hit_key, &alphabet[i + 26]);
    SetWidgetPos(mwidg[i + 30], PLACE_UNDER, mwidg[0],
		 PLACE_RIGHT, mwidg[i + 29]);
  }
  mwidg[27] = MakeMenu(  "Math");
  mwidg[28] = MakeButton("XY", runextern, "xy");
  mwidg[56] = MakeButton("Extern...", hit_key, "$");
  SetWidgetPos(mwidg[27],  PLACE_UNDER, draw_widget, PLACE_RIGHT, mwidg[26]);
  SetWidgetPos(mwidg[28],  PLACE_UNDER, draw_widget, PLACE_RIGHT, mwidg[27]);
  SetWidgetPos(mwidg[56],  PLACE_UNDER, mwidg[27], PLACE_RIGHT, mwidg[55]);

  j = funccount - FUNC0;
  if ((math = malloc(sizeof(Widget *) * j)) == NULL)
    nomalloc(__FILE__, __LINE__);
  if ((intarray = malloc(sizeof(int *) * (j > 16 ? j : 16))) == NULL)
    nomalloc(__FILE__, __LINE__);
  for (i = 0 ; i < (j > 16 ? j : 16) ; i++) {
    if ((intarray[i] = malloc(sizeof(int))) == NULL)
      nomalloc(__FILE__, __LINE__);
    *intarray[i] = i;
    if (i < j) {
      if ((math[i] = malloc(sizeof(Widget))) == NULL)
	nomalloc(__FILE__, __LINE__);
      *math[i] = MakeMenuItem(mwidg[27], funcnames[FUNC0 + i],
			      mathselect, intarray[i]);
    }
  }

  ShowDisplay();

  for (i = 0 ; i < 16 ; i++) {
    color[i] = GetNamedColor(colors[i]);
    colormenu[i + 1] = MakeMenuItem(colormenu[0], colors[i],
				    setcolor, intarray[i]);
  }

  for (i = 0 ; i < 26 ; i++) {
    SetBgColor(mwidg[i + 30], color[0]);
  }

  SetBgColor(draw_widget, color[0]);
  font = GetFont(fontname);
  SetWidgetFont(draw_widget, font);
  ClearDrawArea();
}

void
clear_display()
{
  ClearDrawArea();
}

/* loop until finished */
void
mainloop()
{
  draw_text(1);
  animate(NULL);
  MainLoop();
}

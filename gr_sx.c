/*
 * @(#)$Id: gr_sx.c,v 1.1 1996/02/28 06:09:06 twitham Exp $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the X11 specific pieces of the display
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "oscope.h"		/* program defaults */
#include "x11.h"
#include "display.h"

Widget draw_widget;		/* scope drawing area */
Widget file[4];			/* file menu */
Widget plot[5];			/* plot menu */
Widget grat[4];			/* graticule menu */
Widget x[7];			/* extra horizontal widgets */
Widget y[15];			/* vertical widgets */
Widget c[CHANNELS];		/* channel button widgets */
int XX[] = {640,800,1024,1280};
int XY[] = {480,600, 768,1024};
XFont font;
int color[16];
char *colors[] = {		/* X colors similar to 16 console colors */
  "black",			/* 0 */
  "blue",
  "green",			/* 2 */
  "cyan",
  "red",			/* 4 */
  "magenta",
  "orange",			/* 6 */
  "gray75",
  "gray25",			/* 8 */
  "blue4",
  "green4",			/* 10 */
  "cyan4",
  "red4",			/* 12 */
  "magenta4",
  "yellow",			/* 14 */
  "white"
};

/* a libsx text writer similar to libvgamisc's vga_write */
int
VGA_WRITE(char *s, short x, short y, XFont f, short fg, short bg, char p)
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
  h_points = new_width > 640
    ? new_width - (new_width - 200) % 44
    : 640;
  v_points = new_height > 480
    ? new_height - (new_height - 160) % 64
    : 480;
  offset = v_points / 2;
  clear();
  draw_text(1);
}

/* callback for keypress events on the scope drawing area */
void
keys(Widget w, char *input, int up_or_down, void *data)
{
  if (!up_or_down)		/* 0 press, 1 release */
    return;
  if (input[1] == '\0') {	/* single character to handle */
    handle_key(input[0]);
  }
}

/* menu option callbacks */
void
loadfile(Widget w1, void *data)
{
  char *fname;

  fname = GetString("Enter name of file to load:", "Foobar.c");
  if (fname)
     printf("You entered: %s\n", fname);
  else
    printf("You clicked cancel.\n");
}

void
savefile(Widget w, void *data)
{
  int ans;

  ans = GetYesNo("\nAre you a weenie?\n\n");

  if (ans == TRUE)
     printf("You're a weenie.\n");
  else
    printf("You are not a weenie.\n");
}

void
plotmode(Widget w, void *data)
{
  char *c = (char *)data;

  scope.mode = *c - '0';
  clear();
}

void
graticule(Widget w, void *data)
{
  char *c = (char *)data;

  scope.grat = *c - '0';
  clear();
}

/* close the window */
void
dismiss(Widget w, void *data)
{
  CloseWindow();
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

/* set current state colors, labels, and check marks on widgets */
void
fix_widgets()
{
  int i;
  char *tilt[] = {"-", "/", "\\"};

  for (i = 0 ; i < 4 ; i++) {
    SetMenuItemChecked(plot[i + 1], scope.mode == i);
  }
  for (i = 0 ; i < 3 ; i++) {
    SetMenuItemChecked(grat[i + 1], scope.grat == i);
  }

  SetBgColor(y[2], ch[scope.trigch].color);
  SetBgColor(y[3], ch[scope.trigch].color);
  SetBgColor(y[4], ch[scope.trigch].color);
  SetBgColor(y[5], ch[scope.trigch].color);

  SetLabel(y[3], scope.trigch ? "2" : "1");
  SetLabel(y[4], tilt[scope.trige]);
  SetLabel(y[8], ch[scope.select].show ? "Hide" : "Show");
  SetLabel(y[13], scope.select > 1 ? "Math" : "");

  for (i = 0 ; i < 3 ; i++) {
    SetBgColor(y[i + 7], ch[scope.select].color);
    SetBgColor(y[i + 12], ch[scope.select].color);
  }
  for (i = 0 ; i < CHANNELS ; i++) {
    SetFgColor(c[i], ch[i].show ? color[0] : ch[i].color);
    SetBgColor(c[i], ch[i].show ? ch[i].color : color[0]);
  }
}

/* initialize all the widgets, called by init_screen in display.c */
void
init_widgets()
{
  static char *s[] = {" 1  ", " 2  ", " 3  ", " 4  ",
		      " 5  ", " 6  ", " 7  ", " 8  "};
  int i;

  /* top row of widgets */
  file[0] = MakeMenu(" File ");
  file[1] = MakeMenuItem(file[0], "Load...", loadfile, NULL);
  file[2] = MakeMenuItem(file[0], "Save...", savefile, NULL);
  file[3] = MakeMenuItem(file[0], "Quit", hit_key, "\e");

  plot[0] = MakeMenu(" Plot Mode ");
  plot[1] = MakeMenuItem(plot[0], "Point", plotmode, "0");
  plot[2] = MakeMenuItem(plot[0], "Point Accumulate", plotmode, "1");
  plot[3] = MakeMenuItem(plot[0], "Line", plotmode, "2");
  plot[4] = MakeMenuItem(plot[0], "Line Accumulate", plotmode, "3");

  x[0] = MakeButton("<", hit_key, "(");
  grat[0] = MakeMenu(" Graticule ");
  grat[1] = MakeMenuItem(grat[0], "None", graticule, "0");
  grat[2] = MakeMenuItem(grat[0], "Minor Divisions", graticule, "1");
  grat[3] = MakeMenuItem(grat[0], "Minor & Major", graticule, "2");
  x[1] = MakeButton(">", hit_key, ")");

  x[2] = MakeButton(" Refresh ", hit_key, "\n");
  x[3] = MakeButton(" < ", hit_key, "9");
  x[4] = MakeLabel("Sec/Div");
  x[5] = MakeButton(" > ", hit_key, "0");
  x[6] = MakeButton(" Run/Stop ", hit_key, " ");

  SetWidgetPos(plot[0], PLACE_RIGHT, file[0], NO_CARE, NULL);
  SetWidgetPos(x[0], PLACE_RIGHT, plot[0], NO_CARE, NULL);
  SetWidgetPos(grat[0], PLACE_RIGHT, x[0], NO_CARE, NULL);
  SetWidgetPos(x[1], PLACE_RIGHT, grat[0], NO_CARE, NULL);
  SetWidgetPos(x[2], PLACE_RIGHT, x[1], NO_CARE, NULL);
  SetWidgetPos(x[3], PLACE_RIGHT, x[2], NO_CARE, NULL);
  SetWidgetPos(x[4], PLACE_RIGHT, x[3], NO_CARE, NULL);
  SetWidgetPos(x[5], PLACE_RIGHT, x[4], NO_CARE, NULL);
  SetWidgetPos(x[6], PLACE_RIGHT, x[5], NO_CARE, NULL);

  /* the drawing area for the scope */
  h_points = XX[scope.size];
  v_points = XY[scope.size];
  draw_widget = MakeDrawArea(h_points, v_points, redisplay, NULL);
  SetKeypressCB(draw_widget, keys);
  SetWidgetPos(draw_widget, PLACE_UNDER, file[0], NO_CARE, NULL);

  /* right column of widgets */
  y[0] = MakeButton("Help", help, NULL);

  y[1] = MakeLabel("Trig");
  y[2] = MakeButton(" /\\ ", hit_key, "=");
  y[3] = MakeButton("<", hit_key, "_");
  y[4] = MakeButton(">", hit_key, "+");
  y[5] = MakeButton(" \\/ ", hit_key, "-");

  y[6] = MakeLabel("Pos.");
  y[7] = MakeButton(" /\\ ", hit_key, "}");
  y[8] = MakeButton("Hide", hit_key, "\t");
  y[9] = MakeButton(" \\/ ", hit_key, "{");

  y[10] = MakeLabel("Chan");

  SetWidgetPos(y[0],  PLACE_RIGHT, draw_widget, NO_CARE, NULL);
  SetWidgetPos(y[1],  PLACE_RIGHT, draw_widget, PLACE_UNDER, y[0]);
  SetWidgetPos(y[2],  PLACE_RIGHT, draw_widget, PLACE_UNDER, y[1]);
  SetWidgetPos(y[3],  PLACE_RIGHT, draw_widget, PLACE_UNDER, y[2]);
  SetWidgetPos(y[4],  PLACE_RIGHT, y[3], PLACE_UNDER, y[2]);
  SetWidgetPos(y[5],  PLACE_RIGHT, draw_widget, PLACE_UNDER, y[4]);
  SetWidgetPos(y[6],  PLACE_RIGHT, draw_widget, PLACE_UNDER, y[5]);
  SetWidgetPos(y[7],  PLACE_RIGHT, draw_widget, PLACE_UNDER, y[6]);
  SetWidgetPos(y[8],  PLACE_RIGHT, draw_widget, PLACE_UNDER, y[7]);
  SetWidgetPos(y[9],  PLACE_RIGHT, draw_widget, PLACE_UNDER, y[8]);
  SetWidgetPos(y[10],  PLACE_RIGHT, draw_widget, PLACE_UNDER, y[9]);
  
  for (i = 0 ; i < CHANNELS ; i++) {
    c[i] = MakeButton(s[i], hit_key, &s[i][1]);
    SetWidgetPos(c[i],  PLACE_RIGHT, draw_widget,
		 PLACE_UNDER, i ? c[i - 1] : y[10]);
  }

  y[11] = MakeLabel("Scal");
  y[12] = MakeButton(" /\\ ", hit_key, "]");
  y[13] = MakeButton("Math", hit_key, ";");
  y[14] = MakeButton(" \\/ ", hit_key, "[");

  SetWidgetPos(y[11],  PLACE_RIGHT, draw_widget, PLACE_UNDER, c[CHANNELS - 1]);
  SetWidgetPos(y[12],  PLACE_RIGHT, draw_widget, PLACE_UNDER, y[11]);
  SetWidgetPos(y[13],  PLACE_RIGHT, draw_widget, PLACE_UNDER, y[12]);
  SetWidgetPos(y[14],  PLACE_RIGHT, draw_widget, PLACE_UNDER, y[13]);

  ShowDisplay();

  for (i = 0 ; i < 16 ; i++) {
    color[i] = GetNamedColor(colors[i]);
  }
  font = GetFont(fontname);
  SetWidgetFont(draw_widget, font);
  SetBgColor(draw_widget, color[0]);
}

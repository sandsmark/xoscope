/*
 * @(#)$Id: xy.c,v 1.2 1996/10/06 06:43:50 twitham Rel $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * [x]oscope external math filter command example (xy mode) in C.
 *
 * See operl for an example that can do arbitrary math at run-time.
 *
 * An external oscope command must continuously read two shorts from
 * stdin and write one short to stdout.  It must exit when stdin
 * closes.  The two shorts read in are the current input samples from
 * channel 1 & 2.  The short written out should be the desired math
 * function result.
 *
 * This example provides an X-Y display window as an external command.
 *
 * usage: xy [samples]
 *
 * The command-line argument is the number of samples per screenful.
 * You may need to use this if you resize the window.  (default = 640)
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <libsx.h>
#include "config.h"

Widget quit;			/* quit button */
Widget plot[5];			/* plot menu */
Widget draw_widget;		/* xy drawing area */
int bg, fg, mode = 0, quit_key_pressed = 0, h_points = 640;


/* clear the drawing area and reset the menu check marks */
void
clear()
{
  int i;
  for (i = 0 ; i < 4 ; i++) {
    SetMenuItemChecked(plot[i + 1], mode == i);
  }
  ClearDrawArea();
}

/* callback to redisplay the drawing area */
void
redisplay(Widget w, int new_width, int new_height, void *data) {
  clear();
}

/* callback for keypress events on the drawing area */
void
keys(Widget w, char *input, int up_or_down, void *data)
{
  if (!up_or_down)		/* 0 press, 1 release */
    return;
  if (input[1] == '\0')		/* single character to handle */
    switch (input[0]) {
    case 0:
    case -1:			/* no key pressed */
      break;
    case '\e':			/* Esc */
      quit_key_pressed = 1;
      break;
    case '\r':			/* Enter */
    case '\n':
      clear();
      break;
    case '!':
      mode++;			/* point, point accumulate, line, line acc. */
      if (mode > 3)
	mode = 0;
      clear();
      break;
    }
}

/* quit button callback */
void
dismiss(Widget w, void *data)
{
  quit_key_pressed = 1;
}

/* plot mode menu callback */
void
plotmode(Widget w, void *data)
{
  char *c = (char *)data;

  mode = *c - '0';
  clear();
}

/* get and plot one screen full of data */
void
animate(void *data)
{
  static short x, y, z = 0, X, Y, buff[sizeof(short)];
  static int i, j;

  if (quit_key_pressed)
    exit(0);

  if (!(mode % 2))
    clear();
  for (j = 0 ; j < h_points ; j++) {
    /* Read two shorts from stdin (channel 1 & 2 */
    if ((i = read(0, buff, sizeof(short))) != sizeof(short))
      exit(i);
    x = (short)(*buff);	/* get channel 1 sample, increment pointer */

    if ((i = read(0, buff, sizeof(short))) != sizeof(short))
      exit(i);
    y = (short)(*buff);	/* get channel 2 sample, increment pointer */

    if ((i = write(1, &z, sizeof(short))) != sizeof(short))
      exit(i);		/* write one short to stdout */

    if (mode < 2)
      DrawPixel(128 + x, 127 - y);
    else if (j)
      DrawLine(128 + X, 127 - Y, 128 + x, 127 - y);
    X = x; Y = y;
  }
  SyncDisplay();
  AddTimeOut(MSECREFRESH, animate, NULL);
}

int
main(int argc, char **argv)	/* main program */
{
  if ((argc = OpenDisplay(argc, argv)) == FALSE)
    exit(1);
  if (argc > 1)
    h_points = strtol(argv[1], NULL, 0);

  quit = MakeButton("Quit", dismiss, NULL);

  plot[0] = MakeMenu(" Plot Mode ");
  plot[1] = MakeMenuItem(plot[0], "Point", plotmode, "0");
  plot[2] = MakeMenuItem(plot[0], "Point Accumulate", plotmode, "1");
  plot[3] = MakeMenuItem(plot[0], "Line", plotmode, "2");
  plot[4] = MakeMenuItem(plot[0], "Line Accumulate", plotmode, "3");
  SetWidgetPos(plot[0], PLACE_RIGHT, quit, NO_CARE, NULL);

  draw_widget = MakeDrawArea(256, 256, redisplay, NULL);
  SetKeypressCB(draw_widget, keys);
  SetWidgetPos(draw_widget, PLACE_UNDER, quit, NO_CARE, NULL);

  ShowDisplay();
  bg = GetNamedColor("black");
  fg = GetNamedColor("white");
  SetBgColor(draw_widget, bg);
  SetFgColor(draw_widget, fg);
  clear();
  animate(NULL);
  MainLoop();
  exit(0);
}

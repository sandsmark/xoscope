/*
 * @(#)$Id: xy.c,v 1.1 1996/10/04 05:14:02 twitham Exp $
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

Widget draw_widget;		/* xy drawing area */
int bg, fg, mode = 0, quit_key_pressed = 0, h_points = 640;

/* callback to redisplay the drawing area; snap to a graticule division */
void
redisplay(Widget w, int new_width, int new_height, void *data) {
  ClearDrawArea();
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
      ClearDrawArea();
      break;
    case '!':
      mode++;			/* point, point accumulate, line, line acc. */
      if (mode > 3)
	mode = 0;
      ClearDrawArea();
      break;
    }
}

/* get and plot one screen full of data */
static inline void
animate(void *data)
{
  static short x, y, z = 0, X, Y, buff[sizeof(short)];
  static int i, j;

  if (quit_key_pressed)
    exit(0);

  if (!(mode % 2))
    ClearDrawArea();
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
  draw_widget = MakeDrawArea(256, 256, redisplay, NULL);
  SetKeypressCB(draw_widget, keys);
  ShowDisplay();
  bg = GetNamedColor("black");
  fg = GetNamedColor("white");
  SetBgColor(draw_widget, bg);
  SetFgColor(draw_widget, fg);
  ClearDrawArea();
  animate(NULL);
  MainLoop();
  exit(0);
}

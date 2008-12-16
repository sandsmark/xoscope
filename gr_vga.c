/*
 * @(#)$Id: gr_vga.c,v 1.5 2008/12/16 22:48:52 baccala Exp $
 *
 * Copyright (C) 1996 - 2001 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the svgalib specific pieces of the display
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vga.h>
#include "oscope.h"		/* program defaults */
#ifdef HAVE_LIBVGAMISC
#include <fontutils.h>
#include <miscutils.h>
#elif defined(HAVE_LIBVGAGL)
#include <vgagl.h>
#endif
#include "display.h"
#include "func.h"
#include "file.h"

int row(int y);		/* in display.c */

char fontname[];
int color[];
char fontname[80] = DEF_F;
char fonts[] = "/usr/lib/kbd/consolefonts";
int screen_modes_16color[] = {		/* allowed modes */
  G640x480x16,
  G800x600x16,
  G1024x768x16,
  G1280x1024x16
};
int screen_modes_256color[] = {		/* allowed modes */
  G640x480x256,
  G800x600x256,
  G1024x768x256,
  G1280x1024x256
};
int color[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

/* libvgagl can't handle 16 color modes (but there's no error return;
 * it just dies with mysterious messages).
 */

#if !defined(HAVE_LIBVGAMISC) && defined(HAVE_LIBVGAGL)
#define screen_modes screen_modes_256color
#else
#define screen_modes screen_modes_16color
#endif

#ifdef HAVE_LIBVGAMISC

font_t FONT;
void *font = &FONT;		/* font pointer for display.c */

void text_write(char *str, int x, int y, int fieldsize,
		int fgcolor, int bgcolor, int alignment)
{
  vga_write(str, col(x), row(y), font, fgcolor, bgcolor, alignment);
}

#elif defined(HAVE_LIBVGAGL)

void text_write(char *str, int x, int y, int fieldsize,
		int fgcolor, int bgcolor, int alignment)
{
  if (alignment == ALIGN_CENTER) x -= strlen(str)/2;
  else if (alignment == ALIGN_RIGHT) x -= strlen(str);
  gl_write(col(x), row(y), str);
}

#else

void
text_write(char *s, int x, int y, int fieldsize, int fg, int bg, int alignment)
{
  /* pretend we succeeded */
}

#endif

/* simulate some libsx functions for display.c */
int
OpenDisplay(int argc, char **argv)
{
  return argc;
}

void
DrawPixel(int x, int y)
{
  vga_drawpixel(x, y > 0 ? y : 0);
}

void
DrawLine(int x1, int y1, int x2, int y2)
{
  vga_drawline(x1, y1 > 0 ? y1 : 0, x2, y2 > 0 ? y2 : 0);
}

void
SetColor(int c)
{
  vga_setcolor(c);
}

void
PolyPoint(int color, Point *points, int count)
{
  int i;

  SetColor(color);
  for (i=0; i<count; i++) {
    DrawPixel(points[i].x, points[i].y);
  }
}

void
PolyLine(int color, Point *points, int count)
{
  int i;

  SetColor(color);
  for (i=1; i<count; i++) {
    DrawLine(points[i-1].x, points[i-1].y, points[i].x, points[i].y);
  }
}

/* prompt for a string */
char *
GetString(char *msg, char *def)
{
#ifdef HAVE_LIBVGAMISC
  char *s;

  s = vga_prompt(0, v_points / 2,
		 80 * 8, 8 + FONT.font_height, msg,
		 &FONT, &FONT, TEXT_FG, KEY_FG, TEXT_BG, PROMPT_SCROLLABLE);
  if (s[0] == '\e' || s[0] == '\0')
    return(NULL);
  return(s);
#else
  return(def);
#endif
}

/* get a file name */
char *
GetFile(char *label, char *path, void *func, void *data)
{
#ifdef HAVE_LIBVGAMISC
  return GetString(label, path);
#else
  return filename;
#endif
}

/* ask a yes/no question */
int
GetYesNo(char *msg)
{
#ifdef HAVE_LIBVGAMISC
  char *s;

  s = GetString(msg, "");
  if (s && (s[0] == 'y' || s[0] == 'Y'))
    return(1);
  return(0);
#else
  return(1);			/* assume yes?! since we can't prompt */
#endif
}

void AddTimeOut(unsigned long interval, void (*func)(), void *data) {}

void SyncDisplay() {}

void fix_widgets() {}

/* initialize all the widgets, called by init_screen in display.c */
void
init_widgets()
{
  vga_init();
  vga_setmode(screen_modes[scope.size]);
  v_points = vga_getydim();
  h_points = vga_getxdim();
#ifdef HAVE_LIBVGAMISC
  vga_initfont(fontname, &FONT, 1, 1);
#elif HAVE_LIBVGAGL

#define FONT_WIDTH 8
#define FONT_HEIGHT 8
#define COLOR 15

 {
   void *font;
   gl_setcontextvga(screen_modes[scope.size]);
   font = malloc(256 * FONT_WIDTH * FONT_HEIGHT * BYTESPERPIXEL);
   gl_expandfont(FONT_WIDTH, FONT_HEIGHT, COLOR, gl_font8x8, font);
   gl_setfont(FONT_WIDTH, FONT_HEIGHT, font);
 }


#endif
}

void
cleanup_display()
{
  vga_setmode(TEXT);		/* restore text screen */
}

void
clear_display()
{
  vga_clear();
}

static int inputfd = -1;

void setinputfd(int fd)
{
  inputfd = fd;
}

static struct timeval timeout = {0,0};

void settimeout(int ms)
{
  timeout.tv_usec = 1000 * ms;
}

void
mainloop(void)
{
  fd_set input, except;

  draw_text(1);
  animate(NULL);

  while (1) {

    FD_ZERO(&input);
    FD_ZERO(&except);
    if (inputfd != -1) {
      FD_SET(inputfd, &input);
      FD_SET(inputfd, &except);
    }

    if (timeout.tv_usec > 0) {
      vga_waitevent(VGA_KEYEVENT, &input, NULL, &except, &timeout);
    } else {
      vga_waitevent(VGA_KEYEVENT, &input, NULL, &except, NULL);
    }

    handle_key(vga_getkey());
    animate(NULL);
  }
}

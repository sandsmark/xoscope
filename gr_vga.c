/*
 * @(#)$Id: gr_vga.c,v 1.2 2001/05/06 03:45:16 twitham Rel $
 *
 * Copyright (C) 1996 - 2001 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the svgalib specific pieces of the display
 *
 */

#include <vga.h>
#include "oscope.h"		/* program defaults */
#ifdef HAVE_LIBVGAMISC
#include <fontutils.h>
#include <miscutils.h>
#endif
#include "display.h"
#include "func.h"
#include "file.h"

char fontname[];
int screen_modes[];
int color[];
char fontname[80] = DEF_F;
char fonts[] = "/usr/lib/kbd/consolefonts";
int screen_modes[] = {		/* allowed modes */
  G640x480x16,
  G800x600x16,
  G1024x768x16,
  G1280x1024x16
};
int color[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
#ifdef HAVE_LIBVGAMISC
font_t FONT;
void *font = &FONT;		/* font pointer for display.c */
#else
int
vga_write(char *s, short x, short y, void *f, short fg, short bg, char p)
{
  return 1;			/* pretend we succeeded */
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
#ifdef HAVE_LIBVGAMISC
  vga_initfont(fontname, &FONT, 1, 1);
#endif
  vga_setmode(screen_modes[scope.size]);
  v_points = vga_getydim();
  h_points = vga_getxdim();
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

/* loop until finished */
void
mainloop()
{
  draw_text(1);
  while (!quit_key_pressed) {
    handle_key(vga_getkey());
    animate(NULL);
  }
}

/*
 * @(#)$Id: gr_grx.c,v 1.1 1997/02/23 05:24:40 twitham Rel1_3 $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the DOS (DJGPP GRX) specific pieces of the display
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <grx20.h>
#include "oscope.h"		/* program defaults */
#include "display.h"
#include "func.h"
#include "file.h"

char fontname[80] = "";
char fonts[] = "courier,helv,tms rmn,modern,script,roman";
int gr_color = 0;
int color[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
void *font = NULL;		/* font pointer for display.c */

typedef struct {
    int  w,h;			/* allowed modes */
} gvmode;

gvmode grmodes[16] = {{640, 480}, {800, 600}, {1024, 768}, {1280, 1024}};
		     
/* a text writer similar to libvgamisc's vga_write */
int
vga_write(char *s, short x, short y, void *f, short fg, short bg, char p)
{
  static GrTextOption opt;
  static char map[3];
  static int first = 1;

  if (first) {			/* initialize first time only */
    map[ALIGN_RIGHT] = GR_ALIGN_RIGHT;
    map[ALIGN_LEFT] = GR_ALIGN_LEFT;
    map[ALIGN_CENTER] = GR_ALIGN_CENTER;
    memset(&opt,0,sizeof(opt));
    opt.txo_yalign = GR_ALIGN_TOP;
    opt.txo_direct = GR_TEXT_RIGHT;
    first--;
  }
  opt.txo_font   = font;
  opt.txo_xalign = map[(int)p];
  opt.txo_fgcolor.v = fg;
  opt.txo_bgcolor.v = bg;
  GrDrawString(s, strlen(s), x, y, &opt);
  return(1);
}

/* simulate some libsx functions */
int
OpenDisplay(int argc, char **argv)
{
  return argc;
}

void
DrawPixel(int x, int y)
{
  GrPlot(x, y, gr_color);
}

void
DrawLine(int x1, int y1, int x2, int y2)
{
  GrLine(x1, y1, x2, y2, gr_color);
}

void
SetColor(int c)
{
  gr_color = c;
}

/* prompt for a string */
char *
GetString(char *msg, char *def)
{
  static char response[128] = {128, 0, 0};

  vga_write(msg, 0, 0, font, KEY_FG, TEXT_BG, ALIGN_LEFT);
  cgets(response);
  if (response[1] < 1)
    return(NULL);
  if (response[2] == 0x1b || response[2] == '\0')
    return(NULL);
  return(response + 2);
}

/* get a file name */
char *
GetFile(char *path)
{
  return GetString("Filename:", path);
}

/* ask a yes/no question */
int
GetYesNo(char *msg)
{
  static char s[128] = {128, 0, 0};

  vga_write(msg, 0, 0, font, KEY_FG, TEXT_BG, ALIGN_LEFT);
  cgets(s);
  if (s[1] < 1)
    return(0);
  if (s[2] == 'y' || s[2] == 'Y')
    return(1);
  return(0);
}

void AddTimeOut(unsigned long interval, void (*func)(), void *data) {}

void SyncDisplay() {}

void fix_widgets() {}

int vmcmp(const void *m1,const void *m2)
{
	gvmode *md1 = (gvmode *)m1;
	gvmode *md2 = (gvmode *)m2;
	if(md1->w   != md2->w  ) return(md1->w	 - md2->w  );
	if(md1->h   != md2->h  ) return(md1->h	 - md2->h  );
	return(0);
}

/* initialize all the widgets, called by init_screen in display.c */
void
init_widgets()
{
  GrFrameMode fm;
  const GrVideoMode *mp;
  gvmode *gp;
  int i = 0;

  GrSetDriver(NULL);
  if (GrCurrentVideoDriver() == NULL)
    printf("No graphics driver found\n");
  else {			/* find available 16-color modes for -m */
    gp = grmodes;
    for (fm = GR_frameEGA4; fm <= GR_frameSVGA4; fm++) {
      for (mp = GrFirstVideoMode(fm); mp; mp = GrNextVideoMode(mp)) {
	if (mp->bpp == 4 && mp->width > 639 && mp->height > 479) {
	  gp->w	= mp->width;
	  gp->h	= mp->height;
	  gp++;
	  i++;
	}
      }
    }
    qsort(grmodes, i, sizeof(grmodes[0]), vmcmp);
    h_points = grmodes[scope.size].w;
    v_points = grmodes[scope.size].h;
    GrSetMode(GR_width_height_color_graphics, h_points, v_points, 16);
    font = &GrDefaultFont;
  }
}

void
cleanup_display()
{
  GrSetMode(GR_default_text);
}

void
clear_display()
{
  GrClearScreen(GrBlack());
}

/* loop until finished */
void
mainloop()
{
  draw_text(1);
  while (!quit_key_pressed) {
    handle_key(getch());
    animate(NULL);
  }
}

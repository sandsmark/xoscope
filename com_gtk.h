/*
 * @(#)$Id: com_gtk.h,v 1.3 2003/06/17 22:52:32 baccala Exp $
 *
 * Copyright (C) 1996 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * Some GTK widgets and event handlers common to xoscope and xy.
 *
 */

GdkColor gdkcolor[16];
GdkGC *gc;
GdkRectangle update_rect;
GdkPixmap *pixmap;
GtkWidget *drawing_area;
GtkWidget *menubar;
GtkWidget *vbox;
GtkWidget *window;
char *colors[16];
int color[16];

/* The GDK tutorial suggests doing all your graphic ops to a pixmap,
 * then copying the pixmap to the screen on an expose event.
 * Unfortunately, the pixmap copy is very slow on my X server (XFree86
 * 3.3.6 SVGA).  USE_PIXMAP determines whether we'll draw directly to
 * the screen (0) or draw to a pixmap, then copy it to the screen (1).
 * Using the pixmap is a lot slower, since a pixmap copy is time
 * consuming for an X server, but the current display code doesn't
 * work quite right without it.  Accumulate mode depends on having the
 * pixmap to keep a visual history of the signals, and
 * clearing/redrawing the signal lines looks choppy unless we do it to
 * the off-screen pixmap.  I've thought of using both pixmap and
 * direct draw together, only copying the pixmap when needed by expose
 * events, but the choppy erase/redraw code in display.c's draw_data()
 * would have to be smarter to avoid all the flickering we get now
 * without USE_PIXMAP
 */

#define USE_PIXMAP 1

#if USE_PIXMAP
#define DRAWABLE pixmap
#else
#define DRAWABLE drawing_area->window
#endif

gint expose_event();
gint key_press_event();
void delete_event();
void hit_key();

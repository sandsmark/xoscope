noinst_PROGRAMS         = \
                          basics\
                          keycontrol\
                          colors\
                          marker\
                          grid\
                          signals\
                          lissajous\
                          enable_disable\
                          addremove

basics_SOURCES          = basics.c

keycontrol_SOURCES      = keycontrol.c

colors_SOURCES          = colors.c

marker_SOURCES          = marker.c

grid_SOURCES            = grid.c

signals_SOURCES         = signals.c

lissajous_SOURCES       = lissajous.c

enable_disable_SOURCES  = enable_disable.c

addremove_SOURCES       = addremove.c

INCLUDES		= \
			-I$(top_srcdir)/gtk

AM_CFLAGS			=\
			-O2\
			-Wall -pedantic\
			-DG_DISABLE_DEPRECATED\
			-DGDK_DISABLE_DEPRECATED\
			-DGDK_PIXBUF_DISABLE_DEPRECATED\
			-DGTK_DISABLE_DEPRECATED\
			`pkg-config gtk+-2.0 --cflags`


LDADD 			= $(top_builddir)/gtk/libgtkdatabox.la\
		 	@GTK_LIBS@


## Process this file with automake to produce Makefile.in

# @(#)$Id: Makefile.am,v 2.0 2008/12/17 17:35:46 baccala Exp $

# Copyright (C) 1996 - 2001 Tim Witham <twitham@quiknet.com>

# (see the files README and COPYING for more details)

# Look at ./configure --help before tweaking this.

man_MANS = oscope.1 xoscope.1

noinst_HEADERS = com_gtk.h display.h file.h oscope.h sb.h bitscope.h	\
config.h fft.h func.h proscope.h svasync.h

bin_PROGRAMS = @XOSCOPE@ @OSCOPE@
EXTRA_PROGRAMS = xoscope oscope

pkglibexecdir = $(libexecdir)/@PACKAGE@
pkglibexec_PROGRAMS = offt xy
pkglibexec_SCRIPTS = operl ofreq.ini

EXTRA_DIST = $(man_MANS) $(noinst_HEADERS) HARDWARE Makefile.dos	\
README.f51 TODO TODO.old audio.dat bitscope.dat buff.fig buff.ps	\
buff2.fig buff2.ps count.dat isr.s ofreq.ini oscope.dat oscope.lsm	\
patch.f51 pcb.fig pcb.ps proscope.dat sb.c sb.doc sc_sb.c ser_dos.c	\
svasync.c svasync.doc

src = oscope.c file.c func.c fft.c realfft.c display.c proscope.c	\
bitscope.c sc_linux.c ser_unix.c

if COMEDI
comedisrc = comedi.c
if GTK
COMEDI_GUI_SUBDIR = comedi-glade-src
COMEDI_GUI_LIB = comedi-glade-src/interface.o comedi-glade-src/callbacks.o
endif
else
comedisrc =
endif

# Some bit of explaination is needed here for GUI_LIB.  automake won't
# let us include source files in a subdirectory on a _SOURCES line,
# and we do want all the auto-magically generated glade stuff confined
# to a subdirectory, so we're reduced to using the _LDADD line.
# However, we don't want the files included as a library, even though
# they get built as one, because then the weak attribute override
# trick used in things like comedi.c (see comments there) won't work
# for a library.  So we put .o files in.

if GTK
SUBDIRS = bitscope-glade-src $(COMEDI_GUI_SUBDIR)
INCLUDES = @GTK_CFLAGS@
GUI_LIB = bitscope-glade-src/support.o bitscope-glade-src/interface.o \
	bitscope-glade-src/callbacks.o $(COMEDI_GUI_LIB)
endif

xoscope_SOURCES = $(src) $(comedisrc) com_gtk.c gr_gtk.c sc_linux_gtk.c
xoscope_LDADD = $(GUI_LIB) @GTK_LIBS@

oscope_SOURCES = $(src) $(comedisrc) gr_com.c gr_vga.c

offt_SOURCES = offt.c fft.c realfft.c

xy_SOURCES = com_gtk.c xy_gtk.c xy.c
xy_LDADD = @GTK_LIBS@

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

## Process this file with automake to produce Makefile.in

# @(#)$Id: Makefile.am,v 1.1 2001/05/06 03:45:16 twitham Exp $

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

oscope_SOURCES = $(src) gr_com.c gr_vga.c
oscope_LDADD = -lvga

offt_SOURCES = offt.c fft.c realfft.c

if GTK
#SUBDIRS = src
INCLUDES = @GTK_CFLAGS@
xoscope_SOURCES = $(src) com_gtk.c gr_gtk.c
xoscope_LDADD = @GTK_LIBS@
xy_SOURCES = com_gtk.c xy_gtk.c xy.c
xy_LDADD = @GTK_LIBS@
else
if LIBSX
xoscope_SOURCES = $(src) gr_sx.c gr_com.c
xoscope_LDADD = @X_LIBS@ @X_PRE_LIBS@ -lsx -lXaw -lXt -lX11
xy_SOURCES = xy_sx.c xy.c
xy_LDADD = @X_LIBS@ @X_PRE_LIBS@ -lsx -lXaw -lXt -lX11
endif
endif

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


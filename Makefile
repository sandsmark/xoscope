# @(#)$Id: Makefile,v 1.23 2000/02/26 08:19:16 twitham Exp $

# Copyright (C) 1996 - 1999 Tim Witham <twitham@quiknet.com>

# (see the files README and COPYING for more details)

# !! look at the !! comments for configuration options

# we'll assume you want both console-based oscope and X11-based xoscope
SCOPES	= oscope xoscope
MISC	= -lvgamisc

# !! uncomment this line if you don't want console-based oscope
SCOPES	= xoscope

# !! uncomment this line if you don't have GTK or libsx or don't want xoscope
# SCOPES	= oscope

# !! uncomment these lines if you don't have libvgamisc from the g3vga package
# DFLAGS	= -DNOVGAMISC
# MISC	=

# we'll assume you're using /dev/dsp, but...
ESDCFLAGS=
ESDLDFLAGS=

# !! uncomment these lines if you want to use ESD instead of /dev/dsp
ESDCFLAGS	= -DESD=44100
ESDLDFLAGS	= -lesd

# !! base prefix of where to install
PREFIX	= /usr/local

# where to install the oscilloscope program(s)
BINPATH	= $(PREFIX)/bin

# where to install the manual page(s)
MANPATH	= $(PREFIX)/man/man1

# where to install and find the external math commands
LIBPATH	= $(PREFIX)/lib/oscope

# the external math commands and files to install
EXTERN	= offt operl ofreq.ini xy
# !! uncomment this if you don't have GTK or libsx (since you can't make xy)
# EXTERN	= offt operl ofreq.ini

# !! the first place to look for ProbeScope, if no PROBESCOPE environment var
PROBESCOPE = /dev/probescope

# compiler
CC	= gcc

# compiler flags; -DLIBPATH sets default value of OSCOPEPATH env variable
COMMON	= '-DLIBPATH="$(LIBPATH)"' '-DVER="$(VER)"' $(ESDCFLAGS)\
	'-DPROBESCOPE="$(PROBESCOPE)"' $(DFLAGS) -Wall -m486 -O3

# !! uncomment this to use GTK+ instead of libsx for xoscope
GTKCFLAGS = `gtk-config --cflags`
CFLAGS = $(COMMON) $(GTKCFLAGS)
GTKLDFLAGS = `gtk-config --libs`
XFLAGS = $(GTKLDFLAGS)
X_OBJ	= $(GTK_OBJ)
XY_OBJ	= com_gtk.o xy_gtk.o

# !! uncomment this to use libsx instead of GTK+ for xoscope
# CFLAGS	= $(COMMON)
# XFLAGS	= -L/usr/X11/lib -lsx -lXaw -lXt -lX11
# X_OBJ	= $(SX_OBJ)
# XY_OBJ	= xy_sx.o

# load flags, add -L/extra/lib/path if necessary
LDFLAGS	= -s $(ESDLDFLAGS) $(DFLAGS)

############################################################

# !! uncomment this for building for DOS (via DJGPP/GRX):
# SCOPES	= OSCOPE
# EXTERN	=

# !! uncomment this for compiling under DOS directly:
# CFLAGS	= '-DLIBPATH=""' '-DVER="$(VER)"' $(DFLAGS) \
# 	-I/0/djgpp/contrib/grx20/include -Wall -O3 -m486
# LDFLAGS	= -L/0/djgpp/contrib/grx20/lib

# !! uncomment this for cross-compiling under Linux:
# CC	= dos-gcc
# AR	= dos-ar
# AS	= dos-as
# CFLAGS	= '-DLIBPATH=""' '-DVER="$(VER)"' $(DFLAGS) \
# 	-I/usr/local/src/djgpp/contrib/grx20/include -Wall -O3 -m486
# LDFLAGS	= -L/usr/local/src/djgpp/contrib/grx20/lib

# nothing should need changed below here
############################################################

VER	= 1.5.3
SRC	= oscope.c file.c func.c fft.c realfft.c display.c proscope.c
VGA_SRC = $(SRC) sc_linux.c ser_unix.c gr_com.c gr_vga.c
SX_SRC	= $(SRC) sc_linux.c ser_unix.c gr_com.c gr_sx.c freq.c dirlist.c
GTK_SRC	= $(SRC) sc_linux.c ser_unix.c com_gtk.c gr_gtk.c
DOS_SRC = $(SRC) sc_sb.c sb.c ser_dos.c gr_com.c gr_grx.c

REL	= $(patsubst %,Rel%,$(subst .,_,$(VER)))

VGA_OBJ	= $(VGA_SRC:.c=.o)
SX_OBJ	= $(SX_SRC:.c=.o)
GTK_OBJ	= $(GTK_SRC:.c=.o)
DOS_OBJ	= $(DOS_SRC:.c=.o)

all:	$(SCOPES) $(EXTERN)

install:	all
	-mkdir -p $(BINPATH)
	cp $(SCOPES) $(BINPATH)
	-chmod u+s $(BINPATH)/oscope
	-chmod go-w $(BINPATH)/oscope
	-mkdir -p $(MANPATH)
	cp *.1 $(MANPATH)
	-mkdir -p $(LIBPATH)
	cp $(EXTERN) $(LIBPATH)

clean:
	$(RM) *oscope offt xy *.o core *~ *.exe *.a OSCOPE*

oscope:	$(VGA_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@ $(MISC) -lm -lvga
	chmod u+s oscope

libsv.a:	svasync.o isr.o
	$(AR) -rs libsv.a svasync.o isr.o

OSCOPE:	$(DOS_OBJ) libsv.a
	$(CC) $^ $(LDFLAGS) -o $@ -lgrx20

xoscope:	$(X_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@ -lm $(XFLAGS)

offt:	fft.o offt.o realfft.o
	$(CC) $^ $(LDFLAGS) -o $@ -lm

xy:	$(XY_OBJ) xy.o
	$(CC) $^ -o $@ $(XFLAGS)

dist:
	( cd .. ; tar --exclude oscope-$(VER)/RCS -czvf oscope-$(VER).tar.gz \
		oscope-$(VER) )

release:	clean
	-rcs -N$(REL): -sRel *
	-co -M *

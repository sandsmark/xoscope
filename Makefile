# @(#)$Id: Makefile,v 1.21 1997/06/07 21:36:54 twitham Rel $

# Copyright (C) 1996 - 1997 Tim Witham <twitham@pcocd2.intel.com>

# (see the files README and COPYING for more details)


# we'll assume you want both console-based oscope and X11-based xoscope
SCOPES	= oscope xoscope
MISC	= -lvgamisc

# configuration defines

# uncomment these lines if you don't have libvgamisc from the g3vga package
#DFLAGS	= -DNOVGAMISC
#MISC	=

# uncomment this line if you don't want console-based oscope
#SCOPES	= xoscope

# uncomment this line if you don't have libsx or if you don't want xoscope
#SCOPES	= oscope

# base prefix of where to install
PREFIX	= /usr/local

# where to install the oscilloscope program(s)
BINPATH	= $(PREFIX)/bin

# where to install the manual page(s)
MANPATH	= $(PREFIX)/man/man1

# where to install and find the external math commands
LIBPATH	= $(PREFIX)/lib/oscope

# the external math commands and files to install
EXTERN	= offt operl ofreq.ini xy

# the first place to look for ProbeScope, if no PROBESCOPE environment var
PROBESCOPE = /dev/probescope

# compiler
CC	= gcc

# compiler flags; -DLIBPATH sets default value of OSCOPEPATH env variable
CFLAGS	= '-DLIBPATH="$(LIBPATH)"' '-DVER="$(VER)"' \
	'-DPROBESCOPE="$(PROBESCOPE)"' $(DFLAGS) -Wall -O3 -m486

# load flags, add -L/extra/lib/path if necessary
LDFLAGS	= -s

# load flags to find the needed X libraries
XFLAGS	= -L/usr/X11/lib -lsx -lXaw -lXt -lX11

############################################################
# uncomment this for building for DOS (via DJGPP/GRX):
#SCOPES	= OSCOPE
#EXTERN	= 

# for compiling under DOS directly:
#CFLAGS	= '-DLIBPATH=""' '-DVER="$(VER)"' $(DFLAGS) \
#	-I/0/djgpp/contrib/grx20/include -Wall -O3 -m486
#LDFLAGS	= -L/0/djgpp/contrib/grx20/lib

# for cross-compiling under Linux:
#CC	= dos-gcc
#AR	= dos-ar
#AS	= dos-as
#CFLAGS	= '-DLIBPATH=""' '-DVER="$(VER)"' $(DFLAGS) \
#	-I/usr/local/src/djgpp/contrib/grx20/include -Wall -O3 -m486
#LDFLAGS	= -L/usr/local/src/djgpp/contrib/grx20/lib

# nothing should need changed below here
############################################################

VER	= 1.4
SRC	= oscope.c file.c func.c fft.c realfft.c display.c proscope.c
VGA_SRC = $(SRC) sc_linux.c ser_unix.c gr_vga.c
X11_SRC	= $(SRC) sc_linux.c ser_unix.c gr_sx.c freq.c dirlist.c
DOS_SRC = $(SRC) sc_sb.c sb.c ser_dos.c gr_grx.c

REL	= $(patsubst %,Rel%,$(subst .,_,$(VER)))

VGA_OBJ	= $(VGA_SRC:.c=.o)
X11_OBJ	= $(X11_SRC:.c=.o)
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

xoscope:	$(X11_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@ -lm $(XFLAGS)

offt:	fft.o offt.o realfft.o
	$(CC) $^ $(LDFLAGS) -o $@ -lm

xy:	xy.o
	$(CC) $^ -o $@ $(XFLAGS)

dist:
	( cd .. ; tar --exclude oscope-$(VER)/RCS -czvf oscope-$(VER).tar.gz \
		oscope-$(VER) )

release:	clean
	-rcs -N$(REL): -sRel *
	-co -M *

# @(#)$Id: Makefile,v 1.17 1996/10/12 07:51:37 twitham Rel1_2 $

# Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>

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

# compiler
CC	= gcc

# compiler flags; -DLIBPATH sets default value of OSCOPEPATH env variable
CFLAGS	= '-DLIBPATH="$(LIBPATH)"' $(DFLAGS) -Wall -O3 -m486

# load flags
LDFLAGS	= -s

# nothing should need changed below here
############################################################

VER	= 1.2
SRC	= oscope.c file.c func.c fft.c realfft.c display.c
VGA_SRC = $(SRC) vga.o
X11_SRC	= $(SRC) x11.c freq.c dirlist.c
XFLAGS	= -L/usr/X11/lib -lsx -lXaw -lXt -lX11

REL	= $(patsubst %,Rel%,$(subst .,_,$(VER)))

VGA_OBJ	= $(VGA_SRC:.c=.o)
X11_OBJ	= $(X11_SRC:.c=.o)

all:	$(SCOPES) $(EXTERN)

install:	all
	-mkdir -p $(BINPATH)
	cp -p $(SCOPES) $(BINPATH)
	-chmod u+s $(BINPATH)/oscope
	-chmod go-w $(BINPATH)/oscope
	-mkdir -p $(MANPATH)
	cp *.1 $(MANPATH)
	-mkdir -p $(LIBPATH)
	cp -p $(EXTERN) $(LIBPATH)

clean:
	$(RM) *oscope offt xy *.o core *~

oscope:	$(VGA_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@ $(MISC) -lm -lvga
	chmod u+s oscope

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
	rcs -N$(REL): -s$(REL) *
	co -M *

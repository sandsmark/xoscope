# @(#)$Id: Makefile,v 1.15 1996/08/03 22:42:52 twitham Rel1_1 $

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
EXTERN	= offt operl ofreq.ini

# compiler
CC	= gcc

# compiler flags; -DLIBPATH sets default value of OSCOPEPATH env variable
CFLAGS	= '-DLIBPATH="$(LIBPATH)"' $(DFLAGS) -Wall -O4 -m486

# loader
LD	= gcc

# load flags
LDFLAGS	= -s

# nothing should need changed below here
############################################################

VER	= 1.1
ALLSRC	= oscope.c file.c func.c fft.c realfft.c
SRC	= display.c $(ALLSRC)
X11_SRC	= xdisplay.c x11.c freq.c dirlist.c $(ALLSRC)

OBJ	= $(SRC:.c=.o)
X11_OBJ	= $(X11_SRC:.c=.o)

all:	$(SCOPES) $(EXTERN)

oscope:	$(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@ $(MISC) -lm -lvga
	chmod u+s oscope

xoscope:	$(X11_OBJ)
	$(CC) $(X11_OBJ) $(LDFLAGS) -o $@ \
		-lm -L/usr/X11/lib -lsx -lXaw -lXt -lX11

offt:	fft.o offt.o realfft.o
	$(CC) $^ $(LDFLAGS) -o $@ -lm

install:	$(SCOPES) $(EXTERN)
	cp -p $(SCOPES) $(BINPATH)
	chmod u+s $(BINPATH)/oscope
	chmod go-w $(BINPATH)/oscope
	cp *.1 $(MANPATH)
	mkdir -p $(LIBPATH)
	cp -p $(EXTERN) $(LIBPATH)

clean:
	$(RM) *oscope offt *.o core *~

dist:	clean
	( cd .. ; tar --exclude oscope-$(VER)/RCS -czvf oscope-$(VER).tar.gz \
		oscope-$(VER) )

# x*.c files depend on their non-x versions like this:
x%.o:	%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c x$< -o $@

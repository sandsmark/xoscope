# @(#)$Id: Makefile,v 1.9 1996/01/31 07:55:06 twitham Exp $

# Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
# Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>

# (see oscope.c and the file COPYING for more details)

# we'll assume you want both console-based oscope and X11-based xoscope
TARGET	= oscope xoscope

# configuration defines

# uncomment this line if you don't have libvgamisc from the g3fax package
#DFLAGS	= -DNOVGAMISC

# uncomment this line if you don't want console-based oscope
#TARGET = xoscope

# uncomment these 2 if you don't have libsx or if you don't want xoscope
#EFLAGS	= -DNOSX
#TARGET	= oscope

# where to install:
PREFIX	= /usr/local

# compiler
CC	= gcc

# compiler flags
CFLAGS	= $(DFLAGS) $(EFLAGS) \
		-Wall -O4 -fomit-frame-pointer -funroll-loops -m486

# loader
LD	= gcc

# load flags
LDFLAGS = -s

# nothing should need changed below here
############################################################

VER	= 0.2
SRC	= display.c oscope.c
X11_SRC	= xdisplay.c oscope.c

OBJ	= $(SRC:.c=.o)
X11_OBJ	= $(X11_SRC:.c=.o)

all:	$(TARGET)

oscope:	$(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o oscope -lvgamisc -lvga
	chmod u+s oscope

xoscope:	$(X11_OBJ)
	$(CC) $(X11_OBJ) $(LDFLAGS) -o xoscope \
		-lsx -lXaw -lXt -lX11 -L/usr/X11/lib

install: oscope
	cp -p *oscope $(PREFIX)/bin
	chmod u+s $(PREFIX)/bin/oscope
	cp *.1 $(PREFIX)/man/man1

clean:
	$(RM) *oscope *.o core *~

dist:	clean
	( cd .. ; tar --exclude oscope-$(VER)/RCS -czvf oscope-$(VER).tar.gz \
		oscope-$(VER) )

# x*.c files depend on their non-x versions like this:
x%.o:	%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) x$< -o $@

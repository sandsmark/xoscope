# @(#)$Id: Makefile,v 1.6 1996/01/23 08:00:10 twitham Exp $

# Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
# Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>

# (see scope.c and the file COPYING for more details)

# we'll assume you want both console-based scope and X11-based xscope
TARGET	= scope xscope

# configuration defines

# uncomment this line if you don't have libvgamisc from the g3fax package
#DFLAGS	= -DNOVGAMISC

# uncomment these 2 if you don't have libsx or if you don't want xscope
#EFLAGS	= -DNOSX
#TARGET	= scope

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

VER	= 1.0
SRC	= display.c scope.c
X11_SRC	= xdisplay.c scope.c

OBJ	= $(SRC:.c=.o)
X11_OBJ	= $(X11_SRC:.c=.o)

all:	$(TARGET)

scope:	$(OBJ) scope.h
	$(CC) $(OBJ) $(LDFLAGS) -o scope -lvgamisc -lvga
	chmod u+s scope

xscope:	$(X11_OBJ) scope.h
	$(CC) $(X11_OBJ) $(LDFLAGS) -o xscope \
		-lsx -lXaw -lXt -lX11 -L/usr/X11/lib

install: scope
	cp -p *scope $(PREFIX)/bin
	chmod u+s $(PREFIX)/bin/scope
	cp scope.1 $(PREFIX)/man/man1/scope.1

clean:
	$(RM) *scope *.o core *~

dist:	clean
	( cd .. ; tar --exclude scope-$(VER)/RCS -czvf scope-$(VER).tar.gz \
		scope-$(VER) )

# x*.c files depend on their non-x versions like this:
x%.o:	%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) x$< -o $@

# @(#)$Id: Makefile,v 1.10 1996/02/04 22:01:23 twitham Exp $

# Copyright (C) 1994 Jeff Tranter (Jeff_Tranter@Mitel.COM)
# Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>

# (see oscope.c and the file COPYING for more details)

# we'll assume you want both console-based oscope and X11-based xoscope
TARGET	= oscope xoscope
MISC	= -lvgamisc

# configuration defines

# uncomment these lines if you don't have libvgamisc from the g3vga package
#DFLAGS	= -DNOVGAMISC
#MISC	=

# uncomment this line if you don't want console-based oscope
#TARGET	= xoscope

# uncomment this line if you don't have libsx or if you don't want xoscope
#TARGET	= oscope

# where to install (/bin and /man/man1 will be appended):
PREFIX	= /usr/local

# compiler
CC	= gcc

# compiler flags
CFLAGS	= $(DFLAGS) -Wall -O4 -fomit-frame-pointer -funroll-loops -m486

# loader
LD	= gcc

# load flags
LDFLAGS	= -s

# nothing should need changed below here
############################################################

VER	= 0.3
SRC	= display.c oscope.c func.c
X11_SRC	= xdisplay.c oscope.c func.c

OBJ	= $(SRC:.c=.o)
X11_OBJ	= $(X11_SRC:.c=.o)

all:	$(TARGET)

oscope:	$(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o oscope $(MISC) -lvga
	chmod u+s oscope

xoscope:	$(X11_OBJ)
	$(CC) $(X11_OBJ) $(LDFLAGS) -o xoscope \
		-lsx -lXaw -lXt -lX11 -L/usr/X11/lib

install: all
	cp -p *oscope $(PREFIX)/bin
	chmod u+s $(PREFIX)/bin/oscope
	chmod go-w $(PREFIX)/bin/oscope
	cp *.1 $(PREFIX)/man/man1

clean:
	$(RM) *oscope *.o core *~

dist:	clean
	( cd .. ; tar --exclude oscope-$(VER)/RCS -czvf oscope-$(VER).tar.gz \
		oscope-$(VER) )

# x*.c files depend on their non-x versions like this:
x%.o:	%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) x$< -o $@

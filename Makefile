# @(#)$Id: Makefile,v 1.14 1996/04/21 02:38:06 twitham Rel1_0 $

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

# we'll assume you want the example external math commands
TARGET = $(SCOPES) oscope.fft oscope.perl

# uncomment this line if you don't want the example external programs
#TARGET = $(SCOPES)

# where to install (/bin and /man/man1 will be appended)
PREFIX	= /usr/local

# compiler
CC	= gcc

# compiler flags
CFLAGS	= $(DFLAGS) -Wall -O4 -m486

# loader
LD	= gcc

# load flags
LDFLAGS	= -s

# nothing should need changed below here
############################################################

VER	= 1.0
ALLSRC	= oscope.c file.c func.c fft.c realfft.c
SRC	= display.c $(ALLSRC)
X11_SRC	= xdisplay.c x11.c freq.c dirlist.c $(ALLSRC)

OBJ	= $(SRC:.c=.o)
X11_OBJ	= $(X11_SRC:.c=.o)

all:	$(TARGET)

oscope:	$(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@ $(MISC) -lm -lvga
	chmod u+s oscope

xoscope:	$(X11_OBJ)
	$(CC) $(X11_OBJ) $(LDFLAGS) -o $@ \
		-lm -lsx -L/usr/X11/lib -lXaw -lXt -lX11

oscope.fft:	fft.o oscopefft.o realfft.o
	$(CC) $^ $(LDFLAGS) -o $@ -lm

install:	$(TARGET)
	cp -p $^ $(PREFIX)/bin
	chmod u+s $(PREFIX)/bin/oscope
	chmod go-w $(PREFIX)/bin/oscope
	cp *.1 $(PREFIX)/man/man1

clean:
	$(RM) *oscope oscope.fft *.o core *~

dist:	clean
	( cd .. ; tar --exclude oscope-$(VER)/RCS -czvf oscope-$(VER).tar.gz \
		oscope-$(VER) )

# x*.c files depend on their non-x versions like this:
x%.o:	%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c x$< -o $@

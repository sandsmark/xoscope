# where to install:
PREFIX	= /usr/local

VER	= 1.0

CC	= gcc
CFLAGS	= -Wall -O2 -fomit-frame-pointer
LD	= gcc
LDFLAGS = -s

SRC	= scope.c
X11_SRC	= xscope.c

OBJ	= $(SRC:.c=.o)
X11_OBJ	= $(X11_SRC:.c=.o)

all:	scope xscope

scope:	$(OBJ) scope.h
	$(CC) $(OBJ) $(LDFLAGS) -o scope -lvgamisc -lvga
	chmod u+s scope

xscope:	$(X11_OBJ) scope.h
	$(CC) $(X11_OBJ) $(LDFLAGS) -o xscope \
		-lsx -lXaw -lXt -lX11 -L/usr/X11/lib

install: scope
	cp -p scope $(PREFIX)/bin/scope
	chmod u+s $(PREFIX)/bin/scope
	cp scope.1 $(PREFIX)/man/man1/scope.1

clean:
	$(RM) scope xscope *.o core *~

dist:	clean
	( cd .. ; tar --exclude scope-$(VER)/RCS -czvf scope-$(VER).tar.gz \
		scope-$(VER) )

# x*.c files depend on their non-x versions like this:
x%.o:	%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) x$< -o $@

PREFIX	= /usr/local

VER	= 1.0

CC	= gcc
CFLAGS	= -Wall -O2 -fomit-frame-pointer
LD	= gcc
LDFLAGS = -s

scope:	scope.o
	$(LD) $(LDFLAGS) -o scope scope.o -lvga
	chmod u+s scope

scope.o: scope.c scope.h
	$(CC) $(CFLAGS) -c scope.c

install: scope
	cp scope $(PREFIX)/bin/scope
	chmod u+s $(PREFIX)/bin/scope
	cp scope.1 $(PREFIX)/man/man1/scope.1

clean:
	$(RM) scope scope.o core *~

dist:	scope.c scope.h Makefile scope.1 COPYING README
	$(RM) scope scope.o core scope*.tar.gz *~
	tar -C.. --exclude scope-$(VER)/RCS -czvf scope-$(VER).tar.gz \
		scope-$(VER)

CC	= gcc
CFLAGS	= -Wall -O2 -fomit-frame-pointer
LD	= gcc
LDFLAGS = -s

scope:	scope.o
	$(LD) $(LDFLAGS) -o scope scope.o -lvga

scope.o: scope.c scope.h
	$(CC) $(CFLAGS) -c scope.c

install: scope
	cp scope /usr/local/bin/scope
	chmod u+s /usr/local/bin/scope
	cp scope.1 /usr/local/man/man1/scope.1

clean:
	$(RM) scope scope.o core *~

dist:	scope.c Makefile scope.1 COPYING README
	$(RM) scope scope.o core scope-0.1.tar.gz *~
	tar -C.. -czvf scope-0.1.tar.gz scope-0.1

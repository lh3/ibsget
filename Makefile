CC=			gcc
CFLAGS=		-g -Wall -O2 -Wno-unused-function
CPPFLAGS=
INCLUDES=	
OBJS=		ibsget.o kurl.o kson.o
PROG=		ibsget
LIBS=		-lcurl

.SUFFIXES:.c .o

.c.o:
		$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $< -o $@

all:$(PROG)

ibsget:$(OBJS)
		$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
		rm -fr gmon.out *.o ext/*.o a.out $(PROG) *~ *.a *.dSYM session*

depend:
		(LC_ALL=C; export LC_ALL; makedepend -Y -- $(CFLAGS) $(DFLAGS) -- *.c)
# DO NOT DELETE

ibsget.o: kurl.h kson.h
kson.o: kson.h
kurl.o: kurl.h

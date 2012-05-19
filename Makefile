.PHONY : log.o rdwr.o llist.o chatsrv.o chatsrv 

# Set compiler to use
CC=gcc
CFLAGS=
DEBUG=1

ifeq ($(DEBUG),1)
	CFLAGS+=-g
else
	CFLAGS+=-O2
endif

chatsrv : log.o llist.o chatsrv.o
	$(CC) $(CFLAGS) -o chatsrv log.o rdwr.o llist.o chatsrv.o -lpthread

chatsrv.o : log.o llist.o
	$(CC) $(CFLAGS) -c chatsrv.c -o chatsrv.o

llist.o: rdwr.o
	$(CC) $(CFLAGS) -c llist.c -o llist.o

rdwr.o:
	$(CC) $(CFLAGS) -c rdwr.c -o rdwr.o

log.o :
	$(CC) $(CFLAGS) -c log.c -o log.o

clean : 
	rm -f chatsrv
	rm -f *.o
	rm -f *~

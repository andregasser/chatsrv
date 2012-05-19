.PHONY : log.o dns.o dnsmap.o dnsmap 

# Set compiler to use
CC=gcc
CFLAGS=
DEBUG=1

ifeq ($(DEBUG),1)
	CFLAGS+=-g
else
	CFLAGS+=-O2
endif

chatsrv : log.o chatsrv.o
	$(CC) $(CFLAGS) -o chatsrv log.o chatsrv.o -lpthread

chatsrv.o : log.o
	$(CC) $(CFLAGS) -c chatsrv.c -o chatsrv.o

log.o :
	$(CC) $(CFLAGS) -c log.c -o log.o

clean : 
	rm -f chatsrv
	rm -f *.o
	rm -f *~

#! /bin/sh

tar --create --file=chatsrv-0.5.tar chatsrv.c llist2.c llist2.h log.c log.h bool.h colors.h Makefile COPYING README
gzip chatsrv-0.5.tar

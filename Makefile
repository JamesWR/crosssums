CC = gcc
CINCLUDES = `pkg-config --cflags gtk+-2.0`
CLIBS = `pkg-config --libs gtk+-2.0`
COPTS = $(CINCLUDES) $(CLIBS)

targets:	crosssums crosssumsprint

crosssums:	crosssums.c
	$(CC) -o crosssums crosssums.c $(COPTS)

crosssumsprint:	crosssumsprint.c
	$(CC) -o crosssumsprint crosssumsprint.c $(COPTS)


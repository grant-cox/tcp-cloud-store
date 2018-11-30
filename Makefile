# CC = g++
CC = gcc

CFLAGS = -Wall -I . -g
LDLIBS = -lpthread

PROGS =	client\
	server
EXES = mycloudclient mycloudserver

all: $(PROGS)
	mv client mycloudclient && mv server mycloudserver
$(PROGS): csapp.o proj4.o
csapp.o: csapp.c csapp.h
proj4.o: csapp.o proj4.h proj4.c

# Programs that need more than one .o file
server: server.o csapp.o proj4.o
client: client.o csapp.o proj4.o

clean:
	rm -f $(EXES) *.o *~

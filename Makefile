.PHONY: all clean testserver testclient

CC=gcc
CFLAGS=-lpthread
objects= bibserver bibclient

all: bibserver bibclient
	rm -f freeBook.o bookToRecord.o unboundedqueue.o bibclient.o bibserver.o

bibserver: bibserver.o unboundedqueue.o bookToRecord.o freeBook.o
	gcc -o bibserver bibserver.o freeBook.o bookToRecord.o unboundedqueue.o -lpthread

bibserver.o: bibserver.c bibserver.h 
	gcc -c bibserver.c -o bibserver.o -lpthread

bibclient: bibclient.o freeBook.o bookToRecord.o structures.h
	gcc -o bibclient bibclient.o freeBook.o bookToRecord.o

bibclient.o: bibclient.c structures.h
	gcc -c bibclient.c -o bibclient.o

freeBook.o: freeBook.c freeBook.h
	gcc -c freeBook.c -o freeBook.o

bookToRecord.o: bookToRecord.c bookToRecord.h
	gcc -c bookToRecord.c -o bookToRecord.o

unboundedqueue.o: unboundedqueue.c unboundedqueue.h
	gcc -c unboundedqueue.c -o unboundedqueue.o

testserver:	bibserver
	echo "running server"
	./bibserver Vercelli bib1.txt 3

testclient: bibclient
	echo "running client"
	./bibclient --editore="EQR"

clean:
	rm -f *.o bibserver bibclient

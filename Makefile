myProg: bibserver bibclient
	rm -f freeBook.o bookToRecord.o unboundedqueue.o bibclient.o bibserver.o

bibserver: bibserver.o unboundedqueue.o bookToRecord.o freeBook.o
	gcc bibserver.o freeBook.o bookToRecord.o unboundedqueue.o -o bibserver -lpthread

bibserver.o: bibserver.c bibserver.h 
	gcc -c bibserver.c -o bibserver.o -lpthread

bibclient: bibclient.o freeBook.o bookToRecord.o structures.h
	gcc bibclient.o freeBook.o bookToRecord.o -o bibclient

bibclient.o: bibclient.c structures.h
	gcc -c bibclient.c -o bibclient.o

freeBook.o: freeBook.c freeBook.h
	gcc -c freeBook.c -o freeBook.o

bookToRecord.o: bookToRecord.c bookToRecord.h
	gcc -c bookToRecord.c -o bookToRecord.o

unboundedqueue.o: unboundedqueue.c unboundedqueue.h
	gcc -c unboundedqueue.c -o unboundedqueue.o

clean:
	rm -f *.o bibserver bibclient

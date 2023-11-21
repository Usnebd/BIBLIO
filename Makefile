.PHONY: all clean testserver testclient

CC=gcc
CFLAGS=-lpthread
objects=bibserver bibclient
garbage=freeBook.o bookToRecord.o unboundedqueue.o bibclient.o bibserver.o

all: $(objects)
	rm -f $(garbage)

bibserver: bibserver.o unboundedqueue.o bookToRecord.o freeBook.o
	$(CC) -o $@ $^ $(CFLAGS)

bibserver.o: bibserver.c bibserver.h 
	$(CC) -c $< -o $@ $(CFLAGS)

bibclient: bibclient.o freeBook.o bookToRecord.o structures.h
	$(CC) -o $@ $^

bibclient.o: bibclient.c structures.h
	$(CC) -c $< -o $@

freeBook.o: freeBook.c freeBook.h
	$(CC) -c $< -o $@

bookToRecord.o: bookToRecord.c bookToRecord.h
	$(CC) -c $< -o $@

unboundedqueue.o: unboundedqueue.c unboundedqueue.h
	$(CC) -c $< -o $@

test: $(objects)
	cat /dev/null > bib.conf
	rm -f $(garbage)
	echo "running test"
	./bibserver Pisa bib1.txt 5 &
	./bibserver Carrara bib2.txt 4 &
	./bibserver Siena bib3.txt 3 &
	./bibserver Arezzo bib4.txt 2 &
	./bibserver Firenze bib5.txt 1 &
	sleep 1
	./testclient.sh
	pkill -f -SIGINT "bibserver"
	sleep 10
	./bibaccess.sh --query Pisa.log Carrara.log Siena.log Arezzo.log Firenze.log
clean:
	rm -f *.o *.log $(objects)
	cat /dev/null > bib.conf

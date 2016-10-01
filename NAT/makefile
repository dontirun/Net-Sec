##Start of Makefile##

all: DNSClient NATServer
	make clean
DNSClient: DNSClient.o
	gcc -o DNSClient DNSClient.o
NATServer: NATServer.o LinkedList.o
	gcc -o NATServer NATServer.o LinkedList.o -lpthread
LinkedList.o: LinkedList.h LinkedList.c
	gcc -Wall -c -g LinkedList.c
DNSClient.o: DNSClient.c
	gcc -Wall -c -g DNSClient.c
NATServer.o: NATServer.c
	gcc -Wall -c -g NATServer.c
clean:
	rm *.o

##End of makefile##

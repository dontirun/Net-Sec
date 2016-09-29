##Start of Makefile##

all: DNSClient NATServer
	make clean
DNSClient: DNSClient.o
	gcc -o DNSClient DNSClient.o
NATServer: NATServer.o
	gcc -o NATServer NATServer.o
DNSClient.o: DNSClient.c
	gcc -Wall -c -g DNSClient.c
NATServer.o: NATServer.c
	gcc -Wall -c -g NATServer.c
clean:
	rm *.o

##End of makefile##

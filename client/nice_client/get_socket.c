/*
common.c
Written by: Jesse Earisman
Date: 18-9-2014

Methods common to client.c and server.c
right now, it is just the get_socket() function, but any future functions that are used
by both client.c and server.c should go in here
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "get_socket.h"
#include "dnsutils.h"


/*
	 Given a host and a destination port, gets a socket

	 This function uses getaddrinfo to find an appropriate socket
	 On each result getaddrinfo returns, it creates a socket with the results, and checks
	 to see if that is a valid socket.
	 If it is a valid socket, it tries to connect.
	 If either of these steps fails, it keeps trying until it has exhausted the list.
	 If no results are good, it returns -1
	 Otherwise, retruns 0

	 I found I was duplicating a lot of code between server and client, so everything is
	 now part of get_socket()
	 servers call this function with is_server equal to 1
	 clients call it with is_server == 0

	 @param host: the hostname e.g. "www.cnn.com" or NULL
	 @param port: the port we want to connect on e.g. "80"
	 @param s: where we put the socket we successfully connect to
	 @param is_server: determines whether we use bind or connect

	 @return: -1 on failure, 0 on success
 */



int get_socket( const char* host, const char* port, int* s, const char* iface) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	struct in_addr in = resolve( (unsigned char*)host );
	return get_socket_by_in_addr( in, htons( atoi( port ) ), s, iface );
}

int get_socket_by_in_addr( struct in_addr in, uint16_t port, int* s, const char* iface ) {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr = in;
	addr.sin_port = port;
	memset( &(addr.sin_zero), 0, 8 );

	if( in.s_addr == -1 ) {
		perror( "resolve\n" );
		return -1;
	}

	*s = socket( AF_INET, SOCK_STREAM, 0 );
	if(*s == -1) {
		perror("socket\n");
		return -1;
	}

	int yes = 0;
	//Allows us to reuse ports as they wait for the kernel to clear them
	if( setsockopt(*s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ) {
		perror( "SO_REUSEADDR\n" );
		return -1;
	}
	if( bind_to_iface( *s, iface ) ) {
		perror( "SO_BINDTODEVICE\n" );
		return -1;
	}

	if( connect(*s, (struct sockaddr*)&addr, sizeof( addr ) ) ) {
		close(*s);
		perror( "connect\n" );
		return -1;
	}
	return 0; //we did it
}

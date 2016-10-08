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

#include "include/dnsutils.h"
#include "include/get_socket.h"


/**
 * get socket by an in_addr
 * port should be in network byte order
 */
int get_socket_by_in_addr( struct in_addr in, uint16_t port, int* s, struct in_addr srcip ) {
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
	if( bind_to_addr( *s, srcip ) ) {
		return -1;
	}

	if( connect(*s, (struct sockaddr*)&addr, sizeof( addr ) ) ) {
		close(*s);
		perror( "connect\n" );
		return -1;
	}
	return 0; //we did it
}

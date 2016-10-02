/*
server.c
Written by: Jesse Earisman
Date: 18-9-2014

A basic http client
Given a url and a port, this program will send an http GET request to that
address, and print the webpage it returns to stdout

if the -p flag is given, this program also calculates the round trip time
*/

#include <string.h>
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
#include <time.h>

#include "get_socket.h"

int get_rtt( const char* host, const char* port, const char* file, const char* iface, uint32_t* rtt );

#define NUM_TRIALS 10


/*
	 this is the client for the simple http client/server
	 a lot ended up in main(), so i'll comment as i go
 */
int main(int argc, char* argv[]) {
	if( argc != 3 ) {
		return -1;
	}

	//Retrieve hostname and what file we want, if nothing is present after the hostname, assume we wanted /
	char* host = strdup( argv[argc-2] );
	char* port = strdup( argv[argc-1] );
	char* file = strchr(host, '/');
	if(file) {
		*file = '\0';
		++file;
	} else {
		file = "";
	}
	uint32_t rtt;
	uint32_t rtt_max = 0x0;
	uint32_t rtt_min = 0xffffffff;
	uint32_t rtt_sum = 0;
	uint32_t errs = 0;
	for( int i = 0; i < NUM_TRIALS; i++ ) {
		int err = get_rtt( host, port, file, "wlp9s0", &rtt );
		if( err ) {
			perror( "rtt" );
			errs++;
		}
		rtt_max = rtt > rtt_max ? rtt : rtt_max;
		rtt_min = rtt < rtt_min ? rtt : rtt_min;
		rtt_sum += rtt;
	}
	free( host );
	free( port );
	printf( "Avg : %dus\n", rtt_sum/NUM_TRIALS );
	printf( "Min : %dus\n", rtt_min );
	printf( "Max : %dus\n", rtt_max );
	printf( "There were %d errors\n", errs );
}


int get_rtt( const char* host, const char* port, const char* file, const char* iface, uint32_t* rtt ) {
	struct timespec t_start;
	struct timespec t_end;

	//put the entire request in one string, and store it for later
	char input[100 + strlen(host) + strlen(file)];
	input[0] = '\0';
	sprintf(input, "GET /%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: curl/1.0\r\nConnection: close\r\n\r\n", file, host);

	int s;
	int err = get_socket(host, port, &s, "wlp9s0");
	if( err ) {
		return -1;
	}
	clock_gettime(CLOCK_MONOTONIC, &t_start);

	//send the request we made earlier, it should resemple GET /[filename] HTTP/1.1
	// with some extra header options like hostname and a basic user agent
	send(s, input, strlen(input), 0);

	//Prepare for the server to give us a webpage
	char byte_buffer;
	//We get the header one character at a time, until we find the \r\n\r\n
	// if we get more than one, we risk receiving some of the body, which makes our lives 
	// harder later on
	err = recv(s, &byte_buffer, 1, 0);
	close(s);
	if( err == -1 || err == 0 ) {
		perror( "recv\n" );
		return -1;
	}

	clock_gettime(CLOCK_MONOTONIC, &t_end);
	*rtt = 1000000*(t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec)/10000;
	return 0;
}

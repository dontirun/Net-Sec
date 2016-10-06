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
#include <curl/curl.h>

#include "include/get_socket.h"
#include "include/nice_client.h"

CURL* setup_curl( uint16_t nport, const char* iface );
int get_rtt_curl(CURL* handle, const unsigned char* host, struct DNS_RESOLVER* res, uint32_t* rtt );

void* spawn_client( void* arg ) {
	struct client_in* in = (struct client_in*)arg;
	int i;
	uint32_t rtt_sum = 0;
	uint32_t errs = 0;
	CURL* handle = setup_curl( in->nport, in->iface );
	if( handle == NULL ) {
		struct client_out* out = malloc( sizeof( *out ) );
		out->retcode = -1;
		out->errs = in->num_trials;
		out->avg_rtt = 0;
		return out;
	}

	for( i = 0; i < in->num_trials; i++ ) {
		uint32_t rtt = 0;
		if( get_rtt_curl( handle, in->host, in->res, &rtt ) ) {
			errs++;
		}
		rtt_sum += rtt;
		sleep( 1 );
	}

	struct client_out* out = malloc( sizeof( *out ) );
	out->retcode = 0;
	out->errs = errs;
	out->avg_rtt = rtt_sum / (in->num_trials - errs);
	return out;
}

int sockopt_callback( void* clientp, curl_socket_t curlfd, curlsocktype purpose ) {
	char* iface = (char*)clientp;
	int yes = 0;
	if( setsockopt(curlfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ) {
		perror( "SO_REUSEADDR\n" );
		return -1;
	}
	if( bind_to_iface( curlfd, iface ) ) {
		perror( "SO_BINDTODEVICE\n" );
		return -1;
	}
	return 0;
}

size_t write_callback( char* ptr, size_t size, size_t nmemb, void* userdata ) {
	// do nothing
	return size*nmemb; //tell curl we did everything
}

/**
 * Sets up a handler with everything but the url
 */
CURL* setup_curl( uint16_t nport, const char* iface ) {
	CURL* handle = curl_easy_init( );
	if( handle == NULL ) {
		return NULL;
	}
	//char* a_ip = inet_ntoa( addr );
	//TODO error checking
	CURLcode err;
	err = curl_easy_setopt( handle, CURLOPT_SOCKOPTFUNCTION, sockopt_callback );
	if( err != CURLE_OK ) { return NULL; }
	err = curl_easy_setopt( handle, CURLOPT_SOCKOPTDATA, iface );
	if( err != CURLE_OK ) { return NULL; }
	err = curl_easy_setopt( handle, CURLOPT_PORT, ntohs( nport ) );
	if( err != CURLE_OK ) { return NULL; }
	err = curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, write_callback );
	if( err != CURLE_OK ) { return NULL; }
	return handle;
}

int get_rtt_curl(CURL* handle, const unsigned char* host, struct DNS_RESOLVER* res, uint32_t* rtt ) {
	struct timespec t_start;
	struct timespec t_end;
	CURLcode err;

	clock_gettime(CLOCK_MONOTONIC, &t_start);
	struct in_addr n_addr = resolve( res, host );
	err = curl_easy_setopt( handle, CURLOPT_URL, inet_ntoa( n_addr ) );
	if( err != CURLE_OK ) { return -1; }
	err = curl_easy_perform( handle );
	if( err != CURLE_OK ) { return -1; }
	clock_gettime(CLOCK_MONOTONIC, &t_end);
	*rtt = 1000000*(t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec)/10000;

	return 0;
}

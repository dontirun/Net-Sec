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

CURL* setup_curl( uint16_t nport, struct in_addr srcip );
int get_rtt_curl(CURL* handle, const unsigned char* host, struct DNS_RESOLVER* res, uint32_t* rtt );

void* spawn_client( void* arg ) {
	struct client_in* in = (struct client_in*)arg;
	int i;
	uint32_t rtt_sum = 0;
	uint32_t errs = 0;
	CURL* handle = setup_curl( in->nport, in->srcip );
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
		usleep( 1000 );
	}
	curl_easy_cleanup( handle );

	struct client_out* out = malloc( sizeof( *out ) );
	out->retcode = 0;
	out->errs = errs;
	if( errs == in->num_trials ) {
		out->avg_rtt = 0;
	} else {
		out->avg_rtt = rtt_sum / (in->num_trials - errs);
	}
	return out;
}

int sockopt_callback( void* clientp, curl_socket_t curlfd, curlsocktype purpose ) {
	int yes = 0;
	if( setsockopt(curlfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ) {
		perror( "SO_REUSEADDR\n" );
		return -1;
	}
	return 0;
}

size_t write_callback( char* ptr, size_t size, size_t nmemb, void* userdata ) {
	if( strstr( ptr, "natdaemon" ) == NULL ) { //failure
		printf("No natdaemon found\n");
		return -1;
	}
	// We should verify data here TODO
	return size*nmemb;
}

/**
 * Sets up a handler with everything but the url
 */
CURL* setup_curl( uint16_t nport, struct in_addr srcip ) {
	CURL* handle = curl_easy_init( );
	if( handle == NULL ) {
		return NULL;
	}
	char* iface_str = inet_ntoa( srcip );
	CURLcode err;
	err = curl_easy_setopt( handle, CURLOPT_SOCKOPTFUNCTION, sockopt_callback );
	if( err != CURLE_OK ) { return NULL; }
	err = curl_easy_setopt( handle, CURLOPT_INTERFACE, iface_str );
	if( err != CURLE_OK ) { return NULL; }
	err = curl_easy_setopt( handle, CURLOPT_PORT, ntohs( nport ) );
	if( err != CURLE_OK ) { return NULL; }
	err = curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, write_callback );
	if( err != CURLE_OK ) { return NULL; }
	err = curl_easy_setopt( handle, CURLOPT_TIMEOUT_MS, 2000 );
	if( err != CURLE_OK ) { return NULL; }
	err = curl_easy_setopt( handle, CURLOPT_CONNECTTIMEOUT, 2 );
	if( err != CURLE_OK ) { return NULL; }
	err = curl_easy_setopt( handle, CURLOPT_NOSIGNAL, 1 );
	if( err != CURLE_OK ) { return NULL; }
	return handle;
}

int get_rtt_curl(CURL* handle, const unsigned char* host, struct DNS_RESOLVER* res, uint32_t* rtt ) {
	struct timespec t_start;
	struct timespec t_end;
	CURLcode err;

	clock_gettime(CLOCK_MONOTONIC, &t_start);
	struct in_addr n_addr = resolve( res, host );
	if( n_addr.s_addr == -1 ) {
		return -1;
	}
	char addr_str[20];
	inet_ntop( AF_INET, &n_addr, addr_str, 20 );
	err = curl_easy_setopt( handle, CURLOPT_URL, addr_str );
	if( err != CURLE_OK ) { return -1; }
	err = curl_easy_perform( handle );
	if( err != CURLE_OK ) { 
		//perror( "curl_easy_perform" );
		return -1; 
	}
	clock_gettime(CLOCK_MONOTONIC, &t_end);
	*rtt = 1000000*(t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec)/10000;

	return 0;
}

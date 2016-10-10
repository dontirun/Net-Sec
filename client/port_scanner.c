#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>
#include <curl/curl.h>
#include <pthread.h>

#include "include/port_scanner.h"
#include "include/dnsutils.h"
#include "include/nice_client.h"

struct scan_addr_in {
	struct in_addr to_scan;
	uint16_t port;
	struct in_addr srcip;
};

size_t pwrite_callback( char* ptr, size_t size, size_t nmemb, void* userdata ) {
	if( strstr( ptr, "natdaemon" ) == NULL ) { //failure
		printf("No natdaemon found\n");
		return -1;
	}
	// We should verify data here TODO
	return size*nmemb;
}



void* scan_addr( void* arg ) {
	struct scan_addr_in* in = (struct scan_addr_in*)arg;

	CURL* handle = curl_easy_init( );
	if( handle == NULL ) {
		return NULL;
	}
	int* ret = malloc( sizeof( *ret ) );
	char iface_str[20];
	char to_scan_str[20];
	if( inet_ntop( AF_INET, &(in->to_scan), to_scan_str, 20 ) == NULL ) {
		*ret = -1; goto finish;
	}
	if( inet_ntop( AF_INET, &(in->srcip), iface_str, 20 ) == NULL ) {
		*ret = -1; goto finish;
	}
	CURLcode err;

	//There should be an easier way to do this, like not checking errers...

	//sockopt_callback just sets reusable sockets, it's defined in nice_client.c
	err = curl_easy_setopt( handle, CURLOPT_SOCKOPTFUNCTION, sockopt_callback );
	if( err != CURLE_OK ) { *ret = -1; goto finish; }
	err = curl_easy_setopt( handle, CURLOPT_INTERFACE, iface_str );
	if( err != CURLE_OK ) { *ret = -1; goto finish; }
	err = curl_easy_setopt( handle, CURLOPT_PORT, in->port );
	if( err != CURLE_OK ) { *ret = -1; goto finish; }
	// sets up callback to verify the webpage is correct, defined in nice_client.c
	err = curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, pwrite_callback );
	if( err != CURLE_OK ) { *ret = -1; goto finish; }
	err = curl_easy_setopt( handle, CURLOPT_TIMEOUT_MS, 2000 );
	if( err != CURLE_OK ) { *ret = -1; goto finish; }
	err = curl_easy_setopt( handle, CURLOPT_CONNECTTIMEOUT, 2 );
	if( err != CURLE_OK ) { *ret = -1; goto finish; }
	err = curl_easy_setopt( handle, CURLOPT_NOSIGNAL, 1 );
	if( err != CURLE_OK ) { *ret = -1; goto finish; }
	err = curl_easy_setopt( handle, CURLOPT_URL, to_scan_str );
	if( err != CURLE_OK ) { *ret = -1; goto finish; }
	err = curl_easy_perform( handle );
	if( err != CURLE_OK ) {
		// assume a timeout, ret should be 1
		*ret = 1;
		goto finish;
	}
	// We found a socket, do some stuff
	*ret = 0;
	goto finish;

	finish:
	curl_easy_cleanup( handle );

	return ret;

}


void* spawn_pscan( void* arg ) {
	struct pscan_in* in = (struct pscan_in*)arg;

	uint32_t low = ntohl( in->start.s_addr );
	uint32_t high = ntohl( in->stop.s_addr );
	if( high < low ) {
		printf( "high is less than low, swapping\n" );
		uint32_t tmp = high;
		high = low;
		low = tmp;
	}
	int num_addr = high - low + 1;

	struct in_addr hits[ num_addr ];
	pthread_t threads[ num_addr ];
	struct scan_addr_in addrs[ num_addr ];

	memset( &hits, 0, sizeof( hits ) );

	for( uint32_t i = low; i <= high; i++ ) {
		//do the port scan
		struct in_addr tmp = {ntohl( i )};
		addrs[i-low].to_scan = tmp;
		addrs[i-low].srcip = in->srcip;
		addrs[i-low].port = 80;
		pthread_create( threads+(i - low), NULL, scan_addr, addrs+(i-low) );
		usleep( 1000 );
	}
	int c_found = 0;
	for( uint32_t i = low; i <= high; i++ ) {
		int* retcode = NULL;
		pthread_join( threads[i-low], (void**)&retcode );
		if( *retcode == 0 ) {
			hits[c_found].s_addr = ntohl( i );
			//printf( "\tFound address %s\n", inet_ntoa( hits[c_found] ) );
			c_found++;
		}
		free( retcode );
	}


	struct pscan_out* out = malloc( sizeof( *out ) );
	out->n_found = c_found;
	out->found = calloc( c_found, sizeof( *(out->found) ) );
	memcpy( out->found, hits, c_found*sizeof(*hits));
	return out;
}

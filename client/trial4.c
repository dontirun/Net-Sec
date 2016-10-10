#include <stdint.h>

#include <arpa/inet.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <curl/curl.h>

#include "include/nice_client.h"
#include "include/dnsutils.h"
#include "include/port_scanner.h"
#include "include/spoofer.h"



#define NUM_ISPS (3)
#define CLIENTS_PER_ISP (3)
#define HOST ((unsigned char*)"natdaemon.com")
#define DNS "10.4.11.193"
#define NUM_ATTEMPTS 0
#define TTL_HIGH 64
#define TTL_LOW 20

int main( int argc, char** argv ) {

	curl_global_init( CURL_GLOBAL_NOTHING );
	pthread_t spoof_thread;
	
	struct spoofer_in in;
	in.src = "10.4.11.17"; //unused
	in.dest = "10.4.11.193";
	in.ttl_high = TTL_HIGH;
	in.ttl_low = TTL_LOW;
	in.n_packets = 100;

	pthread_create( &spoof_thread, NULL, spawn_spoofer, &in );

	pthread_join( spoof_thread, NULL );

	curl_global_cleanup();
}









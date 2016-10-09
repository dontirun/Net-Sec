#include <stdint.h>

#include <arpa/inet.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <curl/curl.h>

#include "include/nice_client.h"
#include "include/dnsutils.h"
#include "include/port_scanner.h"


#define NUM_ISPS (3)
#define CLIENTS_PER_ISP (3)
#define HOST ((unsigned char*)"google.com")
#define DNS "10.4.11.193"
#define NUM_ATTEMPTS 20

int main( int argc, char** argv ) {
	const char* resolver_ips[NUM_ISPS] = 
	{ "10.4.11.65", "10.4.11.129", "10.4.11.9" };
	const char* client_ips[NUM_ISPS][CLIENTS_PER_ISP] = {
		{"10.4.11.66", "10.4.11.67","10.4.11.68" },
		{"10.4.11.130", "10.4.11.131","10.4.11.132" },
		{"10.4.11.10", "10.4.11.11","10.4.11.12" },
	};
	pthread_t threads[NUM_ISPS][CLIENTS_PER_ISP];
	struct client_in args[NUM_ISPS][CLIENTS_PER_ISP];
	curl_global_init( CURL_GLOBAL_NOTHING );
	struct DNS_RESOLVER* res_list[NUM_ISPS];
	pthread_t pscan_thread;
	struct pscan_in scanner_attr;

	//Setup resolvers
	int i,j;
	for( i = 0; i < NUM_ISPS; i++ ) {
		res_list[i] = dns_init( resolver_ips[i], DNS );
		if( res_list[i] == NULL ) {
			perror( "dns_init" );
			return -1;
		}
	}

	//Setup all clients like normal
	for( i = 0; i < NUM_ISPS; i++ ) {
		for( j = 0; j < CLIENTS_PER_ISP; j++ ) {
			inet_aton( client_ips[i][j], & (args[i][j].srcip) );
			args[i][j].host = HOST;
			args[i][j].nport = htons( 80 );
			args[i][j].res = res_list[i];
			args[i][j].num_trials = NUM_ATTEMPTS;
			pthread_create (&(threads[i][j]), NULL, &spawn_client, &(args[i][j]) );
		}
	}

	inet_aton( "10.4.11.0", &(scanner_attr.start) );
	inet_aton( "10.4.11.254", &(scanner_attr.stop) );
	scanner_attr.nport = htons( 80 );
	inet_aton( "10.4.11.13", &(scanner_attr.srcip ) );
	pthread_create( &pscan_thread, NULL, &spawn_pscan, &scanner_attr );

	for( i = 0; i < NUM_ISPS; i++ ) {
		for( j = 0; j < CLIENTS_PER_ISP; j++ ) {
			struct client_out *ret = NULL;
			pthread_join( threads[i][j], (void**)&ret );
			if( ret->retcode == 0 ) {
			printf( "Client %d-%d returned successfully\n", i, j );
			printf( " > returned avg_rtt was %d\n", ret->avg_rtt );
			printf( " > error count was %d\n", ret->errs );
			} else {
				printf( "Client %d-%d encountered an error and could not do its power\n", i,j );
			}
			free( ret );
		}
	}

	// wait for threads to finish
	for( i = 0; i < NUM_ISPS; i++ ) {
		dns_cleanup( res_list[i] );
	}



	struct pscan_out* pret = NULL;
	pthread_join( pscan_thread, (void**)&pret );
	printf( "found %d\n", pret->n_found );

	// wait for threads to finish
	for( i = 0; i < NUM_ISPS; i++ ) {
		dns_cleanup( res_list[i] );
	}
	free( pret->found );
	free( pret );
	// vomit data
	// done
	curl_global_cleanup();
	


}

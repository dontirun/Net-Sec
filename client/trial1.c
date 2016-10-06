#include <stdint.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <curl/curl.h>

#include "include/nice_client.h"
#include "include/dnsutils.h"


#define NUM_ISPS (3)
#define CLIENTS_PER_ISP (4)
#define HOST ((unsigned char*)"www.google.com")
#define DNS "208.67.222.222"
#define NUM_TRIALS 20

int main( int argc, char** argv ) {

	const char* aliases[NUM_ISPS][5] = {
		{"wlp9s0", "wlp9s0","wlp9s0","wlp9s0","wlp9s0"},
		{"wlp9s0", "wlp9s0","wlp9s0","wlp9s0","wlp9s0"},
		{"wlp9s0", "wlp9s0","wlp9s0","wlp9s0","wlp9s0"},
	};
	pthread_t threads[NUM_ISPS][CLIENTS_PER_ISP];
	struct client_in args[NUM_ISPS][CLIENTS_PER_ISP];
	curl_global_init( CURL_GLOBAL_NOTHING );

	//Setup resolvers
	struct DNS_RESOLVER* res_list[NUM_ISPS];
	int i,j;
	for( i = 0; i < NUM_ISPS; i++ ) {
		res_list[i] = dns_init( aliases[i][0], DNS );
		if( res_list[i] == NULL ) {
			perror( "dns_init" );
			return -1;
		}
	}

	for( i = 0; i < NUM_ISPS; i++ ) {
		for( j = 0; j < CLIENTS_PER_ISP; j++ ) {
			args[i][j].iface = aliases[i][j+1];
			args[i][j].host = HOST;
			args[i][j].nport = htons( 80 );
			args[i][j].res = res_list[i];
			args[i][j].num_trials = NUM_TRIALS;
			pthread_create (&(threads[i][j]), NULL, &spawn_client, &(args[i][j]) );
		}
	}
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

	curl_global_cleanup();
	// vomit data
	// done
	


}

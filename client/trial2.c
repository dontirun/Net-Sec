#include <stdint.h>

#include <arpa/inet.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>

#include "include/nice_client.h"
#include "include/dnsutils.h"
#include "include/port_scanner.h"


#define NUM_ISPS (3)
#define CLIENTS_PER_ISP (3)
#define HOST ((unsigned char*)"natdaemon.com")
#define DNS "10.4.11.193"
#define NUM_ATTEMPTS 100

void run_trial( void );
const char* resolver_ips[NUM_ISPS] = 
{ "10.4.11.65", "10.4.11.129", "10.4.11.9" };
const char* client_ips[NUM_ISPS][CLIENTS_PER_ISP] = {
	{"10.4.11.66", "10.4.11.67","10.4.11.68" },
	{"10.4.11.130", "10.4.11.131","10.4.11.132" },
	{"10.4.11.10", "10.4.11.11","10.4.11.12" },
};
pthread_t threads[NUM_ISPS][CLIENTS_PER_ISP];
struct client_in args[NUM_ISPS][CLIENTS_PER_ISP];
struct DNS_RESOLVER* res_list[NUM_ISPS];
pthread_t pscan_thread;
struct pscan_in scanner_attr;

int main( int argc, char** argv ) {
	curl_global_init( CURL_GLOBAL_NOTHING );

	for( int trial = 0; trial < 10; trial++ ) {
		run_trial();
		sleep( 5 );

	}
	curl_global_cleanup();
}
void run_trial( void ) {
	//Setup resolvers
	int i,j;
	for( i = 0; i < NUM_ISPS; i++ ) {
		res_list[i] = dns_init( resolver_ips[i], DNS );
		if( res_list[i] == NULL ) {
			perror( "dns_init" );
			return;
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
	sleep( 1 );

	inet_aton( "10.4.11.195", &(scanner_attr.start) );
	inet_aton( "10.4.11.254", &(scanner_attr.stop) );
	scanner_attr.nport = htons( 80 );
	inet_aton( "10.4.11.13", &(scanner_attr.srcip ) );
	pthread_create( &pscan_thread, NULL, &spawn_pscan, &scanner_attr );

	uint32_t rtt_avg_sum = 0;
	uint32_t rtt_avg_max = 0;
	uint32_t errs_sum = 0;
	uint32_t errs_max = 0;
	for( i = 0; i < NUM_ISPS; i++ ) {
		for( j = 0; j < CLIENTS_PER_ISP; j++ ) {
			struct client_out *ret = NULL;
			pthread_join( threads[i][j], (void**)&ret );
			if( ret->retcode == 0 ) {
				rtt_avg_sum += ret->avg_rtt;
				rtt_avg_max = ret->avg_rtt > rtt_avg_max ? ret->avg_rtt : rtt_avg_max;
				errs_sum += ret -> errs;
				errs_max = ret->errs > errs_max ? ret->errs : errs_max;
			} else {
				printf( "Client %d-%d encountered an error and could not do its power\n", i,j );
			}
			free( ret );
		}
	}
	printf( "Trial Summary:\n");
	printf( "\tAverage rtt: %d\n", rtt_avg_sum / ( NUM_ISPS * CLIENTS_PER_ISP ) );
	printf( "\tMax rtt: %d\n", rtt_avg_max );
	printf( "\tAverage Error rate: %f\n", (1.0 * errs_sum ) / (NUM_ISPS * CLIENTS_PER_ISP * NUM_ATTEMPTS ));
	printf( "\tHighest error rate: %f\n", (1.0 * errs_max ) / (NUM_ATTEMPTS ));

	struct pscan_out* pret = NULL;
	pthread_join( pscan_thread, (void**)&pret );
	printf( "\tfound %d addresses\n", pret->n_found );

	// wait for threads to finish
	for( i = 0; i < NUM_ISPS; i++ ) {
		dns_cleanup( res_list[i] );
	}
	free( pret->found );
	free( pret );
	// vomit data
	// done

}

#ifndef NICE_CLIENT_H
#define NICE_CLIENT_H

#include <arpa/inet.h>
#include "dnsutils.h"

struct client_in {
	struct in_addr srcip;
	const unsigned char* host;
	uint16_t nport; //network byte order
	struct DNS_RESOLVER* res;
	uint32_t num_trials;
};

struct client_out {
	int retcode;
	uint32_t avg_rtt;
	uint32_t errs;
	uint32_t max_rtt;
};

void* spawn_client( void* arg );

int sockopt_callback( void* clientp, curl_socket_t curlfd, curlsocktype purpose );

size_t write_callback( char* ptr, size_t size, size_t nmemb, void* userdata );


#endif

#ifndef NICE_CLIENT_H
#define NICE_CLIENT_H

#include <arpa/inet.h>
#include "dnsutils.h"

struct client_in {
	const char* iface;
	const unsigned char* host;
	uint16_t nport; //network byte order
	struct DNS_RESOLVER* res;
	uint32_t num_trials;
};

struct client_out {
	int retcode;
	uint32_t avg_rtt;
	uint32_t errs;
};

void* spawn_client( void* arg );

#endif

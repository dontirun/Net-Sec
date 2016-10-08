#ifndef PORT_SCANNER_H
#define PORT_SCANNER_H

#include <arpa/inet.h>

struct pscan_in {
	struct in_addr start;
	struct in_addr stop;
	struct in_addr srcip;
	uint16_t nport;
};

struct pscan_out {
	uint32_t n_found;
	struct in_addr* found;
};

void* spawn_pscan( void* arg );

#endif

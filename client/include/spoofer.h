#ifndef BUILD_PACKET_H
#define BUILD_PACKET_H

#include <stdint.h>

#define IP_HEADER_BYTES 20
#define TCP_PROTOCOL 6

#define TCP_HEADER_BYTES 20

struct spoofer_in {
	const char* src;
	const char* dest;
	uint8_t ttl_high;
	uint8_t ttl_low;
	uint32_t n_packets;
};

void* spawn_spoofer( void* args );


#endif



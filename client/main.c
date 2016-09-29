#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "build_packet.h"
#include "dnsutils.h"


#define NUM_PACKETS 0


int main( int argc, char** argv ) {

	//10.10.152.49
	uint32_t src  = 0xdeadbeef;
	
	// 130.215.217.139
	uint32_t dest = 0xdecea5ed;

	char* data = "GET / HTTP/1.1\r\nnConnection: close\r\n\r\n";
	uint16_t len_data = strlen( data );
	uint8_t ttl_high = 0xFF;
	uint8_t ttl_low = 0x00;

	for( int i = 0; i < NUM_PACKETS; i++ ) {
		send_packet( src, dest, ttl_high, ttl_low, data, len_data );
	}


	if( argc < 2 ) {
		return -1;
	}

	//Get the hostname from the terminal
	//Now get the ip of this hostname , A record
	struct RES_RECORD* resp = ngethostbyname( (unsigned char*)argv[0] , T_A);
	printf( "Name : %s\n", resp->name );
	printf( "\tTTL : %u\n", resp->resource->ttl );
	printf( "\tIPv4 address : %x\n", ntohl( *((uint32_t*)resp->rdata ) ) );

	return 0;
}

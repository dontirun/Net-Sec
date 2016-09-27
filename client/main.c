#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_packet.h"


int main( ) {

	//10.10.152.49
	uint32_t src  = 0xdeadbeef;
	
	// 130.215.217.139
	uint32_t dest = 0xdecea5ed;

	char* data = "GET / HTTP/1.1\r\nnConnection: close\r\n\r\n";
	uint16_t len_data = strlen( data );
	uint8_t ttl_high = 0xFF;
	uint8_t ttl_low = 0x00;

	for( int i = 0; i < 100; i++ ) {
		send_packet( src, dest, ttl_high, ttl_low, data, len_data );
	}
	return 0;

}

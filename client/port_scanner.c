#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>

#include "include/port_scanner.h"
#include "include/dnsutils.h"

int scan_addr( struct in_addr in, uint16_t port, struct in_addr srcip ) {

	//put the entire request in one string, and store it for later
	int s;
	int yes = 0;

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr = in;
	addr.sin_port = port;
	memset( &(addr.sin_zero), 0, 8 );

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	s = socket( AF_INET, SOCK_STREAM, 0 );
	if(s == -1) {
		return -1;
	}

	// set some socket options:
	
	// Timeout to 1s
	if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		return -1;
	}
	if (setsockopt (s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
		return -1;
	}

	// Reuse sockets
	if( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ) {
		return -1;
	}

	// Bind to iface
	if( bind_to_addr( s, srcip ) ) {
		return -1;
	}

	if( connect(s, (struct sockaddr*)&addr, sizeof( addr ) ) ) {
		close(s);
		return -1;
	}
	close( s );
	return 0; //we did it
}


void* spawn_pscan( void* arg ) {
	struct pscan_in* in = (struct pscan_in*)arg;

	uint32_t low = ntohl( in->start.s_addr );
	uint32_t high = ntohl( in->stop.s_addr );
	if( high < low ) {
		printf( "high is less than low, swapping\n" );
		uint32_t tmp = high;
		high = low;
		low = tmp;
	}
	printf("high is %x\n", high );
	printf("low is %x\n", low );

	struct in_addr hits[ high - low + 1 ];
	memset( &hits, 0, sizeof( hits ) );
	int c_hits = 0;

	for( uint32_t i = low; i <= high; i++ ) {
		//do the port scan
		struct in_addr tmp;
		tmp.s_addr = htonl( i );
		int addr_exists = scan_addr( tmp, in->nport, in->srcip);
		printf( "%s : ", inet_ntoa( tmp ) );
		if( addr_exists ) {
			printf("MISS" );
		} else {
			printf("HIT" );
			hits[c_hits++] = tmp;
		}
		printf("\n" );
	}
	struct pscan_out* out = malloc( sizeof( *out ) );
	out->n_found = c_hits;
	out->found = calloc( c_hits, sizeof( *(out->found) ) );
	memset( out->found, c_hits, c_hits*sizeof(*hits));
	return out;



}

#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>


int scan_addr( struct in_addr in, uint16_t port, char* iface ) {

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
	struct ifreq ifr;
	memset( &ifr, 0, sizeof( ifr ) );
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), iface);
	if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
		return -1;
	}

	if( connect(s, (struct sockaddr*)&addr, sizeof( addr ) ) ) {
		close(s);
		return -1;
	}
	return 0; //we did it
}

int main( int argc, char** argv ) {
	uint32_t high, low;
	struct in_addr tmp;
	inet_aton( "10.4.11.10", &tmp );
	low = ntohl( tmp.s_addr );
	inet_aton( "10.4.11.20", &tmp );
	high = ntohl( tmp.s_addr );

	if( high < low ) {
		perror( "high is less than low" );
	}
	for( uint32_t i = low; i <= high; i++ ) {
		//do the port scan
		tmp.s_addr = htonl( i );
		int addr_exists = scan_addr( tmp, htons( 80 ),  "wlp9s0");
		printf( "%s : ", inet_ntoa( tmp ) );
		if( addr_exists ) {
			printf("MISS" );
		} else {
			printf("HIT" );
		}
		printf("\n" );
	}



}

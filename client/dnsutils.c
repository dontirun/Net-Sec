//DNS Query Program on Linux
//Author : Jesse Earisman
//Dated : 2016/09/29

//Header Files
#include<stdio.h>	//printf
#include<string.h>	//strlen
#include<stdlib.h>	//malloc
#include<sys/socket.h>	//you know what this is for
#include<arpa/inet.h>	//inet_addr , inet_ntoa , ntohs etc
#include<netinet/in.h>
#include<unistd.h>	//getpid
#include <time.h> //clock_gettime and its ilk
#include <net/if.h>
#include <stropts.h>
#include <pthread.h>
#include <errno.h>

#include "include/dnsutils.h"


// Private function prototypes
void fill_dns_from_host( unsigned char* dns, const unsigned char* host);
unsigned char* ReadName ( unsigned char*,unsigned char*,int*);
void print_header( uint8_t* header, size_t len );
void free_res( struct RES_RECORD* res );


/**
 * Perform a DNS query by sending a packet
 * I copied this from the internet, that's how I know it works -Jesse
 */
struct RES_RECORD* query_dns( const struct DNS_RESOLVER* res,const unsigned char *host , int query_type ) {
	unsigned char buf[BIG_BUFFER_SIZE];
	unsigned char *qname;
	unsigned char *reader;
	int i, j, stop, s;


	struct RES_RECORD *answer = malloc( sizeof( *answer ) );
	struct sockaddr_in dest;

	struct DNS_HEADER *dns = NULL;
	struct QUESTION *qinfo = NULL;


	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //UDP packet for DNS queries
	struct timeval t;
	t.tv_sec = 2;
	t.tv_usec = 0;
	setsockopt( s, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof( t ) );
	setsockopt( s, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof( t ) );
	bind_to_addr( s, res->local_addr ); //local addr and random port

	dest.sin_family = AF_INET;
	dest.sin_port = htons(53);
	dest.sin_addr = (res->dns_server); //dns servers

	//Set the DNS structure to standard queries
	dns = (struct DNS_HEADER *)&buf;

	dns->id = (unsigned short) htons(getpid());
	dns->flags = htons ( RD );
	dns->q_count = htons(1); //we have only 1 question
	dns->ans_count = 0;
	dns->auth_count = 0;
	dns->add_count = 0;

	//point to the query portion
	qname =(unsigned char*)&buf[sizeof(struct DNS_HEADER)];

	fill_dns_from_host(qname, host);
	qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it

	qinfo->qtype = htons( query_type ); //type of the query , A , MX , CNAME , NS etc
	qinfo->qclass = htons(1); //its internet (lol)
	size_t msg_len = sizeof( struct DNS_HEADER) + strlen( (char*)qname ) + 1 + sizeof( struct QUESTION );

	if( sendto(
				s,
				(char*)buf,
				msg_len,
				0,
				(struct sockaddr*)&dest,sizeof(dest)) < 0)
	{
		//perror("sendto failed");
		return NULL;
	}
	//Receive the answer
	i = sizeof dest;
	if(recvfrom (
				s,
				(char*)buf,
				BIG_BUFFER_SIZE,
				0,
				(struct sockaddr*)&dest,
				(socklen_t*)&i ) < 0)
	{
		//perror("recvfrom failed");
		return NULL;
	}


	dns = (struct DNS_HEADER*)buf;
	//check id, ans_count, etc
	if( dns->ans_count < 1 ) {
		return NULL;
	}


	//move ahead of the dns header and the query field
	reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION)];

	//Start reading answers
	stop=0;

	answer->name=ReadName(reader,buf,&stop);
	reader = reader + stop;

	struct R_DATA* data = malloc( sizeof *data );
	memcpy( data, reader, sizeof( *data ) );

	answer->resource = data;

	reader = reader + sizeof(struct R_DATA);

	if(ntohs(answer->resource->type) == 1) { //if its an ipv4 address
		answer->rdata = (unsigned char*)malloc(ntohs(answer->resource->data_len + 1));

		for(j=0 ; j<ntohs(answer->resource->data_len) ; j++) {
			answer->rdata[j]=reader[j];
		}

		answer->rdata[ntohs(answer->resource->data_len)] = '\0';

		reader = reader + ntohs(answer->resource->data_len);
	} else {
		answer->rdata = ReadName(reader,buf,&stop);
		reader = reader + stop;
	}
	return answer;
}

/**
 *
 */
unsigned char* ReadName(unsigned char* reader,unsigned char* buffer,int* count) {
	unsigned char *name;
	unsigned int p=0,jumped=0,offset;
	int i , j;

	*count = 1;
	name = (unsigned char*)malloc(256);

	name[0]='\0';

	//read the names in 3www6google3com format
	while(*reader!=0) {
		if(*reader>=192) {
			offset = (*reader)*256 + *(reader+1) - 49152; //49152 = 11000000 00000000 ;)
			reader = buffer + offset - 1;
			jumped = 1; //we have jumped to another location so counting wont go up!
		} else {
			name[p++]=*reader;
		}

		reader = reader+1;

		if(jumped==0) {
			*count = *count + 1; //if we havent jumped to another location then we can count up
		}
	}

	name[p]='\0'; //string complete
	if(jumped==1) {
		*count = *count + 1; //number of steps we actually moved forward in the packet
	}

	//now convert 3www6google3com0 to www.google.com
	for( i=0; i < (int)strlen((const char*)name); i++) {
		p=name[i];
		for( j=0; j < (int)p; j++) {
			name[i]=name[i+1];
			i=i+1;
		}
		name[i]='.';
	}
	if( i > 0 ) {
		name[i-1]='\0'; //remove the last dot
	}
	return name;
}

/**
 * Fill the dns pointer with a dns-style hostname based on the given host
 * conforms (probably) to rfc1035
 */
void fill_dns_from_host( unsigned char* dns, const unsigned char* host) {
	unsigned char* dns_ptr = dns++;
	int counter = 0;
	for( int i = 0 ; ; i++) {
		if(host[i]=='.' ) {
			*dns_ptr = counter;
			dns_ptr = dns++;
			counter = 0;
		} else if( host[i] == '\0' ) {
			*dns_ptr = counter;
			*(dns++) = '\0';
			counter = 0;
			break;
		} else {
			counter++;
			*(dns++) = host[i];
		}
	}
}

/**
 * Clear an entry from the linked list of table entries
 *  and adjust the head of the linked list if appropriate
 */
void remove_table_entry( struct DNS_RESOLVER* res, struct DNS_TABLE_ENTRY* entry ) {
	
	struct DNS_TABLE_ENTRY* n = entry->next;
	struct DNS_TABLE_ENTRY* p = entry->prev;
	if( n && p) {
		entry->prev = n;
	} else if( p && !n ) {
		p->next = NULL;
	} else if (!p && n) {
		res->head = n;
	} else if( !p && !n ) {
		res->head = NULL;
	}
}

struct DNS_RESOLVER* dns_init( const char* addr, const char* dns_string ) {
	struct DNS_RESOLVER* ret = malloc( sizeof( *ret ) );
	if( ret == NULL ) {
		perror( "malloc" );
		return NULL;
	}
	ret->head = NULL;
	int err = pthread_mutex_init( &(ret->rwlock), NULL );
	if( err ) {
		free( ret );
		return NULL;
	}
	inet_aton( dns_string, &(ret->dns_server) );
	inet_aton( addr, &(ret->local_addr ) );
	return ret;
}

int dns_cleanup( struct DNS_RESOLVER* to_del ) {
	
	// lock the mutex, to make sure that no threads are reading or writing
	pthread_mutex_lock( &(to_del->rwlock) );
	struct DNS_TABLE_ENTRY* head = to_del->head;
	struct DNS_TABLE_ENTRY* next = NULL;
	while( head ) {
		next = head->next;
		head->next = NULL;
		head->prev = NULL;
		free( head );
		head = next;
	}
	pthread_mutex_destroy( &(to_del->rwlock) );
	free( to_del );
	return 0;
}


/**
 * resolve a hostname to a hex ip address
 * DNS entries are cached with a ttl, the cached entry will be returned if it exists
 *
 * New hostnames, or hostnames with expired ttls will require a dns lookup
 */
struct in_addr resolve( struct DNS_RESOLVER* res, const unsigned char* hostname ) {
	struct timespec ts;
	struct in_addr ret;
	int err = clock_gettime( CLOCK_REALTIME, &ts );
	if( err ) {
		perror( "clock error" );
		ret.s_addr = -1;
		return ret;
	}
	time_t timestamp = ts.tv_sec;

	pthread_mutex_lock( &(res->rwlock) );
	for( struct DNS_TABLE_ENTRY* i = res->head; i != NULL; i = i->next ) {
		if( timestamp > i->timestamp + i->ttl ) {
			remove_table_entry( res, i );
			continue;
		}
		if( strcmp( (char*)hostname, (char*)i->name ) == 0 ) {
			ret.s_addr = i->hex_addr;
			pthread_mutex_unlock( &(res->rwlock ) );
			return ret;
		}
	}
	struct timespec t_start;
	struct timespec t_end;
	clock_gettime( CLOCK_MONOTONIC, &t_start );
	// Nothing in the cache, query and add to cache
	struct RES_RECORD* resp = query_dns( res, hostname, T_A );
	if( resp == NULL ) {
		pthread_mutex_unlock( &( res->rwlock ) );
		ret.s_addr = -1;
		return ret;
	}
	clock_gettime( CLOCK_MONOTONIC, &t_end );

	struct DNS_TABLE_ENTRY* new = malloc( sizeof *new );

	new->name = hostname;
	new->hex_addr = ( *((uint32_t*)resp->rdata ) );
	new->ttl = ntohl( resp->resource->ttl );
	new->timestamp = timestamp;
	//Set the newest entry as the head of the LL
	if( res->head ) {
		res->head->prev = new;
	}
	new->next = res->head;
	new->prev = NULL;
	res->head = new;

	pthread_mutex_unlock( &(res->rwlock) );
	free_res( resp );

	ret.s_addr = new->hex_addr;
	return ret;
}


void free_res( struct RES_RECORD* res ) {
	free( res->name );
	free( res->resource );
	free( res->rdata );
	free( res );
}

/**
 * Used for debugging. Prints each byte of a header in groups of 4
 */
void print_header( uint8_t* header, size_t len ) {
	for( size_t i = 0; i < len; i++ ) {
		printf( "%02x ", header[i] & 0xff );
		if( i % 4 == 3 ) {
			printf( "\n" );
		}
	}
}

/**
 * Binds a socket to a particular interface.
 * Useful when you have one machine, and are testing using multiple IP aliases
 * Requires root permission
int bind_to_iface( int sock, const char* iface ) {
	if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE,
				iface, strlen( iface )) < 0) {
		return -1;
	}
	return 0;
}
 */
int bind_to_addr( int sock, struct in_addr addr ) {
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = 0; //any port will do
	sa.sin_addr = addr;
	memset( sa.sin_zero, 0, 8 );
	if( bind(sock, (struct sockaddr*)(&sa), sizeof( sa ) ) ) {
		perror( "bind" );
		return -1;
	}
	return 0;
}


//DNS Query Program on Linux
//Author : Silver Moon and Jesse Earisman
//Dated : 2009/4/29 and 2016/09/29

//Header Files
#include<stdio.h>	//printf
#include<string.h>	//strlen
#include<stdlib.h>	//malloc
#include<sys/socket.h>	//you know what this is for
#include<arpa/inet.h>	//inet_addr , inet_ntoa , ntohs etc
#include<netinet/in.h>
#include<unistd.h>	//getpid
#include <time.h> //clock_gettime and its ilk

#include "dnsutils.h"

// 8.8.8.8
uint32_t dns_server = 0x08080808;

//Global DNS table. Declaring this globally means that this program is *not*
// thread safe
struct DNS_TABLE_ENTRY* t_head = NULL;

/**
 * Perform a DNS query by sending a packet
 * I copied this from the internet, that's how I know it works -Jesse
 */
struct RES_RECORD* query_dns( unsigned char *host , int query_type) {
	unsigned char buf[BIG_BUFFER_SIZE];
	unsigned char *qname;
	unsigned char *reader;
	int i, j, stop, s;


	struct RES_RECORD *answer = malloc( sizeof( *answer ) );
	struct sockaddr_in dest;

	struct DNS_HEADER *dns = NULL;
	struct QUESTION *qinfo = NULL;


	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //UDP packet for DNS queries

	dest.sin_family = AF_INET;
	dest.sin_port = htons(53);
	dest.sin_addr.s_addr = dns_server; //dns servers

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

	ChangetoDnsNameFormat(qname, host);
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
		perror("sendto failed");
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
		perror("recvfrom failed");
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

/*
 * This will convert www.google.com to 3www6google3com 
 * got it :)
 * */
void ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host) {
	int lock = 0;
	strcat((char*)host,".");

	for( int i = 0 ; i < strlen((char*)host) ; i++) {
		if(host[i]=='.') {
			*dns++ = i-lock;
			for(;lock<i;lock++) {
				*dns++=host[lock];
			}
			lock++;
		}
	}
	*dns++='\0';
}



/**
 * Clear an entry from the linked list of table entries
 *  and adjust the head of the linked list if appropriate
 */
void remove_table_entry( struct DNS_TABLE_ENTRY* entry ) {
	struct DNS_TABLE_ENTRY* n = entry->next;
	struct DNS_TABLE_ENTRY* p = entry->prev;
	if( n && p) {
		entry->prev = n;
	} else if( p && !n ) {
		p->next = NULL;
	} else if (!p && n) {
		t_head = n;
	} else if( !p && !n ) {
		t_head = NULL;
	}
}


/**
 * resolve a hostname to a hex ip address
 * DNS entries are cached with a ttl, the cached entry will be returned if it exists
 *
 * New hostnames, or hostnames with expired ttls will require a dns lookup
 */
uint32_t resolve( unsigned char* hostname ) {
	struct timespec ts;
	int err = clock_gettime( CLOCK_REALTIME, &ts );
	if( err ) {
		perror( "clock error" );
		return -1;
	}
	time_t timestamp = ts.tv_sec;

	for( struct DNS_TABLE_ENTRY* i = t_head; i != NULL; i = i->next ) {
		if( timestamp > i->timestamp + i->ttl ) {
			remove_table_entry( i );
			continue;
		}
		if( strcmp( (char*)hostname, (char*)i->name ) == 0 ) {
			return i->hex_addr;
		}
	}
	// Nothing in the cache, query and add to cache
	struct RES_RECORD* resp = query_dns( hostname, T_A );
	struct DNS_TABLE_ENTRY* new = malloc( sizeof *new );

	new->name = hostname;
	new->hex_addr = *((uint32_t*)resp->rdata );
	new->ttl = ntohl( resp->resource->ttl );
	printf( "new ttl is %d\n", new->ttl );
	new->timestamp = timestamp;
	//Set the newest entry as the head of the LL
	if( t_head ) {
		t_head->prev = new;
	}
	new->next = t_head;
	new->prev = NULL;
	t_head = new;
	free_res( resp );

	return new->hex_addr;
}


void free_res( struct RES_RECORD* res ) {
	free( res->name );
	free( res->resource );
	free( res->rdata );
	free( res );
}

void free_table( void ) {
	struct DNS_TABLE_ENTRY* head = t_head;
	struct DNS_TABLE_ENTRY* next = NULL;
	while( head ) {
		next = head->next;
		head->next = NULL;
		head->prev = NULL;
		free( head );
		head = next;
	}
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


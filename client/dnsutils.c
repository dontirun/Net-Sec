//DNS Query Program on Linux
//Author : Silver Moon ([email protected])
//Dated : 29/4/2009

//Header Files
#include<stdio.h>	//printf
#include<string.h>	//strlen
#include<stdlib.h>	//malloc
#include<sys/socket.h>	//you know what this is for
#include<arpa/inet.h>	//inet_addr , inet_ntoa , ntohs etc
#include<netinet/in.h>
#include<unistd.h>	//getpid

#include "dnsutils.h"

uint32_t dns_server = 0x08080808;



/*
 * Perform a DNS query by sending a packet
 * */
struct RES_RECORD* ngethostbyname( unsigned char *host , int query_type)
{
	unsigned char buf[65536],*qname,*reader;
	int i , j , stop , s;


	struct RES_RECORD *answer = malloc( sizeof( *answer ) );
	struct sockaddr_in dest;

	struct DNS_HEADER *dns = NULL;
	struct QUESTION *qinfo = NULL;


	s = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP); //UDP packet for DNS queries

	dest.sin_family = AF_INET;
	dest.sin_port = htons(53);
	dest.sin_addr.s_addr = dns_server; //dns servers

	//Set the DNS structure to standard queries
	dns = (struct DNS_HEADER *)&buf;

	dns->id = (unsigned short) htons(getpid());
	dns->qr = 0; //This is a query
	dns->opcode = 0; //This is a standard query
	dns->aa = 0; //Not Authoritative
	dns->tc = 0; //This message is not truncated
	dns->rd = 1; //Recursion Desired
	dns->ra = 0; //Recursion not available! hey we dont have it (lol)
	dns->z = 0;
	dns->ad = 0;
	dns->cd = 0;
	dns->rcode = 0;
	dns->q_count = htons(1); //we have only 1 question
	dns->ans_count = 0;
	dns->auth_count = 0;
	dns->add_count = 0;

	//point to the query portion
	qname =(unsigned char*)&buf[sizeof(struct DNS_HEADER)];

	ChangetoDnsNameFormat(qname , host);
	qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it

	qinfo->qtype = htons( query_type ); //type of the query , A , MX , CNAME , NS etc
	qinfo->qclass = htons(1); //its internet (lol)

	if( sendto(
			s,
			(char*)buf,sizeof(struct DNS_HEADER)
				+
				(strlen((const char*)qname)+1)
				+
				sizeof(struct QUESTION),
			0,
			(struct sockaddr*)&dest,sizeof(dest)) < 0) {
		perror("sendto failed");
	}
	
	//Receive the answer
	i = sizeof dest;
	if(recvfrom (s,(char*)buf , 65536 , 0 , (struct sockaddr*)&dest , (socklen_t*)&i 
) < 0)
	{
		perror("recvfrom failed");
	}

	dns = (struct DNS_HEADER*) buf;

	//move ahead of the dns header and the query field
	reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + 
sizeof(struct QUESTION)];

	//Start reading answers
	stop=0;

	answer->name=ReadName(reader,buf,&stop);
	reader = reader + stop;

	answer->resource = (struct R_DATA*)(reader);
	reader = reader + sizeof(struct R_DATA);

	if(ntohs(answer->resource->type) == 1) //if its an ipv4 address
	{
		answer->rdata = (unsigned char*)malloc(ntohs(answer->resource->data_len));

		for(j=0 ; j<ntohs(answer->resource->data_len) ; j++) {
			answer->rdata[j]=reader[j];
		}

		answer->rdata[ntohs(answer->resource->data_len)] = '\0';

		reader = reader + ntohs(answer->resource->data_len);
	}
	else {
		answer->rdata = ReadName(reader,buf,&stop);
		reader = reader + stop;
	}
	return answer;
}

/*
 * 
 * */
u_char* ReadName(unsigned char* reader,unsigned char* buffer,int* count)
{
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
		}
		else {
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
	for(i=0;i<(int)strlen((const char*)name);i++) {
		p=name[i];
		for(j=0;j<(int)p;j++) {
			name[i]=name[i+1];
			i=i+1;
		}
		name[i]='.';
	}
	name[i-1]='\0'; //remove the last dot
	return name;
}

/*
 * This will convert www.google.com to 3www6google3com 
 * got it :)
 * */
void ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host) {
	int lock = 0 , i;
	strcat((char*)host,".");
	
	for(i = 0 ; i < strlen((char*)host) ; i++) {
		if(host[i]=='.') {
			*dns++ = i-lock;
			for(;lock<i;lock++) {
				*dns++=host[lock];
			}
			lock++; //or lock=i+1;
		}
	}
	*dns++='\0';
}

#ifndef DNSUTILS_H
#define DNSUTILS_H

//Types of DNS resource records
#define T_A 1 //Ipv4 address
#define T_NS 2 //Nameserver
#define T_CNAME 5 // canonical name
#define T_SOA 6 /* start of authority zone */
#define T_PTR 12 /* domain name pointer */
#define T_MX 15 //Mail server


// DNS header flags
#define QR (1<<15)
#define RD (1<<8)


#define BIG_BUFFER_SIZE 65535



//DNS header structure
struct DNS_HEADER
{
	uint16_t id; // identification number
	uint16_t flags;

	uint16_t q_count; // number of question entries
	uint16_t ans_count; // number of answer entries
	uint16_t auth_count; // number of authority entries
	uint16_t add_count; // number of resource entries
};

//Constant sized fields of query structure
struct QUESTION
{
	uint16_t qtype;
	uint16_t qclass;
};

//Constant sized fields of the resource record structure
#pragma pack(push, 1)
struct R_DATA
{
	uint16_t type;
	uint16_t _class;
	uint32_t ttl;
	uint16_t data_len;
};
#pragma pack(pop)

//Pointers to resource record contents
struct RES_RECORD
{
	unsigned char *name;
	struct R_DATA *resource;
	unsigned char *rdata;
};

//Structure of a Query
typedef struct
{
	unsigned char *name;
	struct QUESTION *ques;
} QUERY;

struct DNS_TABLE_ENTRY {
	unsigned char* name;
	uint32_t hex_addr;
	uint32_t ttl;
	time_t timestamp;
	struct DNS_TABLE_ENTRY* next;
	struct DNS_TABLE_ENTRY* prev;
};

//Function Prototypes
struct RES_RECORD* query_dns ( unsigned char* host, int query_type );
void ChangetoDnsNameFormat ( unsigned char*,unsigned char*);
unsigned char* ReadName ( unsigned char*,unsigned char*,int*);
uint32_t resolve( unsigned char* hostname );
void print_header( uint8_t* header, size_t len );
void free_res( struct RES_RECORD* res );
void free_table( void );

#endif

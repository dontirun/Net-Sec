#ifndef BUILD_PACKET_H
#define BUILD_PACKET_H

#define IP_HEADER_BYTES 20
#define TCP_PROTOCOL 6

#define TCP_HEADER_BYTES 20

//uint8_t* build_ip_header( uint32_t src, uint32_t dest, uint8_t ttl, uint16_t len );

//uint8_t* build_tcp_header( char* data, uint32_t len );

int send_packet( uint32_t src, uint32_t dest, uint8_t ttl, char* data, uint16_t len_data );

void print_header( uint8_t* header, size_t len );
#endif



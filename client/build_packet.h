#ifndef BUILD_PACKET_H
#define BUILD_PACKET_H

#include <stdint.h>

#define IP_HEADER_BYTES 20
#define TCP_PROTOCOL 6

#define TCP_HEADER_BYTES 20

int send_packet( uint32_t src, uint32_t dest, uint8_t ttl_high, uint8_t ttl_low, char* data, uint16_t len_data );


#endif



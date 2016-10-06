#ifndef GET_SOCKET_H
#define GET_SOCKET_H

int get_socket_by_in_addr( struct in_addr in, uint16_t port, int* s, const char* iface );

#endif

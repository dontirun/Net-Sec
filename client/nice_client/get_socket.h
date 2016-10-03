
int get_socket( const char* host, const char* port, int* s, const char* iface);
int get_socket_by_in_addr( struct in_addr in, uint16_t port, int* s, const char* iface );

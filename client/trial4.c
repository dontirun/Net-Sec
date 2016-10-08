#include <stdint.h>

#include <arpa/inet.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <curl/curl.h>

#include "include/nice_client.h"
#include "include/dnsutils.h"
#include "include/port_scanner.h"
#include "include/spoofer.h"



#define NUM_ISPS (3)
#define CLIENTS_PER_ISP (3)
#define HOST ((unsigned char*)"www.google.com")
#define DNS "208.67.222.222"
#define NUM_ATTEMPTS 20
#define TTL_HIGH 64
#define TTL_LOW 20

int main( int argc, char** argv ) {
	return 0;

}









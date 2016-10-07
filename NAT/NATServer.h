#ifndef __NATSERVER_H__
#define __NATSERVER_H__

/*
 * Imports
 */
// Standard libraries
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "LinkedList.h"

// Network Imports
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

// Multithreaded Imports
#include <pthread.h>

/*
 * Global Constants
 */
#define NATIP "10.4.11.192"
#define DNSIP "10.4.11.193"
#define SERVERIP "192.168.1.101"

#define EXPIRATIONTIMESECONDS 7*60
#define GRACEPERIODSECONDS 2*60

#define PUBLICIPRANGESTART 194
#define PUBLICIPRANGEEND 254

#define MAXMESSAGESIZE 21

/*
 * Structures
 */
typedef struct {
    char *message;
    int socket;
} Request;

typedef struct {
    time_t time;
    char *ispPrefix;
    char *publicIP;
} Mapping;

typedef struct {
    LinkedList *list;
    time_t (*expirationTime)(void *elm);
    void (*expirationAction)(void *elm);
} CleanerData;

/*
 * Public Prototypes
 */
void printUsage();

#endif
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

#define EXPIRATIONTIMESECONDS 5
#define GRACEPERIODSECONDS 2

#define PUBLICIPRANGESTART 195
#define PUBLICIPRANGEEND 254
#define SERVERFLUXCLASSAADDRESS "10.4.11.%d"

#define MAXREQUESTMESSAGESIZE 21
#define MAXRESPONSEMESSAGESIZE 25

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
    pthread_mutex_t listLock;
} CleanerData;

typedef struct {
    char *mappedIP;
    int mapTTL;
} SuccessMapping;

/*
 * Public Prototypes
 */
void printUsage();

#endif

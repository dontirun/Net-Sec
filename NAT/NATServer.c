// Standard libraries
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "LinkedList.h"
#include <errno.h>

// Network Imports
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

// Multithreaded Imports
#include <pthread.h>

#define MAXMESSAGESIZE 21

typedef struct {
    char *message;
    int socket;
} Request;

// Function Prototypes
void printUsage();
void term(int signum);
int insertMapping(char *ip);
int send_all(char *message, int messageSize, int socket);
int recv_all(char **message, int socket);
void* handleClientConnections(void* socket);
void* cleanFinishedThreads(void *threadList);
void* handleRequest(void *request);

volatile sig_atomic_t quit = 0;

/**
 * Run in a continuous loop and await commands from the DNS
 */
int main(int argc, char **argv) {

    char hostName[1024];
    int portNum, sockfd;
    struct hostent *host;
    struct in_addr *hostIP;
    struct sockaddr_in servAddr, cliAddr;
    socklen_t cliSize;   

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGINT, &action, NULL);

    // Check and Get the command line arguments
    if(argc < 2 || argc > 2) {
        printUsage();
        exit(0);
    } else {
        portNum = atoi(argv[1]);
    }

    // Open Socket and store the associated file descriptor
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("ERROR - Cannot open socket\n");
        exit(1);
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(portNum);
    memset(servAddr.sin_zero, 0, sizeof(servAddr.sin_zero));

    // Bind process to Socket
    if(bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
        perror("ERROR - Cannot bind to port\n");
        exit(1);
    }
    
    // Get the IP of the host
    gethostname(hostName, 1023);
    host = gethostbyname(hostName);
    hostIP = (struct in_addr *)host->h_addr;
    
    // Print the IP and the listening port on the server
    printf("Server is listening at %s:%d\n", inet_ntoa(*hostIP), portNum); 

    // Establish connection between NAT and Client
    LinkedList *cliThreads = createList();
    while(!quit) {
        listen(sockfd, 10);
        cliSize = sizeof(cliAddr);   
        int* cliSock = malloc(sizeof(int));
        *cliSock = accept(sockfd, (struct sockaddr *)&cliAddr, &cliSize);
        if(cliSock < 0) {
            if(quit == 1)
                break;
            perror("Error - Cannot accept client\n");
            continue;
        }

        pthread_t *cliThread = malloc(sizeof(pthread_t));
        int rc = pthread_create(cliThread, NULL, handleClientConnections, (void*)cliSock);
        if(rc) {
            printf("Error creating thread for client, Code: %d\n", rc);
            continue;
        }

        insertElement(cliThreads, cliThread);
    }
    
    printf("Shutting Down Server...\n");

    int i;
    for(i = 0; i < cliThreads->size; i++) {
        Node *head = cliThreads->head;
        pthread_join(*((pthread_t *)head->elm), NULL);
        head = head->next;
    }

    // Close Sockets
    close(sockfd);
    
    return 0;
}

/**
 * Input: 
 *     Int value of the termination type given
 * Output: 
 *     None
 * 
 * Set the quit flag to 1 when the user chooses to terminate the program.
 */
void term(int signum) {
    quit = 1;
}

/**
 * Input:
 *     None
 * Output: 
 *     None
 *
 * Print the proper usage of the program.
 */
void printUsage() {
    printf("\nUsage:\n");
    printf("    ./http_server port_number\n");
    printf("\n");
}

void* handleClientConnections(void* socket) {
    int cliSock = *((int *)socket);
    
    // Initialize thread list
    LinkedList *activeThreads = createList();

    // Create a thread to clean up the created threads
    pthread_t *threadCleaner = malloc(sizeof(pthread_t));
    int rc = pthread_create(threadCleaner, NULL, cleanFinishedThreads, (void*)activeThreads);
    if (rc) {
        printf("Error creating thread cleaner, Code: %d\n", rc);
        exit(1);
    }

    while(!quit) {
        // Recieve Request
        char *messagePtr = NULL;
        recv_all(&messagePtr, cliSock);
        
        if(quit)
            break;
        
        // Create thread to handle request
        pthread_t *thread = malloc(sizeof(pthread_t));

        // Create Request
        Request *request = malloc(sizeof(Request));
        request->message = messagePtr;
        request->socket = cliSock;

        int response = pthread_create(thread, NULL, handleRequest, (void *)request);
        if(response) {
            printf("Error creating thread, Request %s, Code: %d\n", messagePtr, response);
            // Handle error so request will be finished in some way
            continue;
        } 
                
        insertElement(activeThreads, thread);
    }

    // Wait for thread cleaner
    pthread_join(*threadCleaner, NULL);

    // Close Socket
    close(cliSock);

    return NULL;
}

/**
 * Input:
 *     Pointer to a LinkedList containing the pthread_t of active threads
 * Output:
 *     None
 *
 * Runs in a separate thread and cleans up other threads
 */
void* cleanFinishedThreads(void *threadList) {
    LinkedList *list = (LinkedList *)threadList;

    while(!quit) {
        if(list->size <= 0)
            continue;
        pthread_t *head = popElement(list);
        pthread_join(*head, NULL);
    }

    return NULL;
}

/**
 * Input:
 *     Pointer to an allocated string containing the message from the client
 * Output:
 *     None
 *
 * Handle the request of the client in a separate thread
 */
void* handleRequest(void *request) {
    // Convert given pointer to actual type
    char *message = ((Request *)request)->message;
    int cliSock = ((Request *) request)->socket;
    
    if(strlen(message) <= 0)
        return NULL;
    
    // Retrive the Command and the IP from the request
    char *command = strtok(message, ";");
    char *ip = strtok(NULL, ";");

    // Act based on the command
    if(strcmp(command, "ADD") == 0) {
        // Create mapping for the given IP
        insertMapping(ip);
    } else if(strcmp(command, "BAN") == 0) {
        // Blacklist given IP

    }

    // Send Response
    char *resp;
    int respSize = asprintf(&resp, "FIN;%s", ip) + 1;
    send_all(resp, respSize, cliSock);

    return NULL;
}

/**
 * Input:
 *     IP address given by the DNS, corresponds to the IP prefix of the ISP
 * Output:
 *     0 if mapping was created succesfully, 1 if failure
 */
int insertMapping(char *ip) {
    // Form iptable command
    char *command;

    asprintf(&command, "sudo iptables -t nat -A INPUT -p tcp --dport http -s %s -m state --state NEW -j ACCEPT", ip);

    // Execute command
    int result = system(command);

    // Check command executed successfully
    if(result < 0) {
        
        return 1;
    }    

    return 0;
}

/**
 * Input:
 *     message - Double pointer to an uninitialized character pointer
 *     socket - Communication socket between NAT and DNS
 * Output:
 *     Return 0 if succesful or 1 otherwise.
 *
 * Read the message from the socket if there is one
 */
int recv_all(char **message, int socket) {
    // Check if the message pointer has not been allocated
    if(*message == NULL) {
        *message = malloc(MAXMESSAGESIZE);
    }

    // Read in the message
    memset(*message, 0, MAXMESSAGESIZE);
    int messageSize = read(socket, *message, MAXMESSAGESIZE - 1);
    if(messageSize < 0) {
        if(messageSize == -1 && errno == EAGAIN) {
            free(*message);
            return 0;           
        } else {
            perror("Error - Cannot read from socket\n");
            return 1;
        }
    }
    
    return 0;

}

/**
 * Input:
 *     message - Pointer to a character array
 *     messageSize - Size of the message
 *     socket - communication socket
 * Output:
 *     0 if send was successful, 1 otherwise
 *
 * Send the given message
 */
int send_all(char *message, int messageSize, int socket) {
    int sent = 0;
    int bytes = 0;
    do {
        bytes = send(socket, message+sent, messageSize-sent, MSG_NOSIGNAL);
        if(bytes < 0) {
            exit(1);
        }
        if(bytes == 0)
            break;
        sent += bytes;
    } while(sent < messageSize);

    return 0;
}


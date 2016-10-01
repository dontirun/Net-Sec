// Standard libraries
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "LinkedList.h"

// Network Imports
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

// Multithreaded Imports
#include <pthread.h>

#define MAXMESSAGESIZE 21

// Function Prototypes
void printUsage();
void term();
int insertMapping(char *ip);
int send_all(char *message, int messageSize, int socket);
int recv_all(char **message, int socket);

volatile sig_atomic_t quit = 0;


/**
 * Run in a continuous loop and await commands from the DNS
 */
int main(int argc, char **argv) {

    char hostName[1024];
    int portNum, sockfd, cliSock;
    struct hostent *host;
    struct in_addr *hostIP;
    struct sockaddr_in servAddr, cliAddr;
    socklen_t cliSize;

    pthread_t *activeThreads;    
   
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;

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

    // Establish connection between NAT and DNS
    listen(sockfd, 10);
    cliSize = sizeof(cliAddr);     
    cliSock = accept(sockfd, (struct sockaddr *)&cliAddr, &cliSize);
    if(cliSock < 0) {
        perror("Error - Cannot accept client\n");
        exit(1);
    }

    while(!quit) {
        // Recieve Requests
        char *command = malloc(3);
	    char *ip = malloc(15);
        char *messagePtr = NULL;
        recv_all(&messagePtr, cliSock);
        sscanf(messagePtr, "%s;%s", command, ip);
    	printf("%s\n", command);

        // Send Response
        char *resp;
        int respSize = asprintf(&resp, "FIN;0.0.0.0") + 1;
        send_all(resp, respSize, cliSock);
    }
    
    printf("Shutting Down Server...\n");

    // Close Sockets
    close(sockfd);
    close(cliSock);

    return 0;
}

void handle_request(void *request) {
    
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
    asprintf(&command, "sudo iptables -t nat -A INPUT -p tcp --dport http -s  %s -m state --state NEW -j ACCEPT", ip);

    // Execute command
    int result = system(command);

    // Check command executed successfully
    if(result < 0) {

    }

    // Return 
    

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
        perror("Error - Cannot read from socket\n");
        return 1;
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
    char *resp;
    int respSize = asprintf(&resp, "FIN;0.0.0.0") + 1;
    do {
        bytes = send(socket, resp+sent, respSize-sent, MSG_NOSIGNAL);
        if(bytes < 0) {
            exit(1);
        }
        if(bytes == 0)
            break;
        sent += bytes;
    } while(sent < respSize);

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

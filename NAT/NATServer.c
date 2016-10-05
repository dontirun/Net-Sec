// Standard libraries
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
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

#define NATIP "10.4.11.192"
#define DNSIP "10.4.11.193"
#define SERVERIP "192.168.1.101"

#define PUBLICIPRANGESTART 194
#define PUBLICIPRANGEEND 254

#define MAXMESSAGESIZE 21

typedef struct {
    char *message;
    int socket;
} Request;

// Function Prototypes
void printUsage();
void term(int signum);

void insertNATAccessRules();
void deleteNATAccessRules();

void* handleClientConnections(void* socket);
void* handleRequest(void *request);
char* manipulateMapping(char *ip);

int recv_all(char **message, int socket);
int send_all(char *message, int messageSize, int socket);

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

    // Add IPTables rule to only accept packets from valid NAT and Server
    insertNATAccessRules();

    // Establish connection between NAT and Client
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
        
        // Create a new thread for each client
        pthread_t *cliThread = malloc(sizeof(pthread_t));
        int rc = pthread_create(cliThread, NULL, handleClientConnections, (void*)cliSock);
        if(rc) {
            printf("Error creating thread for client, Code: %d\n", rc);
            continue;
        }

        // Detach thread
        pthread_detach(*cliThread);
    }
    
    printf("\nShutting Down Server...\n");

    // Remove the NAT Access rules
    deleteNATAccessRules();

    // Close Sockets
    close(sockfd);
    
    // Exit main thread
    exit(0);
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

void checkAddRule(char *preCheckRule, char* command, char *postCheckRule) {
    char *checkCommand, *applyCommand;

    asprintf(&checkCommand, "%s -C %s", preCheckRule, postCheckRule);
    asprintf(&applyCommand, "%s %s %s", preCheckRule, command, postCheckRule);

    if(strcmp(command, "-A") == 0) {
        if(system(checkCommand))
            system(applyCommand);
    } else if(strcmp(command, "-D") == 0) {
        if(!system(checkCommand))
            system(applyCommand);
    }
}

void insertNATAccessRules() {
    char *command;

    // Filter packets being processed by NAT
    checkAddRule("sudo iptables -t filter", "-A", "INPUT -i lo -j ACCEPT");
    checkAddRule("sudo iptables -t filter", "-A", "INPUT -p tcp --dport ssh -j ACCEPT");
    asprintf(&command, "INPUT -s %s -j ACCEPT", SERVERIP);
    checkAddRule("sudo iptables -t filter", "-A", command);
    asprintf(&command, "INPUT -s %s -j ACCEPT", DNSIP);
    checkAddRule("sudo iptables -t filter", "-A", command);
    system("sudo iptables -t filter -P INPUT DROP");

    // Filter packets being Forwarded
    asprintf(&command, "FORWARD -p udp -d %s -j ACCEPT", DNSIP);
    checkAddRule("sudo iptables -t filter", "-A", command);
    system("sudo iptables -t filter -P FORWARD DROP");

    free(command);
}

void deleteNATAccessRules() {
    char *command;

    // Remove filters for packets being processed by NAT
    system("sudo iptables -t filter -P INPUT ACCEPT");
    checkAddRule("sudo iptables -t filter", "-D", "INPUT -i lo -j ACCEPT");
    checkAddRule("sudo iptables -t filter", "-D", "INPUT -p tcp --dport ssh -j ACCEPT");
    asprintf(&command, "INPUT -s %s -j ACCEPT", SERVERIP);
    checkAddRule("sudo iptables -t filter", "-D", command);
    asprintf(&command, "INPUT -s %s -j ACCEPT", DNSIP);
    checkAddRule("sudo iptables -t filter", "-D", command);

    // Remove filters for packets being Forwarded
    system("sudo iptables -t filter -P FORWARD ACCEPT");
    asprintf(&command, "FORWARD -p udp -d %s -j ACCEPT", DNSIP);
    checkAddRule("sudo iptables -t filter", "-D", command);

    free(command);
}

/**
 * Input:
 *     socket - Socket for communicating with the client
 * Output:
 *     None
 *
 * Listen for requests from clients and handle them.
 */
void* handleClientConnections(void* socket) {
    int cliSock = *((int *)socket);

    while(!quit) {
        // Recieve Request
        char *messagePtr = NULL;
        recv_all(&messagePtr, cliSock);
        
        // Check quit flag after read call is unblocked 
        if(quit)
            break;
        
        // Check request is valid
        if(strlen(messagePtr) <= 0)
            continue;
        
        // Create Request
        Request *request = malloc(sizeof(Request));
        request->message = messagePtr;
        request->socket = cliSock;
        
        // Create thread to handle request
        pthread_t *thread = malloc(sizeof(pthread_t));
        int response = pthread_create(thread, NULL, handleRequest, (void *)request);
        if(response) {
            printf("Error creating thread, Request %s, Code: %d\n", messagePtr, response);
            // Handle error so request will be finished in some way
            continue;
        }
                
        // Detach thread
        pthread_detach(*thread);
    }

    // Close Socket
    close(cliSock);

    // Free resources
    free(socket);

    // Destroy client thread
    pthread_exit(0);
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
    char *mappedIP;

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
        mappedIP = manipulateMapping(ip);

        if(mappedIP == NULL) {
            // Error creating mapping
            mappedIP = malloc(5);
            mappedIP = "NONE";
        }
    } else if(strcmp(command, "BAN") == 0) {
        // Blacklist given IP
    }

    // Send response with mapping
    char *resp;
    int respSize = asprintf(&resp, "ACK;%s", mappedIP) + 1;
    send_all(resp, respSize, cliSock);

    // Free resources
    free(resp);
    free(message);
    free(request);
    if(mappedIP != NULL)
        free(mappedIP);

    // Destroy request thread
    pthread_exit(0);
}

/**
 * Input:
 *     IP address given by the DNS, corresponds to the IP prefix of the ISP
 * Output:
 *     Public ip from mapping if successful, NULL otherwise
 *
 * Manipulate mappings based on commands from the NAT and Server
 */
char* manipulateMapping(char *ip) {
    int cp1, cp2, cp3, cp4;
    char *ispPrefix, *publicIP;

    // Separate the Client IP into the four parts
    sscanf(ip, "%d.%d.%d.%d\n", &cp1, &cp2, &cp3, &cp4);

    // Calculate the range of the subnet of client ISP
    asprintf(&ispPrefix, "%d.%d.%d.%d/26", cp1 & 255, cp2 & 255, cp3 & 255, cp4 & 192);

    // Randomly generate an int within range of public IPs
    time_t t;
    srand((unsigned) time(&t));
    int i = rand() % (PUBLICIPRANGEEND - PUBLICIPRANGESTART + 1) + PUBLICIPRANGESTART;
    asprintf(&publicIP, "10.4.11.%d", i);
    
    // Form iptable command
    char *command;
    asprintf(&command, "PREROUTING -s %s -d %s -j DNAT --to %s", ispPrefix, publicIP, SERVERIP);
    checkAddRule("sudo iptables -t nat", "-A", command);
    asprintf(&command, "FORWARD -p tcp --dport http -s %s -d %s -j ACCEPT", ispPrefix, publicIP);
    checkAddRule("sudo iptables -t filter", "-A", command);
    asprintf(&command, "POSTROUTING -s %s -d %s -j SNAT --to %s", SERVERIP, ispPrefix, publicIP);
    checkAddRule("sudo iptables -t nat", "-A", command);

    // Free resources
    free(command);
    free(ispPrefix);
    
    return publicIP;
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
            perror("\nError - Cannot read from socket\n");
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

// Standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// Network Imports
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

// Multithreaded Imports
#include <pthread.h>

// Function Prototypes
void printUsage();
void term();
int insertMapping(char *ip);
int send_all(char *message, int messageSize);

volatile sig_atomic_t quit = 0;


/**
 * Run in a continuous loop and await commands from the DNS
 */
int main(int argc, char **argv) {

    char hostName[1024], message[1024];
    int portNum,sockfd, accSock;
    int messageSize;
    struct hostent *host;
    struct in_addr *hostIP;
    struct sockaddr_in servAddr, cliAddr;
    socklen_t cliSize;
   
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
    hostIP = host->h_addr;
    
    // Print the IP and the listening port on the server
    printf("Server is listening at %s:%d\n", inet_ntoa(*hostIP), portNum); 

    while(!quit) {
        // Recieve Requests
        listen(sockfd, 10);
        cliSize = sizeof(cliAddr);     
        accSock = accept(sockfd, (struct sockaddr *)&cliAddr, &cliSize);
        if(accSock < 0) {
            perror("Error - Cannot accept client\n");
            continue;
        }
        memset(message, 0, 1024);
        messageSize = read(accSock, message, 1023);
        if(messageSize < 0) {
            perror("Error - Cannot read from socket\n");
            continue;
        }
        char *messagePtr = message;
        char *command = malloc(3);
	char *ip = malloc(15);
        sscanf(messagePtr, "%s;%s", command, ip);
	printf("%s\n", command);

        // Send Response
        char *resp;
        int respSize = asprintf(&resp, "FIN;0.0.0.0") + 1;
        send_all(resp, respSize);
    }
    printf("Shutting Down Server...\n");

    // Close Sockets
    close(sockfd);
    close(accSock);

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
 *     Pointer to an uninitialized character pointer
 * Output:
 *     None
 *
 * Read the message from the socket if there is one
 */
int recv_all(char **message) {

}

/**
 * Input:
 *     Pointer to a character array
 * Output:
 *     0 if send was successful, 1 otherwise
 *
 * Send the given message
 */
int send_all(char *message, int messageSize) {
    int sent = 0;
    int bytes = 0;
    char *resp;
    int respSize = asprintf(&resp, "FIN;0.0.0.0") + 1
    do {
        bytes = send(accSock, resp+sent, respSize-sent, MSG_NOSIGNAL);
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

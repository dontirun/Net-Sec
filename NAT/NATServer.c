#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>

// Function Prototypes
void printUsage();
void term();

volatile sig_atomic_t quit = 0;

int main(int argc, char **argv) {

    char hostName[1024], message[1024];
    int portNum,sockfd, accSock;
    int messageSize;
    struct hostent *host;
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

    gethostname(hostName, 1023);
    host = gethostbyname(hostName);

    printf("Server is listening at %s:%d\n",host->h_name, portNum); 

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
        int sent = 0;
        int bytes = 0;
        char *resp;
        asprintf(&resp, "FIN;0.0.0.0");
        int respSize = 20;
        do {
            bytes = send(accSock, resp+sent, respSize-sent, MSG_NOSIGNAL);
            if(bytes < 0) {
                exit(1);
            }
            if(bytes == 0)
                break;
            sent += bytes;
        } while(sent < respSize);
    }
    printf("Shutting Down Server...\n");

    // Close Sockets
    close(sockfd);
    close(accSock);

    return 0;
}

void term(int signum) {
    quit = 1;
}

void printUsage() {
    printf("\nUsage:\n");
    printf("    ./http_server port_number\n");
    printf("\n");
}

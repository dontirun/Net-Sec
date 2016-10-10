#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>

#define MAXRESPONSESIZE 21

// Flags
int PFLAG = 0;

// Prototype Functions
void printUsage();
char* pathToFile(char *url, int *pathStart);

int main(int argc, char** argv) {

    struct timeval startTime;
    struct timeval stopTime;
    int portNum = 0;
    char *serverURL = "";

    int sockfd;
    struct hostent *server;
    struct sockaddr_in serverAddr;

    // Setup server url, port number and options
    if(argc < 3 || argc > 4) {
        printUsage();
        return 0;
    } else {
        
        // Check and get the command line options
        char c = ' ';
        while((c = getopt(argc, argv, "p")) != -1) {
            switch(c) {
                case 'p':
                    PFLAG = 1;
                    break;
                default:
                    break; 
            }
        }

        // Check and get the command line arguments
        int index;
        for(index = optind; index < argc; index++) {
            if (argc - 2 == index)
                serverURL = argv[index];
            else
                portNum = atoi(argv[index]);
        }
    }

    // Open Socket and store the associated file descriptor
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("ERROR - Opening Socket\n");
        exit(1);
    }    

    // Get Server with the given name
    if((server = gethostbyname(serverURL)) == NULL) {
        perror("ERROR - Invalid Server Address\n");
        return -1;
    }

    // Connect to Server
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum);
    serverAddr.sin_addr = *((struct in_addr *)server->h_addr);
    int i;
    for(i = 0; i < sizeof(serverAddr.sin_zero); i++) {
        serverAddr.sin_zero[i] = 0;
    }
    if(connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) == -1) {
        perror("ERROR - Cannot connect to Server\n");
        exit(1);
    }

    for(i = 0; i < 20; i++) {

	printf("%d\n", i);
	sleep(1);

	// Construct the GET Request (Test Remove #TODO)
	char *NATRequest;
	int NATRLength = asprintf(&NATRequest, "ADD;192.168.1.%d", i);
	printf("Request: %s\n", NATRequest);

	// Send Request to Server
	int sent = 0;
	int bytes = 0;
	do {
            gettimeofday(&startTime, NULL); // Start Timer
            bytes = write(sockfd, NATRequest + sent, NATRLength - sent);
            if(bytes < 0) {
                perror("ERROR - Cannot write to Socket\n");
                exit(1);
            } else if(bytes == 0)
                break;
            sent += bytes;
        } while(sent < NATRLength);

        // Recieve Response from Server
        char response[MAXRESPONSESIZE];
        memset(response, 0, sizeof(response));
        do {
            bytes = read(sockfd, response, MAXRESPONSESIZE);
	    gettimeofday(&stopTime, NULL); // Stop Timer
            int i;
            for(i = 0; i < MAXRESPONSESIZE; i++) {
	        if(response[i] == '\0')
                    break;
            }
	    printf("%s\n", response);
            memset(response, 0, sizeof(response));
            if(bytes < 0) {
                perror("ERROR - Cannot read from Socket\n");
                exit(1);
            }
            if(bytes > 0)
                break;
        } while(1);
    }

    // Close Socket
    close(sockfd);

    if(PFLAG == 1) {
        struct timeval result;
        timersub(&stopTime, &startTime, &result);
        printf("\n\nRTT Time: %.2f ms\n", (double)(result.tv_sec*1000 + result.tv_usec/1000));
    } else {
        printf("\n");
    }

    return 0;

}

void printUsage() {
    printf("\nUsage:\n");
    printf("    ./http_client [-options] server_url port_number\n");
    printf("Options\n");
    printf("    -p\n");
    printf("        Prints the RTT for accessing the URL\n\n");
}

char* pathToFile(char *url, int *pathStart) {

    char* path = malloc(512*sizeof(char));

    int i = 0;
    int pathSize = 0;
    int pathLoc = 0;
    size_t reallocSize = 512*sizeof(char);
    while(url[i] != '\0') {
        if(url[i] == '/') {
            if(*pathStart == 0)
                *pathStart = i;
            pathLoc = 1;
        }
        if(pathLoc == 1) {
            if(pathSize > 511) {
                reallocSize += 512*sizeof(char);
                char *pathTmp = realloc(path, reallocSize);
                if(pathTmp != NULL) {
                    free(path);
                    path = pathTmp;
                } else {
                    perror("ERROR - Path to file on Server too large\n");
                    exit(1);
                }
            }
            path[pathSize] = url[i];
            pathSize++;
        }
        i++;
    }
    path[pathSize] = '\0';
    
    return path;
}

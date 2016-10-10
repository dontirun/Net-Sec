/**
 * Author: Sai Kiran Vadlamudi
 * Date: 10/08/2016
 */

#include "NATServer.h"

// Private Prototypes
void term(int signum);

void insertNATAccessRules();
void deleteNATAccessRules();

time_t getExpirationTimeMap(void *elm);
void handleExpiredMap(void *elm);
void handleExpiredBan(void *elm);

void* handleClientConnections(void *socket);
void* handleRequest(void *request);
SuccessMapping* manipulateMapping(char *ip);
void* removeExpiredMappings();

int recv_all(char **message, int socket);
int send_all(char *message, int messageSize, int socket);

// Global Variables
volatile sig_atomic_t quit = 0;
pthread_mutex_t mappingListLock;
LinkedList *mappingList;
pthread_mutex_t bannedListLock;
LinkedList *bannedList;

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

    // Create thread to maintain mappings and clean up expired mappings
    mappingList = createList();
    pthread_t *mappingCleaner = malloc(sizeof(pthread_t));
    CleanerData *data = malloc(sizeof(CleanerData));
    data->list = mappingList;
    data->expirationTime = getExpirationTimeMap;
    data->expirationAction = handleExpiredMap;
    data->listLock = mappingListLock;
    int responseCode = pthread_create(mappingCleaner, NULL, removeExpiredMappings, data);
    if(responseCode) {
        printf("Error - Cannot create thread to remove mappings, Code: %d\n", responseCode);
        exit(1);
    }
    pthread_detach(*mappingCleaner);
    free(mappingCleaner);

    // Create thread to remove bans after a set period
    bannedList = createList();
    pthread_t *banRemover = malloc(sizeof(pthread_t));
    data = malloc(sizeof(CleanerData));
    data->list = bannedList;
    data->expirationTime = getExpirationTimeMap;
    data->expirationAction = handleExpiredBan;
    data->listLock = bannedListLock;
    responseCode = pthread_create(banRemover, NULL, removeExpiredMappings, data);
    if(responseCode) {
        printf("Error - Cannot create thread to remove bans, Code: %d\n", responseCode);
        exit(1);
    }
    pthread_detach(*banRemover);
    free(banRemover);

    // Establish connection between NAT and Client
    pthread_t *cliThread = malloc(sizeof(pthread_t));
    while(!quit) {
        listen(sockfd, 10);
        cliSize = sizeof(cliAddr);   
        int* cliSock = malloc(sizeof(int));
        *cliSock = accept(sockfd, (struct sockaddr *)&cliAddr, &cliSize);
        if(*cliSock < 0) {
            if(quit == 1)
                break;
            continue;
        }
        
        // Create a new thread for each client
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
    free(cliThread);

    // Close Sockets
    close(sockfd);
    
    // Exit main thread
    exit(0);
}

/******************************************************************
 *                      Utility Functions                         *
 ******************************************************************/

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

/**
 * Input:
 *     preCheckRule - defines table and initial command Ex: "sudo iptables -t TABLENAME"
 *     command- "-A" to add rule and "-D" to delete rule
 *     postCheckRule - contains chain, filters and actions
 * Output:
 *     void
 *
 * Check if rule exists and only add given rule if it doesn't exist.
 */
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

    free(checkCommand);
    free(applyCommand);
}

/**
 * Input: None
 * Output: None
 * 
 * Setup the rules to only push the certain packets to the NAT application layer.
 */
void insertNATAccessRules() {
    char *command;

    // Filter packets being processed by NAT
    checkAddRule("sudo iptables -t filter", "-A", "INPUT -i lo -j ACCEPT");
    checkAddRule("sudo iptables -t filter", "-A", "INPUT -p tcp --dport ssh -j ACCEPT");
    asprintf(&command, "INPUT -s %s -j ACCEPT", SERVERIP);
    checkAddRule("sudo iptables -t filter", "-A", command);
    free(command);

    asprintf(&command, "INPUT -s %s -j ACCEPT", DNSIP);
    checkAddRule("sudo iptables -t filter", "-A", command);
    free(command);

    system("sudo iptables -t filter -P INPUT DROP");
}

/*
 * Input: None
 * Output: None
 *
 * Remove the rules to only push the certain packets to the NAT application layer.
 */
void deleteNATAccessRules() {
    char *command;

    // Remove filters for packets being processed by NAT
    system("sudo iptables -t filter -P INPUT ACCEPT");
    checkAddRule("sudo iptables -t filter", "-D", "INPUT -i lo -j ACCEPT");
    checkAddRule("sudo iptables -t filter", "-D", "INPUT -p tcp --dport ssh -j ACCEPT");

    asprintf(&command, "INPUT -s %s -j ACCEPT", SERVERIP);
    checkAddRule("sudo iptables -t filter", "-D", command);
    free(command);

    asprintf(&command, "INPUT -s %s -j ACCEPT", DNSIP);
    checkAddRule("sudo iptables -t filter", "-D", command);
    free(command);
}

/******************************************************************
 *                      Call Back Functions                       *
 ******************************************************************/

/**
 * Input:
 *     elm1 - the main element
 *     elm2 - the element being compared
 * Output:
 *     0 if the elements are the same,1 otherwise
 *
 * Compares the given element with the main element
 */
int cmpMaps(void* elm1, void *elm2) {
    // Convert inputs to proper form
    Mapping *map1 = (Mapping *) elm1;
    Mapping *map2 = (Mapping *) elm2;
    int ispCompare, publicCompare, timeCompare, numMatchedFields = 0, numFields = 0;

    if(map1->ispPrefix != NULL) {
        ispCompare = strcmp(map1->ispPrefix, map2->ispPrefix);
        numMatchedFields += ispCompare == 0 ? 1 : 0;
        numFields++;
    }
    
    if(map1->publicIP != NULL) {
        publicCompare = strcmp(map1->publicIP, map2->publicIP);
        numMatchedFields += publicCompare == 0 ? 1 : 0;
        numFields++;
    }

    if(map1->time != 0) {
        timeCompare = map1->time - map2->time;
        numMatchedFields += timeCompare == 0 ? 1 : 0;
        numFields++;
    }

    if(numMatchedFields == numFields)
        return 0;
    else
        return 1;
}

/*
 * Input:
 *     elm - Mapping struct 
 * Output:
 *     Returns the timestamp of the given element
 */
time_t getExpirationTimeMap(void *elm) {
    Mapping *map = (Mapping *) elm;
    return map->time + EXPIRATIONTIMESECONDS + GRACEPERIODSECONDS;
}

/**
 * Input:
 *     elm - Expired Mapping
 * Output:
 *     None
 */
void handleExpiredMap(void *elm) {
    Mapping *map = (Mapping *) elm;
    char *command;
 
    printf("Removing mapping from %s to %s\n", map->ispPrefix, map->publicIP);

    // Add the mapping for the established connections only
    asprintf(&command, "PREROUTING -s %s -d %s -m conntrack --ctstate ESTABLISHED -j DNAT --to %s", map->ispPrefix, map->publicIP, SERVERIP);
    checkAddRule("sudo iptables -t nat", "-A", command);
    free(command);

    asprintf(&command, "POSTROUTING -s %s -d %s  -m conntrack --ctstate ESTABLISHED -j SNAT --to %s", SERVERIP, map->ispPrefix, map->publicIP);
    checkAddRule("sudo iptables -t nat", "-A", command);
    free(command);

    // Remove the mapping from the iptables
    asprintf(&command, "PREROUTING -s %s -d %s -j DNAT --to %s", map->ispPrefix, map->publicIP, SERVERIP);
    checkAddRule("sudo iptables -t nat", "-D", command);
    free(command);

    asprintf(&command, "POSTROUTING -s %s -d %s -j SNAT --to %s", SERVERIP, map->ispPrefix, map->publicIP);
    checkAddRule("sudo iptables -t nat", "-D", command);
    free(command);

    // Free resources
    free(map->ispPrefix);
    free(map->publicIP);
    free(map);
}

/**
 * Input:
 *     elm - expired Ban
 * Output:
 *     None
 */
void handleExpiredBan(void *elm) {
    Mapping *map = (Mapping *) elm;
    char *command;

    // Remove the ban
    asprintf(&command, "FORWARD -s %s -j DROP", map->ispPrefix);
    checkAddRule("sudo iptables -t filter", "-D", command);
    printf("Unbanning IP: %s\n", map->ispPrefix);

    // Free Resources
    free(map->publicIP);
    free(map);
    free(command);
}

/******************************************************************
 *                 Client and Request Handling                    *
 ******************************************************************/

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
    printf("Handling Client on Socket: %d\n", cliSock);

    while(!quit) {
        // Recieve Request
        char *messagePtr = NULL;
        recv_all(&messagePtr, cliSock);

        // Check quit flag after read call is unblocked 
        if(quit) {
	    free(messagePtr);
            break;
	}
        
        // Check request is valid
        if(strlen(messagePtr) <= 0) {
            free(messagePtr);
	    continue;
	}
        
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
	free(thread);
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
    char *mappedIP = NULL; 
    int mappingTTL = 0, addRequest = 1;

    // Convert given pointer to actual type
    char *message = ((Request *)request)->message;
    int cliSock = ((Request *) request)->socket;
    
    if(strlen(message) <= 0) {
        pthread_exit(0);
    }
    
    // Retrive the Command and the IP from the request
    char *command = strtok(message, ";");
    char *ip = strtok(NULL, ";");

    if(ip == NULL || strlen(ip) < 7 || strlen(ip) > 12) {
        free(request);
	if(message != NULL)
	    free(message);

        pthread_exit(0);
    } else if(command == NULL || strlen(command) < 3 || strlen(command) > 3) {
        free(request);
	if(message != NULL)
	    free(message);

        pthread_exit(0);
    }

    printf("Command: %s, IP: %s\n", command, ip);

    // Act based on the command
    if(strcmp(command, "ADD") == 0) {
	// Create mapping for the given IP
        SuccessMapping *sm = manipulateMapping(ip);
        mappedIP = sm->mappedIP;
        mappingTTL = sm->mapTTL;
        free(sm);

        if(mappedIP == NULL) {
            // Error creating mapping
            mappedIP = malloc(5);
            mappedIP = "NONE";
        }
    } else if(strcmp(command, "BAN") == 0) {
        // Blacklist given IP
	addRequest = 0;
        char *rule;
        asprintf(&rule, "FORWARD -s %s -j DROP", ip);
        checkAddRule("sudo iptables -t filter", "-A", rule);
	free(rule);
        mappedIP = ip;
	mappingTTL = EXPIRATIONTIMESECONDS + GRACEPERIODSECONDS;

        // Add IP to banned list
        Mapping *map = malloc(sizeof(Mapping));
        time(&(map->time));
        map->ispPrefix = strdup(ip);
        pthread_mutex_lock(&bannedListLock);
        insertElement(bannedList, map);
        pthread_mutex_unlock(&bannedListLock);
    }

    // Send response with mapping
    char *resp;
    int respSize = asprintf(&resp, "ACK;%s;%d", mappedIP, mappingTTL);
    send_all(resp, respSize, cliSock);

    // Free resources
    free(resp);
    free(request);
    if(message != NULL)
        free(message);
    if(mappedIP != NULL && addRequest == 1)
        free(mappedIP); 

    // Destroy request thread
    pthread_exit(0);
}

/**
 * Input:
 *     IP address given by the DNS, corresponds to the IP prefix of the ISP
 * Output:
 *     Public ip from mapping if successful, NULL otherwise. Do not free given string
 *
 * Manipulate mappings based on commands from the NAT and Server
 */
SuccessMapping* manipulateMapping(char *ip) {
    int cp1, cp2, cp3, cp4;
    char *ispPrefix = NULL, *publicIP = NULL;
    time_t timeNow = 0;

    // Separate the Client IP into the four parts
    sscanf(ip, "%d.%d.%d.%d\n", &cp1, &cp2, &cp3, &cp4);

    // Calculate the range of the subnet of client ISP
    asprintf(&ispPrefix, "%d.%d.%d.%d/26", cp1 & 255, cp2 & 255, cp3 & 255, cp4 & 192);

    // Search list to see if a mapping is present
    Mapping *searchMap = malloc(sizeof(Mapping));
    searchMap->ispPrefix = ispPrefix;
    searchMap->publicIP = NULL;
    searchMap->time = 0;

    pthread_mutex_lock(&mappingListLock);
    Mapping *match = findElement(mappingList, searchMap, cmpMaps);
    pthread_mutex_unlock(&mappingListLock);

    if(match != NULL) {
    printf("Match ISP Prefix: %s, IP: %s, Time: %d\n", match->ispPrefix, match->publicIP, match->time);   

	// Check for non-expired mappings
        time(&timeNow);   
        if(timeNow - match->time < EXPIRATIONTIMESECONDS) {
            SuccessMapping *map = malloc(sizeof(SuccessMapping));
            map->mappedIP = strdup(match->publicIP);
            map->mapTTL = EXPIRATIONTIMESECONDS - (timeNow - match->time);

	    free(searchMap->ispPrefix);
	    free(searchMap);

            return map;
        }
    } 

    // Randomly generate an int within range of public IPs that isn't being used'
    int matchFound = 0;
    do {
	if(publicIP != NULL)
	    free(publicIP);

        srand((unsigned) time(&timeNow));
        int i = rand() % (PUBLICIPRANGEEND - PUBLICIPRANGESTART + 1) + PUBLICIPRANGESTART;
        asprintf(&publicIP, SERVERFLUXCLASSAADDRESS, i);

        searchMap->ispPrefix = NULL;
        searchMap->publicIP = publicIP;
        
        match = NULL;
        pthread_mutex_lock(&mappingListLock);
        match = findElement(mappingList, searchMap, cmpMaps);
        pthread_mutex_unlock(&mappingListLock);
        
        if(match != NULL)
            matchFound = 1;
        else
            matchFound = 0;
    } while(matchFound);

    // Add mappings to IPTables
    char *command;
    asprintf(&command, "PREROUTING -s %s -d %s -j DNAT --to %s", ispPrefix, publicIP, SERVERIP);
    checkAddRule("sudo iptables -t nat", "-A", command);
    free(command);

    asprintf(&command, "POSTROUTING -s %s -d %s -j SNAT --to %s", SERVERIP, ispPrefix, publicIP);
    checkAddRule("sudo iptables -t nat", "-A", command);
    free(command);

    // Store mapping information and time
    Mapping *map = malloc(sizeof(Mapping));
    time(&timeNow);
    map->time = timeNow;
    map->ispPrefix = strdup(ispPrefix);
    map->publicIP = strdup(publicIP);
    
    // Lock the list and insert the mapping into the list
    pthread_mutex_lock(&mappingListLock);
    insertElement(mappingList, map);
    pthread_mutex_unlock(&mappingListLock);

    SuccessMapping *sm = malloc(sizeof(SuccessMapping));
    sm->mappedIP = publicIP;
    sm->mapTTL = EXPIRATIONTIMESECONDS;

    // Free resources
    free(ispPrefix);
    free(searchMap);
    
    return sm;
}

/**
 * Input:
 *     None
 * Output:
 *     NULL
 *
 * Removes expired mappings
 */ 
void* removeExpiredMappings(void *data) {
    time_t timeNow, timeUntilExpiration;
    CleanerData *cld = (CleanerData *) data;

    // Run till the user choose to quit
    while(!quit){
        if(cld->list->head == NULL) {
            sleep(1);
            continue;
        }

        // Sleep until the mapping is supposed to expire
        timeUntilExpiration = cld->expirationTime(cld->list->head->elm);
        time(&timeNow);
        while(timeNow < timeUntilExpiration) {
            sleep(timeUntilExpiration - timeNow);
            time(&timeNow);
        }
        
        // Remove the mapping from the list
	pthread_mutex_lock(&(cld->listLock));
        void *elm = popElement(cld->list);
	pthread_mutex_unlock(&(cld->listLock));

        // Run the expiration action
        cld->expirationAction(elm);
    }

    // Free resources
    free(cld);

    // Destroy thread
    pthread_exit(0);
}

/******************************************************************
 *                     Networking Components                      *
 ******************************************************************/

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
        *message = malloc(MAXREQUESTMESSAGESIZE);
    }

    // Read in the message
    memset(*message, 0, MAXREQUESTMESSAGESIZE);
    int messageSize = read(socket, *message, MAXREQUESTMESSAGESIZE - 1);
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

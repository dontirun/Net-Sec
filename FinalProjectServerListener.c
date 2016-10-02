//Arun Donti Enhanced Client
//This code has been adapted from :
//http://www.tcpdump.org/sniffex.c

// to compile
//gcc -m32 FinalProjectServerListener.c -o test -lm -lpcap
#define SIZE_ETHERNET 14

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <pcap.h>


// IP header 
struct sniff_ip {
	u_char  ip_vhl;                 // version << 4 | header length >> 2
	u_char  ip_tos;                 // type of service
	u_short ip_len;                 // total length
	u_short ip_id;                  // identification
	u_short ip_off;                 // fragment offset field
	#define IP_RF 0x8000            // reserved fragment flag 
	#define IP_DF 0x4000            // dont fragment flag
	#define IP_MF 0x2000            // more fragments flag
	#define IP_OFFMASK 0x1fff       // mask for fragmenting bits
	u_char  ip_ttl;                 // time to live
	u_char  ip_p;                   // protocol
	u_short ip_sum;                 // checksum
	struct  in_addr ip_src,ip_dst;  // source and dest address
};

// Struct of hops + IP, might be more complicated than FILE I/O
/*
struct IPhop{
	inet_ntoa addr;
	int hops;
	*IPhop next;
};

typedef struct IPhop IPhop;
*/

void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

void DieWithUserMessage(const char *msg, const char *detail) {
	fputs(msg, stderr);
	fputs(": ", stderr);
	fputs(detail, stderr);
	fputc('\n', stderr);
	exit(1);
}

void DieWithSystemMessage(const char *msg) {
	perror(msg);
	exit(1);
}

//Creating a stream socket using TCP
int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

int main(int argc, char *argv[]) {

	pcap_t *handle;			// Session handle 
	char *dev;			// The device to sniff on 
	char errbuf[PCAP_ERRBUF_SIZE];	// Error string 
	struct bpf_program fp;		//The compiled filter 
	// char filter_exp[] = "";	// The filter expression
	bpf_u_int32 mask;		// Our netmask
	bpf_u_int32 net;		// Our IP
	struct pcap_pkthdr header;	// The header that pcap gives us
	const u_char *packet;		// The actual packet 

	//private ip for NAT
	char* natIP = "10.4.11.3";
	// Establish connection with NAT
	// Constructing the server address struct
	struct sockaddr_in servAddr; // server address
	memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
	servAddr.sin_family = AF_INET;// IPv4 address family
	// Convert address
	int rtnVal = inet_pton(AF_INET, natIP, &servAddr.sin_addr.s_addr);
	if (rtnVal == 0){
		DieWithUserMessage("inet_pton() failed", "invalid address string");
	}
	else if (rtnVal < 0){
		DieWithSystemMessage("inet_pton() failed");
	}
	in_port_t servPort = 5002;
	servAddr.sin_port = htons(servPort); //Local port

	// Establish the connection to the NAT
	if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
	DieWithSystemMessage("connect() failed");


	// libpcap stuff
	dev = pcap_lookupdev(errbuf);

	if (dev == NULL) {
		fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
		return(2);
	}

	/* Find the properties for the device */
	if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
		fprintf(stderr, "Couldn't get netmask for device %s: %s\n", dev, errbuf);
		net = 0;
		mask = 0;
	}
	/* Open the session in promiscuous mode */
	handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
	if (handle == NULL) {
		fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
		return(2);
	}
	/*
	// Compile and apply the filter 
	if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
		return(2);
	}
	if (pcap_setfilter(handle, &fp) == -1) {
		fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
		return(2);
	}
	*/
	// Grab a packet until error occurs //
	packet = pcap_loop(handle,-1,got_packet,NULL);
	pcap_close(handle);

}

//Maurizio, should add code in here 
void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet){

	FILE *fp;
	char buff[50];
	fp = fopen("hopsByIP.txt","a+");
	static int count = 1;                   // packet counter 
	

	const struct sniff_ip *ip;              // The IP header 

	
	printf("\nPacket number %d:\n", count);
	count++;
	ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);

	// the file already contains this IP

	int getHops = NULL;
	while(fgets(buff, sizeof(buff), fp) != NULL){
		if(strstr(buff,inet_ntoa(ip->ip_src))){
			fgets(buff, sizeof(buff), fp);
			getHops = atoi(buff);
			printf("%d hops listed in list\n ",getHops);
			break;
		}
	}
	if(getHops != NULL){
		// let through if reasonably close hop count
		int hopEQ = ceil((double)getHops *.05) + 1;
		printf("hopEQ is %d\n", hopEQ);
		printf("ttl on incomming packet is %d\n", (int)(ip->ip_ttl));

		if ((int)(ip->ip_ttl) <= getHops + hopEQ && (int)(ip->ip_ttl) >= getHops - hopEQ){
			printf("ttl difference is okay for %s\n",inet_ntoa(ip->ip_src));
		}
		// blacklist this ip , add SERVER NAT communication for blacklist
		else{
			printf("BLACKLIST %s\n",inet_ntoa(ip->ip_src));
			char[20] banCommand; 
			strcat(banCommand,"BAN ");
			strcat(banCommand, inet_ntoa(ip->ip_src));
			ssize_t banLength = strlen(banCommand);

			// Send the string to the server
			ssize_t numBytes = send(sock, banCommand, banLength, 0);
			if (numBytes < 0){
				DieWithSystemMessage("send() failed");
			}
			else if (numBytes != banLength){
				DieWithUserMessage("send()", "sent unexpected number of bytes");
			}


		}
	}
	
	// print source IP  and the TTL if it hasn't connected before' 
	else{
		fprintf(fp,"\n%s \n%d", inet_ntoa(ip->ip_src), (ip->ip_ttl));
	}

	// Maurizio, everything is okay so send okay stuff to client

	fclose(fp);
	return;
}


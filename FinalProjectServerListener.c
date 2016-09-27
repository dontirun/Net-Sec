//Arun Donti Enhanced Client
//This code has been adapted from :
//http://www.tcpdump.org/sniffex.c
#define SIZE_ETHERNET 14

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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


static const int MAXPENDING = 5; // Maximum outstanding connection requests

void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

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


void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
	{

		static int count = 1;                   // packet counter 
		
		// declare pointers to packet headers 
	//	const struct sniff_ethernet *ethernet;  // The ethernet header [1] 
		const struct sniff_ip *ip;              // The IP header 
	//	const struct sniff_tcp *tcp;            // The TCP header 
		const char *payload;                    // Packet payload 

		int size_ip;
	//	int size_tcp;
		int size_payload;
		
		printf("\nPacket number %d:\n", count);
		count++;
		
		// define ethernet header 
	//	ethernet = (struct sniff_ethernet*)(packet);
		
		// define/compute ip header offset 
		ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
		size_ip = IP_HL(ip)*4;
		if (size_ip < 20) {
			printf("   * Invalid IP header length: %u bytes\n", size_ip);
			return;
		}

		// print source and destination IP addresses 
		printf("       From: %s\n", inet_ntoa(ip->ip_src));
		printf("         the ttl is: %s\n", inet_ntoa(ip->ip_ttl));

	return;
	}

}
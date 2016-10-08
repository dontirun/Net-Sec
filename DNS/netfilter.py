from netfilterqueue import NetfilterQueue
from scapy.all import *
from socket import *
import sys

s = socket(AF_INET, SOCK_STREAM)

# I asked Jesse if he trusted my code, he did, so it should work at some point

def exit(s):
    if s:
        s.close()
    sys.exit(0)

def NAT_update(ip):
    s.send('ADD;{}\0'.format(ip))
    ack = s.recv(20)
    print ack
    ipback = ack.strip().split(';')
    print ipback[1]
    return (ipback[1],int(ipback[2]))

def handle_dns(pkt):
    ip = IP()
    udp = UDP()
    ip.src = pkt[IP].src
    ip.dst = pkt[IP].dst
    udp.sport = pkt[UDP].sport
    udp.dport = pkt[UDP].dport

    natreply = NAT_update(pkt[IP].dst)
    print natreply[0]
    print natreply[1]
    dns = DNS(id = qd.id, qr = 1, qdcount = 1, ancount = 1, nscount = 1, rcode = 0)
    dns.qd = qd[DNSQR]
    dns.an = DNSRR(rrname = pkt[UDP][DNSQR].qname, ttl = natreply[1], rdlen = 4, rdata = natreply[0]) 
    dns.ns = DNSRR(rrname = pkt[UDP][DNSQR].qname, ttl = natreply[1], rdlen = 4, rdata = natreply[0]) 
    dns.ar = DNSRR(rrname = pkt[UDP][DNSQR].qname, ttl = natreply[1], rdlen = 4, rdata = natreply[0]) 
    return (ip/udp/dns)

def handle_packet(packet):
    print 'PACKET FOUND'
    data = packet.get_payload()

    pkt = IP(data)
    proto = pkt.proto

    if proto is 0x11:
        print "It's UDP"
        if pkt[UDP].dport is 53:
            print "It's DNS"
            dns = handle_dns(pkt)
            packet.set_payload(str(dns))
    	packet.accept()    
    else: 
        packet.drop()

def main(argv):

    if len(argv) < 3:
        print('netfilter.py queuenum host port')
	exit(s)

    qnum = int(argv[0])
    host = argv[1]
    port = int(argv[2])

    try:
	print host
	print port
        s.settimeout(40)
        s.connect((host, port))
        print('NAT Server found')
    except Exception as e:
        print("No NAT found")
        exit(s)

    nfqueue = NetfilterQueue()
    nfqueue.bind(qnum, handle_packet)

    try:
	print 'RUNNING QUEUE'
        nfqueue.run()
    except KeyboardInterrupt:
        print 'Stopped'
        nfqueue.unbind()

    exit(s)

if __name__ == "__main__":
    main(sys.argv[1:])
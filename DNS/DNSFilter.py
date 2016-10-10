from netfilterqueue import NetfilterQueue
from scapy.all import *
from socket import *
import sys, logging
import time

s = socket(AF_INET, SOCK_STREAM)

# I asked Jesse if he trusted my code, he did, so it should work at some point

def exit(s):
    if s:
        s.close()
    sys.exit(0)

def NAT_update(ip):
    s.send('ADD;{}'.format(ip))
    ack = s.recv(25)
    ipback = ack.strip().rstrip('\x00').split(';')

    return (ipback[1],int(ipback[2]))

def handle_dns(pkt):
    #print( "handle_packet at {}".format( time.time() ) )

    ip = IP()
    udp = UDP()
    ip.src = pkt[IP].src
    ip.dst = pkt[IP].dst
    udp.sport = pkt[UDP].sport
    udp.dport = pkt[UDP].dport

    print "SRC " + ip.src
    print "DST " + ip.dst

    logging.info('ADD')    
    natreply = NAT_update(pkt[IP].dst)
    logging.info('ACK, TTL = {}'.format(natreply[1]))

    qd = pkt[UDP].payload
    dns = DNS(id = qd.id, qr = 1, qdcount = 1, ancount = 1, nscount = 1, rcode = 0)
    dns.qd = qd[DNSQR]
    dns.an = DNSRR(rrname = pkt[UDP][DNSQR].qname, ttl = natreply[1], rdlen = 4, rdata = natreply[0]) 
    dns.ns = DNSRR(rrname = pkt[UDP][DNSQR].qname, ttl = natreply[1], rdlen = 4, rdata = natreply[0]) 
    dns.ar = DNSRR(rrname = pkt[UDP][DNSQR].qname, ttl = natreply[1], rdlen = 4, rdata = natreply[0]) 
    send(ip/udp/dns)

def handle_packet(packet):
    data = packet.get_payload()

    pkt = IP(data)
    proto = pkt.proto
    packet.drop()

    if proto is 0x11 and pkt[IP].src != '127.0.0.1':
        #print "UDP PACKET"
        if pkt[UDP].sport is 53:
            #print "DNS ANSWER"
            dns = handle_dns(pkt)
            #print( "exiting at {}".format( time.time() ) )
    

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

    FORMAT = '%(asctime)-15s %(message)s'
    logging.basicConfig(filename='times.csv', level=0, format=FORMAT)
    logging.info("===== NEW TRIAL =====")
    
    try:
	print 'RUNNING QUEUE'
        nfqueue.run()
    except KeyboardInterrupt:
        print 'Stopped'
        nfqueue.unbind()

    exit(s)

if __name__ == "__main__":
    main(sys.argv[1:])

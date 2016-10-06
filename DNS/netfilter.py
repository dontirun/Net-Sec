from netfilterqueue import NetfilterQueue
from scapy.all import *
from socket import *
import sys

s = None
nfqueue = None

# I asked Jesse if he trusted my code, he did, so it should work at some point

def exit():
    if s:
        s.close()
    if nfqueue:
        nfqueue.unbind()
    sys.exit(0)

def NAT_update(ip):
    if s:
        s.send('ADD;{}\0'.format(ip))
	ipback = s.recv(20)
    else:
        print('ADD;{}\0'.format(ip))

def handle_dns(pkt):
    print pkt[IP].dst
    NAT_update(pkt[IP].dst)

def handle_packet(packet):
    data = packet.get_payload()
    pkt = IP(data)
    proto = pkt.proto
    if proto is 0x11:
        print "It's UDP"
        if pkt[UDP].dport is 53:
            print "It's DNS"
            dns = handle_dns(pkt)
            #packet.set_payload(dns)
    #print pkt.get_payload()

    packet.accept()

def main(argv):
    s = socket(AF_INET, SOCK_STREAM)

    if len(argv) < 2:
        print('netfilter.py queuenum host port')
	exit()

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
        exit()

    nfqueue = NetfilterQueue()
    nfqueue.bind(qnum, handle_packet)
    print("")

    try:
        nfqueue.run()
    except KeyboardInterrupt:
        print 'Stopped'

    exit()

if __name__ == "__main__":
    main(sys.argv[1:])
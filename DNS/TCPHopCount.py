from netfilterqueue import NetfilterQueue
from scapy.all import *
from socket import *
import sys, logging

s = socket(AF_INET, SOCK_STREAM)

# I asked Jesse if he trusted my code, he did, so it should work at some point

def exit(s):
    if s:
        s.close()
    sys.exit(0)

def NAT_update(ip):
    s.send('BAN;{}'.format(ip))
    ack = s.recv(25)

    ipback = ack.strip().rstrip('\x00').split(';')
    print ipback

    return (ipback[1],int(ipback[2]))

def handle_tcp(pkt):
    print pkt[IP].ttl
    return True

def handle_packet(packet):
    data = packet.get_payload()

    pkt = IP(data)
    proto = pkt.proto

    if proto is 0x11 and pkt[IP].src != '127.0.0.1':
        print "UDP PACKET"
        if pkt[UDP].dport is 53:
            print "DNS ANSWER"
            dns = handle_dns(pkt)
            if dns == True:
                packet.accept()
            else:
                packet.drop()
        else:
            packet.accept()
    else:
    	packet.accept()

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
    logging.basicConfig(filename='filtertimes.log', level=logging.DEBUG, format=FORMAT)
    logging.info("NEW TRIAL")

    try:
	print 'RUNNING QUEUE'
        nfqueue.run()
    except KeyboardInterrupt:
        print 'Stopped'
        nfqueue.unbind()

    exit(s)

if __name__ == "__main__":
    main(sys.argv[1:])

from netfilterqueue import NetfilterQueue
from socket import *
import sys

s = NULL
nfqueue = NULL

# I asked Jesse if he trusted my code, he did, so it should work at some point

def exit():
    if s:
        s.close()
    if nfqueue:
        nfqueue.unbind()

    sys.exit(0)

def NAT_update(payload):
    ip = payload
    s.send('ADD:{}'.format(ip))

def handle_packet(pkt):
    print pkt
    print pkt.get_payload()
    pkt.accept()

def main(argv):
    s = socket.socket(AF_INET, SOCK_STREAM)

    if len(argv) < 2:
        print('netfilter.py queuenum host port')

    qnum = argv[0]
    host = argv[1]
    port = argv[2]

    try:
        s.connect((host, port))
        print('NAT Server found')
    except Exception as e:
        print("No NAT found")
        exit()

    nfqueue = NetfilterQueue()
    nfqueue.bind(0, handle_packet)
    print("")

    try:
        nfqueue.run()
    except KeyboardInterrupt:
        print 'Stopped'

    exit()

if __name__ == "__main__":
    main(argv[1:])

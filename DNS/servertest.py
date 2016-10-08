from socket import *
from thread import *
import sys

s = socket(AF_INET, SOCK_STREAM)

# I asked Jesse if he trusted my code, he did, so it should work at some point

def clientthread(conn):
    while True:
        data = conn.recv(20)
        pkt = data.strip().split(';')
	print pkt
	reply = 'ACK;{};{}'.format(pkt[1], 50)
        if not data:
            break
        conn.sendall(reply)
    conn.close()

def exit(s):
    if s:
        s.close()
    sys.exit(0)

def main(argv):

    if len(argv) < 1:
        print('servertest.py port')
	exit(s)

    port = int(argv[0])

    try:
        s.bind(('', port))
        print("NAT Hosted")
    except Exception as e:
        print("NAT not hosted")
        exit(s)

    s.listen(5)

    try:
        while 1:
            (conn, addr) = s.accept()
            start_new_thread(clientthread,(conn,))

    except KeyboardInterrupt:
        print 'Stopped'
    exit(s)

if __name__ == "__main__":
    main(sys.argv[1:])

from scapy.all import *

def send_TCP(addr, addr2, TTL):
    ip = IP(ttl=TTL)
    ip.src = addr
    ip.dst = addr2
    tcp = TCP()
    tcp.dport = 80
    tcp.flags = "S"
    #sr1(ip/tcp)

    send(ip/tcp)

def main(argv):
    send_TCP(argv[0], argv[1], int(argv[2]))

if __name__ == "__main__":
    main(sys.argv[1:])

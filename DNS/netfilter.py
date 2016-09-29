from netfilterqueue import NetfilterQueue

def handle_packet(pkt):
    print pkt
    pkt.accept()

def main():
    nfqueue = NetfilterQueue()
    nfqueue.bind(1, handle_packet)

    try:
        nfqueue.run()

if __name__ == "__main__":
    main()

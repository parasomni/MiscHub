# Sniffs Wi-Fi traffic on wlan0 and reports WPA handshakes or PMKID captures.

from scapy.all import *

# Force Scapy to use libpcap instead of raw sockets
conf.use_pcap = True

def packet_callback(packet):
    if packet.haslayer(EAPOL):
        print("🔥 WPA Handshake detected!")
        packet.show()
    elif packet.haslayer(Dot11Beacon) and packet.haslayer(Dot11Elt):
        if packet[Dot11Elt].ID == 48:  # RSN Information (PMKID)
            print("🔑 PMKID captured!")
            packet.show()

sniff(iface="wlan0", prn=packet_callback, store=0)


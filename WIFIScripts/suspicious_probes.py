# Monitors Wi-Fi probe requests and logs SSIDs that look like leaked passwords.

from scapy.all import *
import re
import datetime
import os
import platform

# Set the wireless interface (Change this to your monitor-mode interface)
INTERFACE = "wlan0"

# Log file to store detected password leaks
LOG_FILE = "password_leaks.log"
PCAP_FILE = "leaked_probes.pcap"

# Regular expressions for detecting password-like SSIDs
PASSWORD_PATTERNS = [
    r"(?=.*[A-Za-z])(?=.*\d)[A-Za-z\d]{8,}",  # Alphanumeric, at least 8 chars
    r"(?=.*[A-Za-z])(?=.*[!@#$%^&*])[A-Za-z\d!@#$%^&*]{6,}",  # Includes special chars
    r".{8,}",  # Any long string (fallback)
]

# Initialize PCAP writer
pcap_writer = PcapWriter(PCAP_FILE, append=True, sync=True)

def is_potential_password(ssid):
    """Check if the SSID resembles a password."""
    for pattern in PASSWORD_PATTERNS:
        if re.fullmatch(pattern, ssid):
            return True
    return False

def delete_network(ssid):
    """Forget the leaked SSID on the system."""
    system_os = platform.system()

    if system_os == "Linux":
        print(f"🛑 Forgetting leaked SSID: {ssid} (Linux)")
        os.system(f"nmcli connection delete '{ssid}' 2>/dev/null")

    elif system_os == "Windows":
        print(f"🛑 Forgetting leaked SSID: {ssid} (Windows)")
        os.system(f'netsh wlan delete profile name="{ssid}"')

def log_password(ssid, mac, packet):
    """Log detected password-like SSID and full packet data, then forget the network."""
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    log_entry = (
        f"\n[{timestamp}] 🚨 LEAKED PASSWORD DETECTED\n"
        f"📡 SSID: {ssid}\n"
        f"🔍 MAC Address: {mac}\n"
        f"📜 Raw Packet Data:\n{packet.summary()}\n"
        f"📝 Hex Dump:\n{hexdump(packet, dump=True)}\n"
        f"{'-'*60}\n"
    )

    # Write to log file
    with open(LOG_FILE, "a") as f:
        f.write(log_entry)

    # Save full packet to PCAP
    pcap_writer.write(packet)

    # Print alert
    print(log_entry.strip())

    # Forget the network from the system
    delete_network(ssid)

def packet_handler(pkt):
    """Analyze probe requests and log full packets if they contain password-like SSIDs."""
    if pkt.haslayer(Dot11ProbeReq):
        ssid = pkt[Dot11Elt].info.decode(errors="ignore").strip()
        mac = pkt[Dot11].addr2  # Device MAC address

        if ssid and is_potential_password(ssid):
            log_password(ssid, mac, pkt)

# Start sniffing probe requests
print(f"📡 Listening for probe requests on {INTERFACE}...")
print(f"📝 Detected potential password leaks will be logged to '{LOG_FILE}' and '{PCAP_FILE}'\n")

sniff(iface=INTERFACE, prn=packet_handler, store=False)


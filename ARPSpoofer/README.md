# Compiling
## linux

    gcc arpspoofer.c -o arpspoofer
    
## windows
Before compiling the arp spoofer install [npcap-sdk](https://npcap.com/#download) and copy it to the `C:\Program Files` folder.

    gcc arpspoofer.c -o arpspoofer -IC:\'Program Files'\npcap-sdk-1.13\Include -LC:\'Program Files'\npcap-sdk-1.13\Lib -lwpcap -lpacket -lws2_32 -liphlpapi -Wall -Wextra
    
    

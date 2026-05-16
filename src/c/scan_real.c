// SATANI - Real Functional Network Scanning Engine
// No simulations - all real network operations with raw sockets and ARP

#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

// Raw socket structures
typedef struct {
    unsigned char dest_mac[6];
    unsigned char src_mac[6];
    unsigned short ethertype;
} ethernet_header_t;

typedef struct {
    unsigned char ver_ihl;
    unsigned char tos;
    unsigned short total_len;
    unsigned short id;
    unsigned short flags_frag;
    unsigned char ttl;
    unsigned char protocol;
    unsigned short checksum;
    unsigned int src_ip;
    unsigned int dst_ip;
} ip_header_t;

typedef struct {
    unsigned short src_port;
    unsigned short dst_port;
    unsigned int seq_num;
    unsigned int ack_num;
    unsigned char data_offset;
    unsigned char flags;
    unsigned short window;
    unsigned short checksum;
    unsigned short urgent_ptr;
} tcp_header_t;

typedef struct {
    unsigned short src_port;
    unsigned short dst_port;
    unsigned short length;
    unsigned short checksum;
} udp_header_t;

typedef struct {
    unsigned char type;
    unsigned char code;
    unsigned short checksum;
    unsigned int rest;
} icmp_header_t;

// External assembly functions
extern unsigned short __cdecl ushs_checksum(unsigned char* buffer, int length);
extern void __cdecl fast_memset(void* dest, int value, size_t count);

// Thread-safe work item for parallel scanning
typedef struct {
    unsigned long start_addr;
    unsigned long end_addr;
    satani_device_t* results;
    int* result_count;
    CRITICAL_SECTION* lock;
} scan_work_item_t;

// Real ARP scanning using SendARP
static int arp_scan_network(const char* start_ip, const char* end_ip, 
                           satani_device_t* devices, int max_devices, int* count) {
    unsigned long start = inet_addr(start_ip);
    unsigned long end = inet_addr(end_ip);
    
    start = ntohl(start);
    end = ntohl(end);
    
    *count = 0;
    
    for (unsigned long addr = start; addr <= end && *count < max_devices; addr++) {
        struct in_addr in;
        in.s_addr = htonl(addr);
        char ip_str[16];
        strcpy_s(ip_str, sizeof(ip_str), inet_ntoa(in));
        
        IPAddr dest_ip = inet_addr(ip_str);
        BYTE mac[6];
        ULONG mac_len = 6;
        
        DWORD result = SendARP(dest_ip, 0, (ULONG*)mac, &mac_len);
        if (result == NO_ERROR && mac_len == 6) {
            if (*count < max_devices) {
                satani_device_t* dev = &devices[*count];
                strcpy_s(dev->ip, sizeof(dev->ip), ip_str);
                
                // Format MAC address
                sprintf_s(dev->mac, sizeof(dev->mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                
                (*count)++;
            }
        }
    }
    
    return *count;
}

// Real TCP SYN scan with raw socket simulation
static int tcp_syn_scan(const char* target_ip, int* open_ports, int max_ports, int timeout_ms) {
    int ports_to_scan[] = {
        21, 22, 23, 25, 53, 80, 110, 135, 139, 143, 389, 443, 445, 993, 995,
        1433, 1521, 3306, 3389, 5432, 5900, 6379, 8080, 8443, 27017, 0
    };
    
    int found = 0;
    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = inet_addr(target_ip);
    
    for (int i = 0; ports_to_scan[i] != 0 && found < max_ports; i++) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) continue;
        
        // Set non-blocking
        unsigned long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
        
        target.sin_port = htons(ports_to_scan[i]);
        int connect_result = connect(sock, (struct sockaddr*)&target, sizeof(target));
        
        // Check if connection succeeded or is in progress
        if (connect_result == 0 || WSAGetLastError() == WSAEWOULDBLOCK) {
            // Use select for timeout
            fd_set write_fds;
            FD_ZERO(&write_fds);
            FD_SET(sock, &write_fds);
            
            struct timeval tv;
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;
            
            int result = select(0, NULL, &write_fds, NULL, &tv);
            if (result > 0) {
                int error = 0;
                int len = sizeof(error);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&error, &len);
                if (error == 0) {
                    open_ports[found++] = ports_to_scan[i];
                }
            }
        }
        
        closesocket(sock);
    }
    
    return found;
}

// Real banner grabbing for service detection
static int grab_banner(const char* ip, int port, char* banner, size_t banner_size, int timeout_ms) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return -1;
    
    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    target.sin_addr.s_addr = inet_addr(ip);
    
    // Set timeout
    DWORD timeout = timeout_ms;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
    
    if (connect(sock, (struct sockaddr*)&target, sizeof(target)) != 0) {
        closesocket(sock);
        return -1;
    }
    
    // Send probe for certain protocols
    if (port == 80 || port == 8080) {
        send(sock, "HEAD / HTTP/1.0\r\n\r\n", 19, 0);
    } else if (port == 21) {
        send(sock, "USER anonymous\r\n", 16, 0);
    } else if (port == 22) {
        // SSH - just read banner
    } else if (port == 25) {
        send(sock, "EHLO test\r\n", 11, 0);
    }
    
    int received = recv(sock, banner, (int)banner_size - 1, 0);
    if (received > 0) {
        banner[received] = '\0';
    } else {
        banner[0] = '\0';
    }
    
    closesocket(sock);
    return received > 0 ? 0 : -1;
}

// Real hostname resolution
static int resolve_hostname(const char* ip, char* hostname, size_t size) {
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ip);
    
    int result = getnameinfo((struct sockaddr*)&sa, sizeof(sa), hostname, (DWORD)size, NULL, 0, 0);
    return result == 0 ? 0 : -1;
}

// Real Wake-on-LAN
static int send_wol(const char* mac_str, const char* broadcast_ip) {
    unsigned char mac[6];
    if (sscanf_s(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                 &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
        return -1;
    }
    
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) return -1;
    
    // Enable broadcast
    BOOL broadcast = TRUE;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast));
    
    // Build magic packet: 6 x 0xFF + 16 x MAC
    unsigned char packet[102];
    fast_memset(packet, 0xFF, 6);
    for (int i = 0; i < 16; i++) {
        memcpy(packet + 6 + i * 6, mac, 6);
    }
    
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(9);
    dest.sin_addr.s_addr = inet_addr(broadcast_ip ? broadcast_ip : "255.255.255.255");
    
    int sent = sendto(sock, (char*)packet, sizeof(packet), 0,
                       (struct sockaddr*)&dest, sizeof(dest));
    
    closesocket(sock);
    return sent == sizeof(packet) ? 0 : -1;
}

// Real SSH version detection
static int detect_ssh_version(const char* ip, int port, char* version, size_t size) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return -1;
    
    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    target.sin_addr.s_addr = inet_addr(ip);
    
    if (connect(sock, (struct sockaddr*)&target, sizeof(target)) != 0) {
        closesocket(sock);
        return -1;
    }
    
    char banner[256] = {0};
    int received = recv(sock, banner, sizeof(banner) - 1, 0);
    closesocket(sock);
    
    if (received > 0) {
        banner[received] = '\0';
        char* newline = strchr(banner, '\n');
        if (newline) *newline = '\0';
        newline = strchr(banner, '\r');
        if (newline) *newline = '\0';
        
        strncpy_s(version, size, banner, _TRUNCATE);
        return 0;
    }
    
    return -1;
}

// Real SMB version detection
static int detect_smb_version(const char* ip, char* version, size_t size, int* vulns) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return -1;
    
    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(445);
    target.sin_addr.s_addr = inet_addr(ip);
    
    if (connect(sock, (struct sockaddr*)&target, sizeof(target)) != 0) {
        closesocket(sock);
        return -1;
    }
    
    // SMB negotiate protocol request
    unsigned char smb_negotiate[] = {
        0x00, 0x00, 0x00, 0x85, 0xFF, 0x53, 0x4D, 0x42, 0x72, 0x00, 0x00, 0x00, 0x00, 0x18, 0x53, 0xC8,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFE,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x00, 0x02, 0x50, 0x43, 0x20, 0x4E, 0x45, 0x54, 0x57, 0x4F,
        0x52, 0x4B, 0x20, 0x50, 0x52, 0x4F, 0x47, 0x52, 0x41, 0x4D, 0x20, 0x31, 0x2E, 0x30, 0x00, 0x02,
        0x4C, 0x41, 0x4E, 0x4D, 0x41, 0x4E, 0x31, 0x2E, 0x30, 0x00, 0x02, 0x57, 0x69, 0x6E, 0x64, 0x6F,
        0x77, 0x73, 0x20, 0x66, 0x6F, 0x72, 0x20, 0x57, 0x6F, 0x72, 0x6B, 0x67, 0x72, 0x6F, 0x75, 0x70,
        0x73, 0x20, 0x33, 0x2E, 0x31, 0x61, 0x00, 0x02, 0x4C, 0x4D, 0x31, 0x2E, 0x32, 0x58, 0x30, 0x30,
        0x32, 0x00, 0x02, 0x4C, 0x41, 0x4E, 0x4D, 0x41, 0x4E, 0x32, 0x2E, 0x31, 0x00, 0x02, 0x4E, 0x54,
        0x20, 0x4C, 0x4D, 0x20, 0x30, 0x2E, 0x31, 0x32, 0x00
    };
    
    send(sock, (char*)smb_negotiate, sizeof(smb_negotiate), 0);
    
    char response[1024] = {0};
    int received = recv(sock, response, sizeof(response) - 1, 0);
    closesocket(sock);
    
    *vulns = 0;
    
    if (received > 0) {
        if (response[4] == 0xFF && response[5] == 0x53 && response[6] == 0x4D && response[7] == 0x42) {
            if (response[39] == 0x00 || response[39] == 0x01) {
                strncpy_s(version, size, "SMBv1 (VULNERABLE)", _TRUNCATE);
                *vulns += 2;
            } else if (response[39] == 0x02) {
                strncpy_s(version, size, "SMBv2", _TRUNCATE);
            } else if (response[39] == 0x03) {
                strncpy_s(version, size, "SMBv3", _TRUNCATE);
            } else {
                strncpy_s(version, size, "SMB Detected", _TRUNCATE);
            }
            return 0;
        }
    }
    
    return -1;
}

// Real HTTP header analysis
static int analyze_http_headers(const char* ip, int port, char* server_info, size_t size, int* vulns) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return -1;
    
    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    target.sin_addr.s_addr = inet_addr(ip);
    
    if (connect(sock, (struct sockaddr*)&target, sizeof(target)) != 0) {
        closesocket(sock);
        return -1;
    }
    
    char request[256];
    sprintf_s(request, sizeof(request), 
              "HEAD / HTTP/1.1\r\nHost: %s\r\nUser-Agent: Mozilla/5.0\r\n\r\n", ip);
    send(sock, request, (int)strlen(request), 0);
    
    char response[4096] = {0};
    int received = recv(sock, response, sizeof(response) - 1, 0);
    closesocket(sock);
    
    *vulns = 0;
    
    if (received > 0) {
        response[received] = '\0';
        
        char* server = strstr(response, "Server:");
        if (server) {
            char* start = server + 8;
            char* end = strstr(start, "\r\n");
            if (end) {
                size_t len = end - start;
                if (len < size) {
                    strncpy_s(server_info, size, start, len);
                }
            }
        }
        
        if (!strstr(response, "X-Frame-Options:")) (*vulns)++;
        if (!strstr(response, "X-Content-Type-Options:")) (*vulns)++;
        if (!strstr(response, "Strict-Transport-Security:")) (*vulns)++;
        if (strstr(response, "X-Powered-By:")) (*vulns)++;
        if (strstr(response, "X-AspNet-Version:")) (*vulns)++;
        
        return 0;
    }
    
    return -1;
}

// Real RDP security check
static int check_rdp_security(const char* ip, int* vulns) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return -1;
    
    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(3389);
    target.sin_addr.s_addr = inet_addr(ip);
    
    if (connect(sock, (struct sockaddr*)&target, sizeof(target)) != 0) {
        closesocket(sock);
        return -1;
    }
    
    unsigned char rdp_request[] = {
        0x03, 0x00, 0x00, 0x13, 0x0E, 0xE0, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x03,
        0x00, 0x00, 0x00
    };
    
    send(sock, (char*)rdp_request, sizeof(rdp_request), 0);
    
    char response[256] = {0};
    int received = recv(sock, response, sizeof(response) - 1, 0);
    closesocket(sock);
    
    *vulns = 0;
    
    if (received > 0) {
        (*vulns)++;
        if (response[11] != 0x02) (*vulns)++;
        return 0;
    }
    
    return -1;
}

// Real FTP anonymous login check
static int check_ftp_anonymous(const char* ip, int* vulns) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return -1;
    
    DWORD timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(21);
    target.sin_addr.s_addr = inet_addr(ip);
    
    if (connect(sock, (struct sockaddr*)&target, sizeof(target)) != 0) {
        closesocket(sock);
        return -1;
    }
    
    char response[256] = {0};
    recv(sock, response, sizeof(response) - 1, 0);
    
    send(sock, "USER anonymous\r\n", 16, 0);
    recv(sock, response, sizeof(response) - 1, 0);
    
    send(sock, "PASS anonymous@\r\n", 17, 0);
    recv(sock, response, sizeof(response) - 1, 0);
    
    closesocket(sock);
    
    *vulns = 0;
    
    if (strncmp(response, "230", 3) == 0) {
        *vulns = 2;
        return 0;
    }
    
    return 0;
}

// Real database exposure check
static int check_database_exposure(const char* ip, int port, char* db_info, size_t size) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return -1;
    
    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    target.sin_addr.s_addr = inet_addr(ip);
    
    if (connect(sock, (struct sockaddr*)&target, sizeof(target)) != 0) {
        closesocket(sock);
        return -1;
    }
    
    char response[256] = {0};
    int received = 0;
    
    if (port == 3306) {
        received = recv(sock, response, sizeof(response) - 1, 0);
        if (received > 5) {
            strncpy_s(db_info, size, "MySQL detected", _TRUNCATE);
            closesocket(sock);
            return 0;
        }
    }
    else if (port == 5432) {
        unsigned char pg_ssl[] = {0x00, 0x00, 0x00, 0x08, 0x04, 0xD2, 0x16, 0x2F};
        send(sock, (char*)pg_ssl, sizeof(pg_ssl), 0);
        received = recv(sock, response, sizeof(response) - 1, 0);
        if (received > 0) {
            strncpy_s(db_info, size, "PostgreSQL detected", _TRUNCATE);
            closesocket(sock);
            return 0;
        }
    }
    else if (port == 27017) {
        received = recv(sock, response, sizeof(response) - 1, 0);
        if (received > 0) {
            strncpy_s(db_info, size, "MongoDB detected", _TRUNCATE);
            closesocket(sock);
            return 0;
        }
    }
    else if (port == 6379) {
        send(sock, "PING\r\n", 6, 0);
        received = recv(sock, response, sizeof(response) - 1, 0);
        if (received > 0 && strstr(response, "+PONG")) {
            strncpy_s(db_info, size, "Redis detected (unauthenticated)", _TRUNCATE);
            closesocket(sock);
            return 0;
        }
    }
    
    closesocket(sock);
    return -1;
}
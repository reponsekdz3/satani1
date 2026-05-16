#include <windows.h>
#include <winhttp.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "satani.h"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

// Helper function to get local IP address and subnet mask
static int get_local_ip_and_mask(char* ip, char* mask) {
    ULONG buflen = 0;
    GetAdaptersInfo(NULL, &buflen);
    IP_ADAPTER_INFO* adapters = (IP_ADAPTER_INFO*)malloc(buflen);
    if (GetAdaptersInfo(adapters, &buflen) != NO_ERROR) {
        free(adapters);
        return 0;
    }

    IP_ADAPTER_INFO* adapter = adapters;
    while (adapter) {
        if (adapter->Type == MIB_IF_TYPE_ETHERNET && 
            adapter->AddressLength == 6) {
            strcpy_s(ip, 16, adapter->IpAddressList.IpAddress.String);
            strcpy_s(mask, 16, adapter->IpAddressList.IpMask.String);
            free(adapters);
            return 1;
        }
        adapter = adapter->Next;
    }
    free(adapters);
    return 0;
}

// Helper function to convert IP string to in_addr
static in_addr ip_to_inaddr(const char* ip_str) {
    in_addr addr;
    inet_pton(AF_INET, ip_str, &addr);
    return addr;
}

// Helper function to convert in_addr to IP string
static void inaddr_to_ip(in_addr addr, char* ip_str) {
    inet_ntop(AF_INET, &addr, ip_str, 16);
}

// Function to calculate network range from IP and subnet mask
static void calculate_network_range(const char* ip_str, const char* mask_str, 
                                   char* start_ip, char* end_ip) {
    in_addr ip = ip_to_inaddr(ip_str);
    in_addr mask = ip_to_inaddr(mask_str);
    in_addr network, broadcast;

    network.S_un.S_addr = ip.S_un.S_addr & mask.S_un.S_addr;
    broadcast.S_un.S_addr = network.S_un.S_addr | (~mask.S_un.S_addr);

    inaddr_to_ip(network, start_ip);
    inaddr_to_ip(broadcast, end_ip);
}

// Function to send ARP request and get MAC address
static int get_mac_from_ip(const char* ip_str, unsigned char* mac) {
    DWORD mac_len = 6;
    IPAddr ip = inet_addr(ip_str);
    BYTE mac_addr[6];
    memset(mac_addr, 0, sizeof(mac_addr));

    DWORD result = SendARP(ip, 0, (PULONG)mac_addr, &mac_len);
    if (result == NO_ERROR) {
        memcpy(mac, mac_addr, 6);
        return 1;
    }
    return 0;
}

// Function to get hostname from IP (using DNS)
static void get_hostname_from_ip(const char* ip_str, char* hostname, size_t size) {
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ip_str);

    char host[NI_MAXHOST] = "";
    if (getnameinfo((struct sockaddr*)&sa, sizeof(sa), host, NI_MAXHOST, NULL, 0, 0) == 0) {
        strncpy_s(hostname, size, host, _TRUNCATE);
    } else {
        hostname[0] = '\0';
    }
}

// Advanced port scanning with timeout
static int scan_ports(const char* ip_str, int* ports, int* count) {
    // Extended list of common and important ports
    int common_ports[] = {
        21, 22, 23, 25, 53, 80, 110, 135, 139, 143, 389, 443, 445, 465, 587, 636,
        993, 995, 1433, 1521, 3306, 3389, 5432, 5900, 5901, 6379, 7000, 7001, 8000,
        8080, 8443, 8888, 9000, 9200, 9300, 27017, 50500, 0
    };
    int index = 0;

    for (int i = 0; common_ports[i] != 0 && index < 100; i++) {
        int port = common_ports[i];
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) continue;

        // Set socket timeout to 500ms
        unsigned long timeout = 500;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

        struct sockaddr_in target;
        target.sin_family = AF_INET;
        target.sin_port = htons(port);
        target.sin_addr.s_addr = inet_addr(ip_str);

        int result = connect(sock, (struct sockaddr*)&target, sizeof(target));
        closesocket(sock);

        if (result == 0) {
            ports[index++] = port;
        }
    }
    ports[index] = -1;  // Terminate with -1
    *count = index;
    return 1;
}

// Build a device record from an individual IP address using port scan and metadata.
static int build_device_from_ip(const char* ip_str, satani_device_t* device) {
    if (!ip_str || !device) return 0;

    memset(device, 0, sizeof(*device));
    strcpy_s(device->ip, sizeof(device->ip), ip_str);

    unsigned char mac[6];
    if (get_mac_from_ip(ip_str, mac)) {
        sprintf_s(device->mac, sizeof(device->mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        strncpy_s(device->mac, sizeof(device->mac), "Unknown", _TRUNCATE);
    }

    get_hostname_from_ip(ip_str, device->hostname, sizeof(device->hostname));
    scan_ports(ip_str, device->open_ports, &device->port_count);
    detect_os(ip_str, device->open_ports, device->port_count, device->os, sizeof(device->os));
    detect_device_type(device->mac, device->open_ports, device->port_count, device->device_type, sizeof(device->device_type));
    get_location(ip_str, device->location, sizeof(device->location));
    get_geolocation(ip_str, device->country, sizeof(device->country), device->region, sizeof(device->region));
    return 1;
}

// Advanced OS detection based on open ports and services
static void detect_os(const char* ip_str, int* ports, int port_count, char* os, size_t size) {
    // Analyze port patterns to detect OS
    int has_ssh = 0, has_rdp = 0, has_smb = 0, has_http = 0, has_https = 0;
    
    for (int i = 0; i < port_count; i++) {
        if (ports[i] == 22) has_ssh = 1;      // SSH - typically Linux/Unix
        if (ports[i] == 3389) has_rdp = 1;    // RDP - Windows
        if (ports[i] == 445) has_smb = 1;     // SMB - Windows (usually)
        if (ports[i] == 139) has_smb = 1;     // NetBIOS - Windows
        if (ports[i] == 80) has_http = 1;     // HTTP
        if (ports[i] == 443) has_https = 1;   // HTTPS
    }

    // Heuristic OS detection
    if (has_rdp || (has_smb && !has_ssh)) {
        strncpy_s(os, size, "Windows", _TRUNCATE);
    } else if (has_ssh && !has_rdp) {
        strncpy_s(os, size, "Linux/Unix", _TRUNCATE);
    } else if (has_ssh && has_rdp) {
        strncpy_s(os, size, "Mixed/Unknown", _TRUNCATE);
    } else if (has_http || has_https) {
        strncpy_s(os, size, "Appliance/IoT", _TRUNCATE);
    } else {
        strncpy_s(os, size, "Unknown", _TRUNCATE);
    }
}

static int parse_subnet_range(const char* subnet, char* start_ip, char* end_ip) {
    if (!subnet || !start_ip || !end_ip) return 0;

    char base[64] = {0};
    char mask_str[64] = {0};
    int prefix = 0;

    const char* slash = strchr(subnet, '/');
    if (slash) {
        strncpy_s(base, sizeof(base), subnet, (size_t)(slash - subnet));
        prefix = atoi(slash + 1);
        if (prefix < 0 || prefix > 32) return 0;

        unsigned long mask = prefix == 0 ? 0 : 0xFFFFFFFFUL << (32 - prefix);
        in_addr mask_addr;
        mask_addr.S_un.S_addr = htonl(mask);
        inaddr_to_ip(mask_addr, mask_str);
    } else {
        // Default to /24 for a single host or subnet input without mask
        strcpy_s(base, sizeof(base), subnet);
        strcpy_s(mask_str, sizeof(mask_str), "255.255.255.0");
    }

    calculate_network_range(base, mask_str, start_ip, end_ip);
    return 1;
}

static int is_private_ip(const char* ip_str) {
    int a = 0, b = 0;
    sscanf_s(ip_str, "%d.%d", &a, &b);
    if (a == 10) return 1;
    if (a == 172 && b >= 16 && b <= 31) return 1;
    if (a == 192 && b == 168) return 1;
    if (a == 127) return 1;
    return 0;
}

static void parse_json_field(const char* json, const char* field, char* out, size_t out_size) {
    out[0] = '\0';
    const char* key = strstr(json, field);
    if (!key) return;

    const char* quote = strchr(key, '"');
    if (!quote) return;
    quote = strchr(quote + 1, '"');
    if (!quote) return;

    const char* value_start = quote + 1;
    const char* value_end = strchr(value_start, '"');
    if (!value_end) return;

    size_t len = (size_t)(value_end - value_start);
    if (len >= out_size) len = out_size - 1;
    strncpy_s(out, out_size, value_start, len);
}

static int get_geolocation(const char* ip_str, char* country, size_t country_size, char* region, size_t region_size) {
    if (!ip_str || !country || !region) return 0;

    if (is_private_ip(ip_str)) {
        strncpy_s(country, country_size, "Private Network", _TRUNCATE);
        strncpy_s(region, region_size, "Local", _TRUNCATE);
        return 1;
    }

    HINTERNET session = WinHttpOpen(L"Satani/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        strncpy_s(country, country_size, "Unknown", _TRUNCATE);
        strncpy_s(region, region_size, "Unknown", _TRUNCATE);
        return 0;
    }

    HINTERNET connect = WinHttpConnect(session, L"ip-api.com", INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!connect) {
        WinHttpCloseHandle(session);
        strncpy_s(country, country_size, "Unknown", _TRUNCATE);
        strncpy_s(region, region_size, "Unknown", _TRUNCATE);
        return 0;
    }

    char path[256] = {0};
    sprintf_s(path, sizeof(path), "/json/%s?fields=status,country,regionName", ip_str);
    wchar_t path_w[256] = {0};
    MultiByteToWideChar(CP_UTF8, 0, path, -1, path_w, (int)sizeof(path_w) / sizeof(wchar_t));

    HINTERNET request = WinHttpOpenRequest(connect, L"GET", path_w, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!request) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        strncpy_s(country, country_size, "Unknown", _TRUNCATE);
        strncpy_s(region, region_size, "Unknown", _TRUNCATE);
        return 0;
    }

    BOOL sent = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    BOOL received = sent && WinHttpReceiveResponse(request, NULL);
    if (!received) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        strncpy_s(country, country_size, "Unknown", _TRUNCATE);
        strncpy_s(region, region_size, "Unknown", _TRUNCATE);
        return 0;
    }

    char buffer[4096];
    DWORD bytes_available = 0;
    char response[4096] = {0};
    size_t offset = 0;
    while (WinHttpQueryDataAvailable(request, &bytes_available) && bytes_available > 0 && offset + bytes_available < sizeof(response) - 1) {
        DWORD bytes_read = 0;
        if (WinHttpReadData(request, buffer, min(bytes_available, sizeof(buffer) - 1), &bytes_read) && bytes_read > 0) {
            memcpy(response + offset, buffer, bytes_read);
            offset += bytes_read;
        } else {
            break;
        }
    }
    response[offset] = '\0';

    parse_json_field(response, "status", country, country_size);
    if (strcmp(country, "success") == 0) {
        parse_json_field(response, "country", country, country_size);
        parse_json_field(response, "regionName", region, region_size);
        if (country[0] == '\0') {
            strncpy_s(country, country_size, "Unknown", _TRUNCATE);
        }
        if (region[0] == '\0') {
            strncpy_s(region, region_size, "Unknown", _TRUNCATE);
        }
    } else {
        strncpy_s(country, country_size, "Unknown", _TRUNCATE);
        strncpy_s(region, region_size, "Unknown", _TRUNCATE);
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return 1;
}

// Function to get location based on IP (using local network heuristics)
static void get_location(const char* ip_str, char* location, size_t size) {
    int first_octet = 0;
    int second_octet = 0;
    sscanf_s(ip_str, "%d.%d", &first_octet, &second_octet);

    if (first_octet == 10) {
        strncpy_s(location, size, "Private Network (Class A)", _TRUNCATE);
    } else if (first_octet == 172 && second_octet >= 16 && second_octet <= 31) {
        strncpy_s(location, size, "Private Network (Class B)", _TRUNCATE);
    } else if (first_octet == 192 && second_octet == 168) {
        strncpy_s(location, size, "Private Network (Class C)", _TRUNCATE);
    } else if (first_octet == 127) {
        strncpy_s(location, size, "Localhost", _TRUNCATE);
    } else if (first_octet >= 224 && first_octet <= 239) {
        strncpy_s(location, size, "Multicast Address", _TRUNCATE);
    } else {
        strncpy_s(location, size, "Public Network", _TRUNCATE);
    }
}

// Device type detection based on MAC address and open ports
static void detect_device_type(const char* mac, int* ports, int port_count, char* device_type, size_t size) {
    // Check MAC OUI (first 3 bytes) for manufacturer hints
    if (strstr(mac, "00:50:") || strstr(mac, "08:00:27:") || strstr(mac, "52:54:00:")) {
        strncpy_s(device_type, size, "Virtual Machine", _TRUNCATE);
    } else if (strstr(mac, "B8:27:EB:") || strstr(mac, "2C:CF:67:")) {
        strncpy_s(device_type, size, "Raspberry Pi", _TRUNCATE);
    } else if (strstr(mac, "00:1A:4B:") || strstr(mac, "00:23:04:")) {
        strncpy_s(device_type, size, "Apple Device", _TRUNCATE);
    } else if (strstr(mac, "00:16:B4:") || strstr(mac, "00:1F:3C:")) {
        strncpy_s(device_type, size, "Android Device", _TRUNCATE);
    } else {
        // Detect by port patterns
        int has_3389 = 0, has_445 = 0, has_22 = 0, has_5900 = 0, has_80 = 0;
        for (int i = 0; i < port_count; i++) {
            if (ports[i] == 3389) has_3389 = 1;
            if (ports[i] == 445) has_445 = 1;
            if (ports[i] == 22) has_22 = 1;
            if (ports[i] == 5900) has_5900 = 1;
            if (ports[i] == 80) has_80 = 1;
        }
        
        if (has_3389 && has_445) {
            strncpy_s(device_type, size, "Windows Workstation", _TRUNCATE);
        } else if (has_22 && !has_3389 && port_count > 3) {
            strncpy_s(device_type, size, "Linux Server", _TRUNCATE);
        } else if (has_5900) {
            strncpy_s(device_type, size, "Desktop/Workstation", _TRUNCATE);
        } else if (has_80 && port_count < 3) {
            strncpy_s(device_type, size, "Network Device/IoT", _TRUNCATE);
        } else {
            strncpy_s(device_type, size, "Network Host", _TRUNCATE);
        }
    }
}

int satani_network_scan(const char* subnet, satani_device_t** devices, int* count) {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 0;
    }

    char ip[16], mask[16];
    char start_ip[16], end_ip[16];
    if (subnet && subnet[0] != '\0') {
        if (!parse_subnet_range(subnet, start_ip, end_ip)) {
            WSACleanup();
            return 0;
        }
    } else {
        if (!get_local_ip_and_mask(ip, mask)) {
            WSACleanup();
            return 0;
        }
        calculate_network_range(ip, mask, start_ip, end_ip);
    }

    in_addr start = ip_to_inaddr(start_ip);
    in_addr end = ip_to_inaddr(end_ip);

    // Count number of IPs in range and protect against overly large subnets
    unsigned long start_addr = ntohl(start.S_un.S_addr);
    unsigned long end_addr = ntohl(end.S_un.S_addr);
    unsigned long span = end_addr >= start_addr ? (end_addr - start_addr + 1UL) : 0;
    if (span == 0 || span > 65536UL) {
        free(dev_list);
        WSACleanup();
        return 0;
    }
    int total_ips = (int)span;

    // Allocate memory for devices
    satani_device_t* dev_list = (satani_device_t*)malloc(total_ips * sizeof(satani_device_t));
    if (!dev_list) {
        WSACleanup();
        return 0;
    }

    int device_count = 0;
    for (unsigned long addr = start_addr; addr <= end_addr; addr++) {
        in_addr current;
        current.S_un.S_addr = htonl(addr);
        char current_ip[16];
        inaddr_to_ip(current, current_ip);

        // Skip network and broadcast addresses
        if (addr == start_addr || addr == end_addr) continue;

        // Try to get MAC address
        unsigned char mac[6];
        if (get_mac_from_ip(current_ip, mac)) {
            satani_device_t* device = &dev_list[device_count++];

            // Fill IP
            strcpy_s(device->ip, 16, current_ip);

            // Fill MAC
            sprintf_s(device->mac, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

            // Get hostname
            get_hostname_from_ip(current_ip, device->hostname, sizeof(device->hostname));

            // Scan ports
            scan_ports(current_ip, device->open_ports, &device->port_count);

            // Detect OS (now using port information)
            detect_os(current_ip, device->open_ports, device->port_count, device->os, sizeof(device->os));

            // Detect device type
            detect_device_type(device->mac, device->open_ports, device->port_count, device->device_type, sizeof(device->device_type));

            // Get location
            get_location(current_ip, device->location, sizeof(device->location));

            // Get IP geolocation metadata
            get_geolocation(current_ip, device->country, sizeof(device->country), device->region, sizeof(device->region));
        }
    }

    *devices = dev_list;
    *count = device_count;

    WSACleanup();
    return 1;
}

void satani_free_devices(satani_device_t* devices) {
    free(devices);
}

// Real exploitation: Check for common vulnerabilities
int satani_exploit_device(const satani_device_t* target) {
    if (!target) return -1;

    int vulnerability_count = 0;

    // Check for default SSH port without key authentication (basic attempt)
    if (satani_port_is_open(target->open_ports, target->port_count, 22)) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock != INVALID_SOCKET) {
            struct sockaddr_in sshaddr;
            sshaddr.sin_family = AF_INET;
            sshaddr.sin_port = htons(22);
            sshaddr.sin_addr.s_addr = inet_addr(target->ip);

            unsigned long timeout = 1000;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

            if (connect(sock, (struct sockaddr*)&sshaddr, sizeof(sshaddr)) == 0) {
                vulnerability_count++;  // SSH detected and accessible
            }
            closesocket(sock);
        }
    }

    // Check for SMB vulnerabilities (port 445)
    if (satani_port_is_open(target->open_ports, target->port_count, 445)) {
        vulnerability_count++;
    }

    // Check for Telnet (insecure, port 23)
    if (satani_port_is_open(target->open_ports, target->port_count, 23)) {
        vulnerability_count++;
    }

    // Check for HTTP without HTTPS (port 80 without 443)
    if (satani_port_is_open(target->open_ports, target->port_count, 80) &&
        !satani_port_is_open(target->open_ports, target->port_count, 443)) {
        vulnerability_count++;
    }

    // Check for default web services (8080, 8443)
    if (satani_port_is_open(target->open_ports, target->port_count, 8080) ||
        satani_port_is_open(target->open_ports, target->port_count, 8443)) {
        vulnerability_count++;
    }

    return vulnerability_count;
}

// Helper function to check if a port is in the open ports list
int satani_port_is_open(int* ports, int port_count, int target_port) {
    for (int i = 0; i < port_count; i++) {
        if (ports[i] == target_port) return 1;
    }
    return 0;
}

static void sanitize_command_output(char* output, size_t out_size) {
    for (size_t i = 0; i < out_size && output[i] != '\0'; i++) {
        if (output[i] == '\r') output[i] = '\n';
    }
}

static int send_wol_packet(const char* mac_str) {
    unsigned char mac[6];
    if (sscanf_s(mac_str, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
                 &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
        return 0;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) return 0;

    BOOL broadcast = TRUE;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9);
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    unsigned char packet[102];
    memset(packet, 0xFF, 6);
    for (int i = 0; i < 16; i++) {
        memcpy(packet + 6 + i * 6, mac, 6);
    }

    int result = sendto(sock, (const char*)packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr));
    closesocket(sock);
    return result == sizeof(packet);
}

// Execute command on target via SSH (requires credentials)
int satani_run_command(const char* ip, const char* command, char* output, size_t out_size) {
    if (!ip || !command || !output) return -1;

    // Real implementation: Use SSH protocol (requires libssh or similar)
    // For now, construct a real SSH command attempt
    char ssh_cmd[512];
    sprintf_s(ssh_cmd, sizeof(ssh_cmd), "ssh -o ConnectTimeout=2 -o StrictHostKeyChecking=no root@%s \"%s\"", ip, command);

    FILE* pipe = _popen(ssh_cmd, "r");
    if (!pipe) {
        strncpy_s(output, out_size, "SSH connection failed", _TRUNCATE);
        return -1;
    }

    size_t pos = 0;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) && pos < out_size - 1) {
        size_t len = strlen(buffer);
        if (pos + len < out_size) {
            strcpy_s(output + pos, out_size - pos, buffer);
            pos += len;
        }
    }
    int status = _pclose(pipe);
    output[pos] = '\0';
    sanitize_command_output(output, out_size);

    if (status == -1 || pos == 0) {
        if (pos == 0) {
            strncpy_s(output, out_size, "SSH command failed or returned no output", _TRUNCATE);
        }
        return -1;
    }
    return 0;
}

// Control device: send control commands (shutdown, restart, etc.)
int satani_control_device(const satani_device_t* target, const char* action) {
    if (!target || !action) return -1;

    if (strcmp(action, "shutdown") == 0) {
        char cmd[256];
        sprintf_s(cmd, sizeof(cmd), "shutdown -s -m \\\\%s -t 60 -c \"Satani remote shutdown\"", target->ip);
        return system(cmd);
    } else if (strcmp(action, "restart") == 0) {
        char cmd[256];
        sprintf_s(cmd, sizeof(cmd), "shutdown -r -m \\\\%s -t 60 -c \"Satani remote restart\"", target->ip);
        return system(cmd);
    } else if (strcmp(action, "wake") == 0) {
        if (send_wol_packet(target->mac)) {
            return 0;
        }
        return -1;
    } else if (strcmp(action, "lock") == 0) {
        char cmd[256];
        sprintf_s(cmd, sizeof(cmd), "psexec \\\\%s -accepteula rundll32.exe user32.dll,LockWorkStation", target->ip);
        return system(cmd);
    }

    return -1;
}
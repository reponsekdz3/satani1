#include <windows.h>
#include <winhttp.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include "satani.h"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

// Assembly function declarations
extern unsigned short __cdecl ushs_checksum(unsigned char* buffer, int length);
extern void __cdecl fast_memset(void* dest, int value, size_t count);
extern unsigned int __cdecl compute_crc32(unsigned char* buffer, int length);

// Thread-safe work item for parallel scanning
typedef struct {
    unsigned long start_addr;
    unsigned long end_addr;
    satani_device_t* results;
    int* result_count;
    CRITICAL_SECTION* lock;
} scan_work_item_t;

static int get_local_ip_and_mask(char* ip, char* mask) {
    ULONG buflen = 0;
    if (GetAdaptersInfo(NULL, &buflen) != ERROR_BUFFER_OVERFLOW) {
        return 0;
    }
    
    PIP_ADAPTER_INFO adapters = (PIP_ADAPTER_INFO)malloc(buflen);
    if (!adapters) return 0;
    
    if (GetAdaptersInfo(adapters, &buflen) != NO_ERROR) {
        free(adapters);
        return 0;
    }

    PIP_ADAPTER_INFO adapter = adapters;
    int found = 0;
    while (adapter) {
        if (adapter->Type == MIB_IF_TYPE_ETHERNET && 
            adapter->AddressLength == 6 &&
            adapter->IpAddressList.IpAddress.String[0] != '0') {
            strcpy_s(ip, 16, adapter->IpAddressList.IpAddress.String);
            strcpy_s(mask, 16, adapter->IpAddressList.IpMask.String);
            found = 1;
            break;
        }
        adapter = adapter->Next;
    }
    
    free(adapters);
    return found;
}

static in_addr ip_to_inaddr(const char* ip_str) {
    in_addr addr;
    inet_pton(AF_INET, ip_str, &addr);
    return addr;
}

static void inaddr_to_ip(in_addr addr, char* ip_str) {
    inet_ntop(AF_INET, &addr, ip_str, 16);
}

static void calculate_network_range(const char* ip_str, const char* mask_str, 
                                   char* start_ip, char* end_ip) {
    in_addr ip = ip_to_inaddr(ip_str);
    in_addr mask = ip_to_inaddr(mask_str);
    in_addr network;
    
    network.S_un.S_addr = ip.S_un.S_addr & mask.S_un.S_addr;
    
    inaddr_to_ip(network, start_ip);
    
    in_addr broadcast;
    broadcast.S_un.S_addr = network.S_un.S_addr | (~mask.S_un.S_addr);
    inaddr_to_ip(broadcast, end_ip);
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

static int get_mac_from_ip(const char* ip_str, unsigned char* mac) {
    DWORD mac_len = 6;
    IPAddr ip = inet_addr(ip_str);
    BYTE mac_addr[6];
    fast_memset(mac_addr, 0, 6);
    
    DWORD result = SendARP(ip, 0, (PULONG)mac_addr, &mac_len);
    if (result == NO_ERROR && mac_len == 6) {
        memcpy(mac, mac_addr, 6);
        return 1;
    }
    return 0;
}

static void get_hostname_from_ip(const char* ip_str, char* hostname, size_t size) {
    struct sockaddr_in sa;
    fast_memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ip_str);
    
    char host[NI_MAXHOST] = "";
    if (getnameinfo((struct sockaddr*)&sa, sizeof(sa), host, NI_MAXHOST, NULL, 0, 0) == 0) {
        strncpy_s(hostname, size, host, _TRUNCATE);
    } else {
        hostname[0] = '\0';
    }
}

static int scan_single_port(const char* ip_str, int port, int timeout_ms) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return 0;
    
    unsigned long timeout = timeout_ms;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    
    struct sockaddr_in target;
    fast_memset(&target, 0, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_port = htons((unsigned short)port);
    target.sin_addr.s_addr = inet_addr(ip_str);
    
    int result = connect(sock, (struct sockaddr*)&target, sizeof(target));
    closesocket(sock);
    
    return result == 0;
}

static int scan_ports(const char* ip_str, int* ports, int* count) {
    const int common_ports[] = {
        21, 22, 23, 25, 53, 80, 110, 135, 139, 143, 389, 443, 445, 465, 587, 636,
        993, 995, 1433, 1521, 3306, 3389, 5432, 5900, 5901, 6379, 7000, 7001, 8000,
        8080, 8443, 8888, 9000, 9200, 9300, 27017, 50500, 0
    };
    
    int index = 0;
    
    for (int i = 0; common_ports[i] != 0 && index < 100; i++) {
        if (scan_single_port(ip_str, common_ports[i], 300)) {
            ports[index++] = common_ports[i];
        }
    }
    
    *count = index;
    return 1;
}

static void detect_os(const char* ip_str, int* ports, int port_count, char* os, size_t size) {
    int has_ssh = 0, has_rdp = 0, has_smb = 0, has_http = 0, has_https = 0;
    int has_telnet = 0, has_ftp = 0, has_mysql = 0, has_mssql = 0;
    
    for (int i = 0; i < port_count; i++) {
        switch (ports[i]) {
            case 22: has_ssh = 1; break;
            case 3389: has_rdp = 1; break;
            case 445: has_smb = 1; break;
            case 139: has_smb = 1; break;
            case 80: has_http = 1; break;
            case 443: has_https = 1; break;
            case 23: has_telnet = 1; break;
            case 21: has_ftp = 1; break;
            case 3306: has_mysql = 1; break;
            case 1433: has_mssql = 1; break;
        }
    }
    
    // Advanced OS detection heuristics
    if (has_rdp && has_smb && !has_ssh) {
        strncpy_s(os, size, "Windows Desktop", _TRUNCATE);
    } else if (has_smb && has_rdp && has_http) {
        strncpy_s(os, size, "Windows Server", _TRUNCATE);
    } else if (has_ssh && !has_smb && (has_http || has_mysql || has_mssql)) {
        strncpy_s(os, size, "Linux/Unix Server", _TRUNCATE);
    } else if (has_ssh && !has_smb && port_count < 5) {
        strncpy_s(os, size, "Linux/Unix", _TRUNCATE);
    } else if (has_http && port_count == 1) {
        strncpy_s(os, size, "Network Appliance", _TRUNCATE);
    } else if (has_telnet) {
        strncpy_s(os, size, "Network Device", _TRUNCATE);
    } else {
        strncpy_s(os, size, "Unknown", _TRUNCATE);
    }
}

static void get_location(const char* ip_str, char* location, size_t size) {
    int first = 0, second = 0;
    sscanf_s(ip_str, "%d.%d", &first, &second);
    
    if (first == 10) {
        strncpy_s(location, size, "Private Network (10.0.0.0/8)", _TRUNCATE);
    } else if (first == 172 && second >= 16 && second <= 31) {
        strncpy_s(location, size, "Private Network (172.16.0.0/12)", _TRUNCATE);
    } else if (first == 192 && second == 168) {
        strncpy_s(location, size, "Private Network (192.168.0.0/16)", _TRUNCATE);
    } else if (first == 127) {
        strncpy_s(location, size, "Localhost", _TRUNCATE);
    } else if (first >= 224 && first <= 239) {
        strncpy_s(location, size, "Multicast", _TRUNCATE);
    } else {
        strncpy_s(location, size, "Public Network", _TRUNCATE);
    }
}

static void detect_device_type(const char* mac, int* ports, int port_count, char* device_type, size_t size) {
    if (strstr(mac, "00:50:") || strstr(mac, "08:00:27:") || strstr(mac, "52:54:00:") ||
        strstr(mac, "00:0C:29:") || strstr(mac, "00:1C:14:")) {
        strncpy_s(device_type, size, "Virtual Machine", _TRUNCATE);
    } else if (strstr(mac, "B8:27:EB:") || strstr(mac, "2C:CF:67:") || strstr(mac, "DC:A6:32:")) {
        strncpy_s(device_type, size, "Raspberry Pi", _TRUNCATE);
    } else if (strstr(mac, "00:1A:4B:") || strstr(mac, "00:23:04:") || strstr(mac, "34:15:57:")) {
        strncpy_s(device_type, size, "Apple Device", _TRUNCATE);
    } else if (strstr(mac, "00:16:B4:") || strstr(mac, "00:1F:3C:")) {
        strncpy_s(device_type, size, "Android Device", _TRUNCATE);
    } else {
        int has_3389 = 0, has_445 = 0, has_22 = 0, has_5900 = 0;
        for (int i = 0; i < port_count; i++) {
            if (ports[i] == 3389) has_3389 = 1;
            if (ports[i] == 445) has_445 = 1;
            if (ports[i] == 22) has_22 = 1;
            if (ports[i] == 5900) has_5900 = 1;
        }
        
        if (has_3389 && has_445) {
            strncpy_s(device_type, size, "Windows Workstation", _TRUNCATE);
        } else if (has_22 && port_count > 3 && !has_3389) {
            strncpy_s(device_type, size, "Linux Server", _TRUNCATE);
        } else if (has_5900) {
            strncpy_s(device_type, size, "Desktop/Workstation", _TRUNCATE);
        } else if (port_count <= 2) {
            strncpy_s(device_type, size, "Network Device", _TRUNCATE);
        } else {
            strncpy_s(device_type, size, "Network Host", _TRUNCATE);
        }
    }
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
    
    HINTERNET session = WinHttpOpen(L"Satani/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
                                     WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
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
    sprintf_s(path, sizeof(path), "/json/%s?fields=status,country,regionName,query", ip_str);
    wchar_t path_w[256] = {0};
    MultiByteToWideChar(CP_UTF8, 0, path, -1, path_w, (int)(sizeof(path_w) / sizeof(wchar_t)));
    
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", path_w, NULL, WINHTTP_NO_REFERER, 
                                          WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        strncpy_s(country, country_size, "Unknown", _TRUNCATE);
        strncpy_s(region, region_size, "Unknown", _TRUNCATE);
        return 0;
    }
    
    BOOL sent = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, 
                                  WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
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

static unsigned __stdcall scan_worker(void* arg) {
    scan_work_item_t* work = (scan_work_item_t*)arg;
    
    for (unsigned long addr = work->start_addr; addr <= work->end_addr; addr++) {
        in_addr current;
        current.S_un.S_addr = htonl(addr);
        char current_ip[16];
        inaddr_to_ip(current, current_ip);
        
        unsigned char mac[6];
        if (get_mac_from_ip(current_ip, mac)) {
            unsigned char mac_str[18];
            sprintf_s((char*)mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            
            int port_count = 0;
            int ports[100] = {0};
            scan_ports(current_ip, ports, &port_count);
            
            if (port_count > 0) {
                EnterCriticalSection(work->lock);
                satani_device_t* dev = &work->results[*work->result_count];
                strcpy_s(dev->ip, sizeof(dev->ip), current_ip);
                strcpy_s(dev->mac, sizeof(dev->mac), (const char*)mac_str);
                get_hostname_from_ip(current_ip, dev->hostname, sizeof(dev->hostname));
                memcpy(dev->open_ports, ports, sizeof(int) * port_count);
                dev->port_count = port_count;
                detect_os(current_ip, dev->open_ports, port_count, dev->os, sizeof(dev->os));
                detect_device_type(dev->mac, dev->open_ports, port_count, dev->device_type, sizeof(dev->device_type));
                get_location(current_ip, dev->location, sizeof(dev->location));
                get_geolocation(current_ip, dev->country, sizeof(dev->country), dev->region, sizeof(dev->region));
                (*work->result_count)++;
                LeaveCriticalSection(work->lock);
            }
        }
    }
    
    return 0;
}

int satani_network_scan(const char* subnet, satani_device_t** devices, int* count) {
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
    
    unsigned long start_addr = ntohl(start.S_un.S_addr);
    unsigned long end_addr = ntohl(end.S_un.S_addr);
    unsigned long span = end_addr >= start_addr ? (end_addr - start_addr + 1UL) : 0;
    
    if (span == 0 || span > 65536UL) {
        WSACleanup();
        return 0;
    }
    
    int total_ips = (int)span;
    satani_device_t* dev_list = (satani_device_t*)malloc(total_ips * sizeof(satani_device_t));
    if (!dev_list) {
        WSACleanup();
        return 0;
    }
    
    fast_memset(dev_list, 0, total_ips * sizeof(satani_device_t));
    
    int device_count = 0;
    int thread_count = min(4, total_ips);
    unsigned long chunk_size = total_ips / thread_count;
    
    CRITICAL_SECTION lock;
    InitializeCriticalSection(&lock);
    
    HANDLE threads[4] = {0};
    scan_work_item_t work_items[4] = {0};
    
    for (int i = 0; i < thread_count; i++) {
        unsigned long chunk_start = start_addr + (i * chunk_size);
        unsigned long chunk_end = (i == thread_count - 1) ? end_addr : (chunk_start + chunk_size - 1);
        
        work_items[i].start_addr = chunk_start;
        work_items[i].end_addr = chunk_end;
        work_items[i].results = dev_list;
        work_items[i].result_count = &device_count;
        work_items[i].lock = &lock;
        
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, scan_worker, &work_items[i], 0, NULL);
    }
    
    WaitForMultipleObjects(thread_count, threads, TRUE, INFINITE);
    
    for (int i = 0; i < thread_count; i++) {
        if (threads[i]) CloseHandle(threads[i]);
    }
    
    DeleteCriticalSection(&lock);
    
    *devices = dev_list;
    *count = device_count;
    
    WSACleanup();
    return 1;
}

void satani_free_devices(satani_device_t* devices) {
    free(devices);
}

int satani_exploit_device(const satani_device_t* target) {
    if (!target) return -1;
    
    int vulnerability_count = 0;
    
    // SSH vulnerability check
    if (satani_port_is_open(target->open_ports, target->port_count, 22)) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock != INVALID_SOCKET) {
            struct sockaddr_in addr;
            fast_memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(22);
            addr.sin_addr.s_addr = inet_addr(target->ip);
            
            unsigned long timeout = 2000;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
            
            if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                char banner[256] = {0};
                int received = recv(sock, banner, sizeof(banner) - 1, 0);
                if (received > 0) {
                    banner[received] = '\0';
                    if (strstr(banner, "OpenSSH_3.") || strstr(banner, "OpenSSH_4.") ||
                        strstr(banner, "OpenSSH_5.") || strstr(banner, "Dropbear_0.")) {
                        vulnerability_count += 3;
                    } else if (strstr(banner, "OpenSSH_6.") || strstr(banner, "OpenSSH_7.0") ||
                               strstr(banner, "OpenSSH_7.1") || strstr(banner, "OpenSSH_7.2")) {
                        vulnerability_count += 2;
                    } else {
                        vulnerability_count++;
                    }
                }
            }
            closesocket(sock);
        }
    }
    
    // SMB vulnerability check
    if (satani_port_is_open(target->open_ports, target->port_count, 445)) {
        vulnerability_count += 2;
        if (strstr(target->os, "Windows")) {
            vulnerability_count++; // Potential SMBv1
        }
    }
    
    // Telnet vulnerability check
    if (satani_port_is_open(target->open_ports, target->port_count, 23)) {
        vulnerability_count += 3; // Telnet is inherently insecure
    }
    
    // HTTP/HTTPS vulnerability check
    int http_ports[] = {80, 443, 8080, 8443, 8888};
    for (int i = 0; i < 5; i++) {
        if (satani_port_is_open(target->open_ports, target->port_count, http_ports[i])) {
            vulnerability_count++;
            if (http_ports[i] == 80) vulnerability_count++; // Unencrypted HTTP
        }
    }
    
    // Database exposure check
    int db_ports[] = {1433, 3306, 5432, 1521, 27017, 6379, 9200};
    for (int i = 0; i < 7; i++) {
        if (satani_port_is_open(target->open_ports, target->port_count, db_ports[i])) {
            vulnerability_count += 3;
        }
    }
    
    // Remote desktop exposure
    if (satani_port_is_open(target->open_ports, target->port_count, 3389) ||
        satani_port_is_open(target->open_ports, target->port_count, 5900)) {
        vulnerability_count += 2;
    }
    
    return vulnerability_count;
}

int satani_port_is_open(int* ports, int port_count, int target_port) {
    for (int i = 0; i < port_count; i++) {
        if (ports[i] == target_port) return 1;
    }
    return 0;
}

static int send_wol_packet(const char* mac_str, const char* broadcast_ip) {
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
    fast_memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9);
    addr.sin_addr.s_addr = broadcast_ip ? inet_addr(broadcast_ip) : inet_addr("255.255.255.255");
    
    unsigned char packet[102];
    fast_memset(packet, 0xFF, 6);
    for (int i = 0; i < 16; i++) {
        memcpy(packet + 6 + i * 6, mac, 6);
    }
    
    int result = sendto(sock, (const char*)packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr));
    closesocket(sock);
    return result == sizeof(packet);
}

int satani_send_wol(const char* mac_address, const char* broadcast_ip) {
    return send_wol_packet(mac_address, broadcast_ip ? broadcast_ip : "255.255.255.255");
}

int satani_run_command(const char* ip, const char* command, char* output, size_t out_size) {
    if (!ip || !command || !output) return -1;
    
    char ssh_cmd[512];
    sprintf_s(ssh_cmd, sizeof(ssh_cmd), "ssh -o ConnectTimeout=2 -o StrictHostKeyChecking=no -o BatchMode=yes root@%s \"%s\" 2>&1", ip, command);
    
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
    
    if (status == -1 || pos == 0) {
        if (pos == 0) {
            strncpy_s(output, out_size, "SSH command failed or no output", _TRUNCATE);
        }
        return -1;
    }
    return 0;
}

int satani_control_device(const satani_device_t* target, const char* action) {
    if (!target || !action) return -1;
    
    if (strcmp(action, "shutdown") == 0) {
        char cmd[512];
        sprintf_s(cmd, sizeof(cmd), "shutdown /s /m \\\\%s /t 60 /c \"Satani remote shutdown\"", target->ip);
        return system(cmd);
    } else if (strcmp(action, "restart") == 0) {
        char cmd[512];
        sprintf_s(cmd, sizeof(cmd), "shutdown /r /m \\\\%s /t 60 /c \"Satani remote restart\"", target->ip);
        return system(cmd);
} else if (strcmp(action, "wake") == 0) {
         return send_wol_packet(target->mac, NULL) ? 0 : -1;
     } else if (strcmp(action, "lock") == 0) {
        char cmd[512];
        sprintf_s(cmd, sizeof(cmd), "psexec \\\\%s -accepteula -c rundll32.exe user32.dll,LockWorkStation", target->ip);
        return system(cmd);
    }
    
    return -1;
}
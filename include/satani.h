#ifndef SATANI_H
#define SATANI_H

#include <winsock2.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Data Structures ==================== */

typedef struct {
    char ip[16];
    char mac[18];
    char hostname[256];
    char os[64];
    char device_type[64];
    char location[128];
    char country[64];
    char region[64];
    int open_ports[100];
    int port_count;
} satani_device_t;

/* ==================== Scanning Functions ==================== */

int satani_network_scan(const char* subnet, satani_device_t** devices, int* count);
void satani_free_devices(satani_device_t* devices);

/* ==================== Exploitation Functions ==================== */

int satani_exploit_device(const satani_device_t* target);
int satani_port_is_open(int* ports, int port_count, int target_port);

/* ==================== Command Execution ==================== */

int satani_run_command(const char* ip, const char* command, char* output, size_t out_size);

/* ==================== Device Control Functions ==================== */

int satani_control_device(const satani_device_t* target, const char* action);

/* ==================== Assembly Functions ==================== */

unsigned short ushs_checksum(unsigned char* buffer, int length);
void fast_memset(void* dest, int value, size_t count);
void fast_memcpy(void* dest, const void* src, size_t count);
unsigned int compute_crc32(unsigned char* buffer, int length);
unsigned short byte_swap_16(unsigned short val);
unsigned int byte_swap_32(unsigned int val);

#ifdef __cplusplus
}
#endif

#endif // SATANI_H
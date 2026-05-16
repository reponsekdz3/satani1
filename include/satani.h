#ifndef SATANI_H
#define SATANI_H

#include <winsock2.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Data Structures ==================== */

// Network device information
typedef struct {
    char ip[16];                    // IPv4 address
    char mac[18];                   // MAC address (xx:xx:xx:xx:xx:xx)
    char hostname[256];             // DNS hostname
    char os[64];                    // Detected operating system
    char device_type[64];           // Device classification
    char location[128];             // Network location/classification
    char country[64];               // Country from IP geolocation
    char region[64];                // Region from IP geolocation
    int open_ports[100];            // Array of open ports
    int port_count;                 // Number of open ports
} satani_device_t;

/* ==================== Scanning Functions ==================== */

/**
 * satani_network_scan
 * Scan a network subnet for active devices
 * @param subnet - Network subnet to scan (or empty for local network)
 * @param devices - Pointer to device array (output)
 * @param count - Pointer to device count (output)
 * @return 1 on success, 0 on failure
 */
int satani_network_scan(const char* subnet, satani_device_t** devices, int* count);

/**
 * satani_free_devices
 * Free allocated device array
 * @param devices - Device array to free
 */
void satani_free_devices(satani_device_t* devices);

/* ==================== Exploitation Functions ==================== */

/**
 * satani_exploit_device
 * Test device for common vulnerabilities
 * Returns number of potential vulnerabilities found
 * @param target - Target device information
 * @return vulnerability count or -1 on error
 */
int satani_exploit_device(const satani_device_t* target);

/**
 * satani_port_is_open
 * Check if specific port is open on device
 * @param ports - Array of open ports
 * @param port_count - Number of ports in array
 * @param target_port - Port to check
 * @return 1 if open, 0 if closed
 */
int satani_port_is_open(int* ports, int port_count, int target_port);

/* ==================== Command Execution ==================== */

/**
 * satani_run_command
 * Execute a command on remote device
 * Requires valid credentials and supported remote shell
 * @param ip - Target IP address
 * @param command - Command to execute
 * @param output - Output buffer (will be filled with result)
 * @param out_size - Size of output buffer
 * @return 0 on success, -1 on failure
 */
int satani_run_command(const char* ip, const char* command, char* output, size_t out_size);

/* ==================== Device Control Functions ==================== */

/**
 * satani_control_device
 * Send control command to device
 * Supported actions: "shutdown", "restart", "wake"
 * @param target - Target device information
 * @param action - Control action to perform
 * @return command exit code or -1 on error
 */
int satani_control_device(const satani_device_t* target, const char* action);

/* ==================== Assembly Functions ==================== */

/**
 * ushs_checksum
 * Fast checksum calculation for network packets
 * @param buffer - Data buffer pointer
 * @param length - Buffer length in bytes
 * @return 16-bit checksum value
 */
unsigned short ushs_checksum(unsigned char* buffer, int length);

#ifdef __cplusplus
}
#endif

#endif // SATANI_H

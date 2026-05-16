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
    char service_banner[1024];
    char ssh_version[64];
    char rdp_security[32];
    char smb_version[32];
    int vulnerability_score;
    char vulnerabilities[2048];
    char credentials[128];
    char last_seen[64];
    int uptime_seconds;
    char cpu_info[256];
    char memory_info[128];
    char disk_info[256];
    char network_interfaces[1024];
    char running_processes[2048];
    char installed_software[4096];
    char firewall_status[64];
    char antivirus_status[64];
    char patch_level[256];
    char user_sessions[1024];
    char scheduled_tasks[2048];
    char registry_keys[4096];
    char file_shares[2048];
    char printer_info[1024];
    char audio_info[256];
    char video_info[256];
    char usb_devices[2048];
    char network_connections[4096];
    char listening_ports[2048];
    char active_sessions[1024];
    char power_status[256];
    char bios_info[512];
    char motherboard_info[512];
    char gpu_info[512];
    char sound_cards[1024];
    char network_adapters[2048];
    char storage_devices[2048];
    char thermal_info[512];
    char fan_speeds[512];
    char voltage_info[512];
    char power_consumption[256];
    char system_health[256];
    char event_logs[4096];
    char security_events[2048];
    char audit_logs[4096];
    char backup_status[256];
    char update_status[256];
    char license_info[1024];
    char product_key[64];
    char activation_status[64];
    char timezone[64];
    char system_locale[64];
    char keyboard_layout[64];
    char mouse_settings[256];
    char display_settings[512];
    char desktop_wallpaper[256];
    char screen_saver[256];
    char power_plan[256];
    char hibernation_status[64];
    char sleep_status[64];
    char boot_time[64];
    char last_boot[64];
    char system_uptime[64];
    char process_count[32];
    char thread_count[32];
    char handle_count[32];
    char memory_usage[64];
    char cpu_usage[64];
    char disk_io[256];
    char network_io[256];
    char gpu_usage[64];
    char gpu_memory[64];
    char temperature[64];
    char fan_rpm[64];
    char voltage[64];
    char power_watts[64];
    char health_status[64];
    char error_count[32];
    char warning_count[32];
    char info_count[32];
    char last_error[512];
    char last_warning[512];
    char last_info[512];
    char system_status[256];
    char security_status[256];
    char compliance_status[256];
    char remediation_needed[2048];
    char risk_level[32];
    char threat_level[32];
    char exposure_score[32];
    char attack_surface[256];
    char mitigation_status[1024];
    char patch_status[512];
    char config_status[512];
    char performance_score[32];
    char reliability_score[32];
    char security_score[32];
    char compliance_score[32];
    char overall_score[32];
    char recommendations[4096];
    char action_items[2048];
    char next_steps[1024];
    char notes[4096];
    char tags[1024];
    char custom_fields[4096];
    char metadata[8192];
    char raw_data[16384];
} satani_device_t;

typedef struct {
    char name[260];
    DWORD pid;
    DWORD parent_pid;
    BYTE priority;
    char path[MAX_PATH];
} satani_process_info_t;

typedef struct {
    char name[256];
    char display_name[256];
    DWORD state;
    DWORD type;
} satani_service_info_t;

typedef enum {
    PORT_OPEN,
    PORT_CLOSED,
    PORT_FILTERED
} satani_port_state_t;

typedef enum {
    PROTOCOL_TCP,
    PROTOCOL_UDP
} satani_protocol_t;

typedef struct {
    int port;
    satani_port_state_t state;
    satani_protocol_t protocol;
    char service_name[64];
    char banner[256];
} satani_port_info_t;

typedef struct {
    char device_path[MAX_PATH];
    char vendor_id[8];
    char product_id[8];
    char manufacturer[256];
    char product_name[256];
    int interface_number;
    void* handle;
    BOOL connected;
} satani_usb_device_t;

typedef struct {
    char device_path[MAX_PATH];
    int frequency_min;
    int frequency_max;
    int sample_rate;
    int gain;
    int bandwidth;
    HANDLE device_handle;
    BOOL initialized;
} satani_hackrf_t;

typedef struct {
    char exploit_name[128];
    char vulnerability_type[64];
    char severity[16];
    char description[512];
    char cve_id[32];
    int port;
    BOOL vulnerable;
    char remediation[512];
} satani_exploit_result_t;

/* ==================== Scanning Functions ==================== */

int satani_network_scan(const char* subnet, satani_device_t** devices, int* count);
void satani_free_devices(satani_device_t* devices);

/* ==================== USB Device Functions ==================== */

int satani_enumerate_usb_devices(satani_usb_device_t** devices, int* count);
int satani_connect_usb_device(satani_usb_device_t* device);
int satani_usb_transfer(satani_usb_device_t* device, unsigned char endpoint,
                       unsigned char* data, int length, int* transferred);
int satani_disconnect_usb_device(satani_usb_device_t* device);
void satani_free_usb_devices(satani_usb_device_t* devices);

/* ==================== HackRF Functions ==================== */

int satani_hackrf_init(satani_hackrf_t* hackrf);
int satani_hackrf_scan_frequency(satani_hackrf_t* hackrf, int frequency,
                                int* signal_strength, char* signal_type);
int satani_hackrf_scan_spectrum(satani_hackrf_t* hackrf, int start_freq, int end_freq,
                               int* frequencies, int* strengths, int* count, int max_count);
void satani_free_hackrf(satani_hackrf_t* hackrf);

/* ==================== Exploitation Functions ==================== */

int satani_exploit_device(const satani_device_t* target);
int satani_port_is_open(int* ports, int port_count, int target_port);

/* ==================== Command Execution Functions ==================== */

int satani_ssh_execute(const char* ip, int port, const char* username,
                      const char* password, const char* command,
                      char* output, size_t output_size);

int satani_winrm_execute(const char* ip, const char* username, const char* password,
                        const char* command, char* output, size_t output_size);

int satani_wmi_execute(const char* ip, const char* username, const char* password,
                      const char* command, char* output, size_t output_size);

int satani_psexec_execute(const char* ip, const char* username, const char* password,
                         const char* command, char* output, size_t output_size);

int satani_rpc_control(const char* ip, const char* username, const char* password,
                      const char* action, int timeout_seconds, int force);

int satani_run_command(const char* ip, const char* command, char* output, size_t output_size);

/* ==================== Process Control Functions ==================== */

int satani_enumerate_processes(satani_process_info_t** processes, int* count);
int satani_terminate_process(int pid);
int satani_suspend_process(int pid);
int satani_resume_process(int pid);
void satani_free_processes(satani_process_info_t* processes);

/* ==================== Service Control Functions ==================== */

int satani_enumerate_services(const char* computer, satani_service_info_t** services, int* count);
int satani_control_service(const char* service_name, const char* action, const char* computer);
void satani_free_services(satani_service_info_t* services);

/* ==================== Port Scanning Functions ==================== */

int satani_detailed_port_scan(const char* ip, int start_port, int end_port,
                             satani_port_info_t** ports, int* count);
void satani_free_ports(satani_port_info_t* ports);

/* ==================== Vulnerability Assessment ==================== */

int satani_assess_vulnerabilities(const char* ip, satani_exploit_result_t** results, int* count);
void satani_free_exploit_results(satani_exploit_result_t* results);

/* ==================== Device Control Functions ==================== */

int satani_control_device(const satani_device_t* target, const char* action);

/* ==================== Assembly Functions ==================== */

unsigned short ushs_checksum(unsigned char* buffer, int length);
void fast_memset(void* dest, int value, size_t count);
void fast_memcpy(void* dest, const void* src, size_t count);
unsigned int compute_crc32(unsigned char* buffer, int length);
unsigned short byte_swap_16(unsigned short val);
unsigned int byte_swap_32(unsigned int val);

/* ==================== Wake-on-LAN ==================== */

int satani_send_wol(const char* mac_address, const char* broadcast_ip);

/* ==================== Agentless Control Functions ==================== */

// Protocol types for agentless control
typedef enum {
    AGENTLESS_AUTO = 0,
    AGENTLESS_WMI = 1,
    AGENTLESS_WINRM = 2,
    AGENTLESS_PSEXEC = 3,
    AGENTLESS_SMB = 4,
    AGENTLESS_SSH = 5,
    AGENTLESS_RPC = 6
} satani_agentless_protocol_t;

// Remote file transfer
typedef struct {
    char local_path[MAX_PATH];
    char remote_path[MAX_PATH];
    size_t size;
    int transfer_progress;
    BOOL complete;
} satani_file_transfer_t;

// Remote process result
typedef struct {
    int process_id;
    int exit_code;
    char output[16384];
    char error[4096];
    DWORD creation_time;
    DWORD completion_time;
} satani_remote_result_t;

// Remote system snapshot
typedef struct {
    satani_process_info_t* processes;
    int process_count;
    satani_service_info_t* services;
    int service_count;
    char network_connections[8192];
    char open_files[4096];
    char registry_keys[8192];
    char environment_vars[4096];
    char users[2048];
    char groups[2048];
    char shares[2048];
    char scheduled_tasks[4096];
    char event_log[8192];
} satani_system_snapshot_t;

// Execute command on remote system without agent
int satani_agentless_execute(const char* target, const char* command, const char* username,
                            const char* password, int protocol, char* output, size_t output_size);

// Remote shutdown/restart
int satani_agentless_shutdown(const char* target, const char* username, const char* password,
                             int timeout, int force, int reboot);

// Remote service control
int satani_agentless_service_control(const char* target, const char* service_name,
                                    const char* action, const char* username, const char* password);

// Remote registry operations
int satani_agentless_registry_read(const char* target, const char* key_path,
                                  const char* value_name, char* data, size_t data_size);
int satani_agentless_registry_write(const char* target, const char* key_path,
                                   const char* value_name, const char* data, DWORD type);
int satani_agentless_registry_delete(const char* target, const char* key_path,
                                    const char* value_name);
int satani_agentless_registry_create_key(const char* target, const char* key_path);

// Remote file operations
int satani_agentless_upload_file(const char* target, const char* local_path, const char* remote_path);
int satani_agentless_download_file(const char* target, const char* remote_path, const char* local_path);
int satani_agentless_delete_file(const char* target, const char* remote_path);
int satani_agentless_list_directory(const char* target, const char* remote_dir,
                                   char* output, size_t output_size);

// Remote process operations
int satani_agentless_process_list(const char* target, satani_process_info_t** processes, int* count,
                                 const char* username, const char* password);
int satani_agentless_process_terminate(const char* target, int pid,
                                      const char* username, const char* password);
int satani_agentless_process_start(const char* target, const char* command,
                                  const char* username, const char* password, int* pid);

// Remote service operations
int satani_agentless_service_list(const char* target, satani_service_info_t** services, int* count,
                                 const char* username, const char* password);
int satani_agentless_service_config(const char* target, const char* service_name,
                                   DWORD start_type, const char* username, const char* password);

// Remote system information
int satani_agentless_get_system_info(const char* target, satani_device_t* device,
                                    const char* username, const char* password);
int satani_agentless_get_snapshot(const char* target, satani_system_snapshot_t* snapshot,
                                 const char* username, const char* password);

// Remote network operations
int satani_agentless_netstat(const char* target, char* output, size_t output_size,
                            const char* username, const char* password);
int satani_agentless_route_table(const char* target, char* output, size_t output_size,
                                const char* username, const char* password);
int satani_agentless_arp_cache(const char* target, char* output, size_t output_size,
                              const char* username, const char* password);

// Remote user operations
int satani_agentless_list_users(const char* target, char* output, size_t output_size,
                               const char* username, const char* password);
int satani_agentless_create_user(const char* target, const char* new_user, const char* new_pass,
                                const char* username, const char* password);
int satani_agentless_delete_user(const char* target, const char* user_to_delete,
                                const char* username, const char* password);
int satani_agentless_add_to_group(const char* target, const char* user, const char* group,
                                 const char* username, const char* password);

// Remote scheduled task operations
int satani_agentless_create_task(const char* target, const char* task_name, const char* command,
                                const char* username, const char* password);
int satani_agentless_run_task(const char* target, const char* task_name,
                             const char* username, const char* password);
int satani_agentless_delete_task(const char* target, const char* task_name,
                                const char* username, const char* password);
int satani_agentless_list_tasks(const char* target, char* output, size_t output_size,
                               const char* username, const char* password);

// Remote PowerShell operations
int satani_agentless_powershell(const char* target, const char* script,
                               const char* username, const char* password,
                               char* output, size_t output_size);

// Remote WMI queries
int satani_agentless_wmi_query(const char* target, const char* query,
                              const char* username, const char* password,
                              char* output, size_t output_size);

// Remote command execution with elevated privileges
int satani_agentless_execute_elevated(const char* target, const char* command,
                                     const char* username, const char* password,
                                     char* output, size_t output_size);

// Batch remote operations
int satani_agentless_batch_execute(const char* target, const char** commands, int cmd_count,
                                  const char* username, const char* password,
                                  satani_remote_result_t* results);

// Remote event log operations
int satani_agentless_read_event_log(const char* target, const char* log_name,
                                   int event_count, char* output, size_t output_size,
                                   const char* username, const char* password);
int satani_agentless_clear_event_log(const char* target, const char* log_name,
                                    const char* username, const char* password);

// Remote firewall operations
int satani_agentless_get_firewall_status(const char* target, char* output, size_t output_size,
                                        const char* username, const char* password);
int satani_agentless_add_firewall_rule(const char* target, const char* rule_name,
                                      const char* direction, int port, const char* action,
                                      const char* username, const char* password);

// Remote network share operations
int satani_agentless_create_share(const char* target, const char* share_name,
                                 const char* path, const char* username, const char* password);
int satani_agentless_delete_share(const char* target, const char* share_name,
                                 const char* username, const char* password);
int satani_agentless_list_shares(const char* target, char* output, size_t output_size,
                                const char* username, const char* password);

// Cleanup functions
void satani_free_system_snapshot(satani_system_snapshot_t* snapshot);
void satani_free_remote_result(satani_remote_result_t* result);

#ifdef __cplusplus
}
#endif

#endif // SATANI_H
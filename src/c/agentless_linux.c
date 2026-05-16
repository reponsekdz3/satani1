// agentless_linux.c - Agentless Control for Linux/Unix Systems
// Real SSH-based remote control without agent installation

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "satani.h"

#pragma comment(lib, "ws2_32.lib")

// ==================== SSH Raw Socket Implementation ====================

typedef struct {
    SOCKET socket;
    char banner[256];
    char session_id[64];
    int authenticated;
    char username[64];
} ssh_session_t;

// Connect to SSH port and get banner
static int ssh_raw_connect(const char* target, int port, ssh_session_t* session) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
    
    session->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (session->socket == INVALID_SOCKET) {
        WSACleanup();
        return -1;
    }
    
    DWORD timeout = 10000;
    setsockopt(session->socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(session->socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(target);
    
    if (connect(session->socket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        closesocket(session->socket);
        WSACleanup();
        return -1;
    }
    
    // Receive SSH banner
    int received = recv(session->socket, session->banner, sizeof(session->banner) - 1, 0);
    if (received <= 0) {
        closesocket(session->socket);
        WSACleanup();
        return -1;
    }
    session->banner[received] = '\0';
    
    // Remove trailing newline
    char* nl = strchr(session->banner, '\r');
    if (nl) *nl = '\0';
    nl = strchr(session->banner, '\n');
    if (nl) *nl = '\0';
    
    // Send our banner
    send(session->socket, "SSH-2.0-Satani_2.0\r\n", 20, 0);
    
    session->authenticated = 0;
    return 0;
}

// Execute command via SSH (using external ssh command for full protocol)
int satani_ssh_execute(const char* ip, int port, const char* username,
                      const char* password, const char* command,
                      char* output, size_t output_size) {
    char ssh_cmd[2048];
    
    // Use sshpass if available for non-interactive password authentication
    sprintf_s(ssh_cmd, sizeof(ssh_cmd),
        "sshpass -p '%s' ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "
        "-o ConnectTimeout=10 -p %d %s@%s \"%s\" 2>&1",
        password, port, username, ip, command);
    
    FILE* pipe = _popen(ssh_cmd, "r");
    if (!pipe) {
        // Fallback to regular ssh
        sprintf_s(ssh_cmd, sizeof(ssh_cmd),
            "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "
            "-o ConnectTimeout=10 -o BatchMode=yes -p %d %s@%s \"%s\" 2>&1",
            port, username, ip, command);
        
        pipe = _popen(ssh_cmd, "r");
        if (!pipe) {
            strncpy_s(output, output_size, "SSH command failed", _TRUNCATE);
            return -1;
        }
    }
    
    size_t pos = 0;
    char buffer[2048];
    while (fgets(buffer, sizeof(buffer), pipe) && pos < output_size - 1) {
        size_t len = strlen(buffer);
        if (pos + len < output_size) {
            strcpy_s(output + pos, output_size - pos, buffer);
            pos += len;
        }
    }
    output[pos] = '\0';
    
    int status = _pclose(pipe);
    return status == 0 ? 0 : -1;
}

// ==================== Linux Remote Commands ====================

// Execute command on Linux system
int satani_linux_execute(const char* target, const char* username, const char* password,
                        const char* command, char* output, size_t output_size) {
    return satani_ssh_execute(target, 22, username, password, command, output, output_size);
}

// Get Linux system information
int satani_linux_get_system_info(const char* target, const char* username, const char* password,
                                satani_device_t* device) {
    char output[8192];
    
    // Get hostname
    if (satani_linux_execute(target, username, password, "hostname", output, sizeof(output)) == 0) {
        strncpy_s(device->hostname, sizeof(device->hostname), output, _TRUNCATE);
    }
    
    // Get OS info
    if (satani_linux_execute(target, username, password, "cat /etc/os-release | grep PRETTY_NAME", output, sizeof(output)) == 0) {
        char* start = strchr(output, '=');
        if (start) {
            start++;
            if (*start == '"') start++;
            char* end = strchr(start, '"');
            if (end) *end = '\0';
            end = strchr(start, '\n');
            if (end) *end = '\0';
            strncpy_s(device->os, sizeof(device->os), start, _TRUNCATE);
        }
    }
    
    // Get CPU info
    if (satani_linux_execute(target, username, password, "lscpu | grep 'Model name'", output, sizeof(output)) == 0) {
        char* start = strchr(output, ':');
        if (start) {
            start++;
            while (*start == ' ') start++;
            char* end = strchr(start, '\n');
            if (end) *end = '\0';
            strncpy_s(device->cpu_info, sizeof(device->cpu_info), start, _TRUNCATE);
        }
    }
    
    // Get memory info
    if (satani_linux_execute(target, username, password, "free -h | grep Mem", output, sizeof(output)) == 0) {
        strncpy_s(device->memory_info, sizeof(device->memory_info), output, _TRUNCATE);
    }
    
    // Get disk info
    if (satani_linux_execute(target, username, password, "df -h", output, sizeof(output)) == 0) {
        strncpy_s(device->disk_info, sizeof(device->disk_info), output, _TRUNCATE);
    }
    
    // Get network interfaces
    if (satani_linux_execute(target, username, password, "ip addr show", output, sizeof(output)) == 0) {
        strncpy_s(device->network_adapters, sizeof(device->network_adapters), output, _TRUNCATE);
    }
    
    return 0;
}

// List processes on Linux
int satani_linux_process_list(const char* target, const char* username, const char* password,
                             satani_process_info_t** processes, int* count) {
    char output[65536];
    char cmd[] = "ps -eo pid,ppid,comm --no-headers";
    
    if (satani_linux_execute(target, username, password, cmd, output, sizeof(output)) != 0) {
        return -1;
    }
    
    // Count lines
    int lines = 0;
    for (char* p = output; *p; p++) {
        if (*p == '\n') lines++;
    }
    
    *processes = (satani_process_info_t*)malloc(sizeof(satani_process_info_t) * lines);
    *count = 0;
    
    // Parse output
    char* line = strtok(output, "\n");
    while (line && *count < lines) {
        int pid, ppid;
        char comm[260];
        
        if (sscanf(line, "%d %d %s", &pid, &ppid, comm) == 3) {
            (*processes)[*count].pid = pid;
            (*processes)[*count].parent_pid = ppid;
            strcpy_s((*processes)[*count].name, sizeof((*processes)[*count].name), comm);
            (*count)++;
        }
        
        line = strtok(NULL, "\n");
    }
    
    return 0;
}

// Terminate process on Linux
int satani_linux_process_terminate(const char* target, const char* username, const char* password, int pid) {
    char cmd[256];
    sprintf_s(cmd, sizeof(cmd), "kill -9 %d", pid);
    
    char output[1024];
    return satani_linux_execute(target, username, password, cmd, output, sizeof(output));
}

// List services on Linux (systemctl)
int satani_linux_service_list(const char* target, const char* username, const char* password,
                             char* output, size_t output_size) {
    return satani_linux_execute(target, username, password, 
                               "systemctl list-units --type=service --no-pager", output, output_size);
}

// Control service on Linux
int satani_linux_service_control(const char* target, const char* username, const char* password,
                                const char* service_name, const char* action) {
    char cmd[512];
    sprintf_s(cmd, sizeof(cmd), "systemctl %s %s", action, service_name);
    
    char output[1024];
    return satani_linux_execute(target, username, password, cmd, output, sizeof(output));
}

// List users on Linux
int satani_linux_list_users(const char* target, const char* username, const char* password,
                           char* output, size_t output_size) {
    return satani_linux_execute(target, username, password, 
                               "cat /etc/passwd | cut -d: -f1,3,6,7", output, output_size);
}

// Get network connections on Linux
int satani_linux_netstat(const char* target, const char* username, const char* password,
                        char* output, size_t output_size) {
    return satani_linux_execute(target, username, password, 
                               "ss -tulnp", output, output_size);
}

// Get open ports on Linux
int satani_linux_open_ports(const char* target, const char* username, const char* password,
                           char* output, size_t output_size) {
    return satani_linux_execute(target, username, password, 
                               "ss -tuln | grep LISTEN", output, output_size);
}

// Read file on Linux
int satani_linux_read_file(const char* target, const char* username, const char* password,
                          const char* file_path, char* output, size_t output_size) {
    char cmd[1024];
    sprintf_s(cmd, sizeof(cmd), "cat '%s' 2>/dev/null || echo 'File not found or no permission'", file_path);
    return satani_linux_execute(target, username, password, cmd, output, output_size);
}

// Write file on Linux
int satani_linux_write_file(const char* target, const char* username, const char* password,
                           const char* file_path, const char* content) {
    // Use heredoc to write file
    char cmd[8192];
    sprintf_s(cmd, sizeof(cmd), "cat > '%s' << 'SATANI_EOF'\n%s\nSATANI_EOF", file_path, content);
    
    char output[1024];
    return satani_linux_execute(target, username, password, cmd, output, sizeof(output));
}

// Execute script on Linux
int satani_linux_execute_script(const char* target, const char* username, const char* password,
                               const char* script, char* output, size_t output_size) {
    char cmd[16384];
    sprintf_s(cmd, sizeof(cmd), "bash -c '%s'", script);
    return satani_linux_execute(target, username, password, cmd, output, output_size);
}

// Install package on Linux
int satani_linux_install_package(const char* target, const char* username, const char* password,
                                const char* package_name) {
    // Try apt first, then yum, then pacman
    char cmd[512];
    char output[4096];
    
    sprintf_s(cmd, sizeof(cmd), "apt-get install -y %s 2>/dev/null || yum install -y %s 2>/dev/null || pacman -S --noconfirm %s", 
              package_name, package_name, package_name);
    
    return satani_linux_execute(target, username, password, cmd, output, sizeof(output));
}

// Get firewall status on Linux
int satani_linux_firewall_status(const char* target, const char* username, const char* password,
                                char* output, size_t output_size) {
    return satani_linux_execute(target, username, password, 
                               "iptables -L -n 2>/dev/null || firewall-cmd --list-all 2>/dev/null || ufw status 2>/dev/null", 
                               output, output_size);
}

// Get cron jobs on Linux
int satani_linux_list_cron(const char* target, const char* username, const char* password,
                          char* output, size_t output_size) {
    return satani_linux_execute(target, username, password, 
                               "cat /etc/crontab; for user in $(cut -f1 -d: /etc/passwd); do echo '=== $user ==='; crontab -u $user -l 2>/dev/null; done", 
                               output, output_size);
}

// Get login history on Linux
int satani_linux_login_history(const char* target, const char* username, const char* password,
                              char* output, size_t output_size) {
    return satani_linux_execute(target, username, password, "last -n 50", output, output_size);
}

// Check sudo access on Linux
int satani_linux_check_sudo(const char* target, const char* username, const char* password,
                           char* output, size_t output_size) {
    return satani_linux_execute(target, username, password, "sudo -l 2>&1", output, output_size);
}

// Execute with sudo on Linux
int satani_linux_sudo_execute(const char* target, const char* username, const char* password,
                             const char* command, char* output, size_t output_size) {
    char cmd[2048];
    sprintf_s(cmd, sizeof(cmd), "echo '%s' | sudo -S %s", password, command);
    return satani_linux_execute(target, username, password, cmd, output, output_size);
}

// Get Docker containers on Linux
int satani_linux_docker_ps(const char* target, const char* username, const char* password,
                          char* output, size_t output_size) {
    return satani_linux_execute(target, username, password, "docker ps -a", output, output_size);
}

// Execute in Docker container
int satani_linux_docker_exec(const char* target, const char* username, const char* password,
                            const char* container_id, const char* command, char* output, size_t output_size) {
    char cmd[1024];
    sprintf_s(cmd, sizeof(cmd), "docker exec %s %s", container_id, command);
    return satani_linux_execute(target, username, password, cmd, output, output_size);
}

// Reboot Linux system
int satani_linux_reboot(const char* target, const char* username, const char* password) {
    char output[1024];
    return satani_linux_execute(target, username, password, "sudo reboot", output, sizeof(output));
}

// Shutdown Linux system
int satani_linux_shutdown(const char* target, const char* username, const char* password) {
    char output[1024];
    return satani_linux_execute(target, username, password, "sudo shutdown -h now", output, sizeof(output));
}

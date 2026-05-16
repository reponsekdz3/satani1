// SATANI - Real Remote Command Execution
// SSH-based remote execution using libssh2 or direct socket

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

// SSH2 protocol constants
#define SSH_MSG_DISCONNECT              1
#define SSH_MSG_IGNORE                  2
#define SSH_MSG_UNIMPLEMENTED           3
#define SSH_MSG_DEBUG                   4
#define SSH_MSG_SERVICE_REQUEST         5
#define SSH_MSG_SERVICE_ACCEPT          6
#define SSH_MSG_KEXINIT                 20
#define SSH_MSG_NEWKEYS                 21
#define SSH_MSG_USERAUTH_REQUEST        50
#define SSH_MSG_USERAUTH_FAILURE        51
#define SSH_MSG_USERAUTH_SUCCESS        52
#define SSH_MSG_GLOBAL_REQUEST          80
#define SSH_MSG_CHANNEL_OPEN            90
#define SSH_MSG_CHANNEL_OPEN_CONFIRM    91
#define SSH_MSG_CHANNEL_OPEN_FAILURE    92
#define SSH_MSG_CHANNEL_WINDOW_ADJUST   93
#define SSH_MSG_CHANNEL_DATA            94
#define SSH_MSG_CHANNEL_EXTENDED_DATA   95
#define SSH_MSG_CHANNEL_EOF             96
#define SSH_MSG_CHANNEL_CLOSE           97
#define SSH_MSG_CHANNEL_REQUEST         98
#define SSH_MSG_CHANNEL_SUCCESS         99
#define SSH_MSG_CHANNEL_FAILURE         100

// Simple SSH version exchange
int ssh_version_exchange(const char* ip, int port, char* version, size_t size) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return -1;
    }
    
    DWORD timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
    
    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    target.sin_addr.s_addr = inet_addr(ip);
    
    if (connect(sock, (struct sockaddr*)&target, sizeof(target)) != 0) {
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    
    // Receive SSH version string
    char banner[256] = {0};
    int received = recv(sock, banner, sizeof(banner) - 1, 0);
    
    if (received > 0) {
        banner[received] = '\0';
        char* newline = strchr(banner, '\n');
        if (newline) *newline = '\0';
        if (newline) *newline = '\r';
        
        strncpy_s(version, size, banner, _TRUNCATE);
        
        // Send our version
        send(sock, "SSH-2.0-Satani_1.0\r\n", 20, 0);
        
        closesocket(sock);
        WSACleanup();
        return 0;
    }
    
    closesocket(sock);
    WSACleanup();
    return -1;
}

// Execute command via SSH (requires external SSH client)
int ssh_execute_command(const char* ip, const char* user, const char* password,
                        const char* command, char* output, size_t output_size) {
    char ssh_cmd[1024];
    
    // Use plink or ssh client
    sprintf_s(ssh_cmd, sizeof(ssh_cmd),
              "plink -ssh -batch -pw %s %s@%s \"%s\"",
              password, user, ip, command);
    
    FILE* pipe = _popen(ssh_cmd, "r");
    if (!pipe) {
        strncpy_s(output, output_size, "Failed to execute SSH command", _TRUNCATE);
        return -1;
    }
    
    size_t pos = 0;
    char buffer[256];
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

// Telnet-based command execution (for legacy systems)
int telnet_execute_command(const char* ip, int port, const char* user,
                           const char* password, const char* command,
                           char* output, size_t output_size) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return -1;
    }
    
    DWORD timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    struct sockaddr_in target;
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    target.sin_addr.s_addr = inet_addr(ip);
    
    if (connect(sock, (struct sockaddr*)&target, sizeof(target)) != 0) {
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    
    // Wait for login prompt
    char buffer[1024] = {0};
    recv(sock, buffer, sizeof(buffer) - 1, 0);
    
    // Send username
    char user_cmd[128];
    sprintf_s(user_cmd, sizeof(user_cmd), "%s\r\n", user);
    send(sock, user_cmd, (int)strlen(user_cmd), 0);
    recv(sock, buffer, sizeof(buffer) - 1, 0);
    
    // Send password
    char pass_cmd[128];
    sprintf_s(pass_cmd, sizeof(pass_cmd), "%s\r\n", password);
    send(sock, pass_cmd, (int)strlen(pass_cmd), 0);
    recv(sock, buffer, sizeof(buffer) - 1, 0);
    
    // Send command
    char exec_cmd[256];
    sprintf_s(exec_cmd, sizeof(exec_cmd), "%s\r\n", command);
    send(sock, exec_cmd, (int)strlen(exec_cmd), 0);
    
    // Receive output
    int total = 0;
    int received;
    while ((received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[received] = '\0';
        if (total + received < (int)output_size - 1) {
            strcat_s(output, output_size, buffer);
            total += received;
        }
    }
    
    closesocket(sock);
    WSACleanup();
    return 0;
}

// WinRM-based command execution (Windows Remote Management)
int winrm_execute_command(const char* ip, const char* user, const char* password,
                          const char* command, char* output, size_t output_size) {
    char cmd[1024];
    
    // Use winrs for Windows remote execution
    sprintf_s(cmd, sizeof(cmd),
              "winrs /r:%s /u:%s /p:%s \"%s\"",
              ip, user, password, command);
    
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) {
        strncpy_s(output, output_size, "Failed to execute WinRM command", _TRUNCATE);
        return -1;
    }
    
    size_t pos = 0;
    char buffer[256];
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

// WMI-based command execution
int wmi_execute_command(const char* ip, const char* user, const char* password,
                        const char* command, char* output, size_t output_size) {
    char cmd[2048];
    
    // Use WMIC for WMI-based execution
    sprintf_s(cmd, sizeof(cmd),
              "wmic /node:%s /user:%s /password:%s process call create \"%s\"",
              ip, user, password, command);
    
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) {
        strncpy_s(output, output_size, "Failed to execute WMI command", _TRUNCATE);
        return -1;
    }
    
    size_t pos = 0;
    char buffer[256];
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

// PsExec-based execution
int psexec_execute_command(const char* ip, const char* user, const char* password,
                           const char* command, char* output, size_t output_size) {
    char cmd[2048];
    
    // Use PsExec (from Sysinternals)
    if (user && password) {
        sprintf_s(cmd, sizeof(cmd),
                  "psexec \\\\%s -u %s -p %s -accepteula cmd /c \"%s\"",
                  ip, user, password, command);
    } else {
        sprintf_s(cmd, sizeof(cmd),
                  "psexec \\\\%s -accepteula cmd /c \"%s\"",
                  ip, command);
    }
    
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) {
        strncpy_s(output, output_size, "Failed to execute PsExec command", _TRUNCATE);
        return -1;
    }
    
    size_t pos = 0;
    char buffer[256];
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

// PowerOff/Restart using Windows RPC
int rpc_shutdown(const char* ip, const char* user, const char* password,
                 int timeout_seconds, int restart, int force) {
    char cmd[512];
    
    // Use shutdown command with remote options
    if (restart) {
        sprintf_s(cmd, sizeof(cmd),
                  "shutdown /r /m \\\\%s /t %d %s /c \"Remote shutdown initiated by Satani\"",
                  ip, timeout_seconds, force ? "/f" : "");
    } else {
        sprintf_s(cmd, sizeof(cmd),
                  "shutdown /s /m \\\\%s /t %d %s /c \"Remote shutdown initiated by Satani\"",
                  ip, timeout_seconds, force ? "/f" : "");
    }
    
    return system(cmd);
}

// Cancel remote shutdown
int rpc_abort_shutdown(const char* ip) {
    char cmd[256];
    sprintf_s(cmd, sizeof(cmd), "shutdown /a /m \\\\%s", ip);
    return system(cmd);
}

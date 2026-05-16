// agentless_control.c - Powerful Agentless Remote Device Control
// Implements real remote control using multiple protocols

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <wtypes.h>
#include <objbase.h>
#include <wbemidl.h>
#include <comdef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <winternl.h>
#include <wininet.h>
#include "satani.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

// WMI Connection Structure
typedef struct {
    IWbemServices* services;
    IWbemLocator* locator;
    BSTR server;
    BOOL connected;
} wmi_connection_t;

// WMI Connection
int wmi_connect(const char* server, const char* username, const char* password, wmi_connection_t* conn) {
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return -1;
    
    hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                              RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    
    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&conn->locator);
    if (FAILED(hr)) { CoUninitialize(); return -1; }
    
    wchar_t wmi_path[512];
    swprintf_s(wmi_path, 512, L"\\\\%S\\root\\cimv2", server);
    
    BSTR path = SysAllocString(wmi_path);
    BSTR user = username ? SysAllocString(_bstr_t(username)) : NULL;
    BSTR pass = password ? SysAllocString(_bstr_t(password)) : NULL;
    
    hr = conn->locator->ConnectServer(path, user, pass, NULL, 0, NULL, NULL, &conn->services);
    
    SysFreeString(path);
    if (user) SysFreeString(user);
    if (pass) SysFreeString(pass);
    
    if (FAILED(hr)) {
        conn->locator->Release();
        CoUninitialize();
        return -1;
    }
    
    hr = CoSetProxyBlanket(conn->services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                           RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    
    if (FAILED(hr)) {
        conn->services->Release();
        conn->locator->Release();
        CoUninitialize();
        return -1;
    }
    
    conn->connected = TRUE;
    return 0;
}

// WMI Query Execution
int wmi_exec_query(wmi_connection_t* conn, const wchar_t* query, char* output, size_t output_size) {
    if (!conn->connected) return -1;
    
    BSTR wql = SysAllocString(L"WQL");
    BSTR q = SysAllocString(query);
    
    IEnumWbemClassObject* enumerator = NULL;
    HRESULT hr = conn->services->ExecQuery(wql, q, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                           NULL, &enumerator);
    
    SysFreeString(wql);
    SysFreeString(q);
    
    if (FAILED(hr)) return -1;
    
    IWbemClassObject* obj = NULL;
    ULONG ret = 0;
    size_t pos = 0;
    
    while (enumerator->Next(WBEM_INFINITE, 1, &obj, &ret) == S_OK && ret > 0) {
        SAFEARRAY* names = NULL;
        obj->GetNames(NULL, WBEM_FLAG_ALWAYS, NULL, &names);
        
        long lower, upper;
        SafeArrayGetLBound(names, 1, &lower);
        SafeArrayGetUBound(names, 1, &upper);
        
        for (long i = lower; i <= upper; i++) {
            BSTR name;
            SafeArrayGetElement(names, &i, &name);
            
            VARIANT val;
            VariantInit(&val);
            obj->Get(name, 0, &val, NULL, NULL);
            
            char prop[1024];
            sprintf_s(prop, sizeof(prop), "%S=%S\n", name, V_BSTR(&val));
            
            if (pos + strlen(prop) < output_size) {
                strcpy_s(output + pos, output_size - pos, prop);
                pos += strlen(prop);
            }
            
            VariantClear(&val);
            SysFreeString(name);
        }
        
        SafeArrayDestroy(names);
        obj->Release();
    }
    
    enumerator->Release();
    return 0;
}

// WMI Process Creation
int wmi_create_process(wmi_connection_t* conn, const char* command, int* process_id) {
    if (!conn->connected) return -1;
    
    BSTR class_name = SysAllocString(L"Win32_Process");
    IWbemClassObject* proc_class = NULL;
    IWbemClassObject* in_params = NULL;
    
    conn->services->GetObject(class_name, 0, NULL, &proc_class, NULL);
    SysFreeString(class_name);
    
    if (!proc_class) return -1;
    
    proc_class->GetMethod(L"Create", 0, &in_params, NULL);
    if (!in_params) { proc_class->Release(); return -1; }
    
    IWbemClassObject* inst = NULL;
    in_params->SpawnInstance(0, &inst);
    
    VARIANT cmd;
    VariantInit(&cmd);
    V_VT(&cmd) = VT_BSTR;
    V_BSTR(&cmd) = SysAllocString(_bstr_t(command));
    inst->Put(L"CommandLine", 0, &cmd, 0);
    VariantClear(&cmd);
    
    IWbemClassObject* results = NULL;
    conn->services->ExecMethod(_bstr_t(L"Win32_Process"), _bstr_t(L"Create"), 0, NULL, inst, &results, NULL);
    
    if (results) {
        VARIANT pid;
        VariantInit(&pid);
        results->Get(L"ProcessId", 0, &pid, NULL, NULL);
        *process_id = V_I4(&pid);
        VariantClear(&pid);
        results->Release();
    }
    
    inst->Release();
    in_params->Release();
    proc_class->Release();
    
    return *process_id > 0 ? 0 : -1;
}

void wmi_disconnect(wmi_connection_t* conn) {
    if (conn->services) { conn->services->Release(); conn->services = NULL; }
    if (conn->locator) { conn->locator->Release(); conn->locator = NULL; }
    conn->connected = FALSE;
    CoUninitialize();
}

// RPC Shutdown
int rpc_shutdown(const char* target, const char* message, int timeout, int force, int reboot) {
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return -1;
    
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
    CloseHandle(hToken);
    
    wchar_t wtarget[256];
    MultiByteToWideChar(CP_UTF8, 0, target, -1, wtarget, 256);
    
    DWORD flags = reboot ? EWX_REBOOT : EWX_SHUTDOWN;
    if (force) flags |= EWX_FORCE;
    
    if (!InitiateShutdownW(wtarget, L"Satani Remote Control", timeout, flags, SHUTDOWN_REASON_MAJOR_OTHER))
        return -1;
    
    return 0;
}

// SMB Admin Share Check
int smb_check_admin_access(const char* target) {
    wchar_t share_path[512];
    swprintf_s(share_path, 512, L"\\\\%S\\ADMIN$", target);
    
    NETRESOURCEW nr = {0};
    nr.dwType = RESOURCETYPE_DISK;
    nr.lpRemoteName = share_path;
    
    DWORD result = WNetAddConnection2W(&nr, NULL, NULL, CONNECT_INTERACTIVE);
    if (result == NO_ERROR) {
        WNetCancelConnection2W(share_path, 0, TRUE);
        return 0;
    }
    
    return -1;
}

// SMB Service Execution
int smb_service_exec(const char* target, const char* service_name, const char* command) {
    SC_HANDLE scm = OpenSCManagerA(target, SERVICES_ACTIVE_DATABASEA, SC_MANAGER_ALL_ACCESS);
    if (!scm) return -1;
    
    SC_HANDLE service = CreateServiceA(scm, service_name, service_name, SERVICE_ALL_ACCESS,
                                       SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START,
                                       SERVICE_ERROR_IGNORE, command, NULL, NULL, NULL, NULL, NULL);
    
    if (!service && GetLastError() != ERROR_SERVICE_EXISTS) {
        CloseServiceHandle(scm);
        return -1;
    }
    
    if (!service) service = OpenServiceA(scm, service_name, SERVICE_ALL_ACCESS);
    if (!service) { CloseServiceHandle(scm); return -1; }
    
    SERVICE_STATUS status;
    StartServiceA(service, 0, NULL);
    
    Sleep(3000);
    
    ControlService(service, SERVICE_CONTROL_STOP, &status);
    DeleteService(service);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    
    return 0;
}

// SMB Copy and Execute
int smb_copy_exec(const char* target, const char* local_file, const char* remote_name) {
    char remote_path[512];
    sprintf_s(remote_path, sizeof(remote_path), "\\\\%s\\ADMIN$\\%s", target, remote_name);
    
    if (!CopyFileA(local_file, remote_path, FALSE))
        return -1;
    
    char exec_cmd[512];
    sprintf_s(exec_cmd, sizeof(exec_cmd), "%%SystemRoot%%\\%s", remote_name);
    
    return smb_service_exec(target, "SataniExec", exec_cmd);
}

// WinRM Execute
int winrm_execute(const char* target, const char* command, const char* username, 
                  const char* password, char* output, size_t output_size) {
    char ps_cmd[2048];
    sprintf_s(ps_cmd, sizeof(ps_cmd),
        "powershell -Command \"$sec = ConvertTo-SecureString '%s' -AsPlainText -Force; "
        "$cred = New-Object System.Management.Automation.PSCredential('%s', $sec); "
        "Invoke-Command -ComputerName %s -Credential $cred -ScriptBlock { %s }\"",
        password, username, target, command);
    
    FILE* pipe = _popen(ps_cmd, "r");
    if (!pipe) return -1;
    
    size_t pos = 0;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) && pos < output_size - 1) {
        size_t len = strlen(buffer);
        if (pos + len < output_size) {
            strcpy_s(output + pos, output_size - pos, buffer);
            pos += len;
        }
    }
    
    _pclose(pipe);
    return pos > 0 ? 0 : -1;
}

// PsExec-style Execution
int psexec_style_exec(const char* target, const char* command, const char* username,
                      const char* password, char* output, size_t output_size) {
    char service_name[64];
    sprintf_s(service_name, sizeof(service_name), "Satani_%d", GetTickCount());
    
    char full_cmd[1024];
    sprintf_s(full_cmd, sizeof(full_cmd), "cmd /c %s > C:\\Windows\\Temp\\satani_out.txt 2>&1", command);
    
    SC_HANDLE scm = OpenSCManagerA(target, SERVICES_ACTIVE_DATABASEA, SC_MANAGER_ALL_ACCESS);
    if (!scm) return -1;
    
    SC_HANDLE service = CreateServiceA(scm, service_name, service_name, SERVICE_ALL_ACCESS,
                                       SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START,
                                       SERVICE_ERROR_IGNORE, full_cmd, NULL, NULL, NULL, username, password);
    
    if (!service) {
        CloseServiceHandle(scm);
        return -1;
    }
    
    StartServiceA(service, 0, NULL);
    
    SERVICE_STATUS status;
    for (int i = 0; i < 30; i++) {
        Sleep(1000);
        QueryServiceStatus(service, &status);
        if (status.dwCurrentState == SERVICE_STOPPED) break;
    }
    
    DeleteService(service);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    
    char output_path[512];
    sprintf_s(output_path, sizeof(output_path), "\\\\%s\\ADMIN$\\Temp\\satani_out.txt", target);
    
    HANDLE hFile = CreateFileA(output_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD bytes;
        ReadFile(hFile, output, (DWORD)output_size - 1, &bytes, NULL);
        output[bytes] = '\0';
        CloseHandle(hFile);
        DeleteFileA(output_path);
    }
    
    return 0;
}

// Remote Registry Read
int remote_registry_read(const char* target, HKEY root_key, const char* subkey, 
                         const char* value_name, char* data, size_t data_size) {
    HKEY hKey;
    LONG result = RegConnectRegistryA(target, root_key, &hKey);
    if (result != ERROR_SUCCESS) return -1;
    
    HKEY hSubKey;
    result = RegOpenKeyExA(hKey, subkey, 0, KEY_READ, &hSubKey);
    if (result != ERROR_SUCCESS) { RegCloseKey(hKey); return -1; }
    
    DWORD size = (DWORD)data_size;
    result = RegQueryValueExA(hSubKey, value_name, NULL, NULL, (LPBYTE)data, &size);
    
    RegCloseKey(hSubKey);
    RegCloseKey(hKey);
    
    return result == ERROR_SUCCESS ? 0 : -1;
}

// Remote Registry Write
int remote_registry_write(const char* target, HKEY root_key, const char* subkey,
                          const char* value_name, DWORD type, const void* data, size_t data_size) {
    HKEY hKey;
    LONG result = RegConnectRegistryA(target, root_key, &hKey);
    if (result != ERROR_SUCCESS) return -1;
    
    HKEY hSubKey;
    result = RegCreateKeyExA(hKey, subkey, 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL);
    if (result != ERROR_SUCCESS) { RegCloseKey(hKey); return -1; }
    
    result = RegSetValueExA(hSubKey, value_name, 0, type, (const BYTE*)data, (DWORD)data_size);
    
    RegCloseKey(hSubKey);
    RegCloseKey(hKey);
    
    return result == ERROR_SUCCESS ? 0 : -1;
}

// Remote Process List
int remote_process_list(const char* target, satani_process_info_t** processes, int* count) {
    wmi_connection_t conn;
    if (wmi_connect(target, NULL, NULL, &conn) != 0) return -1;
    
    char output[65536];
    if (wmi_exec_query(&conn, L"SELECT Name, ProcessId, ParentProcessId, ExecutablePath FROM Win32_Process", 
                       output, sizeof(output)) != 0) {
        wmi_disconnect(&conn);
        return -1;
    }
    
    *processes = (satani_process_info_t*)malloc(sizeof(satani_process_info_t) * 500);
    *count = 0;
    
    char* line = strtok(output, "\n");
    while (line && *count < 500) {
        if (strstr(line, "Name=")) {
            sscanf(line, "Name=%s", (*processes)[*count].name);
            (*count)++;
        }
        line = strtok(NULL, "\n");
    }
    
    wmi_disconnect(&conn);
    return 0;
}

// Remote Process Terminate
int remote_process_terminate(const char* target, int pid) {
    wmi_connection_t conn;
    if (wmi_connect(target, NULL, NULL, &conn) != 0) return -1;
    
    char query[256];
    sprintf_s(query, sizeof(query), "SELECT * FROM Win32_Process WHERE ProcessId = %d", pid);
    
    IEnumWbemClassObject* enumerator = NULL;
    BSTR wql = SysAllocString(L"WQL");
    BSTR q = SysAllocString(_bstr_t(query));
    
    conn.services->ExecQuery(wql, q, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);
    
    SysFreeString(wql);
    SysFreeString(q);
    
    if (!enumerator) { wmi_disconnect(&conn); return -1; }
    
    IWbemClassObject* obj = NULL;
    ULONG ret = 0;
    
    if (enumerator->Next(WBEM_INFINITE, 1, &obj, &ret) == S_OK && ret > 0) {
        IWbemClassObject* results = NULL;
        conn.services->ExecMethod(_bstr_t(L"Win32_Process"), _bstr_t(L"Terminate"), 0, NULL, NULL, &results, NULL);
        if (results) results->Release();
        obj->Release();
    }
    
    enumerator->Release();
    wmi_disconnect(&conn);
    return 0;
}

// Remote Service List
int remote_service_list(const char* target, satani_service_info_t** services, int* count) {
    SC_HANDLE scm = OpenSCManagerA(target, SERVICES_ACTIVE_DATABASEA, SC_MANAGER_ENUMERATE_SERVICE);
    if (!scm) return -1;
    
    DWORD bytes_needed = 0, services_count = 0, resume = 0;
    EnumServicesStatusExA(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
                          NULL, 0, &bytes_needed, &services_count, &resume, NULL);
    
    *services = (satani_service_info_t*)malloc(bytes_needed);
    LPENUM_SERVICE_STATUS_PROCESSA status = (LPENUM_SERVICE_STATUS_PROCESSA)malloc(bytes_needed);
    
    if (!EnumServicesStatusExA(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
                               (LPBYTE)status, bytes_needed, &bytes_needed, &services_count, &resume, NULL)) {
        free(status);
        free(*services);
        CloseServiceHandle(scm);
        return -1;
    }
    
    *count = services_count;
    for (DWORD i = 0; i < services_count; i++) {
        strcpy_s((*services)[i].name, 256, status[i].lpServiceName);
        strcpy_s((*services)[i].display_name, 256, status[i].lpDisplayName);
        (*services)[i].state = status[i].ServiceStatusProcess.dwCurrentState;
        (*services)[i].type = status[i].ServiceStatusProcess.dwServiceType;
    }
    
    free(status);
    CloseServiceHandle(scm);
    return 0;
}

// Remote Service Control
int remote_service_control(const char* target, const char* service_name, DWORD control_code) {
    SC_HANDLE scm = OpenSCManagerA(target, SERVICES_ACTIVE_DATABASEA, SC_MANAGER_CONNECT);
    if (!scm) return -1;
    
    SC_HANDLE service = OpenServiceA(scm, service_name, SERVICE_ALL_ACCESS);
    if (!service) { CloseServiceHandle(scm); return -1; }
    
    SERVICE_STATUS status;
    BOOL result = FALSE;
    
    switch (control_code) {
        case SERVICE_CONTROL_START:
            result = StartServiceA(service, 0, NULL);
            break;
        case SERVICE_CONTROL_STOP:
            result = ControlService(service, SERVICE_CONTROL_STOP, &status);
            break;
        case SERVICE_CONTROL_PAUSE:
            result = ControlService(service, SERVICE_CONTROL_PAUSE, &status);
            break;
        case SERVICE_CONTROL_CONTINUE:
            result = ControlService(service, SERVICE_CONTROL_CONTINUE, &status);
            break;
    }
    
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    
    return result ? 0 : -1;
}

// Remote System Information
int remote_get_system_info(const char* target, satani_device_t* device) {
    wmi_connection_t conn;
    if (wmi_connect(target, NULL, NULL, &conn) != 0) return -1;
    
    char output[4096];
    
    if (wmi_exec_query(&conn, L"SELECT Caption, Version, OSArchitecture FROM Win32_OperatingSystem", 
                       output, sizeof(output)) == 0) {
        strncpy_s(device->os, sizeof(device->os), output, _TRUNCATE);
    }
    
    if (wmi_exec_query(&conn, L"SELECT Manufacturer, Model, TotalPhysicalMemory FROM Win32_ComputerSystem", 
                       output, sizeof(output)) == 0) {
        strncpy_s(device->motherboard_info, sizeof(device->motherboard_info), output, _TRUNCATE);
    }
    
    if (wmi_exec_query(&conn, L"SELECT Name, NumberOfCores, MaxClockSpeed FROM Win32_Processor", 
                       output, sizeof(output)) == 0) {
        strncpy_s(device->cpu_info, sizeof(device->cpu_info), output, _TRUNCATE);
    }
    
    if (wmi_exec_query(&conn, L"SELECT DeviceID, Size, FreeSpace FROM Win32_LogicalDisk", 
                       output, sizeof(output)) == 0) {
        strncpy_s(device->disk_info, sizeof(device->disk_info), output, _TRUNCATE);
    }
    
    if (wmi_exec_query(&conn, L"SELECT Description, IPAddress, MACAddress FROM Win32_NetworkAdapterConfiguration WHERE IPEnabled=TRUE", 
                       output, sizeof(output)) == 0) {
        strncpy_s(device->network_adapters, sizeof(device->network_adapters), output, _TRUNCATE);
    }
    
    wmi_disconnect(&conn);
    return 0;
}

// Unified Agentless Control API
int satani_agentless_execute(const char* target, const char* command, const char* username,
                            const char* password, int protocol, char* output, size_t output_size) {
    switch (protocol) {
        case 1: // WMI
            {
                wmi_connection_t conn;
                if (wmi_connect(target, username, password, &conn) == 0) {
                    int pid;
                    int result = wmi_create_process(&conn, command, &pid);
                    wmi_disconnect(&conn);
                    return result;
                }
            }
            break;
            
        case 2: // WinRM
            return winrm_execute(target, command, username, password, output, output_size);
            
        case 3: // PsExec-style
            return psexec_style_exec(target, command, username, password, output, output_size);
            
        case 4: // SMB Service
            return smb_service_exec(target, "SataniService", command);
            
        default:
            if (winrm_execute(target, command, username, password, output, output_size) == 0)
                return 0;
            if (psexec_style_exec(target, command, username, password, output, output_size) == 0)
                return 0;
            if (smb_service_exec(target, "SataniService", command) == 0)
                return 0;
            break;
    }
    
    return -1;
}

int satani_agentless_shutdown(const char* target, const char* username, const char* password, 
                             int timeout, int force, int reboot) {
    if (rpc_shutdown(target, "Satani Remote Shutdown", timeout, force, reboot) == 0)
        return 0;
    
    wmi_connection_t conn;
    if (wmi_connect(target, username, password, &conn) == 0) {
        char cmd[128];
        sprintf_s(cmd, sizeof(cmd), "shutdown %s /t %d %s", 
                  reboot ? "/r" : "/s", timeout, force ? "/f" : "");
        
        int pid;
        int result = wmi_create_process(&conn, cmd, &pid);
        wmi_disconnect(&conn);
        return result;
    }
    
    return -1;
}

int satani_agentless_service_control(const char* target, const char* service_name, 
                                    const char* action, const char* username, const char* password) {
    DWORD control = 0;
    
    if (strcmp(action, "start") == 0) control = SERVICE_CONTROL_START;
    else if (strcmp(action, "stop") == 0) control = SERVICE_CONTROL_STOP;
    else if (strcmp(action, "pause") == 0) control = SERVICE_CONTROL_PAUSE;
    else if (strcmp(action, "continue") == 0) control = SERVICE_CONTROL_CONTINUE;
    else return -1;
    
    return remote_service_control(target, service_name, control);
}

int satani_agentless_registry_read(const char* target, const char* key_path, 
                                  const char* value_name, char* data, size_t data_size) {
    HKEY root = HKEY_LOCAL_MACHINE;
    if (strncmp(key_path, "HKLM\\", 5) == 0) root = HKEY_LOCAL_MACHINE;
    else if (strncmp(key_path, "HKCU\\", 5) == 0) root = HKEY_CURRENT_USER;
    else if (strncmp(key_path, "HKCR\\", 5) == 0) root = HKEY_CLASSES_ROOT;
    
    const char* subkey = strchr(key_path, '\\');
    if (subkey) subkey++;
    
    return remote_registry_read(target, root, subkey, value_name, data, data_size);
}

int satani_agentless_registry_write(const char* target, const char* key_path,
                                   const char* value_name, const char* data, DWORD type) {
    HKEY root = HKEY_LOCAL_MACHINE;
    if (strncmp(key_path, "HKLM\\", 5) == 0) root = HKEY_LOCAL_MACHINE;
    else if (strncmp(key_path, "HKCU\\", 5) == 0) root = HKEY_CURRENT_USER;
    
    const char* subkey = strchr(key_path, '\\');
    if (subkey) subkey++;
    
    return remote_registry_write(target, root, subkey, value_name, type, data, strlen(data));
}

int satani_agentless_upload_file(const char* target, const char* local_path, const char* remote_path) {
    char smb_path[512];
    sprintf_s(smb_path, sizeof(smb_path), "\\\\%s\\ADMIN$\\%s", target, remote_path);
    
    if (CopyFileA(local_path, smb_path, FALSE))
        return 0;
    
    return -1;
}

int satani_agentless_download_file(const char* target, const char* remote_path, const char* local_path) {
    char smb_path[512];
    sprintf_s(smb_path, sizeof(smb_path), "\\\\%s\\ADMIN$\\%s", target, remote_path);
    
    if (CopyFileA(smb_path, local_path, FALSE))
        return 0;
    
    return -1;
}
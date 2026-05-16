// agentless_advanced.c - Advanced Agentless Control Features
// Extended remote operations without agent installation

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
#include <wininet.h>
#include <shellapi.h>
#include <lm.h>
#include <taskschd.h>
#include "satani.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "credui.lib")
#pragma comment(lib, "comctl32.lib")

// ==================== Remote PowerShell Execution ====================

int satani_agentless_powershell(const char* target, const char* script,
                               const char* username, const char* password,
                               char* output, size_t output_size) {
    // Build encoded command for PowerShell
    char ps_cmd[8192];
    sprintf_s(ps_cmd, sizeof(ps_cmd),
        "powershell -ExecutionPolicy Bypass -Command "
        "\"$sec = ConvertTo-SecureString '%s' -AsPlainText -Force; "
        "$cred = New-Object System.Management.Automation.PSCredential('%s', $sec); "
        "$s = New-PSSession -ComputerName %s -Credential $cred; "
        "$r = Invoke-Command -Session $s -ScriptBlock { %s }; "
        "$r | Out-String; Remove-PSSession $s\"",
        password, username, target, script);
    
    FILE* pipe = _popen(ps_cmd, "r");
    if (!pipe) return -1;
    
    size_t pos = 0;
    char buffer[2048];
    while (fgets(buffer, sizeof(buffer), pipe) && pos < output_size - 1) {
        size_t len = strlen(buffer);
        if (pos + len < output_size) {
            strcpy_s(output + pos, output_size - pos, buffer);
            pos += len;
        }
    }
    
    int status = _pclose(pipe);
    return status == 0 ? 0 : -1;
}

// ==================== Remote WMI Query ====================

int satani_agentless_wmi_query(const char* target, const char* query,
                              const char* username, const char* password,
                              char* output, size_t output_size) {
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return -1;
    
    IWbemLocator* locator = NULL;
    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&locator);
    if (FAILED(hr)) { CoUninitialize(); return -1; }
    
    wchar_t wmi_path[512];
    swprintf_s(wmi_path, 512, L"\\\\\\\\%S\\root\\cimv2", target);
    
    BSTR path = SysAllocString(wmi_path);
    BSTR user = username ? SysAllocString(_bstr_t(username)) : NULL;
    BSTR pass = password ? SysAllocString(_bstr_t(password)) : NULL;
    
    IWbemServices* services = NULL;
    hr = locator->ConnectServer(path, user, pass, NULL, 0, NULL, NULL, &services);
    
    SysFreeString(path);
    if (user) SysFreeString(user);
    if (pass) SysFreeString(pass);
    
    if (FAILED(hr)) {
        locator->Release();
        CoUninitialize();
        return -1;
    }
    
    CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                      RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    
    BSTR wql = SysAllocString(L"WQL");
    BSTR q = SysAllocString(_bstr_t(query));
    
    IEnumWbemClassObject* enumerator = NULL;
    hr = services->ExecQuery(wql, q, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);
    
    SysFreeString(wql);
    SysFreeString(q);
    
    if (FAILED(hr)) {
        services->Release();
        locator->Release();
        CoUninitialize();
        return -1;
    }
    
    size_t pos = 0;
    IWbemClassObject* obj = NULL;
    ULONG ret = 0;
    
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
            
            char prop[2048];
            if (val.vt == VT_BSTR) {
                sprintf_s(prop, sizeof(prop), "%S=%S\\n", name, val.bstrVal);
            } else if (val.vt == VT_I4) {
                sprintf_s(prop, sizeof(prop), "%S=%d\\n", name, val.lVal);
            } else if (val.vt == VT_BOOL) {
                sprintf_s(prop, sizeof(prop), "%S=%s\\n", name, val.boolVal ? "TRUE" : "FALSE");
            } else {
                sprintf_s(prop, sizeof(prop), "%S=(unknown type)\\n", name);
            }
            
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
    services->Release();
    locator->Release();
    CoUninitialize();
    
    return 0;
}

// ==================== Remote Event Log ====================

int satani_agentless_read_event_log(const char* target, const char* log_name,
                                   int event_count, char* output, size_t output_size,
                                   const char* username, const char* password) {
    char wql[512];
    sprintf_s(wql, sizeof(wql),
        "SELECT TOP %d TimeGenerated, SourceName, EventIdentifier, Message FROM Win32_NTLogEvent WHERE LogFile='%s'",
        event_count, log_name);
    
    return satani_agentless_wmi_query(target, wql, username, password, output, output_size);
}

int satani_agentless_clear_event_log(const char* target, const char* log_name,
                                    const char* username, const char* password) {
    char ps_cmd[512];
    sprintf_s(ps_cmd, sizeof(ps_cmd), "Clear-EventLog -LogName '%s'", log_name);
    
    char output[1024];
    return satani_agentless_powershell(target, ps_cmd, username, password, output, sizeof(output));
}

// ==================== Remote Firewall Control ====================

int satani_agentless_get_firewall_status(const char* target, char* output, size_t output_size,
                                        const char* username, const char* password) {
    char ps_cmd[] = "Get-NetFirewallProfile | Select-Object Name, Enabled | Format-Table -AutoSize";
    return satani_agentless_powershell(target, ps_cmd, username, password, output, output_size);
}

int satani_agentless_add_firewall_rule(const char* target, const char* rule_name,
                                      const char* direction, int port, const char* action,
                                      const char* username, const char* password) {
    char ps_cmd[1024];
    sprintf_s(ps_cmd, sizeof(ps_cmd),
        "New-NetFirewallRule -DisplayName '%s' -Direction %s -LocalPort %d -Protocol TCP -Action %s",
        rule_name, direction, port, action);
    
    char output[1024];
    return satani_agentless_powershell(target, ps_cmd, username, password, output, sizeof(output));
}

// ==================== Remote Network Share Control ====================

int satani_agentless_create_share(const char* target, const char* share_name,
                                 const char* path, const char* username, const char* password) {
    SHARE_INFO_2 share_info = {0};
    share_info.shi2_netname = (LPWSTR)share_name;
    share_info.shi2_type = STYPE_DISKTREE;
    share_info.shi2_remark = (LPWSTR)L"Satani Remote Share";
    share_info.shi2_permissions = ACCESS_ALL;
    share_info.shi2_max_uses = (DWORD)-1;
    share_info.shi2_current_uses = 0;
    share_info.shi2_path = (LPWSTR)path;
    share_info.shi2_passwd = NULL;
    
    DWORD parm_err = 0;
    NET_API_STATUS status = NetShareAdd((LPWSTR)_bstr_t(target), 2, (LPBYTE)&share_info, &parm_err);
    
    return status == NERR_Success ? 0 : -1;
}

int satani_agentless_delete_share(const char* target, const char* share_name,
                                 const char* username, const char* password) {
    NET_API_STATUS status = NetShareDel((LPWSTR)_bstr_t(target), (LPWSTR)_bstr_t(share_name), 0);
    return status == NERR_Success ? 0 : -1;
}

int satani_agentless_list_shares(const char* target, char* output, size_t output_size,
                                const char* username, const char* password) {
    char ps_cmd[] = "Get-SmbShare | Select-Object Name, Path, Description | Format-Table -AutoSize";
    return satani_agentless_powershell(target, ps_cmd, username, password, output, output_size);
}

// ==================== Remote Scheduled Task Control ====================

int satani_agentless_create_task(const char* target, const char* task_name, const char* command,
                                const char* username, const char* password) {
    char ps_cmd[1024];
    sprintf_s(ps_cmd, sizeof(ps_cmd),
        "$a = New-ScheduledTaskAction -Execute '%s'; "
        "$t = New-ScheduledTaskTrigger -Once -At (Get-Date); "
        "Register-ScheduledTask -TaskName '%s' -Action $a -Trigger $t",
        command, task_name);
    
    char output[1024];
    return satani_agentless_powershell(target, ps_cmd, username, password, output, sizeof(output));
}

int satani_agentless_run_task(const char* target, const char* task_name,
                             const char* username, const char* password) {
    char ps_cmd[512];
    sprintf_s(ps_cmd, sizeof(ps_cmd), "Start-ScheduledTask -TaskName '%s'", task_name);
    
    char output[1024];
    return satani_agentless_powershell(target, ps_cmd, username, password, output, sizeof(output));
}

int satani_agentless_delete_task(const char* target, const char* task_name,
                                const char* username, const char* password) {
    char ps_cmd[512];
    sprintf_s(ps_cmd, sizeof(ps_cmd), "Unregister-ScheduledTask -TaskName '%s' -Confirm:$false", task_name);
    
    char output[1024];
    return satani_agentless_powershell(target, ps_cmd, username, password, output, sizeof(output));
}

int satani_agentless_list_tasks(const char* target, char* output, size_t output_size,
                               const char* username, const char* password) {
    char ps_cmd[] = "Get-ScheduledTask | Select-Object TaskName, State, LastRunTime | Format-Table -AutoSize";
    return satani_agentless_powershell(target, ps_cmd, username, password, output, output_size);
}

// ==================== Remote User Management ====================

int satani_agentless_list_users(const char* target, char* output, size_t output_size,
                               const char* username, const char* password) {
    char wql[] = "SELECT Name, FullName, Description, Disabled, Lockout FROM Win32_UserAccount";
    return satani_agentless_wmi_query(target, wql, username, password, output, output_size);
}

int satani_agentless_create_user(const char* target, const char* new_user, const char* new_pass,
                                const char* username, const char* password) {
    char ps_cmd[1024];
    sprintf_s(ps_cmd, sizeof(ps_cmd),
        "New-LocalUser -Name '%s' -Password (ConvertTo-SecureString '%s' -AsPlainText -Force) -FullName '%s' -Description 'Created by Satani'",
        new_user, new_pass, new_user);
    
    char output[1024];
    return satani_agentless_powershell(target, ps_cmd, username, password, output, sizeof(output));
}

int satani_agentless_delete_user(const char* target, const char* user_to_delete,
                                const char* username, const char* password) {
    char ps_cmd[512];
    sprintf_s(ps_cmd, sizeof(ps_cmd), "Remove-LocalUser -Name '%s'", user_to_delete);
    
    char output[1024];
    return satani_agentless_powershell(target, ps_cmd, username, password, output, sizeof(output));
}

int satani_agentless_add_to_group(const char* target, const char* user, const char* group,
                                 const char* username, const char* password) {
    char ps_cmd[512];
    sprintf_s(ps_cmd, sizeof(ps_cmd), "Add-LocalGroupMember -Group '%s' -Member '%s'", group, user);
    
    char output[1024];
    return satani_agentless_powershell(target, ps_cmd, username, password, output, sizeof(output));
}

// ==================== Remote Network Operations ====================

int satani_agentless_netstat(const char* target, char* output, size_t output_size,
                            const char* username, const char* password) {
    char ps_cmd[] = "Get-NetTCPConnection | Select-Object LocalAddress, LocalPort, RemoteAddress, RemotePort, State, OwningProcess | Format-Table -AutoSize";
    return satani_agentless_powershell(target, ps_cmd, username, password, output, output_size);
}

int satani_agentless_route_table(const char* target, char* output, size_t output_size,
                                const char* username, const char* password) {
    char ps_cmd[] = "Get-NetRoute | Select-Object DestinationPrefix, NextHop, RouteMetric, ifIndex | Format-Table -AutoSize";
    return satani_agentless_powershell(target, ps_cmd, username, password, output, output_size);
}

int satani_agentless_arp_cache(const char* target, char* output, size_t output_size,
                              const char* username, const char* password) {
    char ps_cmd[] = "Get-NetNeighbor | Select-Object IPAddress, LinkLayerAddress, State, ifIndex | Format-Table -AutoSize";
    return satani_agentless_powershell(target, ps_cmd, username, password, output, output_size);
}

// ==================== Elevated Execution ====================

int satani_agentless_execute_elevated(const char* target, const char* command,
                                     const char* username, const char* password,
                                     char* output, size_t output_size) {
    // Use scheduled task for elevated execution
    char task_name[64];
    sprintf_s(task_name, sizeof(task_name), "Satani_Elevated_%d", GetTickCount());
    
    // Create task with highest privileges
    char ps_create[1024];
    sprintf_s(ps_create, sizeof(ps_create),
        "$p = New-ScheduledTaskPrincipal -UserId 'SYSTEM' -LogonType ServiceAccount -RunLevel Highest; "
        "$a = New-ScheduledTaskAction -Execute 'cmd.exe' -Argument '/c %s > C:\\\\Windows\\\\Temp\\\\satani_elevated.txt 2>&1'; "
        "$t = New-ScheduledTaskTrigger -Once -At (Get-Date).AddSeconds(5); "
        "$s = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries; "
        "Register-ScheduledTask -TaskName '%s' -Principal $p -Action $a -Trigger $t -Settings $s",
        command, task_name);
    
    char temp_output[1024];
    if (satani_agentless_powershell(target, ps_create, username, password, temp_output, sizeof(temp_output)) != 0)
        return -1;
    
    // Wait for task to complete
    Sleep(10000);
    
    // Read output
    char ps_read[512];
    sprintf_s(ps_read, sizeof(ps_read), "Get-Content 'C:\\\\Windows\\\\Temp\\\\satani_elevated.txt'");
    satani_agentless_powershell(target, ps_read, username, password, output, output_size);
    
    // Cleanup
    char ps_clean[512];
    sprintf_s(ps_clean, sizeof(ps_clean),
        "Unregister-ScheduledTask -TaskName '%s' -Confirm:$false; Remove-Item 'C:\\\\Windows\\\\Temp\\\\satani_elevated.txt' -Force",
        task_name);
    satani_agentless_powershell(target, ps_clean, username, password, temp_output, sizeof(temp_output));
    
    return 0;
}

// ==================== Batch Execution ====================

int satani_agentless_batch_execute(const char* target, const char** commands, int cmd_count,
                                  const char* username, const char* password,
                                  satani_remote_result_t* results) {
    for (int i = 0; i < cmd_count; i++) {
        results[i].process_id = 0;
        results[i].exit_code = satani_agentless_execute(target, commands[i], username, password,
                                                        AGENTLESS_AUTO, results[i].output, sizeof(results[i].output));
        results[i].completion_time = GetTickCount();
    }
    return 0;
}

// ==================== Registry Extended Operations ====================

int satani_agentless_registry_delete(const char* target, const char* key_path,
                                    const char* value_name) {
    char ps_cmd[1024];
    sprintf_s(ps_cmd, sizeof(ps_cmd), "Remove-ItemProperty -Path 'Registry::%s' -Name '%s' -Force", key_path, value_name);
    
    char output[1024];
    char* user = NULL, *pass = NULL;
    return satani_agentless_powershell(target, ps_cmd, user, pass, output, sizeof(output));
}

int satani_agentless_registry_create_key(const char* target, const char* key_path) {
    char ps_cmd[1024];
    sprintf_s(ps_cmd, sizeof(ps_cmd), "New-Item -Path 'Registry::%s' -Force", key_path);
    
    char output[1024];
    char* user = NULL, *pass = NULL;
    return satani_agentless_powershell(target, ps_cmd, user, pass, output, sizeof(output));
}

// ==================== File Operations ====================

int satani_agentless_delete_file(const char* target, const char* remote_path) {
    char smb_path[512];
    sprintf_s(smb_path, sizeof(smb_path), "\\\\%s\\ADMIN$\\%s", target, remote_path);
    return DeleteFileA(smb_path) ? 0 : -1;
}

int satani_agentless_list_directory(const char* target, const char* remote_dir,
                                   char* output, size_t output_size) {
    char ps_cmd[1024];
    sprintf_s(ps_cmd, sizeof(ps_cmd), "Get-ChildItem -Path '%s' | Select-Object Name, Length, LastWriteTime, Mode | Format-Table -AutoSize", remote_dir);
    
    char* user = NULL, *pass = NULL;
    return satani_agentless_powershell(target, ps_cmd, user, pass, output, output_size);
}

// ==================== System Snapshot ====================

int satani_agentless_get_snapshot(const char* target, satani_system_snapshot_t* snapshot,
                                 const char* username, const char* password) {
    // Get processes
    satani_agentless_process_list(target, &snapshot->processes, &snapshot->process_count, username, password);
    
    // Get services
    satani_agentless_service_list(target, &snapshot->services, &snapshot->service_count, username, password);
    
    // Get network connections
    satani_agentless_netstat(target, snapshot->network_connections, sizeof(snapshot->network_connections), username, password);
    
    // Get scheduled tasks
    satani_agentless_list_tasks(target, snapshot->scheduled_tasks, sizeof(snapshot->scheduled_tasks), username, password);
    
    // Get event log (last 50 events)
    satani_agentless_read_event_log(target, "Security", 50, snapshot->event_log, sizeof(snapshot->event_log), username, password);
    
    // Get users
    satani_agentless_list_users(target, snapshot->users, sizeof(snapshot->users), username, password);
    
    // Get shares
    satani_agentless_list_shares(target, snapshot->shares, sizeof(snapshot->shares), username, password);
    
    return 0;
}

// ==================== Cleanup Functions ====================

void satani_free_system_snapshot(satani_system_snapshot_t* snapshot) {
    if (snapshot->processes) {
        free(snapshot->processes);
        snapshot->processes = NULL;
    }
    if (snapshot->services) {
        free(snapshot->services);
        snapshot->services = NULL;
    }
}

void satani_free_remote_result(satani_remote_result_t* result) {
    // No dynamic memory in result structure
}

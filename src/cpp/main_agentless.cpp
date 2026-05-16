// main_agentless.cpp - Extended CLI for Agentless Control
// Add this to the existing main.cpp or compile separately

#include <iostream>
#include <string>
#include <cstring>
#include "satani.h"

void print_agentless_help() {
    std::cout << "\n=== AGENTLESS CONTROL COMMANDS ===\n\n";
    
    std::cout << "Remote Execution:\n";
    std::cout << "  satani control <ip> exec --cmd \"command\" --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> powershell --cmd \"script\" --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> wmi --cmd \"WQL query\" --user <user> --pass <pass>\n\n";
    
    std::cout << "System Control:\n";
    std::cout << "  satani control <ip> shutdown [--force] [--timeout 60] --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> restart [--force] --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> info --user <user> --pass <pass>\n\n";
    
    std::cout << "Service Control:\n";
    std::cout << "  satani control <ip> service list --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> service start --name <service> --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> service stop --name <service> --user <user> --pass <pass>\n\n";
    
    std::cout << "Process Control:\n";
    std::cout << "  satani control <ip> process list --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> process kill --pid <pid> --user <user> --pass <pass>\n\n";
    
    std::cout << "File Operations:\n";
    std::cout << "  satani control <ip> file upload --local <file> --remote <path> --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> file download --remote <path> --local <file> --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> file list --remote <path> --user <user> --pass <pass>\n\n";
    
    std::cout << "Registry Operations:\n";
    std::cout << "  satani control <ip> registry read --key <path> --value <name> --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> registry write --key <path> --value <name> --data <data> --user <user> --pass <pass>\n\n";
    
    std::cout << "Network Operations:\n";
    std::cout << "  satani control <ip> netstat --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> firewall --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> shares --user <user> --pass <pass>\n\n";
    
    std::cout << "User Operations:\n";
    std::cout << "  satani control <ip> users --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> user create --new-user <name> --new-pass <pass> --user <admin> --pass <pass>\n";
    std::cout << "  satani control <ip> user addgroup --user <name> --group <group> --admin <admin> --pass <pass>\n\n";
    
    std::cout << "Scheduled Tasks:\n";
    std::cout << "  satani control <ip> tasks --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> task create --name <task> --cmd \"command\" --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> task run --name <task> --user <user> --pass <pass>\n\n";
    
    std::cout << "Event Logs:\n";
    std::cout << "  satani control <ip> events --log <name> --count 50 --user <user> --pass <pass>\n";
    std::cout << "  satani control <ip> events clear --log <name> --user <user> --pass <pass>\n\n";
    
    std::cout << "Protocols:\n";
    std::cout << "  --protocol wmi     Use WMI (default for Windows)\n";
    std::cout << "  --protocol winrm   Use WinRM\n";
    std::cout << "  --protocol psexec  Use PsExec-style execution\n";
    std::cout << "  --protocol smb     Use SMB service creation\n";
    std::cout << "  --protocol ssh     Use SSH (Linux/Unix)\n\n";
}

int handle_agentless_control(int argc, char* argv[]) {
    if (argc < 4) {
        print_agentless_help();
        return 1;
    }
    
    const char* target = argv[2];
    const char* action = argv[3];
    
    // Parse options
    const char* username = NULL;
    const char* password = NULL;
    const char* command = NULL;
    const char* service_name = NULL;
    const char* registry_key = NULL;
    const char* registry_value = NULL;
    const char* registry_data = NULL;
    const char* local_file = NULL;
    const char* remote_file = NULL;
    const char* new_user = NULL;
    const char* new_pass = NULL;
    const char* group_name = NULL;
    const char* task_name = NULL;
    const char* log_name = "Security";
    const char* protocol = "auto";
    int pid = 0;
    int timeout = 60;
    int event_count = 50;
    bool force = false;
    
    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) username = argv[++i];
        else if (strcmp(argv[i], "--pass") == 0 && i + 1 < argc) password = argv[++i];
        else if (strcmp(argv[i], "--cmd") == 0 && i + 1 < argc) command = argv[++i];
        else if (strcmp(argv[i], "--name") == 0 && i + 1 < argc) service_name = argv[++i];
        else if (strcmp(argv[i], "--key") == 0 && i + 1 < argc) registry_key = argv[++i];
        else if (strcmp(argv[i], "--value") == 0 && i + 1 < argc) registry_value = argv[++i];
        else if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) registry_data = argv[++i];
        else if (strcmp(argv[i], "--local") == 0 && i + 1 < argc) local_file = argv[++i];
        else if (strcmp(argv[i], "--remote") == 0 && i + 1 < argc) remote_file = argv[++i];
        else if (strcmp(argv[i], "--new-user") == 0 && i + 1 < argc) new_user = argv[++i];
        else if (strcmp(argv[i], "--new-pass") == 0 && i + 1 < argc) new_pass = argv[++i];
        else if (strcmp(argv[i], "--group") == 0 && i + 1 < argc) group_name = argv[++i];
        else if (strcmp(argv[i], "--task") == 0 && i + 1 < argc) task_name = argv[++i];
        else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc) log_name = argv[++i];
        else if (strcmp(argv[i], "--pid") == 0 && i + 1 < argc) pid = atoi(argv[++i]);
        else if (strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) timeout = atoi(argv[++i]);
        else if (strcmp(argv[i], "--count") == 0 && i + 1 < argc) event_count = atoi(argv[++i]);
        else if (strcmp(argv[i], "--protocol") == 0 && i + 1 < argc) protocol = argv[++i];
        else if (strcmp(argv[i], "--force") == 0) force = true;
    }
    
    char output[16384] = "";
    int result = -1;
    
    printf("[*] Executing agentless control: %s on %s\n", action, target);
    
    // Determine protocol
    int protocol_id = 0;
    if (strcmp(protocol, "wmi") == 0) protocol_id = 1;
    else if (strcmp(protocol, "winrm") == 0) protocol_id = 2;
    else if (strcmp(protocol, "psexec") == 0) protocol_id = 3;
    else if (strcmp(protocol, "smb") == 0) protocol_id = 4;
    else if (strcmp(protocol, "ssh") == 0) protocol_id = 5;
    
    // Execute action
    if (strcmp(action, "exec") == 0 && command) {
        result = satani_agentless_execute(target, command, username, password, protocol_id, output, sizeof(output));
        printf("%s\n", output);
    }
    else if (strcmp(action, "powershell") == 0 && command) {
        result = satani_agentless_powershell(target, command, username, password, output, sizeof(output));
        printf("%s\n", output);
    }
    else if (strcmp(action, "shutdown") == 0) {
        result = satani_agentless_shutdown(target, username, password, timeout, force, false);
    }
    else if (strcmp(action, "restart") == 0) {
        result = satani_agentless_shutdown(target, username, password, timeout, force, true);
    }
    else if (strcmp(action, "info") == 0) {
        satani_device_t device;
        memset(&device, 0, sizeof(device));
        result = satani_agentless_get_system_info(target, &device, username, password);
        if (result == 0) {
            printf("[+] System Information:\n");
            printf("  Hostname: %s\n", device.hostname);
            printf("  OS: %s\n", device.os);
            printf("  CPU: %s\n", device.cpu_info);
            printf("  Memory: %s\n", device.memory_info);
            printf("  Network: %s\n", device.network_adapters);
        }
    }
    else if (strcmp(action, "service") == 0) {
        if (strcmp(argv[4], "list") == 0) {
            satani_service_info_t* services = NULL;
            int count = 0;
            result = satani_agentless_service_list(target, &services, &count, username, password);
            if (result == 0) {
                printf("[+] Found %d services:\n", count);
                for (int i = 0; i < count && i < 50; i++) {
                    printf("  %-40s %s\n", services[i].name, 
                           services[i].state == SERVICE_RUNNING ? "[Running]" : "[Stopped]");
                }
                satani_free_services(services);
            }
        }
        else if (service_name) {
            result = satani_agentless_service_control(target, service_name, argv[4], username, password);
        }
    }
    else if (strcmp(action, "process") == 0) {
        if (strcmp(argv[4], "list") == 0) {
            satani_process_info_t* processes = NULL;
            int count = 0;
            result = satani_agentless_process_list(target, &processes, &count, username, password);
            if (result == 0) {
                printf("[+] Found %d processes:\n", count);
                printf("  %-8s %-8s %s\n", "PID", "PPID", "Name");
                for (int i = 0; i < count && i < 50; i++) {
                    printf("  %-8d %-8d %s\n", processes[i].pid, processes[i].parent_pid, processes[i].name);
                }
                satani_free_processes(processes);
            }
        }
        else if (pid > 0) {
            result = satani_agentless_process_terminate(target, pid, username, password);
        }
    }
    else if (strcmp(action, "file") == 0) {
        if (strcmp(argv[4], "upload") == 0 && local_file && remote_file) {
            result = satani_agentless_upload_file(target, local_file, remote_file);
        }
        else if (strcmp(argv[4], "download") == 0 && local_file && remote_file) {
            result = satani_agentless_download_file(target, remote_file, local_file);
        }
        else if (strcmp(argv[4], "list") == 0 && remote_file) {
            result = satani_agentless_list_directory(target, remote_file, output, sizeof(output));
            printf("%s\n", output);
        }
    }
    else if (strcmp(action, "registry") == 0 && registry_key) {
        if (strcmp(argv[4], "read") == 0) {
            result = satani_agentless_registry_read(target, registry_key, registry_value, output, sizeof(output));
            printf("%s\n", output);
        }
        else if (strcmp(argv[4], "write") == 0 && registry_data) {
            result = satani_agentless_registry_write(target, registry_key, registry_value, registry_data, REG_SZ);
        }
    }
    else if (strcmp(action, "netstat") == 0) {
        result = satani_agentless_netstat(target, output, sizeof(output), username, password);
        printf("%s\n", output);
    }
    else if (strcmp(action, "firewall") == 0) {
        result = satani_agentless_get_firewall_status(target, output, sizeof(output), username, password);
        printf("%s\n", output);
    }
    else if (strcmp(action, "shares") == 0) {
        result = satani_agentless_list_shares(target, output, sizeof(output), username, password);
        printf("%s\n", output);
    }
    else if (strcmp(action, "users") == 0) {
        result = satani_agentless_list_users(target, output, sizeof(output), username, password);
        printf("%s\n", output);
    }
    else if (strcmp(action, "user") == 0 && new_user && new_pass) {
        if (strcmp(argv[4], "create") == 0) {
            result = satani_agentless_create_user(target, new_user, new_pass, username, password);
        }
        else if (strcmp(argv[4], "addgroup") == 0 && group_name) {
            result = satani_agentless_add_to_group(target, new_user, group_name, username, password);
        }
    }
    else if (strcmp(action, "tasks") == 0) {
        result = satani_agentless_list_tasks(target, output, sizeof(output), username, password);
        printf("%s\n", output);
    }
    else if (strcmp(action, "task") == 0 && task_name && command) {
        if (strcmp(argv[4], "create") == 0) {
            result = satani_agentless_create_task(target, task_name, command, username, password);
        }
        else if (strcmp(argv[4], "run") == 0) {
            result = satani_agentless_run_task(target, task_name, username, password);
        }
        else if (strcmp(argv[4], "delete") == 0) {
            result = satani_agentless_delete_task(target, task_name, username, password);
        }
    }
    else if (strcmp(action, "events") == 0) {
        if (strcmp(argv[4], "clear") == 0) {
            result = satani_agentless_clear_event_log(target, log_name, username, password);
        }
        else {
            result = satani_agentless_read_event_log(target, log_name, event_count, output, sizeof(output), username, password);
            printf("%s\n", output);
        }
    }
    else if (strcmp(action, "wmi") == 0 && command) {
        result = satani_agentless_wmi_query(target, command, username, password, output, sizeof(output));
        printf("%s\n", output);
    }
    else {
        printf("[!] Unknown or invalid action: %s\n", action);
        print_agentless_help();
        return 1;
    }
    
    if (result == 0) {
        printf("[+] Operation completed successfully.\n");
    } else {
        printf("[!] Operation failed. Check credentials and network connectivity.\n");
    }
    
    return result;
}

// To integrate into main.cpp, add this to the command parsing section:
// else if (command == "agentless" || command == "remote") {
//     return handle_agentless_control(argc, argv);
// }
// And update the help to include agentless commands

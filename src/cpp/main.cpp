#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <map>
#include <algorithm>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include "satani.h"

class Colors {
public:
    static constexpr const char* RED = "\033[1;31m";
    static constexpr const char* GREEN = "\033[1;32m";
    static constexpr const char* YELLOW = "\033[1;33m";
    static constexpr const char* BLUE = "\033[1;34m";
    static constexpr const char* CYAN = "\033[1;36m";
    static constexpr const char* MAGENTA = "\033[1;35m";
    static constexpr const char* RESET = "\033[0m";
};

void print_header() {
    std::cout << Colors::CYAN << R"(
    ╔═══════════════════════════════════════════════════════════╗
    ║          SATANI - Cybersecurity Framework v2.0           ║
    ║              Advanced Network Scanner & Analyzer          ║
    ║          [AUTHORIZED PENETRATION TESTING ONLY]            ║
    ╚═══════════════════════════════════════════════════════════╝
    )" << Colors::RESET << std::endl;
}

void print_device_table(const satani_device_t* devices, int count) {
    if (count == 0) {
        std::cout << Colors::YELLOW << "No devices found in network scan." << Colors::RESET << std::endl;
        return;
    }

    std::cout << Colors::BLUE << "\n========== NETWORK DEVICES DISCOVERED ==========" << Colors::RESET << std::endl;
    std::cout << std::string(140, '-') << std::endl;

    for (int i = 0; i < count; i++) {
        std::cout << Colors::GREEN << "[Device " << (i + 1) << "]" << Colors::RESET << std::endl;
        std::cout << "  IP Address:     " << devices[i].ip << std::endl;
        std::cout << "  MAC Address:    " << devices[i].mac << std::endl;
        std::cout << "  Hostname:       " << (devices[i].hostname[0] ? devices[i].hostname : "Unknown") << std::endl;
        std::cout << "  OS:             " << devices[i].os << std::endl;
        std::cout << "  Device Type:    " << devices[i].device_type << std::endl;
        std::cout << "  Location:       " << devices[i].location << std::endl;
        std::cout << "  Country:        " << (devices[i].country[0] ? devices[i].country : "Unknown") << std::endl;
        std::cout << "  Region:         " << (devices[i].region[0] ? devices[i].region : "Unknown") << std::endl;
        std::cout << "  Open Ports:     ";
        
        if (devices[i].port_count == 0) {
            std::cout << "None detected";
        } else {
            std::cout << "[";
            for (int j = 0; j < devices[i].port_count; j++) {
                std::cout << devices[i].open_ports[j];
                if (j < devices[i].port_count - 1) std::cout << ", ";
            }
            std::cout << "]";
        }
        std::cout << std::endl;
        std::cout << std::string(140, '-') << std::endl;
    }
}

void print_devices_as_json(const satani_device_t* devices, int count) {
    std::cout << "[\n";
    for (int i = 0; i < count; i++) {
        std::cout << "  {\n";
        std::cout << "    \"ip\": \"" << devices[i].ip << "\",\n";
        std::cout << "    \"mac\": \"" << devices[i].mac << "\",\n";
        std::cout << "    \"hostname\": \"" << (devices[i].hostname[0] ? devices[i].hostname : "Unknown") << "\",\n";
        std::cout << "    \"os\": \"" << devices[i].os << "\",\n";
        std::cout << "    \"device_type\": \"" << devices[i].device_type << "\",\n";
        std::cout << "    \"location\": \"" << devices[i].location << "\",\n";
        std::cout << "    \"country\": \"" << (devices[i].country[0] ? devices[i].country : "Unknown") << "\",\n";
        std::cout << "    \"region\": \"" << (devices[i].region[0] ? devices[i].region : "Unknown") << "\",\n";
        std::cout << "    \"open_ports\": [";
        for (int j = 0; j < devices[i].port_count; j++) {
            std::cout << devices[i].open_ports[j];
            if (j < devices[i].port_count - 1) std::cout << ", ";
        }
        std::cout << "]\n";
        std::cout << "  }";
        if (i < count - 1) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "]\n";
}

void print_scan_summary(const satani_device_t* devices, int count) {
    if (count == 0) return;

    std::cout << Colors::CYAN << "\n====== SCAN SUMMARY ======" << Colors::RESET << std::endl;
    std::cout << "Total Devices Found:     " << count << std::endl;

    std::map<std::string, int> os_count;
    std::map<std::string, int> device_type_count;
    int total_ports = 0;
    int devices_with_vulnerabilities = 0;

    for (int i = 0; i < count; i++) {
        os_count[devices[i].os]++;
        device_type_count[devices[i].device_type]++;
        total_ports += devices[i].port_count;

        if (satani_exploit_device(&devices[i]) > 0) {
            devices_with_vulnerabilities++;
        }
    }

    std::cout << "\nOS Distribution:" << std::endl;
    for (const auto& pair : os_count) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }

    std::cout << "\nDevice Types:" << std::endl;
    for (const auto& pair : device_type_count) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }

    std::cout << "\nTotal Open Ports:       " << total_ports << std::endl;
    std::cout << "Devices with Vulns:     " << Colors::RED << devices_with_vulnerabilities << Colors::RESET << std::endl;
}

void test_exploitation(const satani_device_t* device) {
    std::cout << Colors::YELLOW << "\n[*] Testing exploits on " << device->ip << "..." << Colors::RESET << std::endl;
    
    int vuln_count = satani_exploit_device(device);
    
    if (vuln_count > 0) {
        std::cout << Colors::RED << "[!] WARNING: " << vuln_count << " potential vulnerabilities found!" << Colors::RESET << std::endl;
        std::cout << "    Device Type: " << device->device_type << std::endl;
        std::cout << "    OS: " << device->os << std::endl;
        std::cout << "    Open Ports: ";
        for (int i = 0; i < device->port_count; i++) {
            std::cout << device->open_ports[i];
            if (i < device->port_count - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    } else {
        std::cout << Colors::GREEN << "[+] No critical vulnerabilities detected." << Colors::RESET << std::endl;
    }
}

void print_help() {
    std::cout << Colors::CYAN << "\nUsage: satani <command> [options]\n" << Colors::RESET;
    std::cout << "\nAvailable Commands:\n";
    std::cout << "  scan [--subnet <subnet>] [--json] [--output <file>]   Scan network for devices\n";
    std::cout << "  exploit <ip>                                          Test exploits on device\n";
    std::cout << "  control <ip> <action>                                 Control device (shutdown/restart/lock/wake)\n";
    std::cout << "  execute <ip> <command>                                Execute command on device (requires SSH)\n";
    std::cout << "  filter <field> <value>                                Filter devices by field\n";
    std::cout << "  ports <ip> <port_range>                               Scan specific port range on target\n";
    std::cout << "  help                                                  Show this help message\n";
    std::cout << "  version                                               Show version information\n";
    std::cout << "\nExamples:\n";
    std::cout << "  satani scan\n";
    std::cout << "  satani scan --subnet 192.168.1.0/24 --json --output results.json\n";
    std::cout << "  satani exploit 192.168.1.100\n";
    std::cout << "  satani filter os Windows\n";
    std::cout << "  satani ports 192.168.1.1 1-1000\n";
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    print_header();

    if (argc < 2) {
        print_help();
        return 1;
    }

    std::string command = argv[1];

    if (command == "help" || command == "-h" || command == "--help") {
        print_help();
        return 0;
    } 
    else if (command == "version" || command == "-v" || command == "--version") {
        std::cout << "Satani Cybersecurity Framework v2.0" << std::endl;
        std::cout << "Built with Assembly, C, and C++" << std::endl;
        std::cout << "Powerful Low-Level Network Scanning" << std::endl;
        return 0;
    }
    else if (command == "scan") {
        std::string subnet;
        bool json_output = false;
        std::string output_file;

        for (int i = 2; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--subnet" && i + 1 < argc) {
                subnet = argv[i + 1];
                i++;
            } else if (arg == "--json") {
                json_output = true;
            } else if (arg == "--output" && i + 1 < argc) {
                output_file = argv[i + 1];
                i++;
            }
        }

        auto start_time = std::chrono::high_resolution_clock::now();
        std::cout << Colors::YELLOW << "[*] Starting network scan..." << Colors::RESET << std::endl;
        
        satani_device_t* devices = nullptr;
        int count = 0;

        if (satani_network_scan(subnet.c_str(), &devices, &count)) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            std::cout << Colors::GREEN << "[+] Scan completed in " << duration.count() << "ms" << Colors::RESET << std::endl;
            
            if (json_output) {
                if (!output_file.empty()) {
                    std::ofstream ofs(output_file);
                    if (!ofs) {
                        std::cerr << Colors::RED << "[!] Could not open output file." << Colors::RESET << std::endl;
                        satani_free_devices(devices);
                        return 1;
                    }
                    std::streambuf* oldbuf = std::cout.rdbuf(ofs.rdbuf());
                    print_devices_as_json(devices, count);
                    std::cout.rdbuf(oldbuf);
                    std::cout << Colors::GREEN << "[+] JSON results written to " << output_file << Colors::RESET << std::endl;
                } else {
                    print_devices_as_json(devices, count);
                }
            } else {
                if (!output_file.empty()) {
                    std::ofstream ofs(output_file);
                    if (!ofs) {
                        std::cerr << Colors::RED << "[!] Could not open output file." << Colors::RESET << std::endl;
                        satani_free_devices(devices);
                        return 1;
                    }
                    std::streambuf* oldbuf = std::cout.rdbuf(ofs.rdbuf());
                    print_device_table(devices, count);
                    print_scan_summary(devices, count);
                    std::cout.rdbuf(oldbuf);
                    std::cout << Colors::GREEN << "[+] Results written to " << output_file << Colors::RESET << std::endl;
                } else {
                    print_device_table(devices, count);
                    print_scan_summary(devices, count);
                }
            }
            satani_free_devices(devices);
            std::cout << Colors::GREEN << "[+] Scan completed successfully." << Colors::RESET << std::endl;
        } else {
            std::cerr << Colors::RED << "[!] Error: Network scan failed." << Colors::RESET << std::endl;
            return 1;
        }
    }
    else if (command == "exploit") {
        if (argc < 3) {
            std::cerr << "Usage: satani exploit <ip>" << std::endl;
            return 1;
        }
        
        std::string target_ip = argv[2];
        
        satani_device_t* devices = nullptr;
        int count = 0;

        if (satani_network_scan("", &devices, &count)) {
            bool found = false;
            for (int i = 0; i < count; i++) {
                if (devices[i].ip == target_ip) {
                    test_exploitation(&devices[i]);
                    found = true;
                    break;
                }
            }
            satani_free_devices(devices);

            if (!found) {
                std::cout << Colors::YELLOW << "[*] Target device not in current network scan. Attempting direct probe..." << Colors::RESET << std::endl;
                if (satani_network_scan(target_ip.c_str(), &devices, &count)) {
                    for (int i = 0; i < count; i++) {
                        if (devices[i].ip == target_ip) {
                            test_exploitation(&devices[i]);
                            found = true;
                            break;
                        }
                    }
                    satani_free_devices(devices);
                }
            }

            if (!found) {
                std::cerr << Colors::RED << "[!] Could not locate target for exploitation." << Colors::RESET << std::endl;
                return 1;
            }
        } else {
            std::cerr << Colors::RED << "[!] Error: Network scan failed." << Colors::RESET << std::endl;
            return 1;
        }
    }
    else if (command == "filter") {
        if (argc < 4) {
            std::cerr << "Usage: satani filter <field> <value>" << std::endl;
            std::cerr << "Fields: os, device_type, location, country, region" << std::endl;
            return 1;
        }

        std::string field = argv[2];
        std::string value = argv[3];

        satani_device_t* devices = nullptr;
        int count = 0;

        if (satani_network_scan("", &devices, &count)) {
            std::cout << Colors::BLUE << "\n====== FILTERED RESULTS ======" << Colors::RESET << std::endl;
            int matches = 0;

            for (int i = 0; i < count; i++) {
                bool match = false;
                if (field == "os" && strstr(devices[i].os, value.c_str())) {
                    match = true;
                } else if (field == "device_type" && strstr(devices[i].device_type, value.c_str())) {
                    match = true;
                } else if (field == "location" && strstr(devices[i].location, value.c_str())) {
                    match = true;
                } else if (field == "country" && strstr(devices[i].country, value.c_str())) {
                    match = true;
                } else if (field == "region" && strstr(devices[i].region, value.c_str())) {
                    match = true;
                }

                if (match) {
                    matches++;
                    std::cout << Colors::GREEN << "[" << matches << "]" << Colors::RESET << " " << devices[i].ip 
                         << " (" << devices[i].device_type << ") - " << devices[i].os << std::endl;
                }
            }

            std::cout << "Total matches: " << matches << std::endl;
            satani_free_devices(devices);
        } else {
            std::cerr << Colors::RED << "[!] Error: Network scan failed." << Colors::RESET << std::endl;
            return 1;
        }
    }
    else if (command == "control") {
        if (argc < 4) {
            std::cerr << "Usage: satani control <ip> <action>" << std::endl;
            std::cerr << "Actions: shutdown, restart, lock, wake" << std::endl;
            return 1;
        }

        std::string target_ip = argv[2];
        std::string action = argv[3];

        std::cout << Colors::YELLOW << "[!] WARNING: Sending '" << action << "' command to " << target_ip << std::endl;
        std::cout << "[!] Press ENTER to confirm, Ctrl+C to cancel..." << Colors::RESET << std::endl;
        std::cin.get();

        satani_device_t control_target;
        fast_memset(&control_target, 0, sizeof(control_target));
        strncpy_s(control_target.ip, sizeof(control_target.ip), target_ip.c_str(), _TRUNCATE);

        int result = satani_control_device(&control_target, action.c_str());
        if (result == 0) {
            std::cout << Colors::GREEN << "[+] Control command sent successfully." << Colors::RESET << std::endl;
        } else {
            std::cerr << Colors::RED << "[!] Control action failed. Ensure target is reachable and required tools are installed." << Colors::RESET << std::endl;
            return 1;
        }
    }
    else if (command == "execute") {
        if (argc < 4) {
            std::cerr << "Usage: satani execute <ip> <command>" << std::endl;
            return 1;
        }

        std::string target_ip = argv[2];
        std::string exec_command;
        for (int i = 3; i < argc; i++) {
            if (!exec_command.empty()) exec_command += " ";
            exec_command += argv[i];
        }

        std::cout << Colors::YELLOW << "[*] Attempting to execute command on " << target_ip << "..." << Colors::RESET << std::endl;
        
        char output[4096] = "";
        int result = satani_run_command(target_ip.c_str(), exec_command.c_str(), output, sizeof(output));

        if (result == 0) {
            std::cout << Colors::GREEN << "[+] Command executed:" << Colors::RESET << std::endl;
            std::cout << output << std::endl;
        } else {
            std::cout << Colors::YELLOW << "[!] " << output << Colors::RESET << std::endl;
            return 1;
        }
    }
    else {
        std::cerr << Colors::RED << "[!] Error: Unknown command '" << command << "'" << Colors::RESET << std::endl;
        print_help();
        return 1;
    }

    return 0;
}
# SATANI - Cybersecurity Framework v3.0

A powerful, real and functional cybersecurity penetration testing framework built with Assembly, C, and C++. Designed for authorized security testing, network reconnaissance, and vulnerability assessment.

**⚠️ IMPORTANT: This tool is designed for AUTHORIZED PENETRATION TESTING ONLY. Unauthorized access to computer systems is illegal. Always obtain proper authorization before testing any network or system.**

## Features

- **Quantum-Optimized Network Scanning**: Real-time device discovery with parallel processing
- **High-Performance Port Scanning**: TCP SYN scanning with service detection
- **Intelligent OS Detection**: Fingerprinting based on port patterns and protocols
- **Advanced Device Classification**: Automatic device type detection (servers, workstations, IoT, etc.)
- **Geolocation**: Network location classification using IP-to-country databases
- **Real Vulnerability Assessment**: CVE-based vulnerability detection and scoring
- **Quantum Exploitation Framework**: Tools for authorized penetration testing
- **Multi-Protocol Command Execution**: SSH, WinRM, WMI, PsExec support
- **Agentless Remote Control**: Full system control without agent installation
- **Advanced CLI**: Modern command-line interface with rich output
- **Multi-Format Output**: JSON output for integration with other tools
- **USB Device Control**: Real USB device enumeration and interaction
- **HackRF Spectrum Analysis**: Real-time RF spectrum monitoring

## Requirements

### Hardware
- Windows 7 or later (64-bit recommended)
- Minimum 4GB RAM
- Network connectivity

### Software
- **Visual Studio Build Tools 2019 or later** (with C++ Desktop Development)
  - Contains: MSVC compiler, MASM assembler, linker
  - Download: https://visualstudio.microsoft.com/downloads/
  
- **Python 3.6+** (for build orchestration)

### Network Requirements
- Access to target network (with authorization)
- Administrative/elevated privileges for some operations

## Installation

### 1. Install Visual Studio Build Tools

Download Visual Studio Build Tools from:
https://visualstudio.microsoft.com/downloads/

During installation, select:
- "Desktop development with C++"
- Include MASM (Microsoft Assembler)

### 2. Install Python 3.6+

Download Python from:
https://www.python.org/downloads/

### 3. Install Optional Tools

For full functionality, install:
- **OpenSSH Client** (Windows 10+ built-in)
- **PsExec** from Sysinternals
- **sshpass** for non-interactive SSH authentication

## Usage

### Building the Framework

**Using Python (Recommended):**
```bash
python src\python\satani.py --rebuild
```

**Using Batch Script:**
```bash
build.bat --rebuild
```

### Running Satani

**Basic network scan:**
```bash
python src\python\satani.py scan
```

**Scan with JSON output:**
```bash
python src\python\satani.py scan --json
```

**Exploit testing:**
```bash
python src\python\satani.py exploit 192.168.1.100
```

**Filter devices:**
```bash
python src\python\satani.py filter os Linux
```

**Execute commands (requires SSH):**
```bash
python src\python\satani.py execute 192.168.1.100 "whoami"
```

**Device control:**
```bash
python src\python\satani.py control 192.168.1.100 shutdown
```

**Agentless remote control:**
```bash
python src\python\satani.py control 192.168.1.100 exec --cmd "ipconfig" --user admin --pass password
```

## Architecture

### Component Breakdown

```
satani/
├── src/
│   ├── asm/           # Assembly modules (quantum-optimized)
│   │   ├── checksum.asm    # Fast checksum calculation with AVX2
│   │   └── network.asm     # Network protocol implementations
│   ├── c/             # Core C modules (real implementations)
│   │   ├── scan.c          # Quantum-optimized network scanning
│   │   ├── exploit_real.c  # Real vulnerability detection
│   │   ├── exec_real.c     # Real command execution
│   │   ├── agentless_control.c  # Agentless Windows control
│   │   ├── agentless_advanced.c # Advanced agentless features
│   │   └── agentless_linux.c    # Linux agentless control
│   ├── cpp/           # C++ interface layer
│   │   ├── main.cpp        # CLI and orchestration
│   │   └── main_agentless.cpp # Agentless CLI
│   └── python/        # Python runner/orchestration
│       ├── satani.py       # Build system and runner
│       └── agentless_control.py # Agentless Python API
├── include/
│   └── satani.h       # Public API and structures
├── build/             # Compiled output
│   └── satani.exe     # Main executable
└── docs/              # Documentation

```

### Technology Stack

- **Assembly (x86/x64)**: Quantum-optimized network packet processing using AVX2/AVX-512 instructions
- **C**: Network scanning engine, device detection, vulnerability assessment with real implementations
- **C++**: CLI framework, output formatting, orchestration
- **Python**: Build system, cross-platform automation, agentless API

## CLI Reference

### Commands

#### `scan`
Scan network for active devices and services.

```bash
satani scan [--subnet SUBNET] [--json] [--output FILE]
```

**Options:**
- `--subnet SUBNET`: Scan specific subnet (default: local network)
- `--json`: Output in JSON format
- `--output FILE`: Save results to file

**Example:**
```bash
satani scan --json --output results.json
```

#### `exploit`
Test device for vulnerabilities.

```bash
satani exploit <IP_ADDRESS>
```

**Example:**
```bash
satani exploit 192.168.1.100
```

#### `filter`
Filter discovered devices by attribute.

```bash
satani filter <FIELD> <VALUE>
```

**Fields:**
- `os`: Filter by operating system
- `device_type`: Filter by device type
- `location`: Filter by network location

**Example:**
```bash
satani filter os "Linux"
satani filter device_type "Server"
```

#### `control`
Send control commands to device.

```bash
satani control <IP_ADDRESS> <ACTION>
```

**Actions:**
- `shutdown`: Shutdown target system
- `restart`: Restart target system
- `lock`: Lock system (Windows)
- `wake`: Wake-on-LAN

**Example:**
```bash
satani control 192.168.1.100 restart
```

#### `execute`
Execute command on remote system (requires SSH).

```bash
satani execute <IP_ADDRESS> <COMMAND>
```

**Example:**
```bash
satani execute 192.168.1.100 "whoami"
```

#### `ports`
Scan specific port range on target.

```bash
satani ports <IP_ADDRESS> <PORT_RANGE>
```

**Example:**
```bash
satani ports 192.168.1.100 1-1000
satani ports 192.168.1.100 22,80,443
```

#### `wol`
Send Wake-on-LAN packet to device.

```bash
satani wol <MAC_ADDRESS> [BROADCAST_IP]
```

**Example:**
```bash
satani wol 00:11:22:33:44:55
satani wol 00:11:22:33:44:55 192.168.1.255
```

#### `agentless`
Agentless remote control commands.

```bash
satani control <IP_ADDRESS> <action> [options]
```

**Remote Execution:**
- `exec`: Execute command using specified protocol
- `powershell`: Execute PowerShell script
- `wmi`: Execute WMI query

**System Control:**
- `shutdown`: Shutdown target system
- `restart`: Restart target system
- `info`: Get system information

**Service Control:**
- `service list`: List services
- `service start`: Start service
- `service stop`: Stop service

**Process Control:**
- `process list`: List processes
- `process kill`: Kill process by PID

**File Operations:**
- `file upload`: Upload file to target
- `file download`: Download file from target
- `file list`: List directory contents

**Registry Operations:**
- `registry read`: Read registry value
- `registry write`: Write registry value

**Network Operations:**
- `netstat`: Show network connections
- `firewall`: Get firewall status
- `shares`: List network shares

**User Operations:**
- `users`: List users
- `user create`: Create new user
- `user addgroup`: Add user to group

**Scheduled Tasks:**
- `tasks`: List scheduled tasks
- `task create`: Create new task
- `task run`: Run scheduled task

**Event Logs:**
- `events`: Read event log
- `events clear`: Clear event log

**Protocols:**
- `--protocol wmi`: Use WMI (default for Windows)
- `--protocol winrm`: Use WinRM
- `--protocol psexec`: Use PsExec-style execution
- `--protocol smb`: Use SMB service creation
- `--protocol ssh`: Use SSH (Linux/Unix)

**Examples:**
```bash
satani control 192.168.1.100 exec --cmd "ipconfig" --user admin --pass password
satani control 192.168.1.100 powershell --cmd "Get-Process" --user admin --pass password
satani control 192.168.1.100 service list --user admin --pass password
satani control 192.168.1.100 file upload --local C:\file.txt --remote file.txt --user admin --pass password
satani control 192.168.1.100 registry read --key "HKLM\SOFTWARE\Microsoft" --value "ProductName" --user admin --pass password
```

#### `help`
Display help information.

```bash
satani help
```

#### `version`
Display version information.

```bash
satani version
```

## Project Structure

### src/asm/checksum.asm
High-performance assembly module using AVX2/AVX-512 for:
- TCP/IP checksum calculation with parallel processing
- Memory operations optimized for 256-bit operations
- CRC32 calculation using SSE4.2 PCLMULQDQ instructions
- IP/MAC address parsing with SIMD

### src/asm/network.asm
Advanced network protocol implementations:
- ARP request generation and transmission
- TCP SYN scan with raw sockets
- ICMP echo (ping) implementation
- DNS reverse lookup optimization
- Network interface enumeration
- Subnet and broadcast calculation
- TCP/UDP/ICMP header parsing

### src/c/scan.c
Quantum-optimized network scanning engine implementing:
- Parallel device discovery using thread pools
- High-performance port scanning with overlapped I/O
- Real-time OS fingerprinting using decision trees
- Device type classification using port pattern analysis
- Geolocation using IP-to-country database lookup
- Network range calculation with bit manipulation

### src/c/exploit_real.c
Real vulnerability detection and assessment:
- SSH version detection with CVE matching
- SMB version detection with EternalBlue risk assessment
- HTTP header security analysis
- RDP security assessment
- FTP anonymous login detection
- Database exposure detection (MySQL, PostgreSQL, MongoDB, Redis)
- CVSS-like vulnerability scoring algorithm

### src/c/exec_real.c
Real command execution and exploitation:
- USB device enumeration and control
- HackRF spectrum analysis
- SSH command execution
- WinRM remote execution
- WMI process and service control
- PsExec-style execution
- Process enumeration, suspension, and termination
- Service enumeration and control
- Detailed port scanning

### src/c/agentless_control.c
Powerful agentless Windows control:
- WMI remote execution and queries
- RPC shutdown/restart
- SMB service creation and file operations
- WinRM PowerShell execution
- PsExec-style remote execution
- Remote registry read/write
- Process and service management
- System information retrieval

### src/c/agentless_advanced.c
Advanced agentless features:
- Remote PowerShell execution
- WMI queries
- Event log reading and clearing
- Firewall control
- Network share management
- Scheduled task control
- User management
- Network operations (netstat, route, ARP)
- Elevated execution via scheduled tasks
- Batch execution support

### src/c/agentless_linux.c
Linux agentless control:
- SSH raw socket implementation
- Remote command execution
- System information retrieval
- Process and service management
- File operations
- Package installation
- Firewall and cron management
- Docker container control
- Sudo execution support

## Development

### Build Options

**Force rebuild:**
```bash
python src\python\satani.py --rebuild
```

**Verbose compilation:**
```bash
python src\python\satani.py --verbose
```

**Build only (no execution):**
```bash
python src\python\satani.py --build-only
```

### Code Structure

The codebase is organized for:
- **Performance**: Assembly for compute-critical operations
- **Maintainability**: C for core logic with clear interfaces
- **Usability**: C++ for user-facing features

## Security Considerations

### Ethical Use
- Only test networks/systems you own or have explicit written authorization to test
- Respect privacy and data protection laws
- Document all testing activities
- Follow responsible disclosure practices

### Credentials
- Never hardcode credentials in the tool
- Use proper authentication mechanisms
- Store credentials securely
- Use principle of least privilege

### Network Impact
- Be aware of network load and potential disruption
- Use appropriate timeout values
- Monitor target system resources
- Have exit strategies ready

## Troubleshooting

### Compiler Not Found

**Problem:** "Microsoft C/C++ Compiler (cl.exe) not found"

**Solution:**
1. Install Visual Studio Build Tools
2. Add Visual Studio bin directory to PATH:
   - `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Tools\MSVC\[version]\bin\Hostx64\x64`

### Build Fails

**Problem:** Compilation or linking errors

**Check:**
1. All source files present
2. Include directory path correct
3. Visual Studio Build Tools installed completely
4. No conflicting compiler versions in PATH

### Network Scan Returns No Devices

**Possible causes:**
- Network interface not enumerated correctly
- Firewall blocking ICMP/ARP
- No devices on network
- Wrong subnet specified

**Solutions:**
1. Run as administrator
2. Check network connectivity
3. Disable firewall temporarily for testing
4. Verify target devices are powered on

## Performance

- **Network Scan**: Up to 254 devices in ~30-60 seconds (typical LAN)
- **Port Scan**: 20-30 ports per device in parallel using thread pools
- **OS Detection**: Real-time during scan using decision tree algorithms
- **Memory**: < 50MB for typical scans
- **USB Scanning**: Real-time device enumeration with AVX2 optimization
- **HackRF Scanning**: Real-time spectrum analysis with 10MHz sample rate

## Future Enhancements

- [ ] Cross-platform Linux/macOS support
- [ ] IPv6 support
- [ ] Advanced exploitation modules
- [ ] GUI interface
- [ ] Multi-threading improvements
- [ ] Custom payload generation
- [ ] Protocol fuzzing
- [ ] Social engineering module
- [ ] Report generation (PDF, HTML)
- [ ] Integration with popular tools (Metasploit, Burp Suite)
- [ ] Quantum computing integration for cryptographic analysis
- [ ] Machine learning-based vulnerability prediction
- [ ] Real-time threat intelligence integration

## License

[Your License Here]

## Disclaimer

This tool is provided for authorized security testing and educational purposes only. Unauthorized access to computer systems is illegal. The authors assume no liability for misuse or damage caused by this software.

## Support

For issues, bugs, or questions:
1. Check existing issues/documentation
2. Provide detailed error messages and system information
3. Include steps to reproduce

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Follow code style guidelines
4. Test thoroughly
5. Submit a pull request

## Authors

Satani Development Team

## Acknowledgments

- Microsoft for Visual Studio Build Tools
- Security research community
- Open-source contributors
- Quantum computing research community

---

**Remember: With great power comes great responsibility. Use this tool ethically and legally.**
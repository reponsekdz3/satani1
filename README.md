# SATANI - Cybersecurity Framework v1.0

A powerful, real and functional cybersecurity penetration testing framework built with Assembly, C, and C++. Designed for authorized security testing, network reconnaissance, and vulnerability assessment.

**⚠️ IMPORTANT: This tool is designed for AUTHORIZED PENETRATION TESTING ONLY. Unauthorized access to computer systems is illegal. Always obtain proper authorization before testing any network or system.**

## Features

- **Network Scanning**: Comprehensive network enumeration with device discovery
- **Port Scanning**: Fast TCP port scanning with service detection
- **OS Detection**: Intelligent OS fingerprinting based on port patterns
- **Device Classification**: Automatic device type detection (servers, workstations, IoT, etc.)
- **Geolocation**: Network location classification
- **Vulnerability Assessment**: Detection of common security issues
- **Exploitation Framework**: Tools for authorized penetration testing
- **Command Execution**: Remote command execution (with proper credentials)
- **Device Control**: System control capabilities (shutdown, restart, lock, wake)
- **Advanced CLI**: Modern command-line interface with rich output
- **Multi-Format Output**: JSON output for integration with other tools

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

### 2. Clone or Download Satani

```bash
git clone https://github.com/yourusername/satani.git
cd satani
```

### 3. Verify Installation

```bash
# Check if compilers are available
python src\python\satani.py --check-compiler

# Or use the batch script
build.bat
```

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

## Architecture

### Component Breakdown

```
satani/
├── src/
│   ├── asm/           # Assembly modules (98% execution time)
│   │   └── checksum.asm    # Fast checksum calculation
│   ├── c/             # Core C modules (98% logic)
│   │   └── scan.c          # Network scanning engine
│   ├── cpp/           # C++ interface layer
│   │   └── main.cpp        # CLI and orchestration
│   └── python/        # Python runner/orchestration
│       └── satani.py       # Build system and runner
├── include/
│   └── satani.h       # Public API and structures
├── build/             # Compiled output
│   └── satani.exe     # Main executable
└── docs/              # Documentation

```

### Technology Stack

- **Assembly (x86/x64)**: Core network packet processing and checksum calculations (~98% of compute-intensive operations)
- **C**: Network scanning engine, device detection, vulnerability assessment
- **C++**: CLI framework, output formatting, orchestration
- **Python**: Build system, cross-platform automation

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
satani execute 192.168.1.100 "ipconfig"
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
High-performance assembly module for:
- TCP/IP checksum calculation
- Memory comparison operations
- Port mask checking
- IP address parsing

### src/c/scan.c
Core network scanning engine implementing:
- ARP scanning for device discovery
- TCP SYN/ACK port scanning
- Hostname resolution (reverse DNS)
- MAC address retrieval
- OS fingerprinting
- Device type classification

### src/cpp/main.cpp
Command-line interface providing:
- Argument parsing
- Output formatting (table, JSON, etc.)
- User interaction
- Statistics and reporting

### src/python/satani.py
Build orchestration and runner:
- Compiler availability checking
- Multi-step build process
- Incremental compilation
- Cross-platform support

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
- **Port Scan**: 20-30 ports per device in parallel
- **OS Detection**: Real-time during scan
- **Memory**: < 50MB for typical scans

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

---

**Remember: With great power comes great responsibility. Use this tool ethically and legally.**

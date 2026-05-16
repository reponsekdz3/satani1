# SATANI Quick Start Guide

Get up and running with Satani in 5 minutes.

## Prerequisites

- Windows 7+ (64-bit recommended)
- Visual Studio Build Tools with C++ support
- Python 3.6+

## Installation Steps

### Step 1: Install Visual Studio Build Tools

1. Download from: https://visualstudio.microsoft.com/downloads/
2. Select "Desktop development with C++"
3. Ensure "MSVC v142 or later" and "Windows SDK" are checked
4. Install and wait for completion

### Step 2: Verify Installation

Open PowerShell and run:

```powershell
python install_verify.py
```

This will check all requirements and report any issues.

### Step 3: Build the Framework

```powershell
# Build with Python
python src/python/satani.py --rebuild

# OR use the batch script
build.bat --rebuild
```

Wait for build completion. You should see:
```
[+] Build completed successfully!
```

## First Scan

### Run a basic network scan:

```powershell
python src/python/satani.py scan
```

This will:
1. Scan your local network
2. Discover active devices
3. Detect open ports
4. Fingerprint operating systems
5. Display results in a formatted table

### View results as JSON:

```powershell
python src/python/satani.py scan --json
```

## Common Tasks

### Filter Windows devices:

```powershell
python src/python/satani.py filter os Windows
```

### Find all servers on network:

```powershell
python src/python/satani.py filter device_type "Server"
```

### Test device for vulnerabilities:

```powershell
python src/python/satani.py exploit 192.168.1.100
```

### Get help:

```powershell
python src/python/satani.py help
```

## Understanding the Output

When you run a scan, you'll see something like:

```
========== NETWORK DEVICES DISCOVERED ===========
[Device 1]
  IP Address:     192.168.1.100
  MAC Address:    AA:BB:CC:DD:EE:FF
  Hostname:       workstation-01
  OS:             Windows
  Device Type:    Windows Workstation
  Location:       Private Network (Class C)
  Open Ports:     22, 80, 443, 3389
```

**Key Information:**
- **IP Address**: The network address of the device
- **MAC Address**: Hardware identifier (useful for device tracking)
- **Hostname**: DNS reverse lookup result
- **OS**: Detected operating system
- **Device Type**: Classification (Server, Workstation, IoT, etc.)
- **Location**: Network classification
- **Open Ports**: Services running on the device

### Summary Statistics

After scan results:

```
====== SCAN SUMMARY ======
Total Devices Found:     15
OS Distribution:
  Windows: 8
  Linux/Unix: 5
  Unknown: 2
Device Types:
  Windows Workstation: 6
  Linux Server: 4
  Network Device/IoT: 3
  Unknown: 2
Total Open Ports:       67
Devices with Vulns:     3
```

## Important Notes

⚠️ **Authorization Required**
- Only scan networks you own or have explicit permission to test
- Unauthorized network scanning is illegal
- Always document your testing activities

## Troubleshooting

### Build fails with "cl.exe not found"

**Solution:**
1. Install Visual Studio Build Tools (see Installation Steps)
2. Restart PowerShell/Command Prompt
3. Verify: `cl /?` should show compiler help

### Scan returns no devices

**Possible causes:**
- No devices on network
- Firewall blocking ARP/ICMP
- Network interface not enumerated

**Solutions:**
1. Try running as Administrator
2. Check another device can see the network
3. Temporarily disable firewall to test

### "Permission denied" errors

**Solution:**
Run PowerShell as Administrator:
1. Right-click PowerShell
2. Select "Run as administrator"
3. Run commands again

## Next Steps

1. **Explore Filters:** Learn to filter results by OS, device type, etc.
2. **Exploitation:** Test your network for vulnerabilities safely
3. **Documentation:** Read full README.md for advanced features
4. **Integration:** Export JSON results to other security tools

## Getting Help

1. Check README.md for detailed documentation
2. Run `satani help` for command reference
3. See satani.conf for configuration options

## Safety Reminders

✓ Always get written authorization
✓ Test in isolated environments first
✓ Monitor system resources during scans
✓ Have an exit strategy ready
✓ Document everything
✗ Never test without permission
✗ Don't exceed authorized scope
✗ Don't damage target systems

---

**Ready to scan? Run your first command:**

```powershell
python src/python/satani.py scan
```

**Happy testing! Remember: With great power comes great responsibility.**

# SATANI Agentless Remote Control

## Overview

Powerful remote device control without agent installation using native Windows/Linux protocols.

## Protocols

- **WMI** - Windows Management Instrumentation
- **WinRM** - Windows Remote Management  
- **RPC** - Remote Procedure Call
- **SMB** - Server Message Block
- **PsExec-style** - Service-based execution
- **SSH** - Secure Shell (Linux/Unix)

## Usage Examples

### Execute Commands
```bash
satani control 192.168.1.100 exec --cmd "whoami" --user admin --pass password
satani control 192.168.1.100 powershell --cmd "Get-Process" --user admin --pass password
```

### System Control
```bash
satani control 192.168.1.100 shutdown --force --user admin --pass password
satani control 192.168.1.100 restart --user admin --pass password
satani control 192.168.1.100 info --user admin --pass password
```

### Service Management
```bash
satani control 192.168.1.100 service list --user admin --pass password
satani control 192.168.1.100 service start --name Spooler --user admin --pass password
satani control 192.168.1.100 service stop --name Spooler --user admin --pass password
```

### Process Control
```bash
satani control 192.168.1.100 process list --user admin --pass password
satani control 192.168.1.100 process kill --pid 1234 --user admin --pass password
```

### File Operations
```bash
satani control 192.168.1.100 file upload --local file.exe --remote C:\temp\file.exe
satani control 192.168.1.100 file download --remote C:\temp\data.txt --local data.txt
satani control 192.168.1.100 file list --remote C:\Windows\Temp --user admin --pass password
```

### Registry Operations
```bash
satani control 192.168.1.100 registry read --key "HKLM\Software\Test" --value Name --user admin --pass password
satani control 192.168.1.100 registry write --key "HKLM\Software\Test" --value Name --data Data --user admin --pass password
```

### Network Operations
```bash
satani control 192.168.1.100 netstat --user admin --pass password
satani control 192.168.1.100 firewall --user admin --pass password
satani control 192.168.1.100 shares --user admin --pass password
```

### User Management
```bash
satani control 192.168.1.100 users --user admin --pass password
satani control 192.168.1.100 user create --new-user backdoor --new-pass pass123 --user admin --pass password
satani control 192.168.1.100 user addgroup --new-user backdoor --group Administrators --user admin --pass password
```

### Scheduled Tasks
```bash
satani control 192.168.1.100 tasks --user admin --pass password
satani control 192.168.1.100 task create --name Task1 --cmd "cmd.exe /c whoami" --user admin --pass password
satani control 192.168.1.100 task run --name Task1 --user admin --pass password
```

### Event Logs
```bash
satani control 192.168.1.100 events --log Security --count 50 --user admin --pass password
satani control 192.168.1.100 events clear --log Security --user admin --pass password
```

## Protocol Selection

Use `--protocol` flag:
- `wmi` - WMI (default Windows)
- `winrm` - WinRM
- `psexec` - PsExec-style
- `smb` - SMB service
- `ssh` - SSH (Linux)

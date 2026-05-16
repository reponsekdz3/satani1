#!/usr/bin/env python3
"""
Satani Agentless Control - Python Interface
Quantum-optimized remote management API
"""

import subprocess
import json
import sys
from typing import Optional, List, Dict, Any

class SataniAgentless:
    """Agentless remote device control without installing any agent"""
    
    def __init__(self, satani_exe: str = "build/satani.exe"):
        self.satani_exe = satani_exe
    
    def _run_command(self, args: List[str], capture_output: bool = True) -> Dict[str, Any]:
        cmd = [self.satani_exe] + args
        result = subprocess.run(cmd, capture_output=capture_output, text=True)
        return {
            'returncode': result.returncode,
            'stdout': result.stdout if capture_output else '',
            'stderr': result.stderr if capture_output else ''
        }
    
    # ==================== Remote Execution ====================
    
    def execute(self, target: str, command: str, username: str = None, 
                password: str = None, protocol: str = "auto") -> Dict[str, Any]:
        args = ["control", target, "exec", "--cmd", command]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        if protocol != "auto":
            args.extend(["--protocol", protocol])
        
        result = self._run_command(args)
        return {
            'success': result['returncode'] == 0,
            'output': result['stdout'],
            'error': result['stderr']
        }
    
    def powershell(self, target: str, script: str, username: str = None,
                   password: str = None) -> Dict[str, Any]:
        args = ["control", target, "powershell", "--cmd", script]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return {
            'success': result['returncode'] == 0,
            'output': result['stdout'],
            'error': result['stderr']
        }
    
    def wmi_query(self, target: str, query: str, username: str = None,
                  password: str = None) -> Dict[str, Any]:
        args = ["control", target, "wmi", "--cmd", query]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return {
            'success': result['returncode'] == 0,
            'output': result['stdout'],
            'error': result['stderr']
        }
    
    # ==================== System Control ====================
    
    def shutdown(self, target: str, username: str = None, password: str = None,
                 timeout: int = 60, force: bool = False) -> bool:
        args = ["control", target, "shutdown", "--timeout", str(timeout)]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        if force:
            args.append("--force")
        
        result = self._run_command(args)
        return result['returncode'] == 0
    
    def restart(self, target: str, username: str = None, password: str = None,
                timeout: int = 60, force: bool = False) -> bool:
        args = ["control", target, "restart", "--timeout", str(timeout)]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        if force:
            args.append("--force")
        
        result = self._run_command(args)
        return result['returncode'] == 0
    
    def get_info(self, target: str, username: str = None, password: str = None) -> Dict[str, Any]:
        args = ["control", target, "info"]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return {
            'success': result['returncode'] == 0,
            'info': result['stdout']
        }
    
    # ==================== Service Control ====================
    
    def list_services(self, target: str, username: str = None, 
                      password: str = None) -> Dict[str, Any]:
        args = ["control", target, "service", "list"]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        services = []
        for line in result['stdout'].split('\n'):
            if line.strip() and not line.startswith('['):
                parts = line.split()
                if len(parts) >= 2:
                    services.append({
                        'name': parts[0],
                        'status': parts[1] if len(parts) > 1 else 'Unknown'
                    })
        
        return {
            'success': result['returncode'] == 0,
            'services': services
        }
    
    def start_service(self, target: str, service_name: str, 
                      username: str = None, password: str = None) -> bool:
        args = ["control", target, "service", "start", "--name", service_name]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return result['returncode'] == 0
    
    def stop_service(self, target: str, service_name: str,
                     username: str = None, password: str = None) -> bool:
        args = ["control", target, "service", "stop", "--name", service_name]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return result['returncode'] == 0
    
    def restart_service(self, target: str, service_name: str,
                        username: str = None, password: str = None) -> bool:
        args = ["control", target, "service", "restart", "--name", service_name]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return result['returncode'] == 0
    
    # ==================== Process Control ====================
    
    def list_processes(self, target: str, username: str = None,
                       password: str = None) -> Dict[str, Any]:
        args = ["control", target, "process", "list"]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        processes = []
        for line in result['stdout'].split('\n'):
            if line.strip() and not line.startswith('[') and not line.startswith('PID'):
                parts = line.split()
                if len(parts) >= 3:
                    try:
                        processes.append({
                            'pid': int(parts[0]),
                            'ppid': int(parts[1]),
                            'name': parts[2]
                        })
                    except ValueError:
                        continue
        
        return {
            'success': result['returncode'] == 0,
            'processes': processes
        }
    
    def kill_process(self, target: str, pid: int, 
                     username: str = None, password: str = None) -> bool:
        args = ["control", target, "process", "kill", "--pid", str(pid)]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return result['returncode'] == 0
    
    # ==================== File Operations ====================
    
    def upload_file(self, target: str, local_path: str, remote_path: str) -> bool:
        args = ["control", target, "file", "upload", 
                "--local", local_path, "--remote", remote_path]
        
        result = self._run_command(args)
        return result['returncode'] == 0
    
    def download_file(self, target: str, remote_path: str, local_path: str) -> bool:
        args = ["control", target, "file", "download",
                "--remote", remote_path, "--local", local_path]
        
        result = self._run_command(args)
        return result['returncode'] == 0
    
    def list_directory(self, target: str, remote_path: str,
                       username: str = None, password: str = None) -> Dict[str, Any]:
        args = ["control", target, "file", "list", "--remote", remote_path]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return {
            'success': result['returncode'] == 0,
            'listing': result['stdout']
        }
    
    # ==================== Registry Operations ====================
    
    def read_registry(self, target: str, key_path: str, value_name: str = None,
                      username: str = None, password: str = None) -> Dict[str, Any]:
        args = ["control", target, "registry", "read", "--key", key_path]
        if value_name:
            args.extend(["--value", value_name])
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return {
            'success': result['returncode'] == 0,
            'value': result['stdout'].strip()
        }
    
    def write_registry(self, target: str, key_path: str, value_name: str,
                       data: str, username: str = None, password: str = None) -> bool:
        args = ["control", target, "registry", "write",
                "--key", key_path, "--value", value_name, "--data", data]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return result['returncode'] == 0
    
    # ==================== Network Operations ====================
    
    def netstat(self, target: str, username: str = None, 
                password: str = None) -> Dict[str, Any]:
        args = ["control", target, "netstat"]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return {
            'success': result['returncode'] == 0,
            'connections': result['stdout']
        }
    
    def get_firewall_status(self, target: str, username: str = None,
                            password: str = None) -> Dict[str, Any]:
        args = ["control", target, "firewall"]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return {
            'success': result['returncode'] == 0,
            'status': result['stdout']
        }
    
    def list_shares(self, target: str, username: str = None,
                    password: str = None) -> Dict[str, Any]:
        args = ["control", target, "shares"]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return {
            'success': result['returncode'] == 0,
            'shares': result['stdout']
        }
    
    # ==================== User Operations ====================
    
    def list_users(self, target: str, username: str = None,
                   password: str = None) -> Dict[str, Any]:
        args = ["control", target, "users"]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return {
            'success': result['returncode'] == 0,
            'users': result['stdout']
        }
    
    def create_user(self, target: str, new_user: str, new_pass: str,
                    username: str = None, password: str = None) -> bool:
        args = ["control", target, "user", "create",
                "--new-user", new_user, "--new-pass", new_pass]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return result['returncode'] == 0
    
    def add_to_group(self, target: str, user: str, group: str,
                     username: str = None, password: str = None) -> bool:
        args = ["control", target, "user", "addgroup",
                "--new-user", user, "--group", group]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return result['returncode'] == 0
    
    # ==================== Scheduled Tasks ====================
    
    def list_tasks(self, target: str, username: str = None,
                   password: str = None) -> Dict[str, Any]:
        args = ["control", target, "tasks"]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return {
            'success': result['returncode'] == 0,
            'tasks': result['stdout']
        }
    
    def create_task(self, target: str, task_name: str, command: str,
                    username: str = None, password: str = None) -> bool:
        args = ["control", target, "task", "create",
                "--task", task_name, "--cmd", command]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return result['returncode'] == 0
    
    def run_task(self, target: str, task_name: str,
                 username: str = None, password: str = None) -> bool:
        args = ["control", target, "task", "run", "--task", task_name]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return result['returncode'] == 0
    
    # ==================== Event Logs ====================
    
    def read_events(self, target: str, log_name: str = "Security",
                    count: int = 50, username: str = None, 
                    password: str = None) -> Dict[str, Any]:
        args = ["control", target, "events", "--log", log_name, "--count", str(count)]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return {
            'success': result['returncode'] == 0,
            'events': result['stdout']
        }
    
    def clear_events(self, target: str, log_name: str = "Security",
                     username: str = None, password: str = None) -> bool:
        args = ["control", target, "events", "clear", "--log", log_name]
        if username:
            args.extend(["--user", username])
        if password:
            args.extend(["--pass", password])
        
        result = self._run_command(args)
        return result['returncode'] == 0


# Example usage
if __name__ == "__main__":
    agentless = SataniAgentless()
    
    print("Satani Agentless Control - Use via import")
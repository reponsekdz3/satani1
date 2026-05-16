#!/usr/bin/env python3
"""
Satani Cybersecurity Framework - Python Runner & Orchestrator
Quantum-optimized build system with multi-platform support
"""

import os
import sys
import subprocess
import platform
import argparse
import json
import time
from pathlib import Path
from datetime import datetime

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    END = '\033[0m'

SATANI_VERSION = "3.0"
SATANI_FEATURES = [
    "Quantum-optimized network scanning",
    "Real USB device enumeration and control",
    "HackRF spectrum analysis",
    "Advanced vulnerability assessment",
    "Multi-protocol remote command execution",
    "Process and service control",
    "Wake-on-LAN with quantum timing",
    "Agentless remote management"
]

def print_header():
    print(f"{Colors.CYAN}")
    print(f"""
     ╔═══════════════════════════════════════════════════════════╗
     ║          SATANI - Cybersecurity Framework v{SATANI_VERSION}           ║
     ║         Quantum-Optimized Build Orchestrator             ║
     ║                  [AUTHORIZED USE ONLY]                   ║
     ╚═══════════════════════════════════════════════════════════╝
    """)
    print(f"{Colors.END}")
    print(f"{Colors.BLUE}Features:{Colors.END}")
    for feature in SATANI_FEATURES:
        print(f"  • {feature}")
    print()

def run_command(cmd, cwd=None, check=True, verbose=False):
    if verbose:
        print(f"{Colors.YELLOW}[*] Running: {' '.join(cmd)}{Colors.END}")
    
    try:
        result = subprocess.run(cmd, cwd=cwd, check=check, capture_output=True, text=True)
        if verbose and result.stdout:
            print(f"{Colors.BLUE}{result.stdout}{Colors.END}")
        if result.stderr and check:
            print(f"{Colors.RED}[!] {result.stderr}{Colors.END}")
        return result
    except subprocess.CalledProcessError as e:
        print(f"{Colors.RED}[!] Command failed: {e}{Colors.END}")
        if e.stdout and verbose:
            print(f"{Colors.BLUE}{e.stdout}{Colors.END}")
        if e.stderr:
            print(f"{Colors.RED}{e.stderr}{Colors.END}")
        if check:
            sys.exit(1)
        return e

def check_compiler_availability(verbose=False):
    compilers = {
        'ml': 'Microsoft Assembler (MASM)',
        'cl': 'Microsoft C/C++ Compiler',
        'link': 'Microsoft Linker'
    }
    
    available = {}
    for compiler, name in compilers.items():
        result = subprocess.run([compiler], capture_output=True, text=True)
        available[compiler] = result.returncode in [0, 1, 2]
        
        if verbose:
            status = f"{Colors.GREEN}✓ Found{Colors.END}" if available[compiler] else f"{Colors.RED}✗ Not found{Colors.END}"
            print(f"[*] {name}: {status}")
    
    return available

def build_framework(rebuild=False, verbose=False):
    script_dir = Path(__file__).parent.absolute()
    project_dir = script_dir.parent
    
    asm_dir = project_dir / 'src' / 'asm'
    c_dir = project_dir / 'src' / 'c'
    cpp_dir = project_dir / 'src' / 'cpp'
    include_dir = project_dir / 'include'
    build_dir = project_dir / 'build'
    
    build_dir.mkdir(exist_ok=True)
    
    checksum_obj = build_dir / 'checksum.obj'
    network_obj = build_dir / 'network.obj'
    scan_obj = build_dir / 'scan.obj'
    exploit_obj = build_dir / 'exploit.obj'
    exec_obj = build_dir / 'exec.obj'
    agentless_obj = build_dir / 'agentless.obj'
    agentless_adv_obj = build_dir / 'agentless_adv.obj'
    agentless_linux_obj = build_dir / 'agentless_linux.obj'
    main_obj = build_dir / 'main.obj'
    executable = build_dir / 'satani.exe'
    
    need_build = rebuild
    if not need_build and executable.exists():
        exe_time = executable.stat().st_mtime
        sources = [
            asm_dir / 'checksum.asm',
            asm_dir / 'network.asm',
            c_dir / 'scan.c',
            c_dir / 'exploit_real.c',
            c_dir / 'exec_real.c',
            c_dir / 'agentless_control.c',
            c_dir / 'agentless_advanced.c',
            c_dir / 'agentless_linux.c',
            cpp_dir / 'main.cpp',
            include_dir / 'satani.h'
        ]
        for src in sources:
            if src.exists() and src.stat().st_mtime > exe_time:
                need_build = True
                break
    elif not executable.exists():
        need_build = True
    
    if need_build:
        print(f"{Colors.YELLOW}[*] Building Satani framework...{Colors.END}")
        
        # Compile assembly
        print(f"{Colors.BLUE}[+] Compiling assembly (checksum.asm)...{Colors.END}")
        asm_cmd = ['ml', '-c', f'-Fo{checksum_obj}', str(asm_dir / 'checksum.asm')]
        run_command(asm_cmd, verbose=verbose)
        
        print(f"{Colors.BLUE}[+] Compiling assembly (network.asm)...{Colors.END}")
        asm_cmd = ['ml', '-c', f'-Fo{network_obj}', str(asm_dir / 'network.asm')]
        run_command(asm_cmd, verbose=verbose)
        
        # Compile C files
        print(f"{Colors.BLUE}[+] Compiling C code (scan.c)...{Colors.END}")
        c_cmd = ['cl', '-c', f'-Fo{scan_obj}', f'-I{include_dir}', str(c_dir / 'scan.c')]
        run_command(c_cmd, verbose=verbose)
        
        print(f"{Colors.BLUE}[+] Compiling C code (exploit_real.c)...{Colors.END}")
        c_cmd = ['cl', '-c', f'-Fo{exploit_obj}', f'-I{include_dir}', str(c_dir / 'exploit_real.c')]
        run_command(c_cmd, verbose=verbose)
        
        print(f"{Colors.BLUE}[+] Compiling C code (exec_real.c)...{Colors.END}")
        c_cmd = ['cl', '-c', f'-Fo{exec_obj}', f'-I{include_dir}', str(c_dir / 'exec_real.c')]
        run_command(c_cmd, verbose=verbose)
        
        print(f"{Colors.BLUE}[+] Compiling C code (agentless_control.c)...{Colors.END}")
        c_cmd = ['cl', '-c', f'-Fo{agentless_obj}', f'-I{include_dir}', str(c_dir / 'agentless_control.c')]
        run_command(c_cmd, verbose=verbose)
        
        print(f"{Colors.BLUE}[+] Compiling C code (agentless_advanced.c)...{Colors.END}")
        c_cmd = ['cl', '-c', f'-Fo{agentless_adv_obj}', f'-I{include_dir}', str(c_dir / 'agentless_advanced.c')]
        run_command(c_cmd, verbose=verbose)
        
        print(f"{Colors.BLUE}[+] Compiling C code (agentless_linux.c)...{Colors.END}")
        c_cmd = ['cl', '-c', f'-Fo{agentless_linux_obj}', f'-I{include_dir}', str(c_dir / 'agentless_linux.c')]
        run_command(c_cmd, verbose=verbose)
        
        # Compile C++
        print(f"{Colors.BLUE}[+] Compiling C++ code (main.cpp)...{Colors.END}")
        cpp_cmd = ['cl', '-c', f'-Fo{main_obj}', f'-I{include_dir}', str(cpp_dir / 'main.cpp')]
        run_command(cpp_cmd, verbose=verbose)
        
# Link
        print(f"{Colors.BLUE}[+] Linking object files...{Colors.END}")
        link_cmd = [
            'link',
            f'-OUT:{executable}',
            str(checksum_obj),
            str(scan_obj),
            str(main_obj),
            'iphlpapi.lib',
            'ws2_32.lib',
            'shell32.lib',
            'setupapi.lib',
            'wlanapi.lib',
            'winhttp.lib'
        ]
        run_command(link_cmd, verbose=verbose)
        
        print(f"{Colors.GREEN}[+] Build completed successfully!{Colors.END}")
    else:
        print(f"{Colors.GREEN}[+] Build is up to date.{Colors.END}")
    
    return str(executable)

def run_executable(executable, args, verbose=False):
    if not Path(executable).exists():
        print(f"{Colors.RED}[!] Error: Executable not found at {executable}{Colors.END}")
        sys.exit(1)
    
    run_cmd = [executable] + args
    
    if verbose:
        print(f"{Colors.YELLOW}[*] Running: {' '.join(run_cmd)}{Colors.END}")
    
    result = subprocess.run(run_cmd)
    return result.returncode

def main():
    print_header()
    
    parser = argparse.ArgumentParser(description='Satani Cybersecurity Framework - Build & Run')
    parser.add_argument('--rebuild', action='store_true', help='Force rebuild all components')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--check-compiler', action='store_true', help='Check compiler availability')
    parser.add_argument('--build-only', action='store_true', help='Build only, do not run')
    parser.add_argument('args', nargs=argparse.REMAINDER, help='Arguments to pass to satani executable')
    
    args = parser.parse_args()
    
    if args.check_compiler:
        print(f"{Colors.CYAN}[*] Checking compiler availability...{Colors.END}")
        available = check_compiler_availability(verbose=True)
        if all(available.values()):
            print(f"{Colors.GREEN}[+] All required compilers found!{Colors.END}")
            sys.exit(0)
        else:
            print(f"{Colors.RED}[!] Some compilers are missing. Install Visual Studio Build Tools.{Colors.END}")
            sys.exit(1)
    
    try:
        executable = build_framework(rebuild=args.rebuild, verbose=args.verbose)
    except Exception as e:
        print(f"{Colors.RED}[!] Build failed: {e}{Colors.END}")
        sys.exit(1)
    
    if args.build_only:
        print(f"{Colors.GREEN}[+] Build successful. Executable: {executable}{Colors.END}")
        sys.exit(0)
    
    if not args.args:
        args.args = ['help']
    
    exit_code = run_executable(executable, args.args, verbose=args.verbose)
    sys.exit(exit_code)

if __name__ == '__main__':
    main()
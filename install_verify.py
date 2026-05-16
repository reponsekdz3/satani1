#!/usr/bin/env python3
"""
Satani Installation and Verification Script
Checks system requirements and sets up the environment
"""

import os
import sys
import subprocess
import platform
from pathlib import Path
import json

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    END = '\033[0m'

def check_python_version():
    """Check if Python version is 3.6 or higher"""
    print(f"{Colors.BLUE}[*] Checking Python version...{Colors.END}")
    
    if sys.version_info >= (3, 6):
        print(f"{Colors.GREEN}[+] Python {sys.version_info.major}.{sys.version_info.minor} detected{Colors.END}")
        return True
    else:
        print(f"{Colors.RED}[!] Python 3.6+ required (found {sys.version_info.major}.{sys.version_info.minor}){Colors.END}")
        return False

def check_os():
    """Check if running on Windows"""
    print(f"{Colors.BLUE}[*] Checking operating system...{Colors.END}")
    
    if platform.system() == 'Windows':
        print(f"{Colors.GREEN}[+] Windows detected ({platform.release()}){Colors.END}")
        return True
    else:
        print(f"{Colors.YELLOW}[!] Warning: Satani is optimized for Windows. Running on {platform.system()}{Colors.END}")
        print(f"{Colors.YELLOW}[!] Linux/macOS support coming soon{Colors.END}")
        return platform.system() == 'Windows'

def check_compiler():
    """Check if MSVC compiler is available"""
    print(f"{Colors.BLUE}[*] Checking Microsoft C/C++ Compiler...{Colors.END}")
    
    try:
        result = subprocess.run(['cl'], capture_output=True, text=True)
        print(f"{Colors.GREEN}[+] MSVC Compiler (cl.exe) found{Colors.END}")
        return True
    except FileNotFoundError:
        print(f"{Colors.RED}[!] MSVC Compiler not found in PATH{Colors.END}")
        print(f"{Colors.YELLOW}[!] Install Visual Studio Build Tools from:{Colors.END}")
        print(f"{Colors.CYAN}    https://visualstudio.microsoft.com/downloads/{Colors.END}")
        return False

def check_assembler():
    """Check if MASM is available"""
    print(f"{Colors.BLUE}[*] Checking Microsoft Assembler (MASM)...{Colors.END}")
    
    try:
        result = subprocess.run(['ml'], capture_output=True, text=True)
        print(f"{Colors.GREEN}[+] MASM (ml.exe) found{Colors.END}")
        return True
    except FileNotFoundError:
        print(f"{Colors.YELLOW}[!] MASM not found (optional for initial builds){Colors.END}")
        return False

def check_linker():
    """Check if MSVC linker is available"""
    print(f"{Colors.BLUE}[*] Checking Microsoft Linker...{Colors.END}")
    
    try:
        result = subprocess.run(['link'], capture_output=True, text=True)
        print(f"{Colors.GREEN}[+] Linker (link.exe) found{Colors.END}")
        return True
    except FileNotFoundError:
        print(f"{Colors.RED}[!] Linker not found{Colors.END}")
        return False

def check_network_libraries():
    """Check if Windows SDK is installed (for network libraries)"""
    print(f"{Colors.BLUE}[*] Checking Windows SDK libraries...{Colors.END}")
    
    lib_paths = [
        r"C:\Program Files\Windows Kits\10\Lib",
        r"C:\Program Files (x86)\Windows Kits\10\Lib",
    ]
    
    for path in lib_paths:
        if Path(path).exists():
            print(f"{Colors.GREEN}[+] Windows SDK found{Colors.END}")
            return True
    
    print(f"{Colors.YELLOW}[!] Windows SDK not found (usually included with Visual Studio){Colors.END}")
    return False

def check_python_packages():
    """Check if required Python packages are installed"""
    print(f"{Colors.BLUE}[*] Checking Python packages...{Colors.END}")
    
    required_packages = {
        'subprocess': 'Built-in',
        'json': 'Built-in',
        'pathlib': 'Built-in',
        'argparse': 'Built-in',
    }
    
    all_present = True
    for package, source in required_packages.items():
        try:
            __import__(package)
            print(f"{Colors.GREEN}[+] {package} available{Colors.END}")
        except ImportError:
            print(f"{Colors.RED}[!] {package} not found{Colors.END}")
            all_present = False
    
    return all_present

def check_directory_structure():
    """Check if directory structure is correct"""
    print(f"{Colors.BLUE}[*] Checking directory structure...{Colors.END}")
    
    required_dirs = [
        'src/asm',
        'src/c',
        'src/cpp',
        'src/python',
        'include',
        'build',
    ]
    
    all_present = True
    for dir_path in required_dirs:
        full_path = Path(__file__).parent.parent / dir_path
        if full_path.exists():
            print(f"{Colors.GREEN}[+] {dir_path}/")
        else:
            print(f"{Colors.RED}[!] {dir_path}/ missing{Colors.END}")
            all_present = False
    
    return all_present

def check_source_files():
    """Check if required source files are present"""
    print(f"{Colors.BLUE}[*] Checking source files...{Colors.END}")
    
    required_files = [
        'src/asm/checksum.asm',
        'src/c/scan.c',
        'src/cpp/main.cpp',
        'src/python/satani.py',
        'include/satani.h',
    ]
    
    all_present = True
    for file_path in required_files:
        full_path = Path(__file__).parent.parent / file_path
        if full_path.exists():
            size = full_path.stat().st_size
            print(f"{Colors.GREEN}[+] {file_path} ({size} bytes){Colors.END}")
        else:
            print(f"{Colors.RED}[!] {file_path} missing{Colors.END}")
            all_present = False
    
    return all_present

def generate_report(results):
    """Generate and display a summary report"""
    print(f"\n{Colors.CYAN}{'='*60}")
    print("SYSTEM VERIFICATION REPORT")
    print(f"{'='*60}{Colors.END}\n")
    
    checks = {
        'Python Version': results['python'],
        'Operating System': results['os'],
        'C/C++ Compiler': results['compiler'],
        'Assembler (MASM)': results['assembler'],
        'Linker': results['linker'],
        'Windows SDK': results['sdk'],
        'Python Packages': results['packages'],
        'Directory Structure': results['directories'],
        'Source Files': results['files'],
    }
    
    passed = 0
    failed = 0
    
    for check, result in checks.items():
        if result:
            print(f"{Colors.GREEN}[✓] {check}{Colors.END}")
            passed += 1
        else:
            print(f"{Colors.RED}[✗] {check}{Colors.END}")
            failed += 1
    
    print(f"\n{Colors.CYAN}Summary: {passed} passed, {failed} failed{Colors.END}\n")
    
    if failed == 0:
        print(f"{Colors.GREEN}[+] System is ready for Satani development!{Colors.END}")
        print(f"{Colors.GREEN}[+] You can now build the framework with:{Colors.END}")
        print(f"{Colors.CYAN}    python src/python/satani.py --build-only{Colors.END}\n")
        return True
    else:
        print(f"{Colors.RED}[!] Please install missing components before proceeding{Colors.END}\n")
        return False

def main():
    """Main verification function"""
    print(f"\n{Colors.CYAN}")
    print("="*60)
    print("SATANI INSTALLATION VERIFICATION")
    print("="*60)
    print(f"{Colors.END}\n")
    
    results = {
        'python': check_python_version(),
        'os': check_os(),
        'compiler': check_compiler(),
        'assembler': check_assembler(),
        'linker': check_linker(),
        'sdk': check_network_libraries(),
        'packages': check_python_packages(),
        'directories': check_directory_structure(),
        'files': check_source_files(),
    }
    
    success = generate_report(results)
    
    if success:
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == '__main__':
    main()

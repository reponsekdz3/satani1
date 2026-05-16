# SATANI Agentless Control - Build Configuration

# Agentless control source files
AGENTLESS_SOURCES = [
    'src/c/agentless_control.c',
    'src/c/agentless_advanced.c', 
    'src/c/agentless_linux.c',
    'src/c/exec_real.c',
    'src/c/exploit_real.c',
    'src/c/scan.c'
]

# Additional libraries for agentless control
AGENTLESS_LIBS = [
    'iphlpapi.lib',
    'ws2_32.lib',
    'wbemuuid.lib',
    'wininet.lib',
    'ole32.lib',
    'oleaut32.lib',
    'netapi32.lib',
    'taskschd.lib',
    'credui.lib',
    'comctl32.lib',
    'shell32.lib'
]

# Compiler defines
AGENTLESS_DEFINES = [
    '_WIN32_WINNT=0x0601',
    'WIN32_LEAN_AND_MEAN',
    'UNICODE',
    '_UNICODE'
]

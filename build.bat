@echo off
REM Satani Cybersecurity Framework - Build Script for Windows
REM This script compiles Assembly, C, and C++ components
REM Requires: Visual Studio Build Tools with C++ support

setlocal enabledelayedexpansion

echo.
echo ====================================================
echo     SATANI - Cybersecurity Framework Build
echo ====================================================
echo.

REM Get the directory where this script is located
set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%"

REM Define directories
set ASM_DIR=%SCRIPT_DIR%..\src\asm
set C_DIR=%SCRIPT_DIR%..\src\c
set CPP_DIR=%SCRIPT_DIR%..\src\cpp
set INCLUDE_DIR=%SCRIPT_DIR%..\include
set BUILD_DIR=%SCRIPT_DIR%..\build

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Object and executable files
set CHECKSUM_OBJ=%BUILD_DIR%\checksum.obj
set SCAN_OBJ=%BUILD_DIR%\scan.obj
set EXEC_REAL_OBJ=%BUILD_DIR%\exec_real.obj
set MAIN_OBJ=%BUILD_DIR%\main.obj
set EXECUTABLE=%BUILD_DIR%\satani.exe

REM Check command line arguments
if "%1"=="--rebuild" goto rebuild
if "%1"=="-r" goto rebuild
if "%1"=="--clean" goto clean

REM Normal build (incremental)
goto build

:rebuild
echo [*] Forcing rebuild of all components...
if exist "%EXECUTABLE%" del "%EXECUTABLE%"
if exist "%BUILD_DIR%\*.obj" del /q "%BUILD_DIR%\*.obj"
goto build

:clean
echo [*] Cleaning build directory...
if exist "%BUILD_DIR%" (
    del /q "%BUILD_DIR%\*"
    echo [+] Build directory cleaned.
)
goto end

:build
echo [*] Building Satani framework...
echo.

REM Check if Visual Studio is in PATH
where cl >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [!] Error: Microsoft C/C++ Compiler (cl.exe) not found!
    echo [!] Please install Visual Studio Build Tools with C++ support.
    echo [!] Get it from: https://visualstudio.microsoft.com/downloads/
    goto error
)

REM Compile Assembly
echo [+] Compiling assembly modules...
if exist "%ASM_DIR%\checksum.asm" (
    ml64 -c -Fo"%CHECKSUM_OBJ%" "%ASM_DIR%\checksum.asm" 2>nul
    if %ERRORLEVEL% NEQ 0 (
        echo [!] Assembly step skipped or failed
        del "%CHECKSUM_OBJ%" 2>nul
        set CHECKSUM_OBJ=
    ) else (
        echo     - checksum.asm compiled
    )
) else (
    echo     - checksum.asm not found, skipping
)

REM Compile C (all C files)
echo [+] Compiling C modules...
cl -c -Fo"%SCAN_OBJ%" -I"%INCLUDE_DIR%" "%C_DIR%\scan.c" /W3 /O2
if %ERRORLEVEL% NEQ 0 (
    echo [!] C compilation failed for scan.c!
    goto error
) else (
    echo     - scan.c compiled
)

cl -c -Fo"%BUILD_DIR%\exploit_real.obj" -I"%INCLUDE_DIR%" "%C_DIR%\exploit_real.c" /W3 /O2
if %ERRORLEVEL% NEQ 0 (
    echo [!] C compilation failed for exploit_real.c!
    goto error
) else (
    echo     - exploit_real.c compiled
)

cl -c -Fo"%BUILD_DIR%\aircraft_satellite.obj" -I"%INCLUDE_DIR%" "%C_DIR%\aircraft_satellite.c" /W3 /O2
if %ERRORLEVEL% NEQ 0 (
    echo [!] C compilation failed for aircraft_satellite.c!
    goto error
) else (
    echo     - aircraft_satellite.c compiled
)

cl -c -Fo"%BUILD_DIR%\industrial_control.obj" -I"%INCLUDE_DIR%" "%C_DIR%\industrial_control.c" /W3 /O2
if %ERRORLEVEL% NEQ 0 (
    echo [!] C compilation failed for industrial_control.c!
    goto error
) else (
    echo     - industrial_control.c compiled
)

cl -c -Fo"%BUILD_DIR%\drone_warfare.obj" -I"%INCLUDE_DIR%" "%C_DIR%\drone_warfare.c" /W3 /O2
if %ERRORLEVEL% NEQ 0 (
    echo [!] C compilation failed for drone_warfare.c!
    goto error
) else (
    echo     - drone_warfare.c compiled
)

cl -c -Fo"%BUILD_DIR%\hackrf.obj" -I"%INCLUDE_DIR%" "%C_DIR%\hackrf.c" /W3 /O2
if %ERRORLEVEL% NEQ 0 (
    echo [!] C compilation failed for hackrf.c!
    goto error
) else (
    echo     - hackrf.c compiled
)

cl -c -Fo"%BUILD_DIR%\hex_utils.obj" -I"%INCLUDE_DIR%" "%C_DIR%\hex_utils.c" /W3 /O2
if %ERRORLEVEL% NEQ 0 (
    echo [!] C compilation failed for hex_utils.c!
    goto error
) else (
    echo     - hex_utils.c compiled
)

rem Link
echo [+] Linking object files...
set LINK_OBJS=
if exist "%CHECKSUM_OBJ%" set LINK_OBJS=%LINK_OBJS% "%CHECKSUM_OBJ%"
if exist "%SCAN_OBJ%" set LINK_OBJS=%LINK_OBJS% "%SCAN_OBJ%"
if exist "%BUILD_DIR%\exploit_real.obj" set LINK_OBJS=%LINK_OBJS% "%BUILD_DIR%\exploit_real.obj"
if exist "%BUILD_DIR%\aircraft_satellite.obj" set LINK_OBJS=%LINK_OBJS% "%BUILD_DIR%\aircraft_satellite.obj"
if exist "%BUILD_DIR%\industrial_control.obj" set LINK_OBJS=%LINK_OBJS% "%BUILD_DIR%\industrial_control.obj"
if exist "%BUILD_DIR%\drone_warfare.obj" set LINK_OBJS=%LINK_OBJS% "%BUILD_DIR%\drone_warfare.obj"
if exist "%BUILD_DIR%\hackrf.obj" set LINK_OBJS=%LINK_OBJS% "%BUILD_DIR%\hackrf.obj"
if exist "%BUILD_DIR%\hex_utils.obj" set LINK_OBJS=%LINK_OBJS% "%BUILD_DIR%\hex_utils.obj"
if exist "%MAIN_OBJ%" set LINK_OBJS=%LINK_OBJS% "%MAIN_OBJ%"

link -OUT:"%EXECUTABLE%" %LINK_OBJS% iphlpapi.lib ws2_32.lib shell32.lib /OPT:REF /LTCG

if %ERRORLEVEL% NEQ 0 (
    echo [!] Linking failed!
    goto error
)

echo.
echo [+] Build completed successfully!
echo [+] Executable: %EXECUTABLE%
echo.

goto end

:error
echo.
echo [!] Build failed!
exit /b 1

:end
endlocal
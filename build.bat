@echo off
REM Satani Cybersecurity Framework - Build Script for Windows
REM This script compiles Assembly, C, and C++ components
REM Requires: Visual Studio Build Tools with C++ support

setlocal enabledelayedexpansion

REM Color codes don't work in batch, so we'll use simple messages
echo.
echo ====================================================
echo     SATANI - Cybersecurity Framework Build
echo ====================================================
echo.

REM Get the directory where this script is located
set SCRIPT_DIR=%~dp0
cd /d %SCRIPT_DIR%

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

where ml >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [!] Warning: MASM (ml.exe) not found. Skipping assembly compilation.
    goto skip_asm
)

REM Compile Assembly
echo [+] Compiling assembly module (checksum.asm)...
ml -c -Fo"%CHECKSUM_OBJ%" "%ASM_DIR%\checksum.asm"
if %ERRORLEVEL% NEQ 0 (
    echo [!] Assembly compilation failed!
    goto error
)

:skip_asm
REM Compile C
echo [+] Compiling C module (scan.c)...
cl -c -Fo"%SCAN_OBJ%" -I"%INCLUDE_DIR%" "%C_DIR%\scan.c"
if %ERRORLEVEL% NEQ 0 (
    echo [!] C compilation failed!
    goto error
)

REM Compile C++
echo [+] Compiling C++ module (main.cpp)...
cl -c -Fo"%MAIN_OBJ%" -I"%INCLUDE_DIR%" "%CPP_DIR%\main.cpp"
if %ERRORLEVEL% NEQ 0 (
    echo [!] C++ compilation failed!
    goto error
)

REM Link
echo [+] Linking object files...
link -OUT:"%EXECUTABLE%" "%CHECKSUM_OBJ%" "%SCAN_OBJ%" "%MAIN_OBJ%" ^
      iphlpapi.lib ws2_32.lib shell32.lib

if %ERRORLEVEL% NEQ 0 (
    echo [!] Linking failed!
    goto error
)

echo.
echo [+] Build completed successfully!
echo [+] Executable: %EXECUTABLE%
echo.

REM Run if not just building
if "%2"=="" (
    echo [*] Running Satani framework...
    "%EXECUTABLE%" %*
) else (
    "%EXECUTABLE%" %2 %3 %4 %5 %6 %7 %8 %9
)

goto end

:error
echo.
echo [!] Build failed!
exit /b 1

:end
endlocal

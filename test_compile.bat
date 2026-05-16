@echo off
echo Testing SATANI compilation...
echo.

REM Check for Visual Studio compiler
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Microsoft C/C++ Compiler (cl.exe) not found!
    echo Please install Visual Studio Build Tools with C++ Desktop Development.
    echo Download: https://visualstudio.microsoft.com/downloads/
    exit /b 1
)

REM Check for MASM assembler
where ml >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Microsoft Assembler (ml.exe) not found!
    echo Please install Visual Studio Build Tools with MASM support.
    exit /b 1
)

echo Compilers found:
where cl
where ml
where link
echo.

echo Testing assembly compilation...
ml -c -Fo"build\checksum.obj" "src\asm\checksum.asm"
if %errorlevel% neq 0 (
    echo ERROR: Assembly compilation failed!
    exit /b 1
)

echo Testing C compilation...
cl -c -Fo"build\scan.obj" -I"include" "src\c\scan.c"
if %errorlevel% neq 0 (
    echo ERROR: C compilation failed!
    exit /b 1
)

echo Testing C++ compilation...
cl -c -Fo"build\main.obj" -I"include" "src\cpp\main.cpp"
if %errorlevel% neq 0 (
    echo ERROR: C++ compilation failed!
    exit /b 1
)

echo Testing test compilation...
cl -c -Fo"build\test_exploit.obj" -I"include" "test_exploit.cpp"
if %errorlevel% neq 0 (
    echo ERROR: Test compilation failed!
    exit /b 1
)

echo.
echo All compilation tests passed!
echo.
echo SATANI framework components:
echo - Assembly: checksum.asm (real x86/x64 assembly for network operations)
echo - C: scan.c (real network scanning engine with ARP, port scanning)
echo - C++: main.cpp (real CLI interface with command parsing)
echo - Python: satani.py (real build orchestration and runner)
echo - Test: test_exploit.cpp (real functional test suite)
echo.
echo Framework is ready for building with: python src\python\satani.py --rebuild
echo.
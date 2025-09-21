@echo off
REM BSCS Client Build Script for Windows
REM Requires MinGW/MSYS2 with GCC and GNU Make

echo Building BSCS Client for Windows...

REM Check if make is available
where make >nul 2>&1
if errorlevel 1 (
    echo Error: GNU Make not found. Please install MinGW/MSYS2.
    echo Download from: https://www.msys2.org/
    pause
    exit /b 1
)

REM Check if gcc is available
where gcc >nul 2>&1
if errorlevel 1 (
    echo Error: GCC not found. Please install MinGW toolchain.
    echo From MSYS2, run: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
    pause
    exit /b 1
)

echo Cleaning previous build...
make clean

echo Building debug version...
make debug

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build successful!
echo.
echo To run the client:
echo   bin\bscs_client.exe --help
echo   bin\bscs_client.exe [server_ip] [port]
echo.
pause
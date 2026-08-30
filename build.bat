@echo off
rem FloatCleaner build script (requires MinGW-w64 g++/windres in PATH)
setlocal
cd /d "%~dp0"

where g++ >nul 2>nul
if errorlevel 1 (
    echo [!] g++ not found. Install MinGW-w64 and add it to PATH.
    exit /b 1
)

if not exist build mkdir build

windres resources\app.rc -O coff -o build\app.res
if errorlevel 1 (
    echo [!] windres failed.
    exit /b 1
)

g++ -std=c++17 -O2 -municode -mwindows -Wall -Wextra -Isrc src\main.cpp src\config\Config.cpp src\core\ProcessScanner.cpp src\core\ProtectionList.cpp src\core\ProcessKiller.cpp src\core\SystemMonitor.cpp src\auto\AutoCleaner.cpp src\ui\Theme.cpp src\ui\DrawUtils.cpp src\ui\TrayIcon.cpp src\ui\FloatingBall.cpp src\ui\ProcessPanel.cpp build\app.res -o FloatCleaner.exe -lpsapi -lcomctl32 -lshell32 -lgdi32 -luser32 -ladvapi32 -s

if errorlevel 1 (
    echo [!] build failed.
    exit /b 1
)

echo [OK] FloatCleaner.exe generated.

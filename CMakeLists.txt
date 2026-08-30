cmake_minimum_required(VERSION 3.16)
project(FloatCleaner LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(FloatCleaner WIN32
    src/main.cpp
    src/app/AppContext.hpp
    src/config/Config.hpp
    src/config/Config.cpp
    src/config/defaults.hpp
    src/core/ProcessInfo.hpp
    src/core/ProcessScanner.hpp
    src/core/ProcessScanner.cpp
    src/core/ProtectionList.hpp
    src/core/ProtectionList.cpp
    src/core/ProcessKiller.hpp
    src/core/ProcessKiller.cpp
    src/core/SystemMonitor.hpp
    src/core/SystemMonitor.cpp
    src/auto/AutoCleaner.hpp
    src/auto/AutoCleaner.cpp
    src/ui/Theme.hpp
    src/ui/Theme.cpp
    src/ui/DrawUtils.hpp
    src/ui/DrawUtils.cpp
    src/ui/TrayIcon.hpp
    src/ui/TrayIcon.cpp
    src/ui/FloatingBall.hpp
    src/ui/FloatingBall.cpp
    src/ui/ProcessPanel.hpp
    src/ui/ProcessPanel.cpp
    resources/app.rc
)

target_include_directories(FloatCleaner PRIVATE src)
target_compile_definitions(FloatCleaner PRIVATE
    UNICODE _UNICODE WIN32_LEAN_AND_MEAN)

target_link_libraries(FloatCleaner PRIVATE
    psapi comctl32 shell32 gdi32 user32 advapi32)

if(MINGW)
    target_compile_options(FloatCleaner PRIVATE -O2 -Wall -Wextra)
    target_link_options(FloatCleaner PRIVATE -municode -mwindows -s)
endif()

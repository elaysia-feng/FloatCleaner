#pragma once
#include <cstdint>
#include <string>

namespace fc {

// 进程保护等级（由高到低）
enum class ProtectionLevel {
    System,        // 系统关键进程，永不结束
    UserProtected, // 用户白名单进程
    AutoClean,     // 在自动清理名单中（可手动/自动结束）
    Normal,        // 普通用户进程
};

struct ProcessInfo {
    uint32_t pid = 0;
    std::wstring name;
    std::wstring fullPath;
    uint64_t workingSet = 0; // 物理内存（字节）
    double cpuPercent = 0.0;
    ProtectionLevel level = ProtectionLevel::Normal;
    bool canTerminate = true;
};

} // namespace fc

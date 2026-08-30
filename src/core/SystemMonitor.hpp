#pragma once
#include <cstdint>

namespace fc {

struct MemoryStatus {
    uint32_t percent = 0;     // 物理内存占用率 0-100
    uint64_t totalBytes = 0;
    uint64_t availBytes = 0;
};

class SystemMonitor {
public:
    static MemoryStatus query();
};

} // namespace fc

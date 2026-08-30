#pragma once
#include "ProcessInfo.hpp"
#include <unordered_map>
#include <vector>

namespace fc {

class ProtectionList;

// 进程扫描器：Toolhelp 枚举 + 工作集内存 + CPU 差分采样
class ProcessScanner {
public:
    // 重新枚举全部进程并统计内存/CPU，返回内部列表（按内存降序）
    const std::vector<ProcessInfo>& refresh(const ProtectionList& protection);

    const std::vector<ProcessInfo>& processes() const { return procs_; }

private:
    std::vector<ProcessInfo> procs_;
    // pid -> 累计 CPU 时间（100ns 单位，kernel+user）
    std::unordered_map<uint32_t, uint64_t> prevCpuTime_;
    uint64_t lastTickMs_ = 0;
};

} // namespace fc

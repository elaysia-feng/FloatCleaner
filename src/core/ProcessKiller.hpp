#pragma once
#include "ProcessInfo.hpp"
#include <string>

namespace fc {

struct KillResult {
    bool ok = false;
    std::wstring message; // 失败原因（成功时为空）
};

// 结束进程。结束前校验保护等级：System/UserProtected 一律拒绝。
KillResult terminateProcessById(uint32_t pid, const std::wstring& name,
                                ProtectionLevel level);

} // namespace fc

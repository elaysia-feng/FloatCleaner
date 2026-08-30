#pragma once
#include "../core/ProcessInfo.hpp"
#include <functional>
#include <string>
#include <vector>

namespace fc {

class Config;
class ProtectionList;

// 智能清理：定时检查全局内存占用，超过阈值时结束"自动清理名单"中的进程。
// 只结束用户显式加入名单的进程，绝不触碰其他任何进程。
class AutoCleaner {
public:
    using NotifyFn = std::function<void(const std::wstring& title,
                                        const std::wstring& text)>;

    void setup(const Config* config, const ProtectionList* protection, NotifyFn notify);

    // 每个自动清理周期调用一次（传入最新进程快照）
    void tick(const std::vector<ProcessInfo>& snapshot);

    bool enabled() const;
    void setEnabled(bool enabled);

private:
    void appendLog(const std::wstring& line);

    const Config* config_ = nullptr;
    const ProtectionList* protection_ = nullptr;
    NotifyFn notify_;
    bool enabled_ = true;
};

} // namespace fc

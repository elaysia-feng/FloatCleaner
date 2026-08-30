#include "AutoCleaner.hpp"
#include "../config/Config.hpp"
#include "../config/defaults.hpp"
#include "../core/ProcessKiller.hpp"
#include "../core/SystemMonitor.hpp"

#include <windows.h>

#include <vector>

namespace fc {
namespace {

std::wstring formatBytes(uint64_t bytes)
{
    wchar_t buf[64] = {};
    if (bytes >= 1ull << 30)
        swprintf(buf, 64, L"%.2f GB", static_cast<double>(bytes) / (1ull << 30));
    else
        swprintf(buf, 64, L"%.1f MB", static_cast<double>(bytes) / (1ull << 20));
    return buf;
}

std::wstring timestamp()
{
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buf[32] = {};
    swprintf(buf, 32, L"%04u-%02u-%02u %02u:%02u:%02u", st.wYear, st.wMonth,
             st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buf;
}

} // namespace

void AutoCleaner::setup(const Config* config, const ProtectionList* protection,
                        NotifyFn notify)
{
    config_ = config;
    protection_ = protection;
    notify_ = std::move(notify);
    enabled_ = config_ ? config_->autoCleanEnabled : true;
}

bool AutoCleaner::enabled() const
{
    return enabled_;
}

void AutoCleaner::setEnabled(bool enabled)
{
    enabled_ = enabled;
}

void AutoCleaner::tick(const std::vector<ProcessInfo>& snapshot)
{
    if (!enabled_ || !config_ || !protection_)
        return;

    const MemoryStatus ms = SystemMonitor::query();
    if (ms.percent < static_cast<uint32_t>(config_->memoryThreshold))
        return; // 未超阈值，不动任何进程

    int killed = 0;
    uint64_t freed = 0;
    std::wstring names;
    std::vector<std::wstring> logLines;

    for (const auto& p : snapshot) {
        if (p.level != ProtectionLevel::AutoClean)
            continue; // 只处理自动清理名单中的进程

        const KillResult r = terminateProcessById(p.pid, p.name, p.level);
        if (!r.ok) {
            logLines.push_back(timestamp() + L" [失败] " + p.name + L" - " + r.message);
            continue;
        }
        ++killed;
        freed += p.workingSet;
        if (!names.empty())
            names += L"、";
        names += p.name;
        logLines.push_back(timestamp() + L" [结束] " + p.name + L" (PID " +
                           std::to_wstring(p.pid) + L", 释放 " +
                           formatBytes(p.workingSet) + L")");
    }

    if (killed == 0)
        return;

    for (const auto& line : logLines)
        appendLog(line);

    if (notify_) {
        notify_(L"FloatCleaner 智能清理",
                L"内存占用 " + std::to_wstring(ms.percent) + L"% 已超过阈值 " +
                    std::to_wstring(config_->memoryThreshold) + L"%，自动结束 " +
                    std::to_wstring(killed) + L" 个进程（" + names + L"），释放 " +
                    formatBytes(freed));
    }
}

void AutoCleaner::appendLog(const std::wstring& line)
{
    const std::wstring dir = exeRelativePath(defaults::kLogDir);
    CreateDirectoryW(dir.c_str(), nullptr);
    const std::wstring logPath = dir + L"\\" + defaults::kLogFile;

    HANDLE h = CreateFileW(logPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return;

    // UTF-8 编码追加写入
    const std::wstring full = line + L"\r\n";
    const int size = WideCharToMultiByte(CP_UTF8, 0, full.c_str(),
                                         static_cast<int>(full.size()),
                                         nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, full.c_str(), static_cast<int>(full.size()),
                        utf8.data(), size, nullptr, nullptr);

    DWORD written = 0;
    WriteFile(h, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
    CloseHandle(h);
}

} // namespace fc

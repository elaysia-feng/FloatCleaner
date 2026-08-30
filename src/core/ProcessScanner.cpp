#include "ProcessScanner.hpp"
#include "ProtectionList.hpp"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <algorithm>
#include <system_error>

namespace fc {
namespace {

std::wstring queryImagePath(HANDLE h)
{
    std::wstring path(MAX_PATH, L'\0');
    DWORD size = static_cast<DWORD>(path.size());
    if (!QueryFullProcessImageNameW(h, 0, path.data(), &size)) {
        // 某些系统进程路径较长或不可读，尝试更大缓冲
        path.resize(1024);
        size = static_cast<DWORD>(path.size());
        if (!QueryFullProcessImageNameW(h, 0, path.data(), &size))
            return {};
    }
    path.resize(size);
    return path;
}

} // namespace

const std::vector<ProcessInfo>& ProcessScanner::refresh(const ProtectionList& protection)
{
    const uint64_t nowMs = GetTickCount64();
    const uint64_t wallDeltaMs = lastTickMs_ ? nowMs - lastTickMs_ : 0;

    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    const double cores = si.dwNumberOfProcessors > 0 ? si.dwNumberOfProcessors : 1.0;

    std::vector<ProcessInfo> result;
    std::unordered_map<uint32_t, uint64_t> curCpuTime;

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return procs_;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            ProcessInfo info;
            info.pid = pe.th32ProcessID;
            info.name = pe.szExeFile;

            HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, info.pid);
            if (h) {
                info.fullPath = queryImagePath(h);

                PROCESS_MEMORY_COUNTERS pmc{};
                pmc.cb = sizeof(pmc);
                if (GetProcessMemoryInfo(h, &pmc, sizeof(pmc)))
                    info.workingSet = pmc.WorkingSetSize;

                FILETIME ftCreate{}, ftExit{}, ftKernel{}, ftUser{};
                if (GetProcessTimes(h, &ftCreate, &ftExit, &ftKernel, &ftUser)) {
                    const uint64_t total =
                        (static_cast<uint64_t>(ftKernel.dwHighDateTime) << 32 |
                         ftKernel.dwLowDateTime) +
                        (static_cast<uint64_t>(ftUser.dwHighDateTime) << 32 |
                         ftUser.dwLowDateTime);
                    curCpuTime[info.pid] = total;

                    auto it = prevCpuTime_.find(info.pid);
                    if (it != prevCpuTime_.end() && wallDeltaMs > 0 && total >= it->second) {
                        const double delta100ns =
                            static_cast<double>(total - it->second);
                        const double wall100ns =
                            static_cast<double>(wallDeltaMs) * 10000.0 * cores;
                        if (wall100ns > 0)
                            info.cpuPercent = delta100ns / wall100ns * 100.0;
                    }
                }
                CloseHandle(h);
            }

            info.level = protection.classify(info.pid, info.name, info.fullPath);
            info.canTerminate =
                info.level == ProtectionLevel::Normal ||
                info.level == ProtectionLevel::AutoClean;
            result.push_back(std::move(info));
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);

    std::sort(result.begin(), result.end(),
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.workingSet > b.workingSet;
              });

    procs_ = std::move(result);
    prevCpuTime_ = std::move(curCpuTime);
    lastTickMs_ = nowMs;
    return procs_;
}

} // namespace fc

#include "ProcessKiller.hpp"

#include <windows.h>

#include <system_error>

namespace fc {

KillResult terminateProcessById(uint32_t pid, const std::wstring& name,
                                ProtectionLevel level)
{
    KillResult r;

    if (level == ProtectionLevel::System) {
        r.message = L"系统关键进程受硬保护，拒绝结束：" + name;
        return r;
    }
    if (level == ProtectionLevel::UserProtected) {
        r.message = L"进程在用户白名单中，拒绝结束：" + name;
        return r;
    }

    HANDLE h = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
    if (!h) {
        const DWORD err = GetLastError();
        r.message = L"无法打开进程 " + name + L" (错误码 " +
                    std::to_wstring(err) + L")" +
                    (err == ERROR_ACCESS_DENIED
                         ? L"，可能需要以管理员身份运行 FloatCleaner"
                         : L"");
        return r;
    }

    if (!::TerminateProcess(h, 1)) {
        const DWORD err = GetLastError();
        r.message = L"结束进程失败：" + name + L" (错误码 " + std::to_wstring(err) + L")";
        CloseHandle(h);
        return r;
    }

    WaitForSingleObject(h, 3000);
    CloseHandle(h);

    r.ok = true;
    return r;
}

} // namespace fc

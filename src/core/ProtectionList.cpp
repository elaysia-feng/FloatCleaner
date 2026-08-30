#include "ProtectionList.hpp"

#include <windows.h>

#include <algorithm>
#include <cctype>

namespace fc {
namespace {

// 内置系统硬保护名单（小写）。杀掉任何一个都可能导致系统不稳定或蓝屏。
const std::set<std::wstring>& builtinSystemProcesses()
{
    static const std::set<std::wstring> kSystem = {
        L"system", L"idle", L"secure system", L"registry", L"memory compression",
        L"smss.exe", L"csrss.exe", L"wininit.exe", L"winlogon.exe",
        L"services.exe", L"lsass.exe", L"svchost.exe", L"dwm.exe",
        L"explorer.exe", L"fontdrvhost.exe", L"audiodg.exe",
        L"sihost.exe", L"ctfmon.exe", L"taskhostw.exe",
        // Windows Defender 核心，杀掉等于裸奔
        L"msmpeng.exe", L"nissrv.exe", L"securityhealthservice.exe",
    };
    return kSystem;
}

std::wstring toLower(std::wstring s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](wchar_t c) { return static_cast<wchar_t>(::towlower(c)); });
    return s;
}

bool pathStartsWithWindowsDir(const std::wstring& fullPath, const std::wstring& winDir)
{
    if (fullPath.empty() || winDir.empty())
        return false;
    const std::wstring lower = toLower(fullPath);
    const std::wstring dir = toLower(winDir);
    if (lower.compare(0, dir.size(), dir) != 0)
        return false;
    // 目录后必须紧跟分隔符，避免 C:\WindowsStuff 之类误判
    return lower.size() > dir.size() &&
           (lower[dir.size()] == L'\\' || lower[dir.size()] == L'/');
}

} // namespace

void ProtectionList::init(const std::vector<std::wstring>& whitelist,
                          const std::vector<std::wstring>& autoCleanList)
{
    wchar_t dir[MAX_PATH] = {};
    GetWindowsDirectoryW(dir, MAX_PATH);
    windowsDir_ = dir;

    whitelist_ = toLowerSet(whitelist);
    autoClean_ = toLowerSet(autoCleanList);
}

std::set<std::wstring> ProtectionList::toLowerSet(const std::vector<std::wstring>& names)
{
    std::set<std::wstring> s;
    for (const auto& n : names) {
        std::wstring lower = toLower(n);
        if (!lower.empty())
            s.insert(std::move(lower));
    }
    return s;
}

ProtectionLevel ProtectionList::classify(uint32_t pid, const std::wstring& name,
                                         const std::wstring& fullPath) const
{
    // 自己和系统空闲/中断进程
    if (pid == 0 || pid == 4 || pid == GetCurrentProcessId())
        return ProtectionLevel::System;

    const std::wstring lowerName = toLower(name);
    if (builtinSystemProcesses().count(lowerName))
        return ProtectionLevel::System;

    // 位于 Windows 目录下的进程一律视为系统进程
    if (pathStartsWithWindowsDir(fullPath, windowsDir_))
        return ProtectionLevel::System;

    if (whitelist_.count(lowerName))
        return ProtectionLevel::UserProtected;

    if (autoClean_.count(lowerName))
        return ProtectionLevel::AutoClean;

    return ProtectionLevel::Normal;
}

bool ProtectionList::isWhitelisted(const std::wstring& name) const
{
    return whitelist_.count(toLower(name)) > 0;
}

bool ProtectionList::inAutoClean(const std::wstring& name) const
{
    return autoClean_.count(toLower(name)) > 0;
}

void ProtectionList::addToWhitelist(const std::wstring& name)
{
    whitelist_.insert(toLower(name));
}

void ProtectionList::removeFromWhitelist(const std::wstring& name)
{
    whitelist_.erase(toLower(name));
}

void ProtectionList::addToAutoClean(const std::wstring& name)
{
    autoClean_.insert(toLower(name));
}

void ProtectionList::removeFromAutoClean(const std::wstring& name)
{
    autoClean_.erase(toLower(name));
}

std::vector<std::wstring> ProtectionList::whitelistVector() const
{
    return {whitelist_.begin(), whitelist_.end()};
}

std::vector<std::wstring> ProtectionList::autoCleanVector() const
{
    return {autoClean_.begin(), autoClean_.end()};
}

} // namespace fc

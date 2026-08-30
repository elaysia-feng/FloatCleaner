#pragma once
#include "ProcessInfo.hpp"
#include <set>
#include <string>
#include <vector>

namespace fc {

// 三级保护名单：
//  1. 内置系统硬保护（代码内置，不可修改）+ Windows 目录下进程
//  2. 用户白名单（ini 配置，手动/自动都不杀）
//  3. 自动清理名单（ini 配置，内存超阈值时自动结束）
class ProtectionList {
public:
    void init(const std::vector<std::wstring>& whitelist,
              const std::vector<std::wstring>& autoCleanList);

    ProtectionLevel classify(uint32_t pid, const std::wstring& name,
                             const std::wstring& fullPath) const;

    bool isWhitelisted(const std::wstring& name) const;
    bool inAutoClean(const std::wstring& name) const;

    void addToWhitelist(const std::wstring& name);
    void removeFromWhitelist(const std::wstring& name);
    void addToAutoClean(const std::wstring& name);
    void removeFromAutoClean(const std::wstring& name);

    std::vector<std::wstring> whitelistVector() const;
    std::vector<std::wstring> autoCleanVector() const;

private:
    static std::set<std::wstring> toLowerSet(const std::vector<std::wstring>& names);

    std::set<std::wstring> whitelist_;
    std::set<std::wstring> autoClean_;
    std::wstring windowsDir_; // 例如 C:\WINDOWS
};

} // namespace fc

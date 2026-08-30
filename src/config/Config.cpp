#include "Config.hpp"
#include "defaults.hpp"

#include <windows.h>

namespace fc {
namespace {

std::wstring getString(const std::wstring& ini, const wchar_t* section,
                       const wchar_t* key, const wchar_t* defVal)
{
    std::wstring buf(1024, L'\0');
    DWORD n = GetPrivateProfileStringW(section, key, defVal, buf.data(),
                                       static_cast<DWORD>(buf.size()), ini.c_str());
    return std::wstring(buf.data(), n);
}

} // namespace

std::wstring exeDir()
{
    std::wstring path(MAX_PATH, L'\0');
    DWORD n = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    path.resize(n);
    const size_t pos = path.find_last_of(L"\\/");
    return pos == std::wstring::npos ? L"." : path.substr(0, pos);
}

std::wstring exeRelativePath(const std::wstring& fileName)
{
    return exeDir() + L"\\" + fileName;
}

std::wstring Config::join(const std::vector<std::wstring>& names)
{
    std::wstring out;
    for (const auto& n : names) {
        if (!out.empty())
            out += L',';
        out += n;
    }
    return out;
}

std::vector<std::wstring> Config::split(const std::wstring& s)
{
    std::vector<std::wstring> out;
    std::wstring cur;
    for (wchar_t c : s) {
        if (c == L',') {
            if (!cur.empty())
                out.push_back(cur);
            cur.clear();
        } else if (!iswspace(c)) {
            cur += c;
        }
    }
    if (!cur.empty())
        out.push_back(cur);
    return out;
}

bool Config::load(const std::wstring& iniPath)
{
    // GetPrivateProfileString 对不存在的文件只返回默认值，不做特殊处理
    ballX = GetPrivateProfileIntW(L"general", L"ballX", -1, iniPath.c_str());
    ballY = GetPrivateProfileIntW(L"general", L"ballY", -1, iniPath.c_str());
    dockEdge = GetPrivateProfileIntW(L"general", L"dockEdge", 2, iniPath.c_str());
    if (dockEdge < 0 || dockEdge > 4)
        dockEdge = 2;
    themeIndex = GetPrivateProfileIntW(L"general", L"theme", 0, iniPath.c_str());
    if (themeIndex < 0 || themeIndex > 3)
        themeIndex = 0;

    autoCleanEnabled =
        getString(iniPath, L"autoclean", L"enabled", L"1") != L"0";
    autoCleanIntervalSec = GetPrivateProfileIntW(
        L"autoclean", L"interval_sec", defaults::kDefaultIntervalSec, iniPath.c_str());
    memoryThreshold = GetPrivateProfileIntW(
        L"autoclean", L"memory_threshold", defaults::kDefaultMemoryThreshold,
        iniPath.c_str());
    if (autoCleanIntervalSec < 10)
        autoCleanIntervalSec = 10;
    if (memoryThreshold < 30)
        memoryThreshold = 30;

    whitelist = split(getString(iniPath, L"whitelist", L"names", L""));
    autoCleanList = split(getString(iniPath, L"autocleanlist", L"names", L""));
    return true;
}

void Config::save(const std::wstring& iniPath) const
{
    WritePrivateProfileStringW(L"general", L"ballX", std::to_wstring(ballX).c_str(),
                               iniPath.c_str());
    WritePrivateProfileStringW(L"general", L"ballY", std::to_wstring(ballY).c_str(),
                               iniPath.c_str());
    WritePrivateProfileStringW(L"general", L"dockEdge",
                               std::to_wstring(dockEdge).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"general", L"theme",
                               std::to_wstring(themeIndex).c_str(),
                               iniPath.c_str());

    WritePrivateProfileStringW(L"autoclean", L"enabled",
                               autoCleanEnabled ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"autoclean", L"interval_sec",
                               std::to_wstring(autoCleanIntervalSec).c_str(),
                               iniPath.c_str());
    WritePrivateProfileStringW(L"autoclean", L"memory_threshold",
                               std::to_wstring(memoryThreshold).c_str(),
                               iniPath.c_str());

    WritePrivateProfileStringW(L"whitelist", L"names", join(whitelist).c_str(),
                               iniPath.c_str());
    WritePrivateProfileStringW(L"autocleanlist", L"names", join(autoCleanList).c_str(),
                               iniPath.c_str());
}

} // namespace fc

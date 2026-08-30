#pragma once
#include <string>
#include <vector>

namespace fc {

// 配置（exe 同目录的 ini 文件，人类可读可手改）
struct Config {
    // [general]
    int ballX = -1;
    int ballY = -1;
    int dockEdge = 2;    // 0=自由 1=左 2=右 3=上 4=下
    int themeIndex = 0;  // 主题：0=夜樱 1=月夜 2=初樱 3=花火

    // [autoclean]
    bool autoCleanEnabled = true;
    int autoCleanIntervalSec = 60;
    int memoryThreshold = 85;

    // 名单（小写进程名，逗号分隔存储）
    std::vector<std::wstring> whitelist;
    std::vector<std::wstring> autoCleanList;

    bool load(const std::wstring& iniPath);
    void save(const std::wstring& iniPath) const;

    static std::wstring join(const std::vector<std::wstring>& names);
    static std::vector<std::wstring> split(const std::wstring& s);
};

// exe 所在目录（结尾不含反斜杠）
std::wstring exeDir();

// exe 同目录下的文件路径
std::wstring exeRelativePath(const std::wstring& fileName);

} // namespace fc

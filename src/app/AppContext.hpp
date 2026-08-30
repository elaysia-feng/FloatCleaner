#pragma once
#include <windows.h>

#include "../auto/AutoCleaner.hpp"
#include "../config/Config.hpp"
#include "../core/ProcessScanner.hpp"
#include "../core/ProtectionList.hpp"

namespace fc {

class FloatingBall;
class ProcessPanel;
class TrayIcon;

// 全局应用上下文：持有各模块，由 main.cpp 定义实例
struct AppContext {
    Config config;
    ProtectionList protection;
    ProcessScanner scanner;
    AutoCleaner autoCleaner;

    FloatingBall* ball = nullptr;
    ProcessPanel* panel = nullptr;
    TrayIcon* tray = nullptr;

    HICON appIcon = nullptr;
    std::wstring iniPath;
};

extern AppContext g_app;

// 跨窗口消息
constexpr UINT WM_APP_TRAY = WM_APP + 1;    // 托盘回调
constexpr UINT WM_APP_REFRESH = WM_APP + 2; // 数据刷新通知

// 公共菜单命令（悬浮球右键 / 托盘菜单共用）
constexpr int IDM_TOGGLE_PANEL = 3001;
constexpr int IDM_TOGGLE_AUTOCLEAN = 3002;
constexpr int IDM_EXIT = 3003;

// 弹出主菜单（悬浮球右键与托盘共用），main.cpp 中实现
void popupMainMenu(HWND anchor);

// 运行时绘制的应用图标
HICON createAppIcon(int size);

} // namespace fc

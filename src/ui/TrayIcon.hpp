#pragma once
#include <string>
#include <windows.h>

namespace fc {

// 系统托盘图标 + 气泡通知
class TrayIcon {
public:
    bool create(HWND host, UINT callbackMessage, HICON icon);
    void remove();
    void showBalloon(const std::wstring& title, const std::wstring& text);

private:
    HWND host_ = nullptr;
    UINT callbackMessage_ = 0;
    bool added_ = false;
};

} // namespace fc

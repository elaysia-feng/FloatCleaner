#include "TrayIcon.hpp"

namespace fc {

bool TrayIcon::create(HWND host, UINT callbackMessage, HICON icon)
{
    host_ = host;
    callbackMessage_ = callbackMessage;

    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = host;
    nid.uID = 1;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = callbackMessage;
    nid.hIcon = icon;
    wcscpy_s(nid.szTip, L"FloatCleaner - 进程清理悬浮窗");

    added_ = Shell_NotifyIconW(NIM_ADD, &nid) != FALSE;
    return added_;
}

void TrayIcon::remove()
{
    if (!added_)
        return;
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = host_;
    nid.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    added_ = false;
}

void TrayIcon::showBalloon(const std::wstring& title, const std::wstring& text)
{
    if (!added_)
        return;
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = host_;
    nid.uID = 1;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    wcsncpy_s(nid.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(nid.szInfo, text.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

} // namespace fc

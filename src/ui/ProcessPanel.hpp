#pragma once
#include <windows.h>

#include "DrawUtils.hpp"

namespace fc {

// 进程列表面板：多选手动结束 + 右键管理名单
class ProcessPanel {
public:
    bool create(HINSTANCE hInstance);
    void show();
    void hide();
    bool visible() const { return visible_; }
    HWND hwnd() const { return hwnd_; }

private:
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam);
    void onPaint();
    void onCreate();
    void onDrawItem(const DRAWITEMSTRUCT* dis);
    void onMeasureItem(MEASUREITEMSTRUCT* mis);
    void refreshList();
    void onCommand(int id, int notifyCode);
    void killSelected();
    void onListContextMenu(LPARAM lParam);
    void syncConfigFromProtection();

    HWND hwnd_ = nullptr;
    HWND listBox_ = nullptr;
    DrawUtils::CachedCanvas buffer_;
    bool visible_ = false;
};

} // namespace fc

#include "app/AppContext.hpp"
#include "config/defaults.hpp"
#include "ui/DrawUtils.hpp"
#include "ui/FloatingBall.hpp"
#include "ui/ProcessPanel.hpp"
#include "ui/Theme.hpp"
#include "ui/TrayIcon.hpp"

#include <commctrl.h>
#include <shellapi.h>

#include <algorithm>

namespace fc {

AppContext g_app;

void popupMainMenu(HWND anchor)
{
    HMENU menu = CreatePopupMenu();
    const bool panelVisible = g_app.panel && g_app.panel->visible();
    AppendMenuW(menu, MF_STRING, IDM_TOGGLE_PANEL,
                panelVisible ? L"收起进程面板" : L"打开进程面板");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (g_app.autoCleaner.enabled() ? MF_CHECKED : 0),
                IDM_TOGGLE_AUTOCLEAN, L"智能自动清理");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_EXIT, L"退出");

    SetForegroundWindow(anchor); // 保证菜单点击空白处能收起
    POINT pt;
    if (!GetCursorPos(&pt)) {
        pt = POINT{GetSystemMetrics(SM_CXSCREEN) - 220,
                   GetSystemMetrics(SM_CYSCREEN) - 220};
    }
    const int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0,
                                   anchor, nullptr);
    DestroyMenu(menu);

    switch (cmd) {
    case IDM_TOGGLE_PANEL:
        if (!g_app.panel)
            break;
        if (g_app.panel->visible())
            g_app.panel->hide();
        else
            g_app.panel->show();
        break;
    case IDM_TOGGLE_AUTOCLEAN:
        g_app.autoCleaner.setEnabled(!g_app.autoCleaner.enabled());
        g_app.config.autoCleanEnabled = g_app.autoCleaner.enabled();
        g_app.config.save(g_app.iniPath);
        if (g_app.ball)
            InvalidateRect(g_app.ball->hwnd(), nullptr, FALSE);
        break;
    case IDM_EXIT:
        DestroyWindow(anchor);
        break;
    default:
        break;
    }
}

// 运行时绘制应用图标：薄荷绿圆底 + 白色向下箭头（清理意象）
HICON createAppIcon(int size)
{
    HDC screen = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(screen);
    HBITMAP bmp = CreateCompatibleBitmap(screen, size, size);
    HBITMAP oldBmp = static_cast<HBITMAP>(SelectObject(mem, bmp));

    RECT rc{0, 0, size, size};
    DrawUtils::fillRect(mem, rc, RGB(0, 0, 0));

    HBRUSH bg = CreateSolidBrush(theme::ACCENT);
    HPEN pen = CreatePen(PS_SOLID, 1, theme::ACCENT);
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(mem, bg));
    HPEN oldPen = static_cast<HPEN>(SelectObject(mem, pen));
    Ellipse(mem, 1, 1, size - 1, size - 1);

    // 白色向下箭头
    const int cx = size / 2;
    const int stem = std::max(2, size / 8);
    POINT arrow[7] = {
        {cx - stem, size / 4},
        {cx + stem, size / 4},
        {cx + stem, size * 5 / 9},
        {cx + stem * 2, size * 5 / 9},
        {cx, size * 3 / 4},
        {cx - stem * 2, size * 5 / 9},
        {cx - stem, size * 5 / 9},
    };
    HBRUSH white = CreateSolidBrush(RGB(255, 255, 255));
    HPEN whitePen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    SelectObject(mem, white);
    SelectObject(mem, whitePen);
    Polygon(mem, arrow, 7);

    SelectObject(mem, oldBrush);
    SelectObject(mem, oldPen);
    DeleteObject(bg);
    DeleteObject(pen);
    DeleteObject(white);
    DeleteObject(whitePen);

    // 单色掩码：颜色区域不透明
    HBITMAP mask = CreateBitmap(size, size, 1, 1, nullptr);
    HDC maskDc = CreateCompatibleDC(screen);
    HBITMAP oldMask = static_cast<HBITMAP>(SelectObject(maskDc, mask));
    RECT all{0, 0, size, size};
    DrawUtils::fillRect(maskDc, all, RGB(255, 255, 255));
    HBRUSH black = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    SelectObject(maskDc, black);
    Ellipse(maskDc, 0, 0, size, size);
    SelectObject(maskDc, oldMask);
    DeleteDC(maskDc);

    ICONINFO ii{};
    ii.fIcon = TRUE;
    ii.hbmMask = mask;
    ii.hbmColor = bmp;
    HICON icon = CreateIconIndirect(&ii);

    DeleteObject(mask);
    SelectObject(mem, oldBmp);
    DeleteObject(bmp);
    DeleteDC(mem);
    ReleaseDC(nullptr, screen);
    return icon;
}

} // namespace fc

namespace {

int run(HINSTANCE hInstance)
{
    using namespace fc;
    using namespace fc::defaults;

    // 单实例
    HANDLE mutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"FloatCleaner 已在运行（请查看屏幕右下角悬浮球）。",
                    kAppTitle, MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    g_app.iniPath = exeRelativePath(kIniFile);
    g_app.config.load(g_app.iniPath);
    g_app.protection.init(g_app.config.whitelist, g_app.config.autoCleanList);
    g_app.autoCleaner.setup(&g_app.config, &g_app.protection,
                            [](const std::wstring& title, const std::wstring& text) {
                                if (g_app.tray)
                                    g_app.tray->showBalloon(title, text);
                            });

    static FloatingBall ball;
    static ProcessPanel panel;
    static TrayIcon tray;
    g_app.ball = &ball;
    g_app.panel = &panel;
    g_app.tray = &tray;
    g_app.appIcon = createAppIcon(32);

    if (!ball.create(hInstance)) {
        MessageBoxW(nullptr, L"悬浮球窗口创建失败。", kAppTitle,
                    MB_OK | MB_ICONERROR);
        return 1;
    }
    if (!panel.create(hInstance)) {
        MessageBoxW(nullptr, L"进程面板窗口创建失败。", kAppTitle,
                    MB_OK | MB_ICONERROR);
        return 1;
    }
    tray.create(ball.hwnd(), WM_APP_TRAY, g_app.appIcon);

    // 命令行带 --panel 时启动即展开面板（便于演示与调试）
    if (wcsstr(GetCommandLineW(), L"--panel"))
        panel.show();

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    tray.remove();
    if (mutex) {
        ReleaseMutex(mutex);
        CloseHandle(mutex);
    }
    return 0;
}

} // namespace

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int)
{
    return run(hInstance);
}

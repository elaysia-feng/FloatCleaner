#include "ProcessPanel.hpp"
#include "../app/AppContext.hpp"
#include "../config/defaults.hpp"
#include "../core/ProcessKiller.hpp"
#include "DrawUtils.hpp"
#include "FloatingBall.hpp"
#include "Theme.hpp"

#include <windowsx.h>

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace fc {
namespace {

constexpr int IDC_LIST = 100;
constexpr int IDC_BTN_KILL = 101;
constexpr int IDC_BTN_HIDE = 102;

constexpr int kHeaderH = 44;
constexpr int kFooterH = 56;
constexpr int kMargin = 10;

std::wstring levelTag(const ProcessInfo& p)
{
    switch (p.level) {
    case ProtectionLevel::System:
        return L"系统";
    case ProtectionLevel::UserProtected:
        return L"已保护";
    case ProtectionLevel::AutoClean:
        return L"自动清理";
    default:
        return L"";
    }
}

COLORREF levelTagColor(ProtectionLevel level)
{
    switch (level) {
    case ProtectionLevel::System:
    case ProtectionLevel::UserProtected:
        return theme::PROTECTED;
    case ProtectionLevel::AutoClean:
        return theme::WARN;
    default:
        return theme::TEXT_DIM;
    }
}

} // namespace

bool ProcessPanel::create(HINSTANCE hInstance)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    wc.hIconSm = wc.hIcon;
    wc.lpszClassName = L"FloatCleanerPanel";
    if (!RegisterClassExW(&wc))
        return false;

    hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, wc.lpszClassName,
                            defaults::kAppTitle, WS_POPUP | WS_CLIPSIBLINGS,
                            CW_USEDEFAULT, CW_USEDEFAULT, defaults::kPanelWidth,
                            defaults::kPanelHeight, nullptr, nullptr, hInstance,
                            nullptr);
    if (!hwnd_)
        return false;

    SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    HRGN rgn = CreateRoundRectRgn(0, 0, defaults::kPanelWidth + 1,
                                  defaults::kPanelHeight + 1,
                                  defaults::kPanelRadius * 2,
                                  defaults::kPanelRadius * 2);
    SetWindowRgn(hwnd_, rgn, FALSE);

    // 发送 WM_CREATE 时机已过，手动初始化子控件
    // （这里直接在 create 里建，避免依赖 WM_CREATE 时序）
    listBox_ = CreateWindowExW(0, L"LISTBOX", nullptr,
                               WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                                   LBS_OWNERDRAWFIXED | LBS_MULTIPLESEL |
                                   LBS_NOINTEGRALHEIGHT | LBS_NOTIFY,
                               kMargin, kHeaderH, defaults::kPanelWidth - kMargin * 2,
                               defaults::kPanelHeight - kHeaderH - kFooterH, hwnd_,
                               reinterpret_cast<HMENU>(IDC_LIST), hInstance, nullptr);
    if (!listBox_)
        return false;
    SendMessageW(listBox_, LB_SETITEMHEIGHT, 0, 44);

    CreateWindowExW(0, L"BUTTON", L"结束所选进程",
                    WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                    kMargin, defaults::kPanelHeight - kFooterH + 8, 150, 34, hwnd_,
                    reinterpret_cast<HMENU>(IDC_BTN_KILL), hInstance, nullptr);
    CreateWindowExW(0, L"BUTTON", L"收起",
                    WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                    defaults::kPanelWidth - kMargin - 80,
                    defaults::kPanelHeight - kFooterH + 8, 80, 34, hwnd_,
                    reinterpret_cast<HMENU>(IDC_BTN_HIDE), hInstance, nullptr);

    refreshList();
    return true;
}

void ProcessPanel::refreshList()
{
    if (!listBox_)
        return;
    SendMessageW(listBox_, LB_RESETCONTENT, 0, 0);
    const auto& procs = g_app.scanner.processes();
    for (size_t i = 0; i < procs.size(); ++i) {
        std::wstring label = procs[i].name;
        const int idx = SendMessageW(listBox_, LB_ADDSTRING, 0,
                                     reinterpret_cast<LPARAM>(label.c_str()));
        SendMessageW(listBox_, LB_SETITEMDATA, idx, static_cast<LPARAM>(i));
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void ProcessPanel::show()
{
    // 贴着悬浮球弹出，并保证完整落在屏幕内
    if (g_app.ball && g_app.ball->hwnd()) {
        RECT br;
        GetWindowRect(g_app.ball->hwnd(), &br);
        int x = br.right + 8;
        int y = br.top - 100;
        const int sw = GetSystemMetrics(SM_CXSCREEN);
        const int sh = GetSystemMetrics(SM_CYSCREEN);
        if (x + defaults::kPanelWidth > sw)
            x = br.left - defaults::kPanelWidth - 8;
        x = std::max(0, std::min(x, sw - defaults::kPanelWidth));
        y = std::max(0, std::min(y, sh - defaults::kPanelHeight));
        SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    refreshList();
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    visible_ = true;
}

void ProcessPanel::hide()
{
    ShowWindow(hwnd_, SW_HIDE);
    visible_ = false;
}

void ProcessPanel::onPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd_, &ps);

    RECT rc;
    GetClientRect(hwnd_, &rc);

    DrawUtils::CachedCanvas& buffer = buffer_;
    HDC mem = buffer.begin(hdc, rc.right, rc.bottom);

    DrawUtils::fillRoundRect(mem, rc, defaults::kPanelRadius, theme::BG);

    SetBkMode(mem, TRANSPARENT);
    static HFONT titleFont = DrawUtils::font(15, true);
    static HFONT smallFont = DrawUtils::font(11, false);

    SelectObject(mem, titleFont);
    SetTextColor(mem, theme::TEXT_MAIN);
    TextOutW(mem, kMargin + 4, 10, L"进程管理", 4);

    // 右上角：自动清理状态 + 关闭按钮
    const wchar_t* autoTag =
        g_app.autoCleaner.enabled() ? L"● 自动清理中" : L"○ 自动清理已停";
    SelectObject(mem, smallFont);
    SetTextColor(mem, g_app.autoCleaner.enabled() ? theme::ACCENT : theme::TEXT_DIM);
    TextOutW(mem, rc.right - 190, 14, autoTag, static_cast<int>(wcslen(autoTag)));

    SetTextColor(mem, theme::TEXT_DIM);
    const wchar_t* close = L"✕";
    TextOutW(mem, rc.right - 24, 12, close, 1);

    // 底部统计
    wchar_t stats[96] = {};
    const int count = static_cast<int>(g_app.scanner.processes().size());
    swprintf(stats, 96, L"共 %d 个进程", count);
    SelectObject(mem, smallFont);
    SetTextColor(mem, theme::TEXT_DIM);
    TextOutW(mem, kMargin + 4, rc.bottom - 16, stats,
             static_cast<int>(wcslen(stats)));

    buffer.commit(hdc, rc.right, rc.bottom);
    EndPaint(hwnd_, &ps);
}

void ProcessPanel::onMeasureItem(MEASUREITEMSTRUCT* mis)
{
    if (mis->CtlType == ODT_LISTBOX)
        mis->itemHeight = 44;
}

void ProcessPanel::onDrawItem(const DRAWITEMSTRUCT* dis)
{
    if (dis->CtlType == ODT_BUTTON) {
        const bool isKill = dis->CtlID == IDC_BTN_KILL;
        const bool pressed = dis->itemState & ODS_SELECTED;

        const int selCount = listBox_
                                 ? SendMessageW(listBox_, LB_GETSELCOUNT, 0, 0)
                                 : 0;
        COLORREF bg = isKill ? (selCount > 0 ? theme::DANGER : theme::BG_CARD)
                             : theme::BG_CARD;
        if (pressed)
            bg = theme::BG_HOVER;
        DrawUtils::fillRoundRect(dis->hDC, dis->rcItem, 8, bg);
        DrawUtils::outlineRoundRect(dis->hDC, dis->rcItem, 8, theme::BORDER);

        SetBkMode(dis->hDC, TRANSPARENT);
        wchar_t text[64] = {};
        GetWindowTextW(dis->hwndItem, text, 64);
        std::wstring label = text;
        if (isKill && selCount > 0)
            label = L"结束所选 (" + std::to_wstring(selCount) + L")";
        static HFONT btnFont = DrawUtils::font(13, true);
        SelectObject(dis->hDC, btnFont);
        SetTextColor(dis->hDC,
                     isKill ? (selCount > 0 ? theme::TEXT_MAIN : theme::TEXT_DIM)
                            : theme::TEXT_MAIN);
        SIZE sz{};
        GetTextExtentPoint32W(dis->hDC, label.c_str(),
                              static_cast<int>(label.size()), &sz);
        const int cx = (dis->rcItem.left + dis->rcItem.right - sz.cx) / 2;
        const int cy = (dis->rcItem.top + dis->rcItem.bottom - sz.cy) / 2;
        TextOutW(dis->hDC, cx, cy, label.c_str(), static_cast<int>(label.size()));
        return;
    }

    if (dis->CtlType != ODT_LISTBOX || dis->itemID == static_cast<UINT>(-1))
        return;

    const size_t idx = static_cast<size_t>(SendMessageW(listBox_, LB_GETITEMDATA,
                                                        dis->itemID, 0));
    const auto& procs = g_app.scanner.processes();
    if (idx >= procs.size())
        return;
    const ProcessInfo& p = procs[idx];

    const bool selected = SendMessageW(listBox_, LB_GETSEL, dis->itemID, 0) > 0;
    DrawUtils::fillRect(dis->hDC, dis->rcItem, theme::BG);
    if (selected)
        DrawUtils::fillRect(dis->hDC, dis->rcItem, theme::LIST_SEL);

    const int cy = (dis->rcItem.top + dis->rcItem.bottom) / 2;

    // 复选框（受保护进程画成锁定样式，不可勾选）
    RECT box{dis->rcItem.left + 10, cy - 7, dis->rcItem.left + 24, cy + 7};
    if (p.canTerminate) {
        DrawUtils::outlineRoundRect(dis->hDC, box, 3, theme::BORDER);
        if (selected)
            DrawUtils::fillRoundRect(dis->hDC, RECT{box.left + 3, box.top + 3,
                                                    box.right - 2, box.bottom - 2},
                                     2, theme::ACCENT);
    } else {
        DrawUtils::outlineRoundRect(dis->hDC, box, 3, theme::BORDER);
    }

    SetBkMode(dis->hDC, TRANSPARENT);
    static HFONT nameFont = DrawUtils::font(13, true);
    static HFONT tagFont = DrawUtils::font(10, false);
    static HFONT numFont = DrawUtils::font(12, false);

    // 进程名
    RECT nameRc{dis->rcItem.left + 32, cy - 10, dis->rcItem.right - 150,
                cy + 10};
    SelectObject(dis->hDC, nameFont);
    SetTextColor(dis->hDC, p.canTerminate ? theme::TEXT_MAIN : theme::PROTECTED);
    DrawTextW(dis->hDC, p.name.c_str(), static_cast<int>(p.name.size()), &nameRc,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // 保护等级标签
    const std::wstring tag = levelTag(p);
    if (!tag.empty()) {
        SelectObject(dis->hDC, tagFont);
        SetTextColor(dis->hDC, levelTagColor(p.level));
        RECT tagRc{nameRc.right - 64, cy - 9, nameRc.right + 10, cy + 9};
        DrawTextW(dis->hDC, tag.c_str(), static_cast<int>(tag.size()), &tagRc,
                  DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }

    // 内存
    wchar_t memText[32] = {};
    swprintf(memText, 32, L"%s",
             DrawUtils::formatBytes(p.workingSet).c_str());
    SelectObject(dis->hDC, numFont);
    SetTextColor(dis->hDC, theme::TEXT_MAIN);
    RECT memRc{dis->rcItem.right - 150, cy - 9, dis->rcItem.right - 84, cy + 9};
    DrawTextW(dis->hDC, memText, static_cast<int>(wcslen(memText)), &memRc,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    // CPU
    wchar_t cpuText[24] = {};
    swprintf(cpuText, 24, L"%.1f%%", p.cpuPercent);
    SetTextColor(dis->hDC, p.cpuPercent > 15.0 ? theme::WARN : theme::TEXT_DIM);
    RECT cpuRc{dis->rcItem.right - 76, cy - 9, dis->rcItem.right - 10, cy + 9};
    DrawTextW(dis->hDC, cpuText, static_cast<int>(wcslen(cpuText)), &cpuRc,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
}

void ProcessPanel::syncConfigFromProtection()
{
    g_app.config.whitelist = g_app.protection.whitelistVector();
    g_app.config.autoCleanList = g_app.protection.autoCleanVector();
    g_app.config.save(g_app.iniPath);
}

void ProcessPanel::killSelected()
{
    if (!listBox_)
        return;

    const int count = SendMessageW(listBox_, LB_GETSELCOUNT, 0, 0);
    if (count <= 0)
        return;

    std::vector<int> sel(static_cast<size_t>(count));
    SendMessageW(listBox_, LB_GETSELITEMS, count,
                 reinterpret_cast<LPARAM>(sel.data()));

    const auto& procs = g_app.scanner.processes();

    // 组装确认信息（受保护进程自动剔除）
    std::vector<const ProcessInfo*> targets;
    int skipped = 0;
    for (int i : sel) {
        const size_t idx = static_cast<size_t>(
            SendMessageW(listBox_, LB_GETITEMDATA, i, 0));
        if (idx >= procs.size())
            continue;
        if (procs[idx].canTerminate)
            targets.push_back(&procs[idx]);
        else
            ++skipped;
    }
    if (targets.empty()) {
        MessageBoxW(hwnd_, L"所选进程全部受保护，无法结束。", defaults::kAppTitle,
                    MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::wstring names;
    for (size_t i = 0; i < targets.size() && i < 10; ++i) {
        if (i)
            names += L"、";
        names += targets[i]->name;
    }
    if (targets.size() > 10)
        names += L" 等";
    std::wstring text = L"确定结束以下 " + std::to_wstring(targets.size()) +
                        L" 个进程吗？\r\n\r\n" + names;
    if (skipped > 0)
        text += L"\r\n\r\n（另有 " + std::to_wstring(skipped) +
                L" 个受保护进程已自动跳过）";
    if (MessageBoxW(hwnd_, text.c_str(), L"确认结束进程",
                    MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
        return;

    int okCount = 0;
    std::wstring failures;
    for (const ProcessInfo* p : targets) {
        const KillResult r = terminateProcessById(p->pid, p->name, p->level);
        if (r.ok)
            ++okCount;
        else if (failures.size() < 400)
            failures += (failures.empty() ? L"" : L"\r\n") + r.message;
    }

    g_app.scanner.refresh(g_app.protection);
    refreshList();
    InvalidateRect(g_app.ball ? g_app.ball->hwnd() : nullptr, nullptr, FALSE);

    if (!failures.empty())
        MessageBoxW(hwnd_, failures.c_str(), L"部分进程结束失败",
                    MB_OK | MB_ICONWARNING);
}

void ProcessPanel::onListContextMenu(LPARAM lParam)
{
    if (!listBox_)
        return;

    POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    if (pt.x == -1 && pt.y == -1) {
        // 键盘触发：使用当前选中项
        const int sel = SendMessageW(listBox_, LB_GETCARETINDEX, 0, 0);
        if (sel < 0)
            return;
        RECT rc;
        SendMessageW(listBox_, LB_GETITEMRECT, sel, reinterpret_cast<LPARAM>(&rc));
        POINT itemPt{rc.left + 20, (rc.top + rc.bottom) / 2};
        ClientToScreen(listBox_, &itemPt);
        pt = itemPt;
    } else {
        ScreenToClient(listBox_, &pt);
    }

    // 命中项（HIWORD 非零表示点在客户区外）
    const int hit = static_cast<int>(SendMessageW(listBox_, LB_ITEMFROMPOINT, 0,
                                                  MAKELPARAM(pt.x, pt.y)));
    if (HIWORD(hit))
        return;
    const int index = LOWORD(hit);
    if (index < 0)
        return;

    // 右键同时选中该行（若可结束）
    const size_t idx = static_cast<size_t>(
        SendMessageW(listBox_, LB_GETITEMDATA, index, 0));
    const auto& procs = g_app.scanner.processes();
    if (idx >= procs.size())
        return;
    const ProcessInfo& p = procs[idx];

    // 让右键行的复选框勾上（多选语义）
    if (p.canTerminate)
        SendMessageW(listBox_, LB_SETSEL, TRUE, index);

    POINT screenPt = pt;
    ClientToScreen(listBox_, &screenPt);

    HMENU menu = CreatePopupMenu();
    enum {
        MI_KILL = 1,
        MI_ADD_WHITE = 2,
        MI_REMOVE_WHITE = 3,
        MI_ADD_AUTO = 4,
        MI_REMOVE_AUTO = 5,
    };

    if (p.canTerminate)
        AppendMenuW(menu, MF_STRING, MI_KILL, L"结束进程");
    else
        AppendMenuW(menu, MF_STRING | MF_GRAYED, MI_KILL, L"结束进程（受保护）");

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    if (g_app.protection.isWhitelisted(p.name)) {
        AppendMenuW(menu, MF_STRING, MI_REMOVE_WHITE, L"从白名单移除");
    } else {
        AppendMenuW(menu, MF_STRING, MI_ADD_WHITE, L"加入白名单（永不结束）");
    }
    if (g_app.protection.inAutoClean(p.name)) {
        AppendMenuW(menu, MF_STRING, MI_REMOVE_AUTO, L"从自动清理名单移除");
    } else {
        AppendMenuW(menu, MF_STRING, MI_ADD_AUTO, L"加入自动清理名单");
    }

    const int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY,
                                   screenPt.x, screenPt.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);

    switch (cmd) {
    case MI_KILL: {
        const KillResult r = terminateProcessById(p.pid, p.name, p.level);
        if (!r.ok)
            MessageBoxW(hwnd_, r.message.c_str(), L"结束进程失败",
                        MB_OK | MB_ICONWARNING);
        break;
    }
    case MI_ADD_WHITE:
        g_app.protection.addToWhitelist(p.name);
        g_app.protection.removeFromAutoClean(p.name);
        syncConfigFromProtection();
        break;
    case MI_REMOVE_WHITE:
        g_app.protection.removeFromWhitelist(p.name);
        syncConfigFromProtection();
        break;
    case MI_ADD_AUTO:
        g_app.protection.addToAutoClean(p.name);
        g_app.protection.removeFromWhitelist(p.name);
        syncConfigFromProtection();
        break;
    case MI_REMOVE_AUTO:
        g_app.protection.removeFromAutoClean(p.name);
        syncConfigFromProtection();
        break;
    default:
        return;
    }

    // 重新分类并刷新
    g_app.scanner.refresh(g_app.protection);
    refreshList();
    InvalidateRect(g_app.ball ? g_app.ball->hwnd() : nullptr, nullptr, FALSE);
}

void ProcessPanel::onCommand(int id, int notifyCode)
{
    if (id == IDC_BTN_KILL && notifyCode == BN_CLICKED)
        killSelected();
    else if (id == IDC_BTN_HIDE && notifyCode == BN_CLICKED)
        hide();
}

LRESULT CALLBACK ProcessPanel::wndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                       LPARAM lParam)
{
    auto* self =
        reinterpret_cast<ProcessPanel*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_PAINT:
        if (self)
            self->onPaint();
        return 0;
    case WM_MEASUREITEM:
        if (self)
            self->onMeasureItem(reinterpret_cast<MEASUREITEMSTRUCT*>(lParam));
        return TRUE;
    case WM_DRAWITEM:
        if (self)
            self->onDrawItem(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
        return TRUE;
    case WM_COMMAND:
        if (self)
            self->onCommand(LOWORD(wParam), HIWORD(wParam));
        return 0;
    case WM_APP_REFRESH:
        if (self)
            self->refreshList();
        return 0;
    case WM_CONTEXTMENU:
        if (self && reinterpret_cast<HWND>(wParam) == self->listBox_)
            self->onListContextMenu(lParam);
        return 0;
    case WM_LBUTTONDOWN: {
        // 点击标题栏"✕"关闭
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);
        RECT rc;
        GetClientRect(hwnd, &rc);
        if (x > rc.right - 36 && y < 32 && self)
            self->hide();
        else if (self)
            PostMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0); // 标题栏拖动
        return 0;
    }
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkColor(hdc, theme::BG);
        SetTextColor(hdc, theme::TEXT_MAIN);
        static HBRUSH bgBrush = CreateSolidBrush(theme::BG);
        return reinterpret_cast<LRESULT>(bgBrush);
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        if (self)
            self->buffer_.release();
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace fc

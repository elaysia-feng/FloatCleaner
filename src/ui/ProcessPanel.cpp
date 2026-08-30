#include "ProcessPanel.hpp"
#include "../app/AppContext.hpp"
#include "../config/defaults.hpp"
#include "../core/ProcessKiller.hpp"
#include "DrawUtils.hpp"
#include "FloatingBall.hpp"
#include "Theme.hpp"

#include <windowsx.h>
#include <commctrl.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace fc {
namespace {

constexpr int IDC_LIST = 100;
constexpr int IDC_SEARCH = 103;
constexpr int IDC_BTN_KILL = 101;
constexpr int IDC_BTN_HIDE = 102;

constexpr int kHeaderH = 44;
constexpr int kSearchH = 26;   // 搜索框行高
constexpr int kFooterH = 56;
constexpr int kMargin = 10;
constexpr int kGroupRowH = 38;
constexpr int kChildRowH = 26;
constexpr UINT_PTR kAnimTimer = 4;     // 滑入动画（150ms，结束即杀）
constexpr int kCardPad = 4;            // 列表卡片与 listbox 的间隙
constexpr UINT WM_APP_HOVER = WM_APP + 3; // 按钮子类 → 面板 hover 通知

// listbox 子类化：滚动后让父窗口重绘自绘滚动条
LRESULT CALLBACK listSubclassProc(HWND h, UINT msg, WPARAM w, LPARAM l,
                                  UINT_PTR, DWORD_PTR ref)
{
    LRESULT r = DefSubclassProc(h, msg, w, l);
    if (msg == WM_MOUSEWHEEL || msg == WM_KEYDOWN || msg == WM_VSCROLL)
        InvalidateRect(reinterpret_cast<HWND>(ref), nullptr, FALSE);
    return r;
}

// ===== 看板娘「夜桜ユイ」Q 版头像 =====
// 纯 GDI 保底方案（design/mascot-design.md 第四节），96 逻辑坐标经视口缩放绘制。
// 发色/瞳色/呆毛取自当前主题，切换主题时头像同步变色。
void drawChibi(HDC hdc, int x, int y, int size)
{
    const COLORREF hair = theme::pal().ACCENT;
    const COLORREF hairTip = theme::pal().LAVENDER;
    const COLORREF eye = theme::pal().SKY;
    const COLORREF eyeDeep = RGB(58, 120, 200);
    const COLORREF outline = RGB(70, 56, 110);
    const COLORREF skin = RGB(255, 236, 226);
    const COLORREF blush = RGB(255, 170, 196);
    const COLORREF white = RGB(255, 255, 255);

    const int saved = SaveDC(hdc);
    SetMapMode(hdc, MM_ANISOTROPIC);
    SetWindowExtEx(hdc, 96, 96, nullptr);
    SetViewportExtEx(hdc, size, size, nullptr);
    SetViewportOrgEx(hdc, x, y, nullptr);

    // 实心椭圆辅助（画笔同色，避免默认黑描边）
    auto fillE = [&](int l, int t, int r, int b, COLORREF c) {
        HBRUSH brush = CreateSolidBrush(c);
        HPEN pen = CreatePen(PS_SOLID, 1, c);
        HBRUSH ob = static_cast<HBRUSH>(SelectObject(hdc, brush));
        HPEN op = static_cast<HPEN>(SelectObject(hdc, pen));
        Ellipse(hdc, l, t, r, b);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(brush);
        DeleteObject(pen);
    };

    // 1. 后发主体 + 渐变发梢
    fillE(10, 12, 86, 92, hair);
    fillE(16, 66, 80, 94, hairTip);
    // 2. 两侧垂发 + 发梢
    fillE(10, 30, 30, 88, hair);
    fillE(10, 64, 30, 92, hairTip);
    fillE(66, 30, 86, 88, hair);
    fillE(66, 64, 86, 92, hairTip);
    // 3. 脸
    {
        HBRUSH brush = CreateSolidBrush(skin);
        HPEN pen = CreatePen(PS_SOLID, 2, outline);
        HBRUSH ob = static_cast<HBRUSH>(SelectObject(hdc, brush));
        HPEN op = static_cast<HPEN>(SelectObject(hdc, pen));
        Ellipse(hdc, 20, 26, 76, 82);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(brush);
        DeleteObject(pen);
    }
    // 4. 天蓝大眼（上深下浅 + 星形高光）
    fillE(30, 48, 40, 62, eye);
    fillE(30, 48, 40, 55, eyeDeep);
    fillE(56, 48, 66, 62, eye);
    fillE(56, 48, 66, 55, eyeDeep);
    DrawUtils::drawStar(hdc, 34, 53, 3, white);
    fillE(61, 57, 64, 60, white);
    // 6. 腮红
    fillE(24, 60, 34, 66, blush);
    fillE(62, 60, 72, 66, blush);
    // 7. 刘海：波浪圆齿 + 弧形发际线（关键辨识层）
    {
        POINT pts[16] = {{20, 44},  {24, 52},  {30, 46},  {36, 52},
                         {42, 46},  {48, 52},  {54, 46},  {60, 52},
                         {66, 46},  {72, 52},  {76, 44},  {76, 32},
                         {62, 20},  {48, 16},  {34, 20},  {20, 32}};
        HBRUSH brush = CreateSolidBrush(hair);
        HPEN pen = CreatePen(PS_SOLID, 2, outline);
        HBRUSH ob = static_cast<HBRUSH>(SelectObject(hdc, brush));
        HPEN op = static_cast<HPEN>(SelectObject(hdc, pen));
        Polygon(hdc, pts, 16);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(brush);
        DeleteObject(pen);
        // 中间一缕挑染
        fillE(46, 20, 50, 44, hairTip);
    }
    // 8. 呆毛（翘起的"占用率曲线"）
    {
        POINT pts[4] = {{44, 20}, {52, 20}, {58, 4}, {49, 11}};
        HBRUSH brush = CreateSolidBrush(hair);
        HPEN pen = CreatePen(PS_SOLID, 2, outline);
        HBRUSH ob = static_cast<HBRUSH>(SelectObject(hdc, brush));
        HPEN op = static_cast<HPEN>(SelectObject(hdc, pen));
        Polygon(hdc, pts, 4);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(brush);
        DeleteObject(pen);
    }
    // 9. 樱花发饰（左侧鬓发）
    for (int i = 0; i < 5; ++i) {
        const double a = i * 6.283185307 / 5.0;
        const int cx = 22 + static_cast<int>(5 * cos(a));
        const int cyy = 40 + static_cast<int>(5 * sin(a));
        fillE(cx - 3, cyy - 3, cx + 3, cyy + 3, RGB(255, 190, 214));
    }
    fillE(20, 38, 24, 42, RGB(255, 214, 120));
    // 10. 悬浮球彩蛋（右下角小玻璃球）
    fillE(74, 74, 88, 88, eye);
    fillE(78, 78, 83, 83, white);

    RestoreDC(hdc, saved);
}

// 编辑框子类化（占位：保留扩展点）
LRESULT CALLBACK editSubclassProc(HWND h, UINT msg, WPARAM w, LPARAM l,
                                  UINT_PTR, DWORD_PTR)
{
    return DefSubclassProc(h, msg, w, l);
}

// owner-draw 按钮子类化：转发 hover 状态给面板
LRESULT CALLBACK buttonSubclassProc(HWND h, UINT msg, WPARAM w, LPARAM l,
                                    UINT_PTR, DWORD_PTR ref)
{
    if (msg == WM_MOUSEMOVE) {
        TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, h, 0};
        TrackMouseEvent(&tme);
        SendMessageW(reinterpret_cast<HWND>(ref), WM_APP_HOVER, GetDlgCtrlID(h),
                     0);
    } else if (msg == WM_MOUSELEAVE) {
        SendMessageW(reinterpret_cast<HWND>(ref), WM_APP_HOVER, 0, 0);
    }
    return DefSubclassProc(h, msg, w, l);
}

const ProcessInfo* findProc(const std::unordered_map<uint32_t, size_t>& pidIndex,
                            uint32_t pid)
{
    auto it = pidIndex.find(pid);
    if (it == pidIndex.end())
        return nullptr;
    const auto& procs = g_app.scanner.processes();
    return it->second < procs.size() ? &procs[it->second] : nullptr;
}

// 行的可结束性：组行 = 组内存在可结束成员
bool rowCanTerminate(const PanelRow& row,
                     const std::unordered_map<uint32_t, size_t>& pidIndex)
{
    for (uint32_t pid : row.pids) {
        const ProcessInfo* p = findProc(pidIndex, pid);
        if (p && p->canTerminate)
            return true;
    }
    return false;
}

uint64_t rowMemory(const PanelRow& row,
                   const std::unordered_map<uint32_t, size_t>& pidIndex)
{
    uint64_t total = 0;
    for (uint32_t pid : row.pids) {
        const ProcessInfo* p = findProc(pidIndex, pid);
        if (p)
            total += p->workingSet;
    }
    return total;
}

double rowCpu(const PanelRow& row,
              const std::unordered_map<uint32_t, size_t>& pidIndex)
{
    double total = 0;
    for (uint32_t pid : row.pids) {
        const ProcessInfo* p = findProc(pidIndex, pid);
        if (p)
            total += p->cpuPercent;
    }
    return total;
}

// 行的标记：全保护=系统/已保护；组内含自动清理名单=自动清理
std::wstring rowTag(const PanelRow& row,
                    const std::unordered_map<uint32_t, size_t>& pidIndex)
{
    bool allProtected = true;
    bool hasAuto = false;
    for (uint32_t pid : row.pids) {
        const ProcessInfo* p = findProc(pidIndex, pid);
        if (!p)
            continue;
        if (p->canTerminate)
            allProtected = false;
        if (p->level == ProtectionLevel::AutoClean)
            hasAuto = true;
    }
    if (hasAuto)
        return L"自动清理";
    if (allProtected)
        return L"系统";
    return L"";
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

    // 搜索框（左标签 + 输入框，EN_CHANGE 实时过滤）
    HWND searchLabel = CreateWindowExW(0, L"STATIC", L"搜索",
                                       WS_CHILD | WS_VISIBLE | SS_CENTER,
                                       kMargin + 6, kHeaderH + 4, 34, 18, hwnd_,
                                       nullptr, hInstance, nullptr);
    SendMessageW(searchLabel, WM_SETFONT,
                 reinterpret_cast<WPARAM>(DrawUtils::font(11, false)), TRUE);
    editBox_ = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", nullptr,
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        kMargin + 46, kHeaderH + 2,
        defaults::kPanelWidth - 2 * (kMargin + 4) - 10 - 42, 22, hwnd_,
        reinterpret_cast<HMENU>(IDC_SEARCH), hInstance, nullptr);
    if (!editBox_)
        return false;
    SendMessageW(editBox_, WM_SETFONT,
                 reinterpret_cast<WPARAM>(DrawUtils::font(12, false)), TRUE);
    SetWindowSubclass(editBox_, editSubclassProc, 1,
                      reinterpret_cast<DWORD_PTR>(hwnd_));

    // 列表卡片由 onPaint 绘制在父窗口上；listbox 内缩融入卡片，滚动条自绘
    listBox_ = CreateWindowExW(
        0, L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | LBS_OWNERDRAWVARIABLE | LBS_MULTIPLESEL |
            LBS_NOINTEGRALHEIGHT | LBS_NOTIFY,
        kMargin + 4, kHeaderH + kSearchH - 2,
        defaults::kPanelWidth - 2 * (kMargin + 4) - 10,
        defaults::kPanelHeight - kHeaderH - kSearchH - kFooterH + 4, hwnd_,
        reinterpret_cast<HMENU>(IDC_LIST), hInstance, nullptr);
    if (!listBox_)
        return false;
    SetWindowSubclass(listBox_, listSubclassProc, 1,
                      reinterpret_cast<DWORD_PTR>(hwnd_));

    HWND btnKill = CreateWindowExW(0, L"BUTTON", L"结束所选进程",
                                   WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, kMargin,
                                   defaults::kPanelHeight - kFooterH + 8, 150, 34,
                                   hwnd_, reinterpret_cast<HMENU>(IDC_BTN_KILL),
                                   hInstance, nullptr);
    HWND btnHide = CreateWindowExW(0, L"BUTTON", L"收起",
                                   WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                   defaults::kPanelWidth - kMargin - 80,
                                   defaults::kPanelHeight - kFooterH + 8, 80, 34,
                                   hwnd_, reinterpret_cast<HMENU>(IDC_BTN_HIDE),
                                   hInstance, nullptr);
    if (btnKill)
        SetWindowSubclass(btnKill, buttonSubclassProc, 1,
                          reinterpret_cast<DWORD_PTR>(hwnd_));
    if (btnHide)
        SetWindowSubclass(btnHide, buttonSubclassProc, 2,
                          reinterpret_cast<DWORD_PTR>(hwnd_));

    refreshData();
    return true;
}

void ProcessPanel::refreshData()
{
    if (!listBox_)
        return;

    // pid -> procs 下标索引（每次刷新重建，供绘制与操作查找）
    pidIndex_.clear();
    const auto& procs = g_app.scanner.processes();
    for (size_t i = 0; i < procs.size(); ++i)
        pidIndex_[procs[i].pid] = i;

    // 结构签名：pid 集合。不变则只重绘数值（不重排、不动滚动）
    std::vector<uint32_t> pids;
    pids.reserve(procs.size());
    for (const auto& p : procs)
        pids.push_back(p.pid);
    std::sort(pids.begin(), pids.end());

    if (rows_.empty() || pids != lastPids_ || filter_ != lastFilter_) {
        rebuildRows();
        lastPids_ = std::move(pids);
        lastFilter_ = filter_;
    } else {
        // 结构未变：只重绘数值，保持行序与滚动位置
        InvalidateRect(listBox_, nullptr, FALSE);
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void ProcessPanel::rebuildRows()
{
    // ---- 保存现场：滚动位置、勾选 pid、展开状态 ----
    const int topIndex = listBox_
                             ? static_cast<int>(SendMessageW(listBox_, LB_GETTOPINDEX,
                                                             0, 0))
                             : 0;
    std::vector<uint32_t> selectedPids;
    if (listBox_) {
        const int selCount = SendMessageW(listBox_, LB_GETSELCOUNT, 0, 0);
        if (selCount > 0) {
            std::vector<int> sel(static_cast<size_t>(selCount));
            SendMessageW(listBox_, LB_GETSELITEMS, selCount,
                         reinterpret_cast<LPARAM>(sel.data()));
            for (int i : sel) {
                if (i < 0 || static_cast<size_t>(i) >= rows_.size())
                    continue;
                for (uint32_t pid : rows_[static_cast<size_t>(i)].pids)
                    selectedPids.push_back(pid);
            }
        }
    }
    expandedNames_.clear();
    for (const PanelRow& r : rows_)
        if (r.isGroup && r.expanded)
            expandedNames_.insert(r.name);

    // ---- 按应用（同名 exe）分组，组按合计内存降序 ----
    // 搜索过滤：组名不含关键词的整组跳过
    std::map<std::wstring, std::vector<uint32_t>> groups;
    for (const auto& p : g_app.scanner.processes()) {
        if (!filter_.empty()) {
            std::wstring lower = p.name;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](wchar_t c) { return static_cast<wchar_t>(::towlower(c)); });
            if (lower.find(filter_) == std::wstring::npos)
                continue;
        }
        groups[p.name].push_back(p.pid);
    }

    struct GroupData {
        std::wstring name;
        std::vector<uint32_t> pids;
        uint64_t mem = 0;
    };
    std::vector<GroupData> ordered;
    ordered.reserve(groups.size());
    for (auto& [name, pids] : groups) {
        uint64_t mem = 0;
        for (uint32_t pid : pids) {
            auto it = pidIndex_.find(pid);
            if (it != pidIndex_.end())
                mem += g_app.scanner.processes()[it->second].workingSet;
        }
        ordered.push_back(GroupData{name, pids, mem});
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const GroupData& a, const GroupData& b) {
                  return a.mem != b.mem ? a.mem > b.mem : a.name < b.name;
              });

    // ---- 生成行模型：组行 + （展开时）子行 ----
    rows_.clear();
    for (GroupData& g : ordered) {
        PanelRow row;
        row.isGroup = true;
        row.name = g.name;
        row.pids = g.pids;
        row.expanded = expandedNames_.count(g.name) > 0;
        rows_.push_back(row);
        if (row.expanded) {
            for (uint32_t pid : g.pids) {
                PanelRow child;
                child.isGroup = false;
                child.name = g.name;
                child.pids = {pid};
                rows_.push_back(std::move(child));
            }
        }
    }

    // ---- 重建 listbox 内容并恢复现场 ----
    if (!listBox_)
        return;
    SendMessageW(listBox_, LB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < rows_.size(); ++i) {
        SendMessageW(listBox_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L""));
        SendMessageW(listBox_, LB_SETITEMDATA, static_cast<WPARAM>(i),
                     static_cast<LPARAM>(i));
        SendMessageW(listBox_, LB_SETITEMHEIGHT, static_cast<WPARAM>(i),
                     MAKELPARAM(rows_[i].isGroup ? kGroupRowH : kChildRowH, 0));
    }

    // 恢复勾选：子行按 pid 精确恢复；组行按"全部成员都被勾选"恢复
    for (size_t i = 0; i < rows_.size(); ++i) {
        const PanelRow& r = rows_[i];
        bool select = !r.pids.empty();
        for (uint32_t pid : r.pids) {
            if (std::find(selectedPids.begin(), selectedPids.end(), pid) ==
                selectedPids.end()) {
                select = false;
                break;
            }
        }
        if (select)
            SendMessageW(listBox_, LB_SETSEL, TRUE, static_cast<WPARAM>(i));
    }

    const int maxTop = rows_.size() > 3 ? static_cast<int>(rows_.size() - 3) : 0;
    SendMessageW(listBox_, LB_SETTOPINDEX,
                 std::min(topIndex, maxTop) < 0 ? 0
                                                : std::min(topIndex, maxTop),
                 0);
}

void ProcessPanel::rescanAndResort()
{
    g_app.scanner.refresh(g_app.protection);
    lastPids_.clear(); // 强制下次刷新重建行模型（重新分组排序）
    refreshData();
}

void ProcessPanel::show()
{
    // 贴着悬浮球弹出，并保证完整落在屏幕内
    int finalX = CW_USEDEFAULT, finalY = CW_USEDEFAULT;
    int ballCenterX = -1;
    if (g_app.ball && g_app.ball->hwnd()) {
        RECT br;
        GetWindowRect(g_app.ball->hwnd(), &br);
        ballCenterX = (br.left + br.right) / 2;
        int x = br.right + 8;
        int y = br.top - 100;
        const int sw = GetSystemMetrics(SM_CXSCREEN);
        const int sh = GetSystemMetrics(SM_CYSCREEN);
        if (x + defaults::kPanelWidth > sw)
            x = br.left - defaults::kPanelWidth - 8;
        x = std::max(0, std::min(x, sw - defaults::kPanelWidth));
        y = std::max(0, std::min(y, sh - defaults::kPanelHeight));
        finalX = x;
        finalY = y;
    }
    rescanAndResort(); // 打开面板 = 重新扫描并按内存排序
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    if (finalX != CW_USEDEFAULT) {
        // 滑入动画：从悬浮球一侧偏移 16px 滑入（150ms ease-out，结束即杀定时器）
        animTo_ = POINT{finalX, finalY};
        animDir_ = ballCenterX > finalX + defaults::kPanelWidth / 2 ? 1 : -1;
        animStart_ = GetTickCount();
        animActive_ = true;
        SetWindowPos(hwnd_, nullptr, finalX + animDir_ * 16, finalY, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        SetTimer(hwnd_, kAnimTimer, 16, nullptr);
    }
    SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    visible_ = true;
}

void ProcessPanel::hide()
{
    if (animActive_) {
        KillTimer(hwnd_, kAnimTimer);
        animActive_ = false;
    }
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

    // 夜空渐变底（窗口圆角区域负责裁剪）
    DrawUtils::fillGradient(mem, rc, theme::pal().BG_TOP, theme::pal().BG_BOTTOM);

    // 标题栏：稍亮的夜空渐变带
    RECT header{0, 0, rc.right, kHeaderH};
    DrawUtils::fillGradient(mem, header, theme::pal().HEADER_TOP, theme::pal().HEADER_BOTTOM);

    // 标题栏右上静态花瓣点缀（预混暗色，若隐若现）
    DrawUtils::drawPetal(mem, rc.right - 66, 12, 5, theme::pal().PETAL_HEADER);
    DrawUtils::drawPetal(mem, rc.right - 82, 24, 4, theme::pal().PETAL_HEADER2);

    SetBkMode(mem, TRANSPARENT);
    static HFONT titleFont = DrawUtils::font(15, true);
    static HFONT smallFont = DrawUtils::font(11, false);

    // 标题：矢量星 + 文字（柔影）+ 看板娘ユイ探头
    SelectObject(mem, titleFont);
    DrawUtils::drawStar(mem, kMargin + 12, 22, 6, theme::pal().LAVENDER);
    DrawUtils::textWithShadow(mem, kMargin + 24, 12, L"进程管理",
                              theme::pal().TEXT_MAIN, theme::pal().TEXT_SHADOW);
    drawChibi(mem, rc.right - 152, 0, 44);

    // 自动清理状态（矢量爱心）
    SelectObject(mem, smallFont);
    DrawUtils::drawHeart(mem, rc.right - 202, 19, 5,
                         g_app.autoCleaner.enabled() ? theme::pal().ACCENT
                                                     : theme::pal().TEXT_DIM);
    SetTextColor(mem, g_app.autoCleaner.enabled() ? theme::pal().ACCENT
                                                  : theme::pal().TEXT_DIM);
    TextOutW(mem, rc.right - 192, 14, L"自动清理中", 5);

    // "↻" 重新扫描 + "✕" 关闭（矢量绘制，hover 高亮）
    const bool rescanHover = hoverZone_ == Zone::Rescan;
    const bool closeHover = hoverZone_ == Zone::Close;
    if (rescanHover)
        DrawUtils::fillRoundRect(mem, RECT{rc.right - 58, 8, rc.right - 36, 30}, 4,
                                 theme::pal().BG_HOVER);
    if (closeHover)
        DrawUtils::fillRoundRect(mem, RECT{rc.right - 32, 8, rc.right - 10, 30}, 4,
                                 theme::pal().BG_HOVER);
    DrawUtils::drawRefresh(mem, rc.right - 47, 19, 7,
                           rescanHover ? theme::pal().TEXT_MAIN : theme::pal().LAVENDER);
    DrawUtils::drawX(mem, rc.right - 21, 19, 5,
                     closeHover ? theme::pal().TEXT_MAIN : theme::pal().TEXT_DIM);

    // 列表卡片：圆角底色吸收 listbox 方角
    RECT card{kMargin, kHeaderH - 6, rc.right - kMargin,
              rc.bottom - kFooterH + 2};
    DrawUtils::fillRoundRect(mem, card, 10, theme::pal().BG);
    DrawUtils::outlineRoundRect(mem, card, 10, RGB(84, 70, 128));

    // 自绘滚动条（行高超出可视区才画）
    if (!rows_.empty() && listBox_) {
        LONG totalH = 0;
        for (const PanelRow& r : rows_)
            totalH += r.isGroup ? kGroupRowH : kChildRowH;
        RECT lbRc;
        GetClientRect(listBox_, &lbRc);
        MapWindowPoints(listBox_, hwnd_, reinterpret_cast<POINT*>(&lbRc), 2);
        const int visibleH = lbRc.bottom - lbRc.top;
        if (totalH > visibleH) {
            const int top = static_cast<int>(
                SendMessageW(listBox_, LB_GETTOPINDEX, 0, 0));
            LONG beforeTop = 0;
            for (int i = 0; i < top && i < static_cast<int>(rows_.size()); ++i)
                beforeTop += rows_[static_cast<size_t>(i)].isGroup ? kGroupRowH
                                                                   : kChildRowH;
            const int trackTop = lbRc.top + 2;
            const int trackH = visibleH - 4;
            const int thumbH =
                std::max(20, static_cast<int>(visibleH * visibleH / (totalH > 0 ? totalH : 1)));
            const int scrollable = totalH - visibleH;
            const int thumbY =
                trackTop + (scrollable > 0
                                ? beforeTop * (trackH - thumbH) / scrollable
                                : 0);
            DrawUtils::fillRoundRect(
                mem, RECT{lbRc.right + 2, trackTop, lbRc.right + 6,
                          lbRc.bottom - 2},
                2, theme::pal().SCROLL_TRACK);
            DrawUtils::fillRoundRect(
                mem, RECT{lbRc.right + 1, thumbY, lbRc.right + 7, thumbY + thumbH},
                3, theme::pal().SCROLL_THUMB);
        }
    }

    // 底部统计与操作提示（矢量星 + 文字）
    size_t groupCount = 0;
    for (const PanelRow& r : rows_)
        if (r.isGroup)
            ++groupCount;
    wchar_t stats[128] = {};
    swprintf(stats, 128, L"%zu 个应用 · %zu 个进程 · 双击组行展开", groupCount,
             g_app.scanner.processes().size());
    SelectObject(mem, smallFont);
    DrawUtils::drawStar(mem, kMargin + 8, rc.bottom - 11, 3, theme::pal().LAVENDER);
    SetTextColor(mem, theme::pal().TEXT_DIM);
    TextOutW(mem, kMargin + 16, rc.bottom - 18, stats,
             static_cast<int>(wcslen(stats)));

    buffer.commit(hdc, rc.right, rc.bottom);
    EndPaint(hwnd_, &ps);
}

void ProcessPanel::onMeasureItem(MEASUREITEMSTRUCT* mis)
{
    if (mis->CtlType != ODT_LISTBOX)
        return;
    if (mis->itemID < rows_.size())
        mis->itemHeight = rows_[mis->itemID].isGroup ? kGroupRowH : kChildRowH;
    else
        mis->itemHeight = kGroupRowH;
}

void ProcessPanel::onDrawItem(const DRAWITEMSTRUCT* dis)
{
    if (dis->CtlType == ODT_BUTTON) {
        const bool isKill = dis->CtlID == IDC_BTN_KILL;
        const bool pressed = dis->itemState & ODS_SELECTED;
        const bool hover = isKill ? hoverZone_ == Zone::Kill
                                  : hoverZone_ == Zone::Hide;

        const int selCount = listBox_
                                 ? SendMessageW(listBox_, LB_GETSELCOUNT, 0, 0)
                                 : 0;
        if (isKill) {
            // 结束按钮：樱粉渐变（有选中时点亮），渐变用圆角区域裁剪避免方角溢出
            if (selCount > 0) {
                const COLORREF l = pressed  ? RGB(255, 120, 168)
                                   : hover  ? RGB(255, 150, 190)
                                            : theme::pal().ACCENT;
                const COLORREF r = pressed  ? RGB(214, 84, 130)
                                   : hover  ? RGB(236, 110, 158)
                                            : theme::pal().ACCENT_DEEP;
                HRGN rgn = CreateRoundRectRgn(dis->rcItem.left, dis->rcItem.top,
                                              dis->rcItem.right + 1,
                                              dis->rcItem.bottom + 1, 16, 16);
                SelectClipRgn(dis->hDC, rgn);
                DrawUtils::fillGradientH(dis->hDC, dis->rcItem, l, r);
                SelectClipRgn(dis->hDC, nullptr);
                DeleteObject(rgn);
            } else {
                DrawUtils::fillRoundRect(dis->hDC, dis->rcItem, 8, theme::pal().BG_CARD);
            }
        } else {
            DrawUtils::fillRoundRect(dis->hDC, dis->rcItem, 8,
                                     (pressed || hover) ? theme::pal().BG_HOVER
                                                        : theme::pal().BG_CARD);
        }
        DrawUtils::outlineRoundRect(dis->hDC, dis->rcItem, 8, theme::pal().BORDER);

        SetBkMode(dis->hDC, TRANSPARENT);
        wchar_t text[64] = {};
        GetWindowTextW(dis->hwndItem, text, 64);
        std::wstring label = text;
        if (isKill && selCount > 0) {
            label = L"结束所选 (" + std::to_wstring(selCount) + L")";
        }
        static HFONT btnFont = DrawUtils::font(13, true);
        SelectObject(dis->hDC, btnFont);
        SetTextColor(dis->hDC,
                     isKill ? (selCount > 0 ? theme::pal().TEXT_MAIN : theme::pal().TEXT_DIM)
                            : theme::pal().TEXT_MAIN);
        SIZE sz{};
        GetTextExtentPoint32W(dis->hDC, label.c_str(),
                              static_cast<int>(label.size()), &sz);
        int cx = (dis->rcItem.left + dis->rcItem.right - sz.cx) / 2;
        const int cy = (dis->rcItem.top + dis->rcItem.bottom - sz.cy) / 2;
        if (isKill && selCount > 0) {
            // 矢量爱心徽标 + 文字
            DrawUtils::drawHeart(dis->hDC, cx - 8, cy + sz.cy / 2, 6,
                                 theme::pal().TEXT_MAIN);
            TextOutW(dis->hDC, cx, cy, label.c_str(),
                     static_cast<int>(label.size()));
        } else {
            TextOutW(dis->hDC, cx, cy, label.c_str(),
                     static_cast<int>(label.size()));
        }
        return;
    }

    if (dis->CtlType != ODT_LISTBOX || dis->itemID == static_cast<UINT>(-1))
        return;
    if (dis->itemID >= rows_.size())
        return;
    const PanelRow& row = rows_[dis->itemID];

    const bool selected = SendMessageW(listBox_, LB_GETSEL, dis->itemID, 0) > 0;
    const bool canKill = rowCanTerminate(row, pidIndex_);
    const int cy = (dis->rcItem.top + dis->rcItem.bottom) / 2;

    // 行底色：受保护行微暗一档；选中 = 内嵌圆角块 + 左侧樱粉指示条
    DrawUtils::fillRect(dis->hDC, dis->rcItem,
                        canKill ? theme::pal().BG : theme::pal().ROW_PROTECT);
    if (selected) {
        DrawUtils::fillRoundRect(dis->hDC,
                                 RECT{dis->rcItem.left + 2, dis->rcItem.top + 1,
                                      dis->rcItem.right - 2,
                                      dis->rcItem.bottom},
                                 6, theme::pal().LIST_SEL);
        DrawUtils::fillRoundRect(dis->hDC,
                                 RECT{dis->rcItem.left + 2, dis->rcItem.top + 3,
                                      dis->rcItem.left + 5,
                                      dis->rcItem.bottom - 4},
                                 1, canKill ? theme::pal().ACCENT : theme::pal().PROTECTED);
    }
    // 行分隔线
    DrawUtils::fillRect(dis->hDC,
                        RECT{dis->rcItem.left + 8, dis->rcItem.bottom - 1,
                             dis->rcItem.right - 8, dis->rcItem.bottom},
                        theme::pal().ROW_SEP);

    SetBkMode(dis->hDC, TRANSPARENT);
    static HFONT nameFont = DrawUtils::font(13, true);
    static HFONT childFont = DrawUtils::font(11, false);
    static HFONT tagFont = DrawUtils::font(10, false);
    static HFONT numFont = DrawUtils::font(12, false);

    // 右侧数值区：内存 + CPU
    wchar_t memText[32] = {};
    swprintf(memText, 32, L"%ls", DrawUtils::formatBytes(rowMemory(row, pidIndex_)).c_str());
    wchar_t cpuText[24] = {};
    swprintf(cpuText, 24, L"%.1f%%", rowCpu(row, pidIndex_));

    if (row.isGroup) {
        // ---- 组行：选中圆点 + 矢量展开箭头 + 应用名 ×N + 合计数值 ----
        // 选中指示：樱粉圆点（与展开箭头分离，避免与复选框式样争位）
        if (selected) {
            HBRUSH db = CreateSolidBrush(canKill ? theme::pal().ACCENT : theme::pal().PROTECTED);
            HPEN dp = CreatePen(PS_SOLID, 1, canKill ? theme::pal().ACCENT_DEEP
                                                     : theme::pal().PROTECTED);
            HBRUSH ob = static_cast<HBRUSH>(SelectObject(dis->hDC, db));
            HPEN op = static_cast<HPEN>(SelectObject(dis->hDC, dp));
            Ellipse(dis->hDC, dis->rcItem.left + 8, cy - 5, dis->rcItem.left + 18,
                    cy + 5);
            SelectObject(dis->hDC, ob);
            SelectObject(dis->hDC, op);
            DeleteObject(db);
            DeleteObject(dp);
        }

        // 矢量展开箭头（比字体字形锐利，无字体回退风险）
        DrawUtils::drawTriangle(dis->hDC, dis->rcItem.left + 28, cy, 4,
                                row.expanded, theme::pal().TEXT_DIM);

        wchar_t nameText[256] = {};
        if (row.pids.size() > 1)
            swprintf(nameText, 256, L"%ls ×%zu", row.name.c_str(), row.pids.size());
        else
            swprintf(nameText, 256, L"%ls", row.name.c_str());

        RECT nameRc{dis->rcItem.left + 38, cy - 10, dis->rcItem.right - 150,
                    cy + 10};
        SelectObject(dis->hDC, nameFont);
        SetTextColor(dis->hDC, canKill ? theme::pal().TEXT_MAIN : theme::pal().PROTECTED);
        DrawTextW(dis->hDC, nameText, static_cast<int>(wcslen(nameText)), &nameRc,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        // 标签 pill 化（预混底色徽章，右缘贴数值区）
        const std::wstring tag = rowTag(row, pidIndex_);
        if (!tag.empty()) {
            SelectObject(dis->hDC, tagFont);
            SIZE tsz{};
            GetTextExtentPoint32W(dis->hDC, tag.c_str(),
                                  static_cast<int>(tag.size()), &tsz);
            const bool isAuto = tag == L"自动清理";
            RECT pill{nameRc.right - tsz.cx - 18, cy - 8, nameRc.right - 8,
                      cy + 8};
            DrawUtils::fillRoundRect(dis->hDC, pill, 8,
                                     isAuto ? theme::pal().TAG_BG_AUTO
                                            : theme::pal().TAG_BG_SYS);
            SetTextColor(dis->hDC, isAuto ? theme::pal().TAG_FG_AUTO : theme::pal().TAG_FG_SYS);
            DrawTextW(dis->hDC, tag.c_str(), static_cast<int>(tag.size()), &pill,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    } else {
        // ---- 子行：缩进 + pid + 数值 ----
        RECT box{dis->rcItem.left + 26, cy - 6, dis->rcItem.left + 38, cy + 6};
        DrawUtils::outlineRoundRect(dis->hDC, box, 3, theme::pal().BORDER);
        if (selected)
            DrawUtils::fillRoundRect(dis->hDC,
                                     RECT{box.left + 3, box.top + 3, box.right - 3,
                                          box.bottom - 3},
                                     2, canKill ? theme::pal().ACCENT : theme::pal().PROTECTED);

        wchar_t nameText[64] = {};
        if (row.pids.size() == 1)
            swprintf(nameText, 64, L"PID %u", row.pids[0]);
        RECT nameRc{dis->rcItem.left + 46, cy - 9, dis->rcItem.right - 150,
                    cy + 9};
        SelectObject(dis->hDC, childFont);
        SetTextColor(dis->hDC, canKill ? theme::pal().TEXT_DIM : theme::pal().PROTECTED);
        DrawTextW(dis->hDC, nameText, static_cast<int>(wcslen(nameText)), &nameRc,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    // 内存与 CPU（组/子行通用布局）
    SelectObject(dis->hDC, numFont);
    SetTextColor(dis->hDC, canKill ? theme::pal().TEXT_MAIN : theme::pal().PROTECTED);
    RECT memRc{dis->rcItem.right - 150, cy - 9, dis->rcItem.right - 84, cy + 9};
    DrawTextW(dis->hDC, memText, static_cast<int>(wcslen(memText)), &memRc,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    const double cpu = rowCpu(row, pidIndex_);
    SetTextColor(dis->hDC, cpu > 15.0 ? theme::pal().WARN : theme::pal().TEXT_DIM);
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

ProcessPanel::Zone ProcessPanel::zoneAt(int x, int y) const
{
    RECT rc;
    GetClientRect(hwnd_, &rc);
    if (y < 32) {
        if (x > rc.right - 36 && x <= rc.right - 8)
            return Zone::Close;
        if (x > rc.right - 64 && x <= rc.right - 36)
            return Zone::Rescan;
        return Zone::None;
    }
    if (y > rc.bottom - kFooterH + 4 && y < rc.bottom - kFooterH + 46) {
        if (x >= kMargin && x <= kMargin + 150)
            return Zone::Kill;
        if (x >= rc.right - kMargin - 80 && x <= rc.right - kMargin)
            return Zone::Hide;
    }
    return Zone::None;
}

void ProcessPanel::setHoverZone(Zone z)
{
    if (hoverZone_ == z)
        return;
    hoverZone_ = z;
    InvalidateRect(hwnd_, nullptr, FALSE);
    SetCursor(LoadCursorW(nullptr,
                          z == Zone::None ? IDC_ARROW : IDC_HAND));
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

    // 展开勾选行为进程集合（组行 = 组内全部进程），按 pid 去重
    std::vector<const ProcessInfo*> targets;
    std::vector<uint32_t> seen;
    int skipped = 0;
    std::map<std::wstring, int> nameCount;
    for (int i : sel) {
        if (i < 0 || static_cast<size_t>(i) >= rows_.size())
            continue;
        const PanelRow& row = rows_[static_cast<size_t>(i)];
        for (uint32_t pid : row.pids) {
            if (std::find(seen.begin(), seen.end(), pid) != seen.end())
                continue;
            seen.push_back(pid);
            const ProcessInfo* p = findProc(pidIndex_, pid);
            if (!p)
                continue;
            if (p->canTerminate) {
                targets.push_back(p);
                ++nameCount[p->name];
            } else {
                ++skipped;
            }
        }
    }
    if (targets.empty()) {
        MessageBoxW(hwnd_, L"所选应用全部受保护，无法结束。", defaults::kAppTitle,
                    MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::wstring names;
    for (const auto& [name, n] : nameCount) {
        if (!names.empty())
            names += L"、";
        names += n > 1 ? name + L"(×" + std::to_wstring(n) + L")" : name;
    }
    std::wstring text = L"确定结束以下 " + std::to_wstring(targets.size()) +
                        L" 个进程吗？\r\n\r\n" + names;
    if (skipped > 0)
        text += L"\r\n\r\n（另有 " + std::to_wstring(skipped) +
                L" 个受保护进程已自动跳过）";
    if (MessageBoxW(hwnd_, text.c_str(), L"确认结束进程",
                    MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
        return;

    int failCount = 0;
    std::wstring failures;
    for (const ProcessInfo* p : targets) {
        const KillResult r = terminateProcessById(p->pid, p->name, p->level);
        if (!r.ok) {
            ++failCount;
            if (failures.size() < 400)
                failures += (failures.empty() ? L"" : L"\r\n") + r.message;
        }
    }

    rescanAndResort();
    InvalidateRect(g_app.ball ? g_app.ball->hwnd() : nullptr, nullptr, FALSE);

    if (!failures.empty())
        MessageBoxW(hwnd_, failures.c_str(), L"部分进程结束失败",
                    MB_OK | MB_ICONWARNING);
}

void ProcessPanel::onListDoubleClick()
{
    const int index = static_cast<int>(SendMessageW(listBox_, LB_GETCARETINDEX, 0, 0));
    if (index < 0 || static_cast<size_t>(index) >= rows_.size())
        return;
    const PanelRow& row = rows_[static_cast<size_t>(index)];
    if (!row.isGroup)
        return;

    // 展开/收起该应用分组（保持滚动与勾选）
    const int topIndex =
        static_cast<int>(SendMessageW(listBox_, LB_GETTOPINDEX, 0, 0));
    if (row.expanded)
        expandedNames_.erase(row.name);
    else
        expandedNames_.insert(row.name);
    rebuildRows();
    SendMessageW(listBox_, LB_SETTOPINDEX,
                 topIndex < static_cast<int>(rows_.size()) ? topIndex : 0, 0);
}

void ProcessPanel::onListContextMenu(LPARAM lParam)
{
    if (!listBox_ || rows_.empty())
        return;

    POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    if (pt.x == -1 && pt.y == -1) {
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

    const int hit = static_cast<int>(SendMessageW(listBox_, LB_ITEMFROMPOINT, 0,
                                                  MAKELPARAM(pt.x, pt.y)));
    if (HIWORD(hit))
        return;
    const int index = LOWORD(hit);
    if (index < 0 || static_cast<size_t>(index) >= rows_.size())
        return;
    const PanelRow& row = rows_[static_cast<size_t>(index)];

    // 右键行同时勾选（若可结束）
    if (rowCanTerminate(row, pidIndex_))
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

    const wchar_t* scope = row.isGroup && row.pids.size() > 1 ? L"该应用全部进程"
                                                              : L"该进程";
    if (rowCanTerminate(row, pidIndex_)) {
        wchar_t killLabel[128] = {};
        swprintf(killLabel, 128, L"结束%ls（%zu 个进程）", scope, row.pids.size());
        AppendMenuW(menu, MF_STRING, MI_KILL, killLabel);
    } else {
        AppendMenuW(menu, MF_STRING | MF_GRAYED, MI_KILL, L"结束进程（受保护）");
    }

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    // 名单按进程名匹配，会作用于同名全部进程
    const std::wstring menuScope =
        row.isGroup ? L"该应用全部进程" : L"同名进程";
    if (g_app.protection.isWhitelisted(row.name)) {
        wchar_t label[128] = {};
        swprintf(label, 128, L"从白名单移除（%ls）", menuScope.c_str());
        AppendMenuW(menu, MF_STRING, MI_REMOVE_WHITE, label);
    } else {
        wchar_t label[128] = {};
        swprintf(label, 128, L"加入白名单，永不结束（%ls）", menuScope.c_str());
        AppendMenuW(menu, MF_STRING, MI_ADD_WHITE, label);
    }
    if (g_app.protection.inAutoClean(row.name)) {
        wchar_t label[128] = {};
        swprintf(label, 128, L"从自动清理名单移除（%ls）", menuScope.c_str());
        AppendMenuW(menu, MF_STRING, MI_REMOVE_AUTO, label);
    } else {
        wchar_t label[128] = {};
        swprintf(label, 128, L"加入自动清理名单（%ls）", menuScope.c_str());
        AppendMenuW(menu, MF_STRING, MI_ADD_AUTO, label);
    }

    const int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY,
                                   screenPt.x, screenPt.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);

    switch (cmd) {
    case MI_KILL: {
        int failCount = 0;
        std::wstring failures;
        for (uint32_t pid : row.pids) {
            const ProcessInfo* p = findProc(pidIndex_, pid);
            if (!p || !p->canTerminate)
                continue;
            const KillResult r = terminateProcessById(p->pid, p->name, p->level);
            if (!r.ok) {
                ++failCount;
                if (failures.size() < 400)
                    failures += (failures.empty() ? L"" : L"\r\n") + r.message;
            }
        }
        if (!failures.empty())
            MessageBoxW(hwnd_, failures.c_str(), L"结束进程失败",
                        MB_OK | MB_ICONWARNING);
        break;
    }
    case MI_ADD_WHITE:
        g_app.protection.addToWhitelist(row.name);
        g_app.protection.removeFromAutoClean(row.name);
        syncConfigFromProtection();
        break;
    case MI_REMOVE_WHITE:
        g_app.protection.removeFromWhitelist(row.name);
        syncConfigFromProtection();
        break;
    case MI_ADD_AUTO:
        g_app.protection.addToAutoClean(row.name);
        g_app.protection.removeFromWhitelist(row.name);
        syncConfigFromProtection();
        break;
    case MI_REMOVE_AUTO:
        g_app.protection.removeFromAutoClean(row.name);
        syncConfigFromProtection();
        break;
    default:
        return;
    }

    rescanAndResort();
    InvalidateRect(g_app.ball ? g_app.ball->hwnd() : nullptr, nullptr, FALSE);
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
        if (!self)
            return 0;
        if (LOWORD(wParam) == IDC_BTN_KILL && HIWORD(wParam) == BN_CLICKED)
            self->killSelected();
        else if (LOWORD(wParam) == IDC_BTN_HIDE && HIWORD(wParam) == BN_CLICKED)
            self->hide();
        else if (LOWORD(wParam) == IDC_LIST && HIWORD(wParam) == LBN_DBLCLK)
            self->onListDoubleClick();
        else if (LOWORD(wParam) == IDC_SEARCH && HIWORD(wParam) == EN_CHANGE) {
            // 实时过滤：读输入框 -> 小写 -> 触发行模型重建
            wchar_t buf[64] = {};
            GetWindowTextW(self->editBox_, buf, 64);
            std::wstring f = buf;
            std::transform(f.begin(), f.end(), f.begin(),
                           [](wchar_t c) { return static_cast<wchar_t>(::towlower(c)); });
            self->filter_ = std::move(f);
            self->lastPids_.clear();
            self->refreshData();
        }
        return 0;
    case WM_APP_REFRESH:
        if (self)
            self->refreshData();
        return 0;
    case WM_APP_HOVER: // 按钮 hover（按钮子类转发）
        if (self)
            self->setHoverZone(wParam == IDC_BTN_KILL    ? Zone::Kill
                               : wParam == IDC_BTN_HIDE ? Zone::Hide
                                                        : Zone::None);
        return 0;
    case WM_MOUSEMOVE:
        if (self) {
            self->setHoverZone(
                self->zoneAt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
            TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&tme);
        }
        return 0;
    case WM_MOUSELEAVE:
        if (self)
            self->setHoverZone(Zone::None);
        return 0;
    case WM_TIMER:
        // 滑入动画：ease-out 位移，150ms 后落位并杀定时器
        if (self && wParam == kAnimTimer && self->animActive_) {
            const DWORD t = GetTickCount() - self->animStart_;
            if (t >= 150) {
                KillTimer(hwnd, kAnimTimer);
                self->animActive_ = false;
                SetWindowPos(hwnd, nullptr, self->animTo_.x, self->animTo_.y, 0, 0,
                             SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            } else {
                const double k = 1.0 - static_cast<double>(t) / 150.0;
                const int off = static_cast<int>(16 * k * k);
                SetWindowPos(hwnd, nullptr,
                             self->animTo_.x + self->animDir_ * off,
                             self->animTo_.y, 0, 0,
                             SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        return 0;
    case WM_CONTEXTMENU:
        if (self && reinterpret_cast<HWND>(wParam) == self->listBox_)
            self->onListContextMenu(lParam);
        return 0;
    case WM_LBUTTONDOWN: {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);
        RECT rc;
        GetClientRect(hwnd, &rc);
        if (self && x > rc.right - 36 && y < 32)
            self->hide(); // ✕
        else if (self && x > rc.right - 64 && x <= rc.right - 36 && y < 32)
            self->rescanAndResort(); // ↻
        else if (self)
            PostMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0); // 标题栏拖动
        return 0;
    }
    case WM_CTLCOLOREDIT:     // 搜索框深色化
    case WM_CTLCOLORSTATIC:    // 搜索标签等静态控件
    case WM_CTLCOLORLISTBOX: {
        // 画刷跟随主题（切换主题后重建）
        static HBRUSH bgBrush = nullptr;
        static int brushTheme = -1;
        if (brushTheme != theme::currentThemeIndex()) {
            if (bgBrush)
                DeleteObject(bgBrush);
            bgBrush = CreateSolidBrush(theme::pal().BG);
            brushTheme = theme::currentThemeIndex();
        }
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkColor(hdc, theme::pal().BG);
        SetTextColor(hdc, theme::pal().TEXT_MAIN);
        return reinterpret_cast<LRESULT>(bgBrush);
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        if (self) {
            if (self->animActive_) {
                KillTimer(hwnd, kAnimTimer);
                self->animActive_ = false;
            }
            self->buffer_.release();
        }
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace fc

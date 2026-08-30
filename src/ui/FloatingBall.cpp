#include "FloatingBall.hpp"
#include "../app/AppContext.hpp"
#include "../config/defaults.hpp"
#include "../core/SystemMonitor.hpp"
#include "DrawUtils.hpp"
#include "ProcessPanel.hpp"
#include "Theme.hpp"
#include "TrayIcon.hpp"

#include <windowsx.h>

#include <algorithm>
#include <cstdio>

namespace fc {
namespace {

constexpr UINT_PTR kTimerRefresh = 1;
constexpr UINT_PTR kTimerAutoClean = 2;
constexpr UINT_PTR kTimerPulse = 3; // 高占用呼吸环（仅在占用 >=85% 时运行）

Dock edgeFromConfig(int v)
{
    switch (v) {
    case 0: return Dock::None;
    case 1: return Dock::Left;
    case 3: return Dock::Top;
    case 4: return Dock::Bottom;
    default: return Dock::Right;
    }
}

int edgeToConfig(Dock d)
{
    switch (d) {
    case Dock::None: return 0;
    case Dock::Left: return 1;
    case Dock::Top: return 3;
    case Dock::Bottom: return 4;
    default: return 2;
    }
}

// 竖排文字字体（用于左右贴边条）
HFONT verticalFont()
{
    static HFONT f = CreateFontW(-12, 0, 900, 900, FW_NORMAL, 0, 0, 0,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    return f;
}

} // namespace

bool FloatingBall::create(HINSTANCE hInstance)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(nullptr, IDC_HAND);
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    wc.hIconSm = wc.hIcon;
    wc.lpszClassName = L"FloatCleanerBall";
    if (!RegisterClassExW(&wc))
        return false;

    const int sw = GetSystemMetrics(SM_CXSCREEN);
    const int sh = GetSystemMetrics(SM_CYSCREEN);

    // 配置里的锚点：左右停靠时为 y，上下停靠时为 x，自由时为左上角
    dock_ = edgeFromConfig(g_app.config.dockEdge);
    const bool verticalDock = dock_ == Dock::Left || dock_ == Dock::Right;
    int anchor = verticalDock ? g_app.config.ballY : g_app.config.ballX;
    if (anchor < 0)
        anchor = sh / 2 - defaults::kDockLength / 2;
    if (verticalDock)
        g_app.config.ballY = std::clamp(anchor, 0, sh - defaults::kDockLength);
    else
        g_app.config.ballX = std::clamp(anchor, 0, sw - defaults::kDockLength);

    hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, wc.lpszClassName,
                            defaults::kAppTitle, WS_POPUP, 0, 0, defaults::kBallSize,
                            defaults::kBallSize, nullptr, nullptr, hInstance, nullptr);
    if (!hwnd_)
        return false;

    SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    SetTimer(hwnd_, kTimerRefresh, defaults::kRefreshIntervalMs, nullptr);
    SetTimer(hwnd_, kTimerAutoClean,
             static_cast<UINT>(g_app.config.autoCleanIntervalSec) * 1000, nullptr);

    applyGeometry();
    g_app.scanner.refresh(g_app.protection);
    show();
    return true;
}

void FloatingBall::show()
{
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

POINT FloatingBall::ballCenter() const
{
    RECT rc;
    GetWindowRect(hwnd_, &rc);
    return POINT{(rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2};
}

void FloatingBall::applyGeometry()
{
    const int sw = GetSystemMetrics(SM_CXSCREEN);
    const int sh = GetSystemMetrics(SM_CYSCREEN);
    const int size = defaults::kBallSize;
    const int len = defaults::kDockLength;
    const int th = defaults::kDockThickness;

    int x, y, w, h;
    HRGN rgn = nullptr;

    switch (dock_) {
    case Dock::Right:
        y = std::clamp(g_app.config.ballY, 0, sh - len);
        g_app.config.ballY = y;
        x = sw - th;
        w = th;
        h = len;
        // 左端半圆头朝屏幕内侧，右端被屏幕边缘裁平
        rgn = CreateRoundRectRgn(0, 0, th + len, len, th * 2, th * 2);
        break;
    case Dock::Left:
        y = std::clamp(g_app.config.ballY, 0, sh - len);
        g_app.config.ballY = y;
        x = 0;
        w = th;
        h = len;
        rgn = CreateRoundRectRgn(-len, 0, th, len, th * 2, th * 2);
        break;
    case Dock::Top:
        x = std::clamp(g_app.config.ballX, 0, sw - len);
        g_app.config.ballX = x;
        y = 0;
        w = len;
        h = th;
        rgn = CreateRoundRectRgn(0, -len, len, th, th * 2, th * 2);
        break;
    case Dock::Bottom:
        x = std::clamp(g_app.config.ballX, 0, sw - len);
        g_app.config.ballX = x;
        y = sh - th;
        w = len;
        h = th;
        rgn = CreateRoundRectRgn(0, 0, len, th + len, th * 2, th * 2);
        break;
    default: {
        x = std::clamp(g_app.config.ballX, 0, sw - size);
        y = std::clamp(g_app.config.ballY, 0, sh - size);
        g_app.config.ballX = x;
        g_app.config.ballY = y;
        w = size;
        h = size;
        rgn = CreateRoundRectRgn(0, 0, size + 1, size + 1, size, size);
        break;
    }
    }

    SetWindowPos(hwnd_, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowRgn(hwnd_, rgn, TRUE);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void FloatingBall::dockTo(Dock edge)
{
    // 用当前球心在边缘上的投影作为锚点
    const POINT c = ballCenter();
    const int sw = GetSystemMetrics(SM_CXSCREEN);
    const int sh = GetSystemMetrics(SM_CYSCREEN);

    switch (edge) {
    case Dock::Right:
    case Dock::Left:
        g_app.config.ballY =
            std::clamp(static_cast<int>(c.y) - defaults::kDockLength / 2, 0,
                       sh - defaults::kDockLength);
        break;
    case Dock::Top:
    case Dock::Bottom:
        g_app.config.ballX =
            std::clamp(static_cast<int>(c.x) - defaults::kDockLength / 2, 0,
                       sw - defaults::kDockLength);
        break;
    default:
        break;
    }
    dock_ = edge;
    g_app.config.dockEdge = edgeToConfig(edge);
    applyGeometry();
    g_app.config.save(g_app.iniPath);
}

void FloatingBall::undockToBall(int centerX, int centerY)
{
    const int sw = GetSystemMetrics(SM_CXSCREEN);
    const int sh = GetSystemMetrics(SM_CYSCREEN);
    dock_ = Dock::None;
    g_app.config.dockEdge = 0;
    g_app.config.ballX =
        std::clamp(centerX - defaults::kBallSize / 2, 0, sw - defaults::kBallSize);
    g_app.config.ballY =
        std::clamp(centerY - defaults::kBallSize / 2, 0, sh - defaults::kBallSize);
    applyGeometry();
    g_app.config.save(g_app.iniPath);
}

void FloatingBall::onPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd_, &ps);

    RECT rc;
    GetClientRect(hwnd_, &rc);

    DrawUtils::CachedCanvas& buffer = buffer_;
    HDC mem = buffer.begin(hdc, rc.right, rc.bottom);

    const MemoryStatus ms = SystemMonitor::query();
    const COLORREF status = fc::theme::usageColor(ms.percent);
    wchar_t pct[16] = {};
    swprintf(pct, 16, L"%u%%", ms.percent);
    const uint64_t usedBytes = ms.totalBytes > ms.availBytes
                                   ? ms.totalBytes - ms.availBytes
                                   : 0;
    wchar_t usedText[32] = {};
    swprintf(usedText, 32, L"%.1f/%.0fG",
             static_cast<double>(usedBytes) / (1ull << 30),
             static_cast<double>(ms.totalBytes) / (1ull << 30));

    SetBkMode(mem, TRANSPARENT);

    if (!isDocked()) {
        // ---- 夜樱宝石球：分层渐变 + 暗衬环 + 两段高光 + 底部反光弧 ----
        DrawUtils::fillGradient(mem, rc, theme::pal().BALL_TOP, theme::pal().BALL_BOTTOM);

        // 状态环：暗衬环(1px) + 内缩 3px 的状态环；呼吸/悬停时加粗
        HPEN rimPen = CreatePen(PS_SOLID, 1, theme::pal().BALL_RIM_DARK);
        HBRUSH hollow = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
        HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(mem, hollow));
        HPEN oldPen = static_cast<HPEN>(SelectObject(mem, rimPen));
        Ellipse(mem, rc.left + 1, rc.top + 1, rc.right, rc.bottom);
        DeleteObject(rimPen);

        int ringWidth = 3;
        if (pulseActive_)
            ringWidth = 3 + (pulsePhase_ < 3 ? pulsePhase_ : 5 - pulsePhase_);
        else if (hover_)
            ringWidth = 4;
        HPEN ring = CreatePen(PS_SOLID, ringWidth, status);
        oldPen = static_cast<HPEN>(SelectObject(mem, ring));
        Ellipse(mem, rc.left + 3, rc.top + 3, rc.right - 2, rc.bottom - 2);
        SelectObject(mem, oldBrush);
        SelectObject(mem, oldPen);
        DeleteObject(ring);

        // 左上两段式高光（玻璃反射）
        HBRUSH gloss = CreateSolidBrush(theme::pal().BALL_GLOSS);
        HPEN glossPen = CreatePen(PS_SOLID, 1, RGB(255, 214, 232));
        oldBrush = static_cast<HBRUSH>(SelectObject(mem, gloss));
        oldPen = static_cast<HPEN>(SelectObject(mem, glossPen));
        Ellipse(mem, rc.left + 9, rc.top + 7, rc.left + 25, rc.top + 16);
        DeleteObject(gloss);
        DeleteObject(glossPen);
        HBRUSH glossCore = CreateSolidBrush(theme::pal().BALL_GLOSS_CORE);
        HPEN corePen = CreatePen(PS_SOLID, 1, theme::pal().BALL_GLOSS_CORE);
        oldBrush = static_cast<HBRUSH>(SelectObject(mem, glossCore));
        oldPen = static_cast<HPEN>(SelectObject(mem, corePen));
        Ellipse(mem, rc.left + 13, rc.top + 9, rc.left + 21, rc.top + 13);
        SelectObject(mem, oldBrush);
        SelectObject(mem, oldPen);
        DeleteObject(glossCore);
        DeleteObject(corePen);

        // 底部反光弧（环境光）
        HPEN arcPen = CreatePen(PS_SOLID, 2, theme::pal().BALL_ARC);
        oldPen = static_cast<HPEN>(SelectObject(mem, arcPen));
        oldBrush = static_cast<HBRUSH>(SelectObject(mem, hollow));
        Arc(mem, rc.left + 12, rc.top + 30, rc.right - 8, rc.bottom + 22,
            rc.right - 14, rc.bottom - 8, rc.left + 16, rc.bottom - 8);
        SelectObject(mem, oldBrush);
        SelectObject(mem, oldPen);
        DeleteObject(arcPen);

        // 右上沿一片暗粉花瓣（与高光错开）
        DrawUtils::drawPetal(mem, rc.right - 14, rc.top + 12, 4,
                             theme::pal().PETAL_BALL);

        static HFONT pctFont = DrawUtils::font(16, true);
        static HFONT subFont = DrawUtils::font(10, false);

        SelectObject(mem, pctFont);
        SIZE sz{};
        GetTextExtentPoint32W(mem, pct, static_cast<int>(wcslen(pct)), &sz);
        DrawUtils::textWithShadow(mem, (rc.right - sz.cx) / 2, 11, pct,
                                  theme::pal().TEXT_MAIN, theme::pal().BALL_TEXT_SHADOW);

        // 第二行：已用/总内存（GB）
        SelectObject(mem, subFont);
        GetTextExtentPoint32W(mem, usedText, static_cast<int>(wcslen(usedText)),
                              &sz);
        SetTextColor(mem, RGB(244, 226, 252));
        TextOutW(mem, (rc.right - sz.cx) / 2, 36, usedText,
                 static_cast<int>(wcslen(usedText)));
    } else {
        // ---- 贴边条：沿厚度方向的圆柱渐变 + 高光线 + 衬环状态点 ----
        if (dock_ == Dock::Left || dock_ == Dock::Right)
            DrawUtils::fillGradientH(mem, rc, theme::pal().DOCK_INNER, theme::pal().DOCK_OUTER);
        else
            DrawUtils::fillGradient(mem, rc, theme::pal().DOCK_INNER, theme::pal().DOCK_OUTER);
        DrawUtils::outlineRoundRect(mem, rc, defaults::kDockThickness / 2,
                                    theme::pal().BORDER);

        // 内高光线（离屏幕最远的 1/4 厚度处）
        if (dock_ == Dock::Left || dock_ == Dock::Right) {
            const int hx = dock_ == Dock::Left ? rc.right - 6 : 5;
            DrawUtils::fillRect(mem, RECT{hx, 10, hx + 1, rc.bottom - 10},
                                theme::pal().DOCK_HIGHLIGHT);
        } else {
            const int hy = dock_ == Dock::Top ? rc.bottom - 6 : 5;
            DrawUtils::fillRect(mem, RECT{10, hy, rc.right - 10, hy + 1},
                                theme::pal().DOCK_HIGHLIGHT);
        }

        if (dock_ == Dock::Left || dock_ == Dock::Right) {
            // 状态点：暗衬环 + 状态色
            const int cx = rc.right / 2;
            HBRUSH back = CreateSolidBrush(theme::pal().DOT_BACKING);
            HPEN backPen = CreatePen(PS_SOLID, 1, theme::pal().DOT_BACKING);
            HBRUSH ob = static_cast<HBRUSH>(SelectObject(mem, back));
            HPEN op = static_cast<HPEN>(SelectObject(mem, backPen));
            Ellipse(mem, cx - 4, 6, cx + 4, 16);
            SelectObject(mem, ob);
            SelectObject(mem, op);
            DeleteObject(back);
            DeleteObject(backPen);
            HBRUSH db = CreateSolidBrush(status);
            HPEN dp = CreatePen(PS_SOLID, 1, status);
            ob = static_cast<HBRUSH>(SelectObject(mem, db));
            op = static_cast<HPEN>(SelectObject(mem, dp));
            Ellipse(mem, cx - 3, 8, cx + 3, 14);
            SelectObject(mem, ob);
            SelectObject(mem, op);
            DeleteObject(db);
            DeleteObject(dp);

            // 竖排数值：逐字堆叠（旋转文字会被 GDI 关闭 ClearType，堆叠保持清晰）
            wchar_t vertical[48] = {};
            swprintf(vertical, 48, L"%ls%.1fG", pct,
                     static_cast<double>(usedBytes) / (1ull << 30));
            SelectObject(mem, verticalFont());
            SetTextColor(mem, theme::pal().ACCENT);
            SetTextAlign(mem, TA_CENTER | TA_TOP);
            const int len = static_cast<int>(wcslen(vertical));
            const int avail = rc.bottom - 24;
            const int step = std::max(9, avail / std::max(1, len));
            int y = 20;
            for (const wchar_t* p = vertical; *p; ++p) {
                RECT charRc{cx - 8, y, cx + 8, y + step};
                DrawTextW(mem, p, 1, &charRc,
                          DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                y += step;
            }
            SetTextAlign(mem, TA_LEFT | TA_TOP);
        } else {
            const int cy = rc.bottom / 2;
            HBRUSH back = CreateSolidBrush(theme::pal().DOT_BACKING);
            HPEN backPen = CreatePen(PS_SOLID, 1, theme::pal().DOT_BACKING);
            HBRUSH ob = static_cast<HBRUSH>(SelectObject(mem, back));
            HPEN op = static_cast<HPEN>(SelectObject(mem, backPen));
            Ellipse(mem, 6, cy - 4, 16, cy + 4);
            SelectObject(mem, ob);
            SelectObject(mem, op);
            DeleteObject(back);
            DeleteObject(backPen);
            HBRUSH db = CreateSolidBrush(status);
            HPEN dp = CreatePen(PS_SOLID, 1, status);
            ob = static_cast<HBRUSH>(SelectObject(mem, db));
            op = static_cast<HPEN>(SelectObject(mem, dp));
            Ellipse(mem, 8, cy - 3, 14, cy + 3);
            SelectObject(mem, ob);
            SelectObject(mem, op);
            DeleteObject(db);
            DeleteObject(dp);

            static HFONT small = DrawUtils::font(11, true);
            SelectObject(mem, small);
            SetTextColor(mem, theme::pal().ACCENT);
            wchar_t horizontal[48] = {};
            swprintf(horizontal, 48, L"%ls %.1fG", pct,
                     static_cast<double>(usedBytes) / (1ull << 30));
            SetTextAlign(mem, TA_BASELINE | TA_LEFT);
            TextOutW(mem, defaults::kDockThickness + 6, cy + 5, horizontal,
                     static_cast<int>(wcslen(horizontal)));
            SetTextAlign(mem, TA_LEFT | TA_TOP);
        }
    }

    buffer.commit(hdc, rc.right, rc.bottom);
    EndPaint(hwnd_, &ps);
}

void FloatingBall::onTimer(UINT_PTR id)
{
    if (id == kTimerRefresh) {
        g_app.scanner.refresh(g_app.protection);

        // 高占用呼吸环：只有占用 >=85% 时才存在定时器（动画纪律：结束即杀）
        const MemoryStatus ms = SystemMonitor::query();
        const bool shouldPulse = ms.percent >= 85;
        if (shouldPulse && !pulseActive_) {
            pulseActive_ = true;
            pulsePhase_ = 0;
            SetTimer(hwnd_, kTimerPulse, 200, nullptr);
        } else if (!shouldPulse && pulseActive_) {
            pulseActive_ = false;
            KillTimer(hwnd_, kTimerPulse);
        }

        InvalidateRect(hwnd_, nullptr, FALSE);
        if (g_app.panel && g_app.panel->visible())
            PostMessageW(g_app.panel->hwnd(), WM_APP_REFRESH, 0, 0);
    } else if (id == kTimerAutoClean) {
        g_app.autoCleaner.tick(g_app.scanner.processes());
    } else if (id == kTimerPulse) {
        // 呼吸环：环宽 3~5px 正弦半周期摆动
        pulsePhase_ = (pulsePhase_ + 1) % 5;
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void FloatingBall::onMouseDown(LPARAM lParam)
{
    dragging_ = true;
    moved_ = false;
    dragStart_ = POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    RECT rc;
    GetWindowRect(hwnd_, &rc);
    windowStart_ = POINT{rc.left, rc.top};
    SetCapture(hwnd_);
}

void FloatingBall::onMouseMove(LPARAM lParam)
{
    if (!dragging_)
        return;
    const int dx = GET_X_LPARAM(lParam) - dragStart_.x;
    const int dy = GET_Y_LPARAM(lParam) - dragStart_.y;
    if (!moved_ && (abs(dx) > 3 || abs(dy) > 3)) {
        moved_ = true;
        if (isDocked()) {
            // 从吸附状态拖离：先变回球体（保持窗口左上角在光标下）
            undockToBall(windowStart_.x + dx + defaults::kDockThickness / 2,
                         windowStart_.y + dy + defaults::kDockLength / 2);
            RECT rc;
            GetWindowRect(hwnd_, &rc);
            windowStart_ = POINT{rc.left, rc.top};
            dragStart_ = POINT{dx, dy};
            return;
        }
    }
    if (moved_) {
        g_app.config.ballX = windowStart_.x + dx;
        g_app.config.ballY = windowStart_.y + dy;
        applyGeometry();
    }
}

void FloatingBall::onMouseUp()
{
    if (!dragging_)
        return;
    dragging_ = false;
    ReleaseCapture();

    if (moved_) {
        // 边缘吸附判定：球心距任一屏幕边缘 < 阈值则吸附
        const POINT c = ballCenter();
        const int sw = GetSystemMetrics(SM_CXSCREEN);
        const int sh = GetSystemMetrics(SM_CYSCREEN);
        if (c.x < defaults::kSnapDistance)
            dockTo(Dock::Left);
        else if (sw - c.x < defaults::kSnapDistance)
            dockTo(Dock::Right);
        else if (c.y < defaults::kSnapDistance)
            dockTo(Dock::Top);
        else if (sh - c.y < defaults::kSnapDistance)
            dockTo(Dock::Bottom);
        else
            g_app.config.save(g_app.iniPath);
        return;
    }

    // 单击
    if (isDocked()) {
        // 贴边条弹回球体（向屏幕内侧偏移）
        const POINT c = ballCenter();
        const int sw = GetSystemMetrics(SM_CXSCREEN);
        const int sh = GetSystemMetrics(SM_CYSCREEN);
        int cx = c.x, cy = c.y;
        const int offset = defaults::kBallSize / 2 + defaults::kDockThickness;
        switch (dock_) {
        case Dock::Left: cx = offset; break;
        case Dock::Right: cx = sw - offset; break;
        case Dock::Top: cy = offset; break;
        case Dock::Bottom: cy = sh - offset; break;
        default: break;
        }
        undockToBall(cx, cy);
    } else if (g_app.panel) {
        if (g_app.panel->visible())
            g_app.panel->hide();
        else
            g_app.panel->show();
    }
}

void FloatingBall::onContextMenu(LPARAM lParam)
{
    POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    if (pt.x == -1 && pt.y == -1) {
        RECT rc;
        GetWindowRect(hwnd_, &rc);
        pt = POINT{rc.left, rc.bottom};
    }
    popupMainMenu(hwnd_);
}

LRESULT CALLBACK FloatingBall::wndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                       LPARAM lParam)
{
    auto* self = reinterpret_cast<FloatingBall*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_PAINT:
        if (self)
            self->onPaint();
        return 0;
    case WM_TIMER:
        if (self)
            self->onTimer(wParam);
        return 0;
    case WM_LBUTTONDOWN:
        if (self)
            self->onMouseDown(lParam);
        return 0;
    case WM_MOUSEMOVE:
        if (self) {
            self->onMouseMove(lParam);
            if (!self->hover_ && !self->dragging_) {
                // 悬停反馈：状态环加粗一档暗示可抓取
                TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, hwnd, 0};
                TrackMouseEvent(&tme);
                self->hover_ = true;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;
    case WM_MOUSELEAVE:
        if (self && self->hover_) {
            self->hover_ = false;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_LBUTTONUP:
        if (self)
            self->onMouseUp();
        return 0;
    case WM_CAPTURECHANGED:
        if (self)
            self->dragging_ = false;
        return 0;
    case WM_CONTEXTMENU:
        if (self)
            self->onContextMenu(lParam);
        return 0;
    case WM_APP_TRAY: // 托盘事件（宿主窗口为本窗口）
        if (LOWORD(lParam) == WM_LBUTTONUP && g_app.panel) {
            if (g_app.panel->visible())
                g_app.panel->hide();
            else
                g_app.panel->show();
        } else if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_CONTEXTMENU) {
            if (self)
                self->onContextMenu(-1);
        }
        return 0;
    case WM_ERASEBKGND:
        return 1; // 全部自绘，避免闪烁
    case WM_DESTROY:
        // 必须移除托盘图标并投递 WM_QUIT，否则消息循环永不退出，进程僵死
        if (g_app.tray)
            g_app.tray->remove();
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace fc

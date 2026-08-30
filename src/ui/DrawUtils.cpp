#include "DrawUtils.hpp"

#include <cmath>
#include <cstdio>

namespace fc::DrawUtils {

void fillRect(HDC hdc, const RECT& rc, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rc, brush);
    DeleteObject(brush);
}

void fillRoundRect(HDC hdc, const RECT& rc, int radius, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, brush));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, pen));
    RoundRect(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1, radius * 2,
              radius * 2);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void outlineRoundRect(HDC hdc, const RECT& rc, int radius, COLORREF color)
{
    HBRUSH brush = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, brush));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, pen));
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius * 2, radius * 2);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

COLORREF lerpColor(COLORREF a, COLORREF b, double t)
{
    return RGB(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t,
               GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t,
               GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t);
}

// msimg32!GradientFill（系统自带，动态加载；失败时退回逐行画刷）
using GradientFillFn = BOOL(WINAPI *)(HDC, PTRIVERTEX, ULONG, PVOID, ULONG, ULONG);

static GradientFillFn gradientFillFn()
{
    static const GradientFillFn fn = [] {
        HMODULE m = GetModuleHandleW(L"msimg32.dll");
        if (!m)
            m = LoadLibraryW(L"msimg32.dll");
        return m ? reinterpret_cast<GradientFillFn>(
                       reinterpret_cast<void *>(
                           GetProcAddress(m, "GradientFill")))
                 : nullptr;
    }();
    return fn;
}

static TRIVERTEX makeVertex(int x, int y, COLORREF c)
{
    TRIVERTEX v{};
    v.x = x;
    v.y = y;
    v.Red = GetRValue(c) << 8;
    v.Green = GetGValue(c) << 8;
    v.Blue = GetBValue(c) << 8;
    v.Alpha = 0;
    return v;
}

void fillGradient(HDC hdc, const RECT& rc, COLORREF top, COLORREF bottom)
{
    const int height = rc.bottom - rc.top;
    if (height <= 0)
        return;
    if (height == 1) {
        fillRect(hdc, rc, top);
        return;
    }
    if (auto fn = gradientFillFn()) {
        TRIVERTEX v[2] = {makeVertex(rc.left, rc.top, top),
                          makeVertex(rc.right, rc.bottom, bottom)};
        GRADIENT_RECT g{0, 1};
        fn(hdc, v, 2, &g, 1, GRADIENT_FILL_RECT_V);
        return;
    }
    for (int y = 0; y < height; ++y) {
        RECT line{rc.left, rc.top + y, rc.right, rc.top + y + 1};
        fillRect(hdc, line,
                 lerpColor(top, bottom, static_cast<double>(y) / (height - 1)));
    }
}

void fillGradientH(HDC hdc, const RECT& rc, COLORREF left, COLORREF right)
{
    const int width = rc.right - rc.left;
    if (width <= 0)
        return;
    if (width == 1) {
        fillRect(hdc, rc, left);
        return;
    }
    if (auto fn = gradientFillFn()) {
        TRIVERTEX v[2] = {makeVertex(rc.left, rc.top, left),
                          makeVertex(rc.right, rc.bottom, right)};
        GRADIENT_RECT g{0, 1};
        fn(hdc, v, 2, &g, 1, GRADIENT_FILL_RECT_H);
        return;
    }
    for (int x = 0; x < width; ++x) {
        RECT line{rc.left + x, rc.top, rc.left + x + 1, rc.bottom};
        fillRect(hdc, line,
                 lerpColor(left, right, static_cast<double>(x) / (width - 1)));
    }
}

void textWithShadow(HDC hdc, int x, int y, const std::wstring& text,
                    COLORREF main, COLORREF shadow)
{
    SetTextColor(hdc, shadow);
    TextOutW(hdc, x + 1, y + 1, text.c_str(), static_cast<int>(text.size()));
    SetTextColor(hdc, main);
    TextOutW(hdc, x, y, text.c_str(), static_cast<int>(text.size()));
}

// ===== 矢量装饰图形 =====

namespace {

void fillPoly(HDC hdc, const POINT* pts, int count, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, brush));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, pen));
    Polygon(hdc, pts, count);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

} // namespace

void drawStar(HDC hdc, int x, int y, int r, COLORREF color)
{
    // 四角闪星：外接圆 4 点 + 内接圆 4 点（内半径 0.38r）
    POINT pts[8];
    for (int i = 0; i < 8; ++i) {
        const double angle = i * 3.14159265358979 / 4.0 - 3.14159265358979 / 2.0;
        const double radius = (i % 2 == 0) ? r : r * 0.38;
        pts[i].x = x + static_cast<int>(radius * cos(angle));
        pts[i].y = y + static_cast<int>(radius * sin(angle));
    }
    fillPoly(hdc, pts, 8, color);
}

void drawHeart(HDC hdc, int x, int y, int r, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, brush));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, pen));
    // 两个上半圆
    Ellipse(hdc, x - r, y - r, x, y);
    Ellipse(hdc, x, y - r, x + r, y);
    // 下三角
    POINT tri[3] = {{x - r, y - r / 3}, {x + r, y - r / 3}, {x, y + r}};
    Polygon(hdc, tri, 3);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void drawTriangle(HDC hdc, int x, int y, int r, bool down, COLORREF color)
{
    POINT pts[3];
    if (down) {
        pts[0] = {x - r, y - r * 7 / 10};
        pts[1] = {x + r, y - r * 7 / 10};
        pts[2] = {x, y + r * 7 / 10};
    } else {
        pts[0] = {x - r * 7 / 10, y - r};
        pts[1] = {x - r * 7 / 10, y + r};
        pts[2] = {x + r * 7 / 10, y};
    }
    fillPoly(hdc, pts, 3, color);
}

void drawRefresh(HDC hdc, int x, int y, int r, COLORREF color)
{
    HPEN pen = CreatePen(PS_SOLID, 2, color);
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, pen));
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
    // 300° 圆弧（右侧留缺口）
    Arc(hdc, x - r, y - r, x + r, y + r, x + r, y - r / 2, x + r, y + r / 2);
    // 缺口处箭头
    POINT tri[3] = {{x + r - 1, y - r / 2 - 3}, {x + r - 1, y - r / 2 + 4},
                    {x + r + 3, y - r / 2}};
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    fillPoly(hdc, tri, 3, color);
    DeleteObject(pen);
}

void drawX(HDC hdc, int x, int y, int r, COLORREF color)
{
    HPEN pen = CreatePen(PS_SOLID, 2, color);
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, pen));
    MoveToEx(hdc, x - r, y - r, nullptr);
    LineTo(hdc, x + r, y + r);
    MoveToEx(hdc, x + r, y - r, nullptr);
    LineTo(hdc, x - r, y + r);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void drawPetal(HDC hdc, int x, int y, int size, COLORREF color)
{
    // 樱花瓣：8 点叶片形（y 方向拉伸 1.3，尖端朝下）
    POINT pts[8];
    for (int i = 0; i < 8; ++i) {
        const double angle = i * 3.14159265358979 / 4.0;
        const double radius = (i % 2 == 0) ? size : size * 0.55;
        pts[i].x = x + static_cast<int>(radius * sin(angle));
        pts[i].y = y + static_cast<int>(radius * cos(angle) * 1.3);
    }
    fillPoly(hdc, pts, 8, color);
}

HFONT font(int size, bool bold)
{
    // size 以像素为单位
    return CreateFontW(-size, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL, 0, 0, 0,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

Canvas::Canvas(HDC windowDc, int width, int height)
    : windowDc_(windowDc), width_(width), height_(height)
{
    memDc_ = CreateCompatibleDC(windowDc);
    bitmap_ = CreateCompatibleBitmap(windowDc, width, height);
    oldBitmap_ = static_cast<HBITMAP>(SelectObject(memDc_, bitmap_));
}

Canvas::~Canvas()
{
    SelectObject(memDc_, oldBitmap_);
    DeleteObject(bitmap_);
    DeleteDC(memDc_);
}

void Canvas::commit()
{
    BitBlt(windowDc_, 0, 0, width_, height_, memDc_, 0, 0, SRCCOPY);
}

HDC CachedCanvas::begin(HDC windowDc, int width, int height)
{
    if (!memDc_ || width_ != width || height_ != height) {
        release();
        memDc_ = CreateCompatibleDC(windowDc);
        bitmap_ = CreateCompatibleBitmap(windowDc, width, height);
        oldBitmap_ = static_cast<HBITMAP>(SelectObject(memDc_, bitmap_));
        width_ = width;
        height_ = height;
    }
    return memDc_;
}

void CachedCanvas::commit(HDC windowDc, int width, int height)
{
    if (memDc_)
        BitBlt(windowDc, 0, 0, width, height, memDc_, 0, 0, SRCCOPY);
}

void CachedCanvas::release()
{
    if (memDc_) {
        SelectObject(memDc_, oldBitmap_);
        DeleteObject(bitmap_);
        DeleteDC(memDc_);
        memDc_ = nullptr;
        bitmap_ = nullptr;
    }
}

std::wstring formatBytes(uint64_t bytes)
{
    wchar_t buf[64] = {};
    if (bytes >= 1ull << 30)
        swprintf(buf, 64, L"%.2f GB", static_cast<double>(bytes) / (1ull << 30));
    else
        swprintf(buf, 64, L"%.1f MB", static_cast<double>(bytes) / (1ull << 20));
    return buf;
}

} // namespace fc::DrawUtils

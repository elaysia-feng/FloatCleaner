#include "DrawUtils.hpp"

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

void fillGradient(HDC hdc, const RECT& rc, COLORREF top, COLORREF bottom)
{
    const int height = rc.bottom - rc.top;
    if (height <= 0)
        return;
    if (height == 1) {
        fillRect(hdc, rc, top);
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

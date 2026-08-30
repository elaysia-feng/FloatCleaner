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

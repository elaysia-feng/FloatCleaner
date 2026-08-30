#pragma once
#include <cstdint>
#include <string>
#include <windows.h>

namespace fc {

// GDI 绘制工具 + 格式化工具
namespace DrawUtils {

void fillRoundRect(HDC hdc, const RECT& rc, int radius, COLORREF color);
void fillRect(HDC hdc, const RECT& rc, COLORREF color);
void outlineRoundRect(HDC hdc, const RECT& rc, int radius, COLORREF color);

HFONT font(int size, bool bold = false);

// 双缓冲画布：构造时创建内存 DC，析构时提交并清理
class Canvas {
public:
    Canvas(HDC windowDc, int width, int height);
    ~Canvas();
    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;

    HDC get() const { return memDc_; }
    void commit();

private:
    HDC windowDc_;
    HDC memDc_;
    HBITMAP bitmap_;
    HBITMAP oldBitmap_;
    int width_;
    int height_;
};

// 常驻缓存画布：位图只在尺寸变化时重建，避免每次重绘分配大块内存
class CachedCanvas {
public:
    CachedCanvas() = default;
    ~CachedCanvas() { release(); }
    CachedCanvas(const CachedCanvas&) = delete;
    CachedCanvas& operator=(const CachedCanvas&) = delete;

    HDC begin(HDC windowDc, int width, int height);
    void commit(HDC windowDc, int width, int height);
    void release();

private:
    HDC memDc_ = nullptr;
    HBITMAP bitmap_ = nullptr;
    HBITMAP oldBitmap_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

std::wstring formatBytes(uint64_t bytes);

} // namespace DrawUtils
} // namespace fc

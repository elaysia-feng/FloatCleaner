#pragma once
#include <windows.h>

namespace fc::theme {

// 深色主题
constexpr COLORREF BG        = RGB(24, 26, 32);   // 窗口背景
constexpr COLORREF BG_CARD   = RGB(34, 37, 46);   // 卡片/按钮
constexpr COLORREF BG_HOVER  = RGB(45, 49, 60);
constexpr COLORREF LIST_SEL  = RGB(48, 54, 68);   // 列表选中行
constexpr COLORREF BORDER    = RGB(56, 60, 72);
constexpr COLORREF ACCENT    = RGB(0, 199, 140);  // 主色：薄荷绿
constexpr COLORREF TEXT_MAIN = RGB(235, 238, 245);
constexpr COLORREF TEXT_DIM  = RGB(130, 136, 150);
constexpr COLORREF WARN      = RGB(255, 176, 32);
constexpr COLORREF DANGER    = RGB(255, 92, 92);
constexpr COLORREF PROTECTED = RGB(150, 156, 170);

// 按内存占用率取状态色：绿 -> 黄 -> 红
inline COLORREF usageColor(uint32_t percent)
{
    if (percent < 60)
        return ACCENT;
    if (percent < 85)
        return WARN;
    return DANGER;
}

} // namespace fc::theme

#pragma once
#include <windows.h>

namespace fc::theme {

// ===== 夜樱主题（二次元）=====
// 深蓝紫夜空底 + 樱花粉主色 + 天蓝辅助，星星/爱心点缀

// 夜空背景（渐变端点）
constexpr COLORREF BG_TOP     = RGB(34, 30, 58);   // 夜空顶：深蓝紫
constexpr COLORREF BG_BOTTOM  = RGB(56, 44, 92);   // 夜空底：暗紫
constexpr COLORREF HEADER_TOP = RGB(46, 38, 82);   // 标题栏：稍亮夜空
constexpr COLORREF HEADER_BOTTOM = RGB(66, 50, 108);

// 表面
constexpr COLORREF BG         = RGB(40, 34, 70);   // 夜空基色（列表底/控件底）
constexpr COLORREF ROW_ALT    = RGB(64, 52, 100);  // 偶数行微亮
constexpr COLORREF LIST_SEL   = RGB(94, 62, 128);  // 选中：樱紫
constexpr COLORREF BORDER     = RGB(110, 90, 160);
constexpr COLORREF BTN_PURPLE = RGB(88, 70, 138);  // 次要按钮
constexpr COLORREF BG_CARD    = BTN_PURPLE;
constexpr COLORREF BG_HOVER   = RGB(112, 88, 164);
constexpr COLORREF DANGER     = RGB(255, 92, 122);

// 强调色
constexpr COLORREF ACCENT     = RGB(255, 138, 178); // 樱花粉（主）
constexpr COLORREF ACCENT_DEEP= RGB(226, 100, 148); // 樱粉深（按钮底）
constexpr COLORREF SKY        = RGB(126, 200, 255); // 天蓝（辅助）
constexpr COLORREF LAVENDER   = RGB(196, 168, 255); // 浅藤紫

// 文字
constexpr COLORREF TEXT_MAIN  = RGB(248, 242, 255); // 近白微紫
constexpr COLORREF TEXT_DIM   = RGB(172, 162, 205);
constexpr COLORREF PROTECTED  = RGB(140, 130, 168); // 受保护灰紫

// 状态色（随内存占用率）
constexpr COLORREF STATUS_LOW = SKY;                 // <60% 天蓝
constexpr COLORREF STATUS_MID = ACCENT;              // <85% 樱粉
constexpr COLORREF STATUS_HIGH= RGB(255, 92, 122);   // >=85% 深樱红
constexpr COLORREF WARN       = RGB(255, 190, 120);  // CPU 高亮（暖橙）

// 按内存占用率取状态色：天蓝 -> 樱粉 -> 深樱红
inline COLORREF usageColor(uint32_t percent)
{
    if (percent < 60)
        return STATUS_LOW;
    if (percent < 85)
        return STATUS_MID;
    return STATUS_HIGH;
}

} // namespace fc::theme

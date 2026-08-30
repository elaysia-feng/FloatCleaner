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

// 行层次
constexpr COLORREF ROW_SEP    = RGB(52, 44, 84);   // 行分隔线（BG 与 BORDER 中间值）
constexpr COLORREF ROW_PROTECT= RGB(44, 38, 74);   // 受保护行底色（微暗一档）

// 标签 pill（预混底色 + 提亮文字）
constexpr COLORREF TAG_BG_AUTO = RGB(74, 44, 76);
constexpr COLORREF TAG_FG_AUTO = RGB(255, 168, 200);
constexpr COLORREF TAG_BG_SYS  = RGB(58, 50, 88);
constexpr COLORREF TAG_FG_SYS  = RGB(158, 148, 188);

// 滚动条
constexpr COLORREF SCROLL_TRACK = RGB(56, 46, 88);
constexpr COLORREF SCROLL_THUMB = RGB(120, 104, 170);

// 悬浮球（宝石感分层 + 反光）
constexpr COLORREF BALL_TOP    = RGB(255, 172, 206);
constexpr COLORREF BALL_BOTTOM = RGB(150, 118, 226);
constexpr COLORREF BALL_DEEP   = RGB(120, 92, 200);
constexpr COLORREF BALL_MID    = RGB(196, 140, 214);
constexpr COLORREF BALL_GLOSS  = RGB(255, 232, 244);
constexpr COLORREF BALL_GLOSS_CORE = RGB(255, 246, 252);
constexpr COLORREF BALL_RIM_DARK   = RGB(60, 40, 90);
constexpr COLORREF BALL_ARC        = RGB(255, 190, 220);
constexpr COLORREF BALL_TEXT_SHADOW= RGB(70, 40, 90);

// 贴边条（沿厚度方向渐变：屏幕内侧亮、贴边暗）
constexpr COLORREF DOCK_INNER = RGB(88, 74, 140);
constexpr COLORREF DOCK_OUTER = RGB(40, 34, 66);
constexpr COLORREF DOCK_HIGHLIGHT = RGB(160, 140, 220);
constexpr COLORREF DOT_BACKING    = RGB(50, 40, 80);

// 花瓣（预混暗色，若隐若现）
constexpr COLORREF PETAL_HEADER = RGB(120, 86, 150);
constexpr COLORREF PETAL_HEADER2= RGB(104, 78, 134);
constexpr COLORREF PETAL_BALL   = RGB(236, 150, 190);

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

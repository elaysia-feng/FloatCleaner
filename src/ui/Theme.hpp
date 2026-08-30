#pragma once
#include <cstdint>
#include <windows.h>

// ===== 主题系统 =====
// 四套二次元皮肤（子 agent 设计，见 design/theme-variants.md），
// 运行时通过 ini 的 [general] theme 索引或右键菜单切换。

namespace fc::theme {

struct ThemeColors {
    const wchar_t *name; // 菜单显示名
    COLORREF BG_TOP, BG_BOTTOM, HEADER_TOP, HEADER_BOTTOM;
    COLORREF BG, ROW_ALT, LIST_SEL, BORDER, BTN_PURPLE, BG_CARD, BG_HOVER, DANGER;
    COLORREF ACCENT, ACCENT_DEEP, SKY, LAVENDER;
    COLORREF TEXT_MAIN, TEXT_DIM, PROTECTED, TEXT_SHADOW;
    COLORREF STATUS_LOW, STATUS_MID, STATUS_HIGH, WARN;
    COLORREF ROW_SEP, ROW_PROTECT;
    COLORREF TAG_BG_AUTO, TAG_FG_AUTO, TAG_BG_SYS, TAG_FG_SYS;
    COLORREF SCROLL_TRACK, SCROLL_THUMB;
    COLORREF BALL_TOP, BALL_BOTTOM, BALL_DEEP, BALL_MID, BALL_GLOSS,
        BALL_GLOSS_CORE, BALL_RIM_DARK, BALL_ARC, BALL_TEXT_SHADOW;
    COLORREF DOCK_INNER, DOCK_OUTER, DOCK_HIGHLIGHT, DOT_BACKING;
    COLORREF PETAL_HEADER, PETAL_HEADER2, PETAL_BALL;
};

// 当前主题调色板与全部可选主题（pal = 当前生效配色）
const ThemeColors &pal();
const ThemeColors *allThemes(int *count);
int currentThemeIndex();
void setThemeByIndex(int index); // 调用方负责切换后重绘各窗口

// 按内存占用率取状态色：低/中/高三档
COLORREF usageColor(uint32_t percent);

// 主题数量（菜单循环用）
constexpr int kThemeCount = 4;

} // namespace fc::theme

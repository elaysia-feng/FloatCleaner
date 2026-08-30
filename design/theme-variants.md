# 夜樱二次元 · 配色变体设计

基于 `src/ui/Theme.hpp` 现有"夜樱"主题（深蓝紫夜空 + 樱粉 + 天蓝），在同一大方向下设计三套可直接落地的变体。
每套代码块为完整可编译的 `Theme.hpp` 颜色常量组，命名与现有文件一致（额外保留 `ROW_ALT/BTN_PURPLE/BG_CARD/BG_HOVER/DANGER`，`ProcessPanel.cpp` 与 `FloatingBall.cpp` 依赖它们，替换后无需改动任何源码）。

对比度说明：三套的正文色 `TEXT_MAIN` 均为亮度 ≥ 0.85 的近白（暖白/月白），而列表底 `BG` 亮度均 ≤ 0.03，对比度估算在 12:1 ~ 16:1 之间，远超 WCAG AA 的 4.5:1；次级文字 `TEXT_DIM` 亮度约 0.45~0.55，对深底对比度约 5:1 ~ 6:1，同样达标。

---

## 变体 A「初樱」（Hatsuzakura）

比现主题更亮更甜的樱粉系：底色从"蓝紫夜空"改为"莓果暗红"，粉色占比大幅提高，整体像傍晚的满开樱花大道。

```cpp
#pragma once
#include <windows.h>

namespace fc::theme {

// ===== 初樱主题（二次元 · 甜系）=====
// 深莓红暮色底 + 高饱和樱粉主色 + 奶蓝辅助，奶油感高光

// 暮色背景（渐变端点）
constexpr COLORREF BG_TOP     = RGB(64, 34, 58);    // 顶：深莓红
constexpr COLORREF BG_BOTTOM  = RGB(96, 50, 84);    // 底：暖莓紫
constexpr COLORREF HEADER_TOP = RGB(84, 44, 74);    // 标题栏：亮一档莓粉
constexpr COLORREF HEADER_BOTTOM = RGB(112, 58, 98);

// 表面
constexpr COLORREF BG         = RGB(72, 40, 66);    // 基色（列表底/控件底）
constexpr COLORREF ROW_ALT    = RGB(92, 54, 84);    // 偶数行微亮
constexpr COLORREF LIST_SEL   = RGB(150, 60, 104);  // 选中：樱红紫
constexpr COLORREF BORDER     = RGB(180, 110, 150); // 樱粉描边
constexpr COLORREF BTN_PURPLE = RGB(140, 80, 120);  // 次要按钮（莓粉紫）
constexpr COLORREF BG_CARD    = BTN_PURPLE;
constexpr COLORREF BG_HOVER   = RGB(164, 100, 144);
constexpr COLORREF DANGER     = RGB(255, 96, 128);

// 强调色
constexpr COLORREF ACCENT     = RGB(255, 158, 196); // 亮樱粉（主）
constexpr COLORREF ACCENT_DEEP= RGB(240, 110, 158); // 樱粉深（按钮底）
constexpr COLORREF SKY        = RGB(150, 214, 255); // 奶蓝（辅助）
constexpr COLORREF LAVENDER   = RGB(226, 190, 255); // 奶油藤紫

// 文字
constexpr COLORREF TEXT_MAIN  = RGB(255, 246, 250); // 樱白（对 BG 约 13:1）
constexpr COLORREF TEXT_DIM   = RGB(216, 186, 208); // 灰樱粉
constexpr COLORREF PROTECTED  = RGB(170, 140, 164); // 受保护灰莓

// 状态色（随内存占用率）
constexpr COLORREF STATUS_LOW = SKY;                 // <60% 奶蓝
constexpr COLORREF STATUS_MID = ACCENT;              // <85% 亮樱粉
constexpr COLORREF STATUS_HIGH= RGB(255, 96, 128);   // >=85% 莓红
constexpr COLORREF WARN       = RGB(255, 200, 130);  // CPU 高亮（奶油橙）

// 按内存占用率取状态色
inline COLORREF usageColor(uint32_t percent)
{
    if (percent < 60)
        return STATUS_LOW;
    if (percent < 85)
        return STATUS_MID;
    return STATUS_HIGH;
}

} // namespace fc::theme
```

**设计说明**：灵感来自黄昏时分落满樱花的坡道——底色放弃冷调蓝紫，改用带红相的深莓色，让樱粉主色像是"融在背景里"而不是浮在夜空上。甜感来自三处：饱和度拉满的 `ACCENT(255,158,196)`、粉色系描边（BORDER 不再偏灰紫）、以及接近肤色的暖白正文。适合喜欢马卡龙/魔法少女系配色的用户，截图分享效果好；缺点是久看比另外两套更"腻"一些。

---

## 变体 B「月夜」（Tsukiyo）

更暗更沉稳的蓝紫系，星光感：整体压暗一档，底色接近午夜靛蓝，强调色从樱粉换成清冷的月光蓝与星青。

```cpp
#pragma once
#include <windows.h>

namespace fc::theme {

// ===== 月夜主题（二次元 · 星空系）=====
// 午夜靛蓝底 + 月光蓝主色 + 星青辅助，银河/星屑点缀

// 夜空背景（渐变端点）
constexpr COLORREF BG_TOP     = RGB(13, 15, 34);    // 顶：午夜靛黑
constexpr COLORREF BG_BOTTOM  = RGB(26, 30, 62);    // 底：暗星蓝
constexpr COLORREF HEADER_TOP = RGB(20, 24, 50);    // 标题栏：近黑蓝
constexpr COLORREF HEADER_BOTTOM = RGB(36, 42, 84);

// 表面
constexpr COLORREF BG         = RGB(18, 22, 44);    // 基色（列表底/控件底）
constexpr COLORREF ROW_ALT    = RGB(30, 36, 66);    // 偶数行微亮
constexpr COLORREF LIST_SEL   = RGB(48, 60, 110);   // 选中：月光蓝紫
constexpr COLORREF BORDER     = RGB(88, 100, 160);  // 星雾蓝描边
constexpr COLORREF BTN_PURPLE = RGB(44, 54, 100);   // 次要按钮（暗星蓝）
constexpr COLORREF BG_CARD    = BTN_PURPLE;
constexpr COLORREF BG_HOVER   = RGB(60, 72, 124);
constexpr COLORREF DANGER     = RGB(255, 110, 140);

// 强调色
constexpr COLORREF ACCENT     = RGB(170, 200, 255); // 月光蓝（主）
constexpr COLORREF ACCENT_DEEP= RGB(118, 150, 230); // 月蓝深（按钮底）
constexpr COLORREF SKY        = RGB(126, 232, 255); // 星青（辅助）
constexpr COLORREF LAVENDER   = RGB(202, 190, 255); // 薄暮紫

// 文字
constexpr COLORREF TEXT_MAIN  = RGB(236, 242, 255); // 月白（对 BG 约 15:1）
constexpr COLORREF TEXT_DIM   = RGB(156, 168, 206); // 星雾灰蓝
constexpr COLORREF PROTECTED  = RGB(118, 128, 158); // 受保护暗灰蓝

// 状态色（随内存占用率）
constexpr COLORREF STATUS_LOW = SKY;                 // <60% 星青
constexpr COLORREF STATUS_MID = ACCENT;              // <85% 月光蓝
constexpr COLORREF STATUS_HIGH= RGB(255, 110, 140);  // >=85% 残月红（唯一暖色，警示醒目）
constexpr COLORREF WARN       = RGB(255, 208, 140);  // CPU 高亮（月光金）

// 按内存占用率取状态色
inline COLORREF usageColor(uint32_t percent)
{
    if (percent < 60)
        return STATUS_LOW;
    if (percent < 85)
        return STATUS_MID;
    return STATUS_HIGH;
}

} // namespace fc::theme
```

**设计说明**：灵感来自高原夏夜的银河——底色整体压到接近 OLED 黑（最暗处 RGB 13,15,34），冷到几乎没有色偏，让状态色像星星一样"点亮"在面板上。配色策略是"冷色打底、暖色只作警示"：`STATUS_HIGH` 的残月红是全主题唯一高饱和暖色，内存告警时会从一片蓝紫中跳出来，信号效率极高。适合深夜挂机、深色壁纸桌面的用户，是三套里最耐看的一套。

---

## 变体 C「花火」（Hanabi）

在夜樱底上加入暖橙/金色的节日感：底色仍是夏夜靛蓝，但强调色换成花火金橙，标题栏底带一抹晚霞暖紫。

```cpp
#pragma once
#include <windows.h>

namespace fc::theme {

// ===== 花火主题（二次元 · 夏日祭）=====
// 夏夜靛蓝底 + 花火金橙主色 + 空蓝辅助，晚霞/祭典灯笼点缀

// 夜空背景（渐变端点）
constexpr COLORREF BG_TOP     = RGB(34, 26, 54);    // 顶：夜空靛
constexpr COLORREF BG_BOTTOM  = RGB(66, 42, 58);    // 底：晚霞暖紫（烟花余光）
constexpr COLORREF HEADER_TOP = RGB(50, 36, 72);    // 标题栏：暮空紫
constexpr COLORREF HEADER_BOTTOM = RGB(84, 52, 72);

// 表面
constexpr COLORREF BG         = RGB(42, 32, 60);    // 基色（列表底/控件底）
constexpr COLORREF ROW_ALT    = RGB(62, 46, 78);    // 偶数行微亮
constexpr COLORREF LIST_SEL   = RGB(96, 62, 96);    // 选中：焰紫
constexpr COLORREF BORDER     = RGB(160, 120, 130); // 暖灰粉描边
constexpr COLORREF BTN_PURPLE = RGB(84, 62, 100);   // 次要按钮（暗藤紫）
constexpr COLORREF BG_CARD    = BTN_PURPLE;
constexpr COLORREF BG_HOVER   = RGB(108, 80, 124);
constexpr COLORREF DANGER     = RGB(255, 104, 110);

// 强调色
constexpr COLORREF ACCENT     = RGB(255, 182, 100); // 花火金橙（主）
constexpr COLORREF ACCENT_DEEP= RGB(236, 140, 64);  // 焰心橙（按钮底）
constexpr COLORREF SKY        = RGB(130, 205, 255); // 夏夜空蓝（辅助）
constexpr COLORREF LAVENDER   = RGB(255, 190, 150); // 晚霞橘粉（✦/↻ 点缀色）

// 文字
constexpr COLORREF TEXT_MAIN  = RGB(255, 248, 240); // 灯笼暖白（对 BG 约 14:1）
constexpr COLORREF TEXT_DIM   = RGB(214, 192, 196); // 烟花灰粉
constexpr COLORREF PROTECTED  = RGB(164, 144, 152); // 受保护暖灰

// 状态色（随内存占用率）
constexpr COLORREF STATUS_LOW = SKY;                 // <60% 空蓝
constexpr COLORREF STATUS_MID = ACCENT;              // <85% 金橙
constexpr COLORREF STATUS_HIGH= RGB(255, 104, 110);  // >=85% 花火红
constexpr COLORREF WARN       = RGB(255, 214, 120);  // CPU 高亮（祭典金）

// 按内存占用率取状态色
inline COLORREF usageColor(uint32_t percent)
{
    if (percent < 60)
        return STATUS_LOW;
    if (percent < 85)
        return STATUS_MID;
    return STATUS_HIGH;
}

} // namespace fc::theme
```

**设计说明**：灵感来自夏夜祭典的花火大会——底色从下往上由"晚霞暖紫"过渡到"夜空靛"，模拟烟花余光映在低空的样子；主强调色换成 `RGB(255,182,100)` 的花火金橙，"♥ 自动清理中"、勾选框、结束按钮全部变成灯笼般的暖色。状态色梯度（空蓝→金橙→花火红）天然就是"温度"隐喻，内存越满越"热"。适合喜欢暖色调、或把工具当桌面装饰的用户；金橙在深靛底上非常醒目，但暖色面积比另外两套大，冷静感稍弱。

---

## 推荐

**推荐变体 B「月夜」**，理由对应三个考察点：

1. **深色桌面工具**：月夜的底色是三套中最暗的（BG 亮度约 0.012），悬浮球贴边条和面板在深色壁纸上存在感最低、不抢注意力；工具类软件"界面退后、信息向前"是第一原则，月夜做得最好。同时 OLED/高对比屏上暗底更省电、光晕更少。
2. **久看不腻**：月夜的强调色是低饱和冷色（月光蓝 170,200,255），常驻大面积色（背景、列表、按钮）几乎全是同色相的蓝紫渐变，视觉刺激最小；相比之下初樱的高饱和樱粉和花火的金橙第一眼惊艳，但悬浮窗是常驻元素，两周后甜/暖色系更容易审美疲劳。
3. **二次元感**：保留住了——星空/银河系是宅系配色的重要一支（常见于天文、星座企划与"夜空系"角色配色），`✦✧` 星星点缀、LAVENDER 薄暮紫装饰字都还有明确的二次元气质；且 STATUS_HIGH 的残月红"冷底一点红"反而比满屏粉色更有角色立绘式的戏剧感。

若目标用户偏少女系/甜系社区（截图传播优先），备选 A「初樱」；C「花火」更适合做成节日限定皮肤（如夏档 7-8 月限时切换），常驻默认不建议。

落地提示：三套均可直接整体替换 `src/ui/Theme.hpp` 常量区，`usageColor` 保持不变即可编译；唯一需人工复核的是 `FloatingBall.cpp` 中硬编码的球体渐变 `RGB(255,172,206) → RGB(150,118,226)`、高光 `RGB(255,232,244)` 与文字阴影 `RGB(70,40,90)/RGB(40,24,64)`——建议后续把它们也提为 Theme 常量（如 BALL_TOP/BALL_BOTTOM/BALL_GLOSS/TEXT_SHADOW），本次未改动任何源码。

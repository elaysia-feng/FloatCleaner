# FloatCleaner「夜樱」主题 UI 打磨评审

评审范围：`src/ui/Theme.hpp`、`src/ui/DrawUtils.hpp/.cpp`、`src/ui/FloatingBall.cpp`、`src/ui/ProcessPanel.cpp`（尺寸参考 `src/config/defaults.hpp`：球 64px、贴边条 22×88、面板 360×540 r=12）。

硬约束回顾：纯 GDI（无 alpha、无 D2D）、不引入新依赖、exe 体积不显著增大、空闲 CPU 接近 0（常驻工具，所有定时器动画必须"只在需要时存在"）。以下所有方案均满足这些约束。

---

## P0 — 立即做

### 1. 主按钮渐变直角溢出（渲染 bug）

**现状**：`ProcessPanel.cpp::onDrawItem` 的"结束所选"按钮在 `selCount > 0` 时用 `fillGradientH(dis->hDC, dis->rcItem, ...)` 填充整个矩形，随后才用 `outlineRoundRect(..., 8, ...)` 画 8px 圆角描边。渐变填到了方角，描边只勾圆角——按钮四个角会露出粉色方角，压在深色夜空背景上非常扎眼。而 `selCount == 0` 时用的是 `fillRoundRect`（真圆角），两种状态角部形状还不一致。

**GDI 方案**：
1. 渐变填充前把裁剪区限制为圆角矩形：
   ```cpp
   HRGN rgn = CreateRoundRectRgn(rc.left, rc.top, rc.right + 1, rc.bottom + 1, 16, 16);
   SelectClipRgn(dis->hDC, rgn);
   fillGradientH(dis->hDC, rc, ...);          // 渐变被裁成圆角
   SelectClipRgn(dis->hDC, nullptr);
   DeleteObject(rgn);
   ```
2. 再 `outlineRoundRect(..., 8, ...)` 画描边（1px 描边圆角半径与填充一致即可，半径用同一个常量）。
3. 无选中/次要按钮（"收起"）同样路径处理，保证所有按钮角部形状恒定。
4. 顺手把半径 8 提成 `Theme.hpp` 里的 `constexpr int kRadiusBtn = 8;`，与第 3 条的圆角体系统一管理。

**优先级**：P0（可见渲染缺陷）。

---

### 2. 组行展开箭头与复选框重叠（层级错乱 bug）

**现状**：`onDrawItem` 组行分支里，箭头 `▼/▶` 画在 `rcItem.left + 8`，而复选框画在 `{left+6, cy-7, left+20, cy+7}`——箭头整个落在复选框框线内部；且复选框是后画的，勾选状态下 `fillRoundRect(box内缩3px)` 会直接把箭头盖掉一半。两个控件在争同一个 14px 的位置。

**GDI 方案**（二选一，推荐 A）：
- **A. 合并成一个控件**：删掉独立复选框框线，把箭头作为"展开钮"保留在 `left+8`，勾选态改为在箭头左侧画一个 10px 实心樱花粉圆点（`Ellipse`，选中时填充 `ACCENT`、未选时不画），点与箭头间隔 4px。这样一列之内"点=选、箭头=展开"语义清晰，行首也更透气。
- **B. 分列**：复选框保持 `left+6..left+20`，箭头右移到 `left+26`，应用名起点右移到 `left+44`；子行复选框与其对齐。

同时修正勾选内缩的不对称：`RECT{box.left+3, box.top+3, box.right-2, box.bottom-2}` 左右各缩 3/2 不对称，统一为四周各缩 3（`+3, +3, -3, -3`）。

**优先级**：P0（可见布局缺陷）。

---

### 3. 圆角一致性体系 + 列表"卡片化"

**现状**：圆角五花八门——面板 12、按钮 8、复选框 3、贴边条 11（th/2）、球是正圆；而且列表区是一块 `WM_CTLCOLORLISTBOX` 填 `theme::BG` 的**方角** listbox，硬生生糊在渐变背景上，四角露出一个矩形色块，和整体的圆角语言冲突。`Theme.hpp` 里 `ROW_ALT` 定义了但从未使用。

**GDI 方案**：
1. 在 `Theme.hpp` 建立圆角刻度并全部引用常量：`kRadiusPanel=12`、`kRadiusCard=10`、`kRadiusBtn=8`、`kRadiusChip=高度/2`（pill）、`kRadiusCheck=4`。
2. **列表卡片化**：在 `onPaint` 里、listbox 区域（`{kMargin-4, kHeaderH-2, w-kMargin+4, h-kFooterH+2}`）先画一张圆角卡片：`fillRoundRect(..., kRadiusCard, theme::BG)` + `outlineRoundRect(..., kRadiusCard, 一个比 BORDER 更暗的颜色，如 RGB(84,70,128))`。listbox 的 `BG` 与卡片同色，方角问题即被卡片圆角"吸收"。
3. listbox 从 `kMargin` 内缩到 `kMargin+4`，让卡片四周留 4px 渐变底边，形成"面板里浮着一张卡"的层次。
4. 顺带删除或启用 `ROW_ALT`（见第 4 条），避免死常量。

**优先级**：P0。

---

### 4. 列表行的层次感（选中条 / 斑马纹 / 分隔线）

**现状**：行背景是整幅方角 `fillRect(BG)`，选中是整幅方角 `fillRect(LIST_SEL)`——选中块糊满整行、没有方向感；行与行之间零分隔，密集列表看起来是一坨同色块；每 2s `refreshData` 还会整面板重绘一次，观感单调。

**GDI 方案**：
1. **选中态改为内嵌圆角块 + 左侧指示条**：
   - 选中行：`fillRoundRect(RECT{rcItem.left+2, rcItem.top+1, rcItem.right-2, rcItem.bottom}, 6, LIST_SEL)`；
   - 再画左侧 3px 樱粉竖条：`fillRoundRect(RECT{rcItem.left+2, rcItem.top+3, rcItem.left+5, rcItem.bottom-4}, 1, theme::ACCENT)`。这是二次元游戏列表最常用的"选中位"语言。
2. **斑马纹**：`itemID % 2 == 1` 时行底填 `theme::ROW_ALT`（已定义未用），并把 `ROW_ALT` 调暗一点到 `RGB(56,46,88)`，只比 `BG` 亮一档，避免花。
3. **分隔线**：子行与组行之间天然有高度差，同级行之间画 1px `RGB(52,44,84)`（`BG` 与 `BORDER` 的中间值）横线：`fillRect(RECT{rcItem.left+8, rcItem.bottom-1, rcItem.right-8, rcItem.bottom}, ...)`。斑马纹和分隔线二选一即可，建议**只留分隔线**（更干净），`ROW_ALT` 删除。
4. 保护行（`canKill == false`）整行再加一层"降透明度"的 GDI 等价物：文字已用 `PROTECTED` 色，可再给该行背景混入 5% 灰紫 `RGB(44,38,74)`，与可结束行拉开一档。

**优先级**：P0。

---

## P1 — 下版本

### 5. 渐变绘制性能与质感：`GradientFill` 替换逐行 `CreateSolidBrush`

**现状**：`fillGradient` 对每条扫描线 `CreateSolidBrush → FillRect → DeleteObject`。360×540 面板一次 WM_PAINT 要创建/销毁 **540 个 GDI 画刷**，而面板每 2s 刷新一次、每次列表交互也触发 `InvalidateRect(hwnd_)`——这是白烧的 CPU/GDI 句柄压力，也是常驻工具的大忌。

**GDI 方案**：
1. `msimg32.dll` 是每个 Windows 都自带的系统 DLL（不算新依赖，exe 体积零增加）：`DrawUtils.cpp` 里静态加载一次：
   ```cpp
   static const auto pGradientFill = [] {
       HMODULE m = LoadLibraryW(L"msimg32.dll");
       return m ? reinterpret_cast<decltype(&::GradientFill)>(GetProcAddress(m, "GradientFill")) : nullptr;
   }();
   ```
2. 垂直渐变：两个 `TRIVERTEX`（top/bottom）+ 一个 `GRADIENT_RECT`，一次调用替代 540 次循环；`COLORREF→TRIVERTEX` 做 8bit→16bit 移位（`c<<8`）。水平渐变同理。
3. 保底分支：加载失败时退回现有逐行实现（实际上永远不会走到）。
4. 额外质感收益：`GradientFill` 的顶点模式还支持三角渐变，后面第 6 条的球体"假径向"可以直接用三个顶点的三角渐变做柔和过渡。

**优先级**：P1（性能 + 为后续渐变效果铺路）。

---

### 6. 悬浮球"宝石感"升级

**现状**：球体是垂直线性渐变（粉→紫）+ 左上一颗实心高光椭圆 + 3px 状态环。整体像"贴纸"而不是"宝石"：线性渐变方向感太强，高光是一块硬边白斑，状态环与渐变边缘之间没有过渡。

**GDI 方案**（全部纯 GDI，一次 WM_PAINT 内完成）：
1. **假径向渐变底**：先整圆填最深的底色 `RGB(120,92,200)`（球心偏下的紫色），再用 `GradientFill` 的三角顶点模式画一个从"球心偏左上"到球底的柔和过渡；若嫌三角渐变生硬，就退化为 3 层同心椭圆叠色：外层 `RGB(150,118,226)` → 中层 `RGB(196,140,214)` → 内层（球心左上偏移 4px）`RGB(255,172,206)`，每层椭圆半径递减 30%、圆心逐层向左上偏 2~3px，形成柔和的"内发光"。
2. **高光改两段式**：大椭圆高光保持，但在其内部再画一个更小、更亮的 `RGB(255,246,252)` 小椭圆，制造"两次反射"的玻璃感；高光外缘用 1px `RGB(255,214,232)` 描边软化硬边。
3. **底部反光弧**：在球下沿 1/4 处用 `Arc()`（`PS_SOLID, 2, RGB(255,190,220)`）画一条 120° 的细弧，模拟环境反光——一颗椭圆 + 一条弧，成本可忽略。
4. **状态环内移 1px 并加暗衬**：环描边从 `+2/-1` 改为统一内缩 3px（`left+3, top+3, right-3, bottom-3`，半径对称），并在环外侧先画一圈 1px `RGB(60,40,90)` 暗线，让状态色"嵌"进球体而不是浮在上面。
5. 文字阴影 `RGB(70,40,90)` 保留，但把百分比字号从 15 提到 16 并用 `FW_DEMIBOLD`，球上信息是 3m 外看的，再大胆一点。

**优先级**：P1。

---

### 7. 贴边条圆柱质感 + 竖排文字清晰度

**现状**：贴边条（22×88）用 `fillGradient(top→bottom)`——对竖条来说渐变方向是沿长度方向的，厚度方向纯平，看起来像纸条不像胶囊；竖排文字用 900/900 旋转角 + `CLEARTYPE_QUALITY`，**GDI 对旋转文字会关闭 ClearType**，12px 数字实际是灰阶锯齿渲染，发虚。

**GDI 方案**：
1. **沿厚度方向渐变**（竖条 = 水平渐变，横条 = 垂直渐变）：靠屏幕内侧一端亮 `RGB(88,74,140)`、贴屏幕边一端暗 `RGB(40,34,66)`，立刻有"圆柱/胶囊"的体积感。`fillGradientH`/`fillGradient` 已有能力，只是换方向。
2. **内高光线**：在半圆头内缘画 1px `RGB(160,140,220)` 竖线（`fillRect` 1px 宽），位置在离屏幕最远的 1/4 厚度处——一条高光就够。
3. **状态点加衬环**：状态点 6px 先画 8px 暗底（`RGB(50,40,80)`）再叠 6px 状态色，小屏幕上更醒目。
4. **竖排文字改"逐字堆叠"**：放弃 900/900 旋转，改为每个字符单独 `TextOutW`，从半圆头下方开始每字符占 11px 竖直堆叠（"85%" 3 个字符 ≈ 33px，"3.2G" 4 个 ≈ 44px，88px 长度装得下）。字符保持直立、`CLEARTYPE_QUALITY` 生效，清晰度立刻恢复；每个字符 x 居中（`SetTextAlign(TA_CENTER)`）。
5. 若嫌堆叠排版复杂，最低限度也应把 `verticalFont()` 的 quality 改为 `ANTIALIASED_QUALITY`（旋转文字下 ClearType 无效，显式声明灰阶抗锯齿至少边缘均匀）。

**优先级**：P1。

---

### 8. 装饰符号矢量化（✦ ✧ ♥ ♡ ↻ ★ ▼ ▶）

**现状**：面板标题、状态、按钮里大量使用 Unicode 字符（`✦ 进程管理`、`♥ 自动清理中`、`↻`、`★ %zu 个应用`、`▼/▶`）。GDI 没有自动字体回退，`✦(U+2726) ✧(U+2727) ♡(U+2661) ↻(U+21BB)` 在部分系统/字体链下会渲染成空心方框；即使能显示，13px 下这些字形发灰、和整体几何风格不统一。

**GDI 方案**（在 `DrawUtils` 增加几个一次性小函数，每个 10~20 行，体积可忽略）：
1. `drawStar(hdc, x, y, r, color)`：4 角星（夜樱主题用四角闪星 ✦ 形）——`Polygon` 8 个顶点：外接圆 4 点 + 内接圆 4 点（内半径 r*0.38），外点在 0°/90°/180°/270°。
2. `drawHeart(hdc, x, y, r, color)`：两个实心椭圆（圆心 `x±r/2, y-r/4`，半径 r/2）+ 一个倒三角（`Polygon` 三点：`x-r, y-r/4`、`x+r, y-r/4`、`x, y+r`）。描边版本用 `NULL_BRUSH` + pen。
3. `drawTriangle(hdc, rc, dir, color)`：替代 `▼/▶`，等边小三角 `Polygon`，展开/收起用同一函数换方向，比字体字形锐利得多。
4. `drawRefresh(hdc, x, y, r, color)`：`Arc()` 画 300° 圆弧 + 末端一个小实心三角箭头。
5. 标题行 `✦ 进程管理` 改为：先 `drawStar`（4px，LAVENDER）再 `TextOutW` 纯文字"进程管理"，x 偏移 star 右侧 6px；底部统计行的 `★` 同理换 `drawStar`（3px，TEXT_DIM）。
6. 按钮文字 `♥ 结束所选 (n)` 改为 `drawHeart` + 文字。

一次性把这些 12~16bit 码位从代码里清干净，任何系统上渲染一致。

**优先级**：P1。

---

### 9. 状态标签 pill 化 + 弱化文字对比度微调

**现状**："自动清理 / 系统"标签是 10px 纯文字，颜色 `PROTECTED RGB(140,130,168)` 直接压在 `BG/ROW_ALT` 上，小字号低对比，既不醒目也缺少"徽章"的游戏 UI 语言。

**GDI 方案**：
1. `Theme.hpp` 增加两个预混底色（无 alpha，直接给最终值）：
   - `TAG_BG_AUTO = RGB(74, 44, 76)`（樱粉压入夜空的暗粉紫）
   - `TAG_BG_SYS  = RGB(58, 50, 88)`（灰紫压入夜空）
2. 标签绘制：先用 `GetTextExtentPoint32W` 量宽，画 `fillRoundRect(RECT{x, cy-8, x+w+10, cy+8}, 8, TAG_BG_xxx)`，再居中 `DrawTextW`；文字色：自动清理 = `RGB(255,168,200)`（樱粉亮一档），系统 = `PROTECTED` 提亮到 `RGB(158,148,188)`（10px 文字对比度不够，整体提亮一档，同时第 4 条里保护行文字也受益）。
3. 标签右移贴住数值区左侧 8px 处（当前 `nameRc.right - 64` 的硬编码 64 改为实测宽度），避免长标签与内存数值相撞。

**优先级**：P1。

---

### 10. 交互反馈体系：hover 态全覆盖

**现状**：两个 owner-draw 按钮只有 `ODS_SELECTED` 按下态，没有 hover；标题栏的 `↻ / ✕` 是纯命中区域，无任何视觉反馈，用户不知道能点；悬浮球没有 hover 态（常驻桌面元素有 hover 反馈会显得"活"）。

**GDI 方案**：
1. `ProcessPanel` 增加 `TrackMouseEvent`（`WM_MOUSELEAVE`）：`WM_MOUSEMOVE` 时记录 hover 目标（kill 按钮 / 收起按钮 / `↻` / `✕`），变化时 `InvalidateRect` 对应小区域。
2. 按钮 hover：kill 按钮渐变色提亮一档（`RGB(255,150,190) → RGB(236,110,158)`）；收起按钮底色 `BG_CARD → BG_HOVER`（现成常量）。
3. `↻ / ✕` hover chip：hover 时在该图标外围画 `fillRoundRect(20×20, 4, BG_HOVER)` + 图标色从 `TEXT_DIM/LAVENDER` 提亮到 `TEXT_MAIN`；并 `SetCursor(LoadCursor(nullptr, IDC_HAND))`。
4. 悬浮球 hover（可选）：非拖拽、非贴边时状态环由 3px 变 4px（重绘一次即可，无定时器），暗示"可抓取"。
5. 所有 hover 判定逻辑集中在 `WM_MOUSEMOVE`，无定时器，空闲 CPU 依旧为 0。

**优先级**：P1。

---

### 11. 深色自绘滚动条

**现状**：listbox 带原生 `WS_VSCROLL`，经典灰色滚动条压在深紫主题上非常出戏（本进程没法用 `SetWindowTheme` 深色化 LISTBOX 滚动条）。

**GDI 方案**：
1. listbox 去掉 `WS_VSCROLL`；滚动逻辑自管：父窗口 `WM_MOUSEWHEEL` → `LB_SETTOPINDEX(top ± 3)`；拖拽 thumb 时 `WM_VSCROLL` 自拟消息。
2. 在 `onDrawItem` 之外、面板 `onPaint` 尾部（或 listbox 右缘覆盖绘制）画自绘滚动条：轨道 `fillRoundRect(4px 宽圆角条, RGB(56,46,88))`，thumb `fillRoundRect(6px 宽, 3, RGB(120,104,170))`，hover/拖拽时提亮为 `ACCENT`。thumb 高度 = 可视行高 / 总行高 比例，top = `LB_GETTOPINDEX / rows.size()`。
3. listbox 右侧预留 10px 空间（卡片化之后本来就有内边距），thumb 画在卡片右缘内 2px 处。
4. 行数少到不需要滚动时（`LB_GETCOUNT × 行高 ≤ 客户区高`）整条不画。

工作量中等但一步到位，是"游戏 UI 感"里最容易被注意到的细节之一。

**优先级**：P1。

---

### 12. 克制的动画：面板滑入 + 高占用呼吸（带定时器纪律）

**现状**：面板 `show()` 是瞬移出现；球体状态环完全静态。约束说 200ms 内简单定时器动画可以——目前一个都没用上，缺"活感"；但也绝不能加常驻动画。

**GDI 方案**（两条都遵守"动画结束即杀定时器，空闲 CPU 为 0"）：
1. **面板滑入**：`show()` 时从悬浮球一侧偏移 16px 起步（球在右则从右往左滑），用 `SetTimer(hwnd_, kAnimTimer, 16, nullptr)`，每帧 `SetWindowPos` 插值偏移（ease-out：`offset = 16 * (1-t)^2`），约 9 帧 / 150ms 后 `KillTimer` + 落位。仅位移、无 alpha、用现成 `CachedCanvas` 重绘，单帧成本 ≈ 一次普通 WM_PAINT。动画期间忽略新的 show 请求。隐藏不动画（直接 hide，避免残影）。
2. **高占用呼吸环**：仅当 `ms.percent >= 85` 时启动 `SetTimer(kPulseTimer, 200)`，让状态环宽度在 3px↔5px 间以正弦半周期摆动（每帧重画球，`Canvas` 已缓存）；回落到 <85% 时 `KillTimer` 并重画一次复原。正常占用下零定时器、零 CPU。
3. 纪律写进代码注释：任何新动画必须"有触发条件才 SetTimer、结束/失焦立刻 KillTimer"。

**优先级**：P1。

---

## P3 — 锦上添花

### 13. 夜樱点缀：静态花瓣 + 星尘（静态缓存，零运行时成本）

**现状**：主题叫"夜樱"，但整个 UI 里一片花瓣都没有——目前只有 ✦/♥ 字符点缀，主题辨识度全靠粉紫配色撑着。也正因如此，加装饰必须极度克制，避免"二次元贴纸化"。

**GDI 方案**：
1. `DrawUtils` 增加 `drawPetal(hdc, x, y, size, angle, color)`：樱花瓣 = 一个旋转椭圆 + 顶端 V 形缺口。GDI 无旋转椭圆，等价画法：`Polygon` 7~9 点近似花瓣轮廓（泪滴形，尖端朝下），缺口用背景色小三角或直接省略（远看就是花瓣）。预计算 2~3 组顶点缓存到 `static` 数组。
2. **面板**：header 渐变带右上角放 2 片花瓣（`RGB(120,86,150)` 与 `RGB(104,78,134)`——都是花瓣粉与 header 紫的预混暗色，克制到"若隐若现"），左下 footer 一片 + `drawStar` 一颗 2px 星尘（LAVENDER 压暗）。全部在 `onPaint` 的渐变之后、文字之前画，共 3 次 `Polygon`，无性能影响。
3. **悬浮球**：球面右上沿弧线放 1 片 4px 花瓣（压暗粉 `RGB(236,150,190)`），与高光错开；贴边条半圆头内放 0 片（太挤，不加）。
4. **不做花瓣飘落动画**：常驻工具，克制的静态点缀就是成熟度；真要动，只允许第 12 条的高占用呼吸。
5. 每片花瓣位置用窗口尺寸的相对坐标（`w*0.82, 10`），缩放安全。

**优先级**：P3。

---

## 附：整体检查清单（评审过程中确认过的既有优点，保持住）

- `CachedCanvas` 双缓冲 + `WM_ERASEBKGND` 返回 1，无闪烁、无重复分配——后续所有改动保持这个模式。
- `refreshData` 的"结构签名不变只重绘数值"策略很好，列表刷新不跳动。
- `usageColor` 三档状态色（天蓝→樱粉→深樱红）语义清晰，是全 UI 最好的"功能色"设计。
- 圆形窗口 `SetWindowRgn` 裁剪方案正确，`fillGradient` 逐行填充在换成 `GradientFill`（第 5 条）后也没有边缘接缝问题。

## 建议实施顺序

1. 第 1、2 条（两个渲染 bug，半天内）
2. 第 3、4 条（圆角体系 + 行层次，一次 PR 搞定面板观感的 70%）
3. 第 5 条（GradientFill，为 6/12/13 铺路）
4. 第 6、7、8 条（球体、贴边条、符号矢量化）
5. 第 9~12 条（pill、hover、滚动条、动画）
6. 第 13 条（点缀，最后加，加完整体回看一遍防止过饰）

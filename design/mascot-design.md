# FloatCleaner 看板娘角色设计（Mascot Design）

> 主题基准（取自 `src/ui/Theme.hpp`）：深蓝紫夜空底（`#221E3A → #382C5C`）、
> 樱花粉主强调（`#FF8AB2`）、天蓝辅助（`#7EC8FF`）、浅藤紫（`#C4A8FF`）、
> 星星/爱心点缀。产品关键词：轻量（140 KB exe）、贴边悬浮、安全白名单制清理。
>
> 用途：面板顶部 360×110 横幅、应用图标、GitHub 仓库封面、README 插图。

---

## 一、三个候选概念

### 概念 A：夜桜 ユイ（Yozakura Yui / 夜樱结衣）—— "记忆夜空的魔法少女"

- **外貌设定**：
  - 发色发型：樱花粉中长发（发梢渐变浅藤紫 `#C4A8FF`），头顶一根翘起的呆毛
    （形似内存占用曲线的小尖峰），左侧别一朵樱花发饰。
  - 瞳色：天蓝色 `#7EC8FF`，瞳孔里有星形高光。
  - 服装：深蓝紫斗篷短裙魔法服（对应夜空底色），裙摆与袖口缀樱花粉缎带，
    白色过膝袜配一双樱粉色小短靴。
  - 配饰：手持一把比人还高的**樱花扫帚**（扫帚穗是樱花枝+粉色花瓣），
    腰间挂一个小玻璃瓶，瓶里飘着几颗发光的"记忆泡泡"。
- **性格一句话**：嘴上说着"真是的，又堆了这么多垃圾进程呢"，手上却把屏幕
  边缘扫得一尘不染的傲娇勤劳型。
- **与功能的结合点**：
  - 樱花扫帚 = 清理动作本体：一挥扫走僵尸进程与内存垃圾。
  - 腰间玻璃瓶 = 悬浮球本体：收集被清掉的进程化作的泡泡，瓶身可显示占用率。
  - 呆毛尖峰 = 内存占用曲线：占用越高呆毛翘得越直（可作为面板里的小彩蛋表情）。
  - 白名单设定：她"祓除"前会先看名单，绝不乱碰——对应产品的三级保护名单。

### 概念 B：恵宮 つかさ（Megumiya Tsukasa / 惠宫司）—— "记忆神社的巫女"

- **外貌设定**：
  - 发色发型：黑紫色长直发（夜空色），齐刘海，两侧垂发用天蓝色发绳扎成
    小束，头上系白色巫女发带。
  - 瞳色：深樱红 `#FF5C7A`。
  - 服装：传统白上衣+朱绯色袴，但袴配色改为樱花粉×夜空紫的二次元渐变。
  - 配饰：手持**御币（祓串）**，纸垂改成一串樱花瓣形状；身前摆一个
    小型"绘马"看板，上面挂着白色符札（= 白名单封印）。
- **性格一句话**：一丝不苟、名单至上，"不在名册上的进程我一个手指头都不会碰"。
- **与功能的结合点**：
  - 祓串挥动 = 进程祓除仪式，纸垂花瓣飘散 = 释放内存的动画意象。
  - 符札 = 白名单/保护名单：贴了符札的进程她绝不祓除，直观传达"安全第一"。
  - 绘马看板 = 进程面板：挂在神社木架上的小牌子就像列表里的一行行进程。
- **风险**：巫女服元素在 360×110 超宽横幅里辨识度容易散掉，Q 版化后特征
  （御币+符札）也比"扫帚+泡泡"更难在 32~64 px 图标里读出来。

### 概念 C：天音 らむ（Amane Ram / 天音蓝梦）—— "住在内存里的数据精灵"

- **外貌设定**：
  - 发色发型：天蓝色 `#7EC8FF` 短双马尾（发梢泛白，像数据流的拖尾），
    发根处有一圈发光的环形天使环（= 内存条的缺口造型彩蛋）。
  - 瞳色：浅藤紫 `#C4A8FF`，瞳孔里是像素/星点纹理。
  - 服装：贴身的数据体操服式短裙，胸前有内存条形状的徽章，身后飘着
    半透明的全息披巾。
  - 配饰：拿一根**泡泡棒**，吹出的每颗泡泡里封着一个迷你的僵尸进程图标，
    泡泡破裂时化作樱花瓣消散。
- **性格一句话**：元气满满的电波系小不点，把杀进程叫做"戳泡泡"。
- **与功能的结合点**：
  - 泡泡 = 进程：悬浮球本身就叫"ball"，泡泡棒和悬浮球形态天然呼应。
  - 天使环 = 内存条缺口：一眼就能把角色和"内存"绑定。
  - 戳泡泡 = 结束进程的轻量化表达，降低"杀进程"的攻击感。
- **风险**：纯科技感与"夜樱"和风主题稍有割裂，樱花元素只能靠泡泡消散
  动画补足，横幅里夜空+樱花的氛围利用率不如概念 A。

---

## 二、选定推荐：概念 A —— 夜桜 ユイ（Yozakura Yui）

**理由**：
1. 名字即主题："夜桜"直接对应"夜樱二次元主题"，横幅里深蓝紫夜空 + 樱花
   粉的角色配色不需要任何妥协。
2. 道具可缩放性最好：扫帚（大剪影）+ 玻璃瓶泡泡（小图形）在 360×110 横幅、
   1:1 徽章、甚至 32 px 图标里都能保住辨识度；概念 B 的御币、概念 C 的
   全息披巾在极小尺寸下都会糊成一团。
3. "扫帚扫走垃圾 + 瓶子收集" 的隐喻和"清理 + 释放内存"一一对应，
   傲娇勤劳的性格也贴合"防卡顿工具本身不该成为卡顿源"的产品宣言。

**角色三视图关键词（供后续生图复用）**：sakura-pink hair with lavender
tips, ahoge, star-shaped highlight sky-blue eyes, navy-purple cape dress,
sakura hairpin, giant sakura broom, glass bottle with glowing bubbles.

---

## 三、AI 生图提示词

### 3.1 横幅用（面板顶部 360×110，左侧留白给标题）

生成时建议按 3:1 或更宽（如 1440×480 / 2880×960）出图，然后裁剪为 360×110。

**正面提示词（Prompt）**：

```text
anime chibi magical girl mascot, waist-up, positioned on the right third of the frame, looking left with a gentle smile, sakura-pink medium hair with lavender-purple gradient tips, one prominent ahoge curling like a heartbeat line, large sky-blue eyes with star-shaped highlights, small sakura flower hairpin, wearing a deep navy-purple cape dress with sakura-pink ribbons, holding a giant broom made of cherry blossom branches with glowing pink petals, a small floating glass bottle beside her filled with tiny glowing bubbles, deep blue-purple night sky background with vertical gradient from dark indigo to soft violet, scattered glowing stars, drifting sakura petals, soft bokeh light particles, flat clean cel-shading, pastel color palette of pink #FF8AB2, sky blue #7EC8FF and lavender #C4A8FF, cute night-sakura theme, ultra-wide horizontal banner composition 3:1, character occupying only the right 30% of the canvas, the entire left 70% is clean empty gradient sky reserved for a title, no text, no letters, no logo, no watermark, no signature, no border
```

**负面提示词（Negative / 如工具支持）**：

```text
text, watermark, signature, letters, logo, border, frame, character on the left side, crowded composition, realistic, 3d render, photo, dark horror, extra fingers, deformed hands
```

**构图与裁剪说明**：
- 角色放在画面右侧约 30% 区域，左侧 70% 保持纯净的夜空渐变，用于叠加
  "FloatCleaner" 标题与副标题。
- 裁剪为 360×110 时从出图垂直方向取角色**腰部以上**的区域：头部顶到画面
  上缘约 8% 处，扫帚穗与泡泡落在角色周围、不越过中线。
- 背景渐变方向与 `Theme.hpp` 一致（上 `#221E3A` → 下 `#382C5C`），
  直接叠进面板时无需抠图，可整体作为 header 位图。

### 3.2 图标用（1:1、居中、Q 版徽章式、纯色背景便于抠图）

**正面提示词（Prompt）**：

```text
cute chibi anime girl head icon, perfectly centered, symmetrical front view, bust only, sakura-pink hair with lavender-purple gradient tips, one small ahoge on top, large sparkling sky-blue eyes with star highlights, tiny blush marks, gentle closed smile, sakura flower hairpin on the left side, wearing a navy-purple hood shaped like a night sky with tiny star prints, flat vector style sticker, bold clean outlines, simple cel shading, kawaii mascot emblem, plain solid single flat pastel lavender background color #C4A8FF, the character fills 80% of the square canvas with even margins on all sides, 1:1 square composition, no text, no watermark, no shadow on background, no gradient background, no decorative elements outside the character
```

**负面提示词（Negative / 如工具支持）**：

```text
text, watermark, signature, background scene, stars or petals on the background, gradient background, complex background, realistic, 3d, photo, cropped head, off-center
```

**抠图说明**：
- 背景指定为单一纯色浅藤紫 `#C4A8FF`（与角色发梢同色系但明度不同），
  魔棒/颜色容差一键去除；若发梢半透明边缘带紫色，可改指定纯白背景
  并用"正片叠底感知"的抠图流程。
- 抠图后缩到 256 / 64 / 32 / 16 px 检查 16 px 下呆毛与樱花发饰是否仍可读，
  不可读时为 16 px 单独出一张"只保留头+呆毛"的极简版。

### 3.3 GitHub 仓库封面 / README 插图复用

- 仓库封面（社交预览 1280×640）：沿用 3.1 提示词，把比例改为 2:1、
  在提示词末尾将 "left 70%" 改为 "left 55%"，让角色占比更大，
  标题空间依然充足。
- README 插图：直接复用横幅成图（360×110 原尺寸或 2x = 720×220），
  居中插入即可；另可放一张 3.2 的透明底头像做"贡献者徽章"式点缀。

---

## 四、程序绘制保底方案（纯 GDI，AI 生图不可用时）

目标：不依赖任何图片资源，用 `Ellipse` / `Polygon` / `Pie` / `RoundRect`
画一个 96×96（逻辑坐标，可整体缩放）的 Q 版ユイ头像，
供悬浮球、托盘图标或面板标题使用。所有颜色取自 `Theme.hpp` 或邻近色。

### 4.1 画布与整体布局

- 画布 96×96，中心线 x = 48。
- 分层顺序：后发 → 脸 → 五官 → 前发（刘海）→ 呆毛/发饰 → 腮红/嘴 → 描边。
- 统一描边色 `RGB(70, 56, 110)`（深夜紫，比纯黑柔和）。

### 4.2 逐步绘制

1. **后发（头发主体轮廓）**：一个大椭圆盖住整个头后方。
   - `Ellipse(10, 12, 86, 92)`，填充樱花粉 `ACCENT = RGB(255,138,178)`。
   - 发梢渐变：在椭圆底部再叠一个 `Ellipse(16, 66, 80, 94)`，
     填充浅藤紫 `LAVENDER = RGB(196,168,255)`（用 `CreateRoundRgn` 或直接
     覆盖绘制即可，Q 版不必真做渐变）。
2. **两侧垂发**：两条竖长椭圆贴在脸两侧，制造"中长发"轮廓。
   - 左 `Ellipse(10, 30, 30, 88)`，右 `Ellipse(66, 30, 86, 88)`，
     同樱花粉；底端各叠小椭圆 `RGB(196,168,255)` 做渐变梢。
3. **脸**：圆形脸压在后发之上。
   - `Ellipse(20, 26, 76, 82)`，填充肤色 `RGB(255,236,226)`，
     描边 `RGB(70,56,110)`。
4. **眼睛**（天蓝大眼，两颗）：
   - 眼白可省略（Q 版直接画瞳）；左 `Ellipse(30, 48, 40, 62)`，
     右 `Ellipse(56, 48, 66, 62)`，填充 `SKY = RGB(126,200,255)`。
   - 瞳孔上缘：各叠一个小椭圆 `RGB(58,120,200)`（深天蓝）压住瞳孔上半，
     形成上深下浅。
   - 星形高光：在每只瞳孔左上角画 4~5 点的星形多边形 `Polygon`，
     填充纯白 `RGB(255,255,255)`；再补一个 1~2 px 的白色小圆点在右下角。
   - 眉毛：两条 2 px 短弧线（`Arc` 或细 `RoundRect`），色 `RGB(200,110,150)`。
5. **嘴**：小弧线微笑。
   - `Arc(43, 66, 53, 74, ...)` 取下半弧，画笔 `RGB(160,60,90)`，2 px。
6. **腮红**：两颗半透明粉色椭圆（GDI 无 alpha 时用比肤色深的实色近似）。
   - 左 `Ellipse(24, 60, 34, 66)`，右 `Ellipse(62, 60, 72, 66)`，
     填充 `RGB(255,170,196)`。
7. **刘海（前发，关键辨识层）**：一排圆弧齿形多边形盖在脸上缘。
   - 用 `Polygon` 画一条从左 `(20, 44)` 到右 `(76, 44)` 的波浪顶边：
     5 个下垂的圆齿（每齿宽约 11 px，齿尖低至 y≈52，齿谷回到 y≈40），
     顶边上方收到 `(48, 18)` 的弧形发际线；整体填充樱花粉，
     描边 `RGB(70,56,110)`。
   - 中间一缕用 `RGB(196,168,255)` 画一条竖向小三角做"挑染"，呼应渐变发梢。
8. **呆毛（ahoge，最易识别的特征，必画）**：
   - `Polygon`：底端两点 `(46, 20)`、`(50, 20)`，尖端 `(54, 6)`，
     用二次弧感（可拆两段 `PolylineTo`）让它向右卷出一个小钩——
     像一条翘起的占用率曲线。填充樱花粉，描边同上。
9. **樱花发饰**（左侧鬓发上）：
   - 5 片花瓣：以 `(22, 40)` 为中心，画 5 个小椭圆（每片约 6×4 px，
     绕中心旋转 72°，GDI 下可预先算好多边形顶点用 `Polygon` 一次画），
     填充 `RGB(255,190,214)`（比头发浅的樱粉）；
     中心一个 2 px 黄色小圆 `RGB(255,214,120)` 做花蕊。
10. **（可选）悬浮球彩蛋**：在头像右下角画一颗 14 px 小球——
    `Ellipse(74, 74, 88, 88)`，填充 `SKY`，加白高光点，作为"腰间玻璃瓶"
    的 Q 版替身，暗示产品本体。

### 4.3 伪代码骨架

```cpp
// 96x96 逻辑坐标，按需 ScaleWindowExtEx 缩放
COLORREF edge = RGB(70, 56, 110), skin = RGB(255, 236, 226);
COLORREF pink = RGB(255, 138, 178), lav = RGB(196, 168, 255);
COLORREF sky  = RGB(126, 200, 255), blush = RGB(255, 170, 196);

FillEllipse(hdc, 10, 12, 86, 92, pink, edge);   // 1 后发
FillEllipse(hdc, 16, 66, 80, 94, lav,  edge);   //   发梢渐变
FillEllipse(hdc, 10, 30, 30, 88, pink, edge);   // 2 左垂发
FillEllipse(hdc, 66, 30, 86, 88, pink, edge);   //   右垂发
FillEllipse(hdc, 20, 26, 76, 82, skin, edge);   // 3 脸
FillEllipse(hdc, 30, 48, 40, 62, sky,  edge);   // 4 左眼
FillEllipse(hdc, 56, 48, 66, 62, sky,  edge);   //   右眼（+深色上缘/星高光/白点）
DrawArc (hdc, 43, 66, 53, 74, RGB(160,60,90));  // 5 嘴（下半弧）
FillEllipse(hdc, 24, 60, 34, 66, blush, NULL);  // 6 腮红 ×2
FillPoly  (hdc, bangsPts,  pink, edge);         // 7 刘海圆齿多边形
FillPoly  (hdc, ahogePts,  pink, edge);         // 8 呆毛（钩状曲线多边形）
DrawSakura(hdc, 22, 40, 6);                     // 9 五瓣樱花 + 黄花蕊
FillEllipse(hdc, 74, 74, 88, 88, sky, edge);    // 10 可选悬浮球彩蛋
```

### 4.4 缩放与使用建议

- 该头像在 96 px 以上效果最好；16 px 托盘图标建议只保留"后发圆 + 刘海 +
  双眼 + 呆毛"五个元素（去掉腮红、嘴、花饰、彩蛋），否则糊成一团。
- 若横幅也要程序绘制保底：以夜空渐变（`BG_TOP → BG_BOTTOM`）铺满 360×110，
  左 70% 留白，右侧放本头像（约 96 px）+ 两三片 `Polygon` 花瓣 +
  数颗 1~2 px 白色星点即可，无需额外素材。
- 所有坐标写成一个 `Scale(s)` 包装（`x*s`）即可一套代码多尺寸复用。

# FloatCleaner

> Windows 进程清理悬浮窗 · Win32 C++ · 单 exe 约 140 KB · 自身内存占用约 10 MB

一个贴在屏幕边缘的小悬浮窗，实时显示内存占用；点开就是进程管理面板，
手动结束高占用进程，也可以让"智能清理"在内存吃紧时自动结束你指定的后台进程。
防卡顿工具本身不该成为卡顿源——所以用纯 Win32 API 编写，无任何运行时依赖。

## 功能

### 悬浮窗（双形态）

- **自由状态（圆形小球）**：显示内存占用率百分比（绿 → 黄 → 红）与已用/总内存
  （如 `5.2/16G`）；按住可拖动。
- **吸附状态（贴边条）**：拖到屏幕左/右/上/下边缘附近（约 36px 内）自动吸附成
  半圆头小条，显示状态点、占用率与已用内存；点击贴边条会弹回球体。
- 位置与形态自动记忆（exe 同目录 `FloatCleaner.ini`）。
- 右键菜单：打开/收起面板、智能自动清理开关、退出。

### 进程面板（按应用分组）

- 点击悬浮球展开，**同一应用的多个进程聚合为一组**（如 `chrome.exe ×12`，显示合计
  内存与 CPU），双击组行展开查看子进程（PID 级）
- **列表不跳动**：定时刷新只更新数值，不重排、不重置滚动位置；打开面板或点标题栏
  "↻" 才按内存重新扫描排序
- 勾选组 = 结束该应用全部进程；也可以展开后只结束单个子进程；确认框列出明细
- 右键组行/子进程：
  - 结束（组 = 该应用全部进程）
  - 加入白名单（同名全部进程永不结束）/ 从白名单移除
  - 加入自动清理名单 / 从自动清理名单移除
- 系统进程以"系统"标记灰显，不可结束

### 智能自动清理（opt-in 白名单制，安全第一）

- **只结束你显式加入"自动清理名单"的进程**，绝不自动杀其他任何进程。
- 触发条件：全局内存占用超过阈值（默认 85%，可配）才动作，默认每 60 秒检查一次。
- 动作后弹托盘气泡通知（结束了什么、释放了多少），并写入 `logs/autoclean.log`，
  全程可追溯。

## 安全设计

| 层级 | 内容 | 规则 |
|------|------|------|
| 硬保护 | System/lsass/svchost/dwm/explorer 等内置名单 + `C:\Windows\` 目录下所有进程 + 自身 | 永不结束，UI 灰显 |
| 用户白名单 | `FloatCleaner.ini` `[whitelist]` | 手动/自动都不杀 |
| 自动清理名单 | `FloatCleaner.ini` `[autocleanlist]` | 手动可杀；内存超阈值时自动杀 |
| 普通进程 | 其他 | 仅手动结束（需确认） |

所有结束操作在执行前都会经过 `ProcessKiller` 的保护等级二次校验。

## 编译

需要 [MinGW-w64](https://winlibs.com/)（g++ 与 windres 在 PATH 中）：

```bat
build.bat
```

或使用 CMake：

```bat
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

生成 `FloatCleaner.exe`（约 140 KB），双击即用，无需安装。

命令行参数：`FloatCleaner.exe --panel` 启动时直接展开进程面板。

## 配置文件（FloatCleaner.ini，运行后自动生成）

```ini
[general]
ballX=2404          ; 悬浮窗位置锚点
ballY=628
dockEdge=2          ; 0=自由球体 1=左 2=右 3=上 4=下

[autoclean]
enabled=1           ; 智能清理开关
interval_sec=60     ; 检查周期（最小 10）
memory_threshold=85 ; 内存占用阈值（%）

[whitelist]
names=wechat.exe,cloudmusic.exe   ; 永不结束的进程

[autocleanlist]
names=xunlei_update.exe           ; 内存超阈值时允许自动结束的进程
```

## 以管理员身份运行

普通权限即可结束大多数用户程序。若需要结束管理员权限的进程
（面板提示"拒绝访问"时），右键 `FloatCleaner.exe` → "以管理员身份运行"。

## 项目结构

```
src/
├── main.cpp            # 入口：模块组装、主菜单、托盘、单实例
├── app/AppContext.hpp  # 全局上下文与公共消息/菜单 ID
├── core/               # 纯逻辑层（不依赖 UI）
│   ├── ProcessScanner  # Toolhelp 枚举 + 内存 + CPU 差分采样
│   ├── ProtectionList  # 三级保护名单与分类
│   ├── ProcessKiller   # 结束进程 + 保护校验
│   └── SystemMonitor   # 全局内存状态
├── config/             # ini 读写与默认值
├── auto/AutoCleaner    # 阈值触发的智能清理 + 日志
└── ui/                 # 悬浮球 / 进程面板 / 托盘 / 绘制工具 / 主题
```

## License

MIT

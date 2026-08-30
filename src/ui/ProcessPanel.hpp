#pragma once
#include <windows.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "DrawUtils.hpp"

namespace fc {

// 面板行：组行（同名 exe 聚合）或子行（单个进程）
struct PanelRow {
    bool isGroup = true;
    std::wstring name;
    std::vector<uint32_t> pids; // 组: 全部成员；子行: 单个
    bool expanded = false;      // 仅组行
};

// 进程列表面板：按应用分组（腾讯管家式），支持多选结束与名单管理
//
// 行模型：
//   组行  —— 同名 exe 聚合，显示合计内存/CPU，双击展开子进程
//   子行  —— 组内单个进程（pid 级）
//
// 稳定性设计：
//   - 定时刷新只更新数值（不重排、不重置滚动位置）
//   - 仅在进程集合变化或用户点"↻"时重建行模型，重建时恢复滚动与勾选
class ProcessPanel {
public:
    bool create(HINSTANCE hInstance);
    void show();
    void hide();
    bool visible() const { return visible_; }
    HWND hwnd() const { return hwnd_; }

private:
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam);
    void onPaint();
    void onDrawItem(const DRAWITEMSTRUCT* dis);
    void onMeasureItem(MEASUREITEMSTRUCT* mis);
    void refreshData();        // 定时刷新入口
    void rebuildRows();        // 重新分组 + 重排（保持滚动与勾选）
    void killSelected();
    void onListContextMenu(LPARAM lParam);
    void onListDoubleClick();
    void rescanAndResort();
    void syncConfigFromProtection();

    HWND hwnd_ = nullptr;
    HWND listBox_ = nullptr;
    DrawUtils::CachedCanvas buffer_;
    bool visible_ = false;

    std::vector<PanelRow> rows_;
    std::vector<uint32_t> lastPids_; // 行模型对应的 pid 集合（升序），用于结构变化检测
    std::unordered_map<uint32_t, size_t> pidIndex_; // pid -> procs 下标
    std::set<std::wstring> expandedNames_;
};

} // namespace fc

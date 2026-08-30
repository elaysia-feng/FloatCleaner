#pragma once
#include <windows.h>

#include "DrawUtils.hpp"

namespace fc {

// 悬浮窗双形态：
//  自由状态 —— 圆形小球，显示内存占用率，可拖动
//  吸附状态 —— 贴屏幕边缘的小条（左/右/上/下），点击弹回球体
enum class Dock {
    None,
    Left,
    Right,
    Top,
    Bottom,
};

class FloatingBall {
public:
    bool create(HINSTANCE hInstance);
    void show();
    HWND hwnd() const { return hwnd_; }

private:
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam);

    void onPaint();
    void onTimer(UINT_PTR id);
    void onMouseDown(LPARAM lParam);
    void onMouseMove(LPARAM lParam);
    void onMouseUp();
    void onContextMenu(LPARAM lParam);

    // 根据当前形态与锚点，重设窗口位置/尺寸/区域
    void applyGeometry();
    void dockTo(Dock edge);
    void undockToBall(int centerX, int centerY);
    bool isDocked() const { return dock_ != Dock::None; }
    POINT ballCenter() const;

    HWND hwnd_ = nullptr;
    DrawUtils::CachedCanvas buffer_;
    Dock dock_ = Dock::Right;
    bool dragging_ = false;
    bool moved_ = false;
    bool hover_ = false;       // 鼠标悬停（环加粗暗示可抓取）
    bool pulseActive_ = false; // 高占用呼吸环定时器是否在跑
    int pulsePhase_ = 0;       // 呼吸相位（决定环宽 3~5px）
    POINT dragStart_{};
    POINT windowStart_{};
};

} // namespace fc

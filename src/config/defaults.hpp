#pragma once

namespace fc::defaults {

constexpr int kBallSize = 64;      // 自由状态：圆形球体直径
constexpr int kDockLength = 88;    // 吸附状态：贴边条长度
constexpr int kDockThickness = 22; // 吸附状态：贴边条厚度
constexpr int kSnapDistance = 36;  // 拖动时触发吸附的边缘距离

constexpr int kPanelWidth = 360;
constexpr int kPanelHeight = 540;
constexpr int kPanelRadius = 12;

constexpr int kRefreshIntervalMs = 2000; // 进程列表刷新周期
constexpr int kDefaultIntervalSec = 60;  // 自动清理检查周期
constexpr int kDefaultMemoryThreshold = 85;

constexpr wchar_t kIniFile[] = L"FloatCleaner.ini";
constexpr wchar_t kLogDir[] = L"logs";
constexpr wchar_t kLogFile[] = L"autoclean.log";
constexpr wchar_t kAppTitle[] = L"FloatCleaner";
constexpr wchar_t kMutexName[] = L"FloatCleaner.SingleInstance";

} // namespace fc::defaults

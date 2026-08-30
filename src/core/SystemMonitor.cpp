#include "SystemMonitor.hpp"

#include <windows.h>

namespace fc {

MemoryStatus SystemMonitor::query()
{
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    MemoryStatus st;
    if (GlobalMemoryStatusEx(&ms)) {
        st.percent = ms.dwMemoryLoad;
        st.totalBytes = ms.ullTotalPhys;
        st.availBytes = ms.ullAvailPhys;
    }
    return st;
}

} // namespace fc

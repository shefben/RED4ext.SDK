#include "SectorLODController.hpp"
#include "../net/Connection.hpp"
#include "../net/Net.hpp"
#include "SnapshotHeap.hpp"
#include "TextureGuard.hpp"
#include "RenderDevice.hpp"
#include <iostream>
#ifdef __GLIBC__
#include <sys/sysinfo.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace CoopNet
{
static float g_timer = 0.f;
static uint8_t g_currentLod = 0;

void SectorLODController_Tick(float dt)
{
    g_timer += dt;
    if (g_timer < 30.f)
        return;
    g_timer = 0.f;
    float memRatio = 0.f;
#ifdef __GLIBC__
    struct sysinfo si;
    if (sysinfo(&si) == 0)
        memRatio = 1.f - (float)si.freeram / (float)si.totalram;
#elif defined(_WIN32)
    MEMORYSTATUSEX st;
    st.dwLength = sizeof(st);
    if (GlobalMemoryStatusEx(&st))
    {
        unsigned long long total = st.ullTotalPhys;
        unsigned long long avail = st.ullAvailPhys;
        if (total > 0)
            memRatio = 1.f - static_cast<float>(avail) / static_cast<float>(total);
    }
#endif
    float vramRatio = 0.f;
    float budget = RenderDevice_GetVRAMBudget();
    if (budget > 0.f)
        vramRatio = RenderDevice_GetVRAMUsage() / budget;
    uint8_t target = g_currentLod;
    if ((memRatio > 0.8f || vramRatio > 0.95f))
        target = 1;
    else if (memRatio < 0.7f && vramRatio < 0.85f)
        target = 0;
    if (target != g_currentLod)
    {
        g_currentLod = target;
        for (auto* c : Net_GetConnections())
            Net_BroadcastSectorLOD(c->currentSector, g_currentLod);
        std::cerr << "[Perf] Sector LOD -> " << int(g_currentLod) << std::endl;
    }
}

} // namespace CoopNet

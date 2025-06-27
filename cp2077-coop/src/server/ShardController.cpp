#include "ShardController.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include <cstdlib>

namespace CoopNet
{
static bool g_active = false;
static uint32_t g_phase = 0;
static uint32_t g_seed = 0;
static float g_progress = 0.f;
static float g_send = 0.f;

void ShardController_Start(uint32_t phaseId)
{
    g_active = true;
    g_phase = phaseId;
    g_seed = static_cast<uint32_t>(std::rand());
    g_progress = 0.f;
    g_send = 0.f;
    Net_BroadcastTileGameStart(phaseId, g_seed);
}

void ShardController_HandleSelect(uint32_t peerId, uint8_t row, uint8_t col)
{
    if (!g_active)
        return;
    Net_BroadcastTileSelect(peerId, g_phase, row, col);
}

void ShardController_ServerTick(float dt)
{
    if (!g_active)
        return;
    g_progress += dt / 10000.f * 100.f; // 10s full
    g_send += dt;
    if (g_send >= 500.f)
    {
        g_send = 0.f;
        uint8_t pct = static_cast<uint8_t>(g_progress);
        if (pct > 100)
            pct = 100;
        Net_BroadcastShardProgress(g_phase, pct);
        if (pct >= 100)
            g_active = false;
    }
}

} // namespace CoopNet

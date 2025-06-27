#include "PoliceDispatch.hpp"
#include "../core/Hash.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include <RED4ext/RED4ext.hpp>
#include <cstring>

namespace CoopNet
{
static uint32_t g_timer = 0;
static uint8_t g_waveIdx = 0;
static uint8_t g_heat = 0;
static uint32_t g_maxtac = 0;

void PoliceDispatch_OnHeatChange(uint8_t level)
{
    if (level > g_heat)
    {
        g_timer = 0;
        g_waveIdx = 0;
    }
    g_heat = level;
    RED4ext::ExecuteFunction("PoliceDispatch", "OnHeat", nullptr, level);
}

void PoliceDispatch_Tick(float dt)
{
    if (g_heat == 0)
        return;
    g_timer += static_cast<uint32_t>(dt);
    g_maxtac += static_cast<uint32_t>(dt);
    uint32_t interval = (g_heat >= 3 ? 15000u : 30000u);
    if (g_timer >= interval)
    {
        g_timer = 0;
        uint32_t seeds[4];
        uint32_t count = static_cast<uint32_t>(Net_GetConnections().size());
        for (int i = 0; i < 4; ++i)
            seeds[i] = Fnv1a32(std::to_string(count).c_str()) ^ (g_waveIdx * 31u + i);
        Net_BroadcastNpcSpawnCruiser(g_waveIdx, seeds);
        g_waveIdx++;
    }
    if (g_heat >= 5)
    {
        if (g_maxtac >= 60000u)
        {
            g_maxtac = 0;
            Net_BroadcastCineStart(Fnv1a32("maxtac_av"), 0, 0, false);
        }
    }
    else
    {
        g_maxtac = 0;
    }
}
} // namespace CoopNet

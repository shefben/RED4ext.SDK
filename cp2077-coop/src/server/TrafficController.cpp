#include "TrafficController.hpp"
#include "../core/GameClock.hpp"
#include "../net/Net.hpp"
#include <unordered_map>

namespace CoopNet
{
static float g_seedTimer = 0.f;

void TrafficController_Tick(float dtMs)
{
    g_seedTimer += dtMs;
    if (g_seedTimer >= 10000.f)
    {
        g_seedTimer = 0.f;
        uint64_t sector = 0;
        if (!Net_GetConnections().empty())
            sector = Net_GetConnections()[0]->currentSector;
        uint64_t seed = static_cast<uint64_t>(CoopNet::GameClock::GetCurrentTick());
        Net_BroadcastTrafficSeed(sector, seed);
    }
}

void TrafficController_OnDespawn(uint32_t vehId)
{
    Net_BroadcastTrafficDespawn(vehId);
}

} // namespace CoopNet

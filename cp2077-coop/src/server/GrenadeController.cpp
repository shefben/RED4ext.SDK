#include "GrenadeController.hpp"
#include "../core/GameClock.hpp"
#include "../net/Net.hpp"
#include <unordered_map>

namespace CoopNet
{
struct GrenadeState
{
    uint32_t start;
    RED4ext::Vector3 pos;
    RED4ext::Vector3 vel;
    float timer;
};
static std::unordered_map<uint32_t, GrenadeState> g_map;

void GrenadeController_Prime(uint32_t entityId, uint32_t startTick)
{
    g_map[entityId] = {startTick, {}, {}, 0.f};
    GrenadePrimePacket pkt{entityId, startTick};
    Net_Broadcast(EMsg::GrenadePrime, &pkt, sizeof(pkt));
}

void GrenadeController_Remove(uint32_t entityId)
{
    g_map.erase(entityId);
}

void GrenadeController_Tick(float dt)
{
    for (auto it = g_map.begin(); it != g_map.end();)
    {
        it->second.timer += dt;
        if (it->second.timer >= 50.f)
        {
            it->second.timer = 0.f;
            GrenadeSnapPacket pkt{it->first, it->second.pos, it->second.vel};
            Net_Broadcast(EMsg::GrenadeSnap, &pkt, sizeof(pkt));
        }
        ++it;
    }
}
} // namespace CoopNet

#include "CarryController.hpp"
#include "../net/Net.hpp"
#include <unordered_map>
#include <mutex>

namespace CoopNet
{
struct CarryState
{
    uint32_t carrier;
    float timer;
};
static std::unordered_map<uint32_t, CarryState> g_map;
static std::mutex g_mapMutex;

void CarryController_Begin(uint32_t carrierId, uint32_t entityId)
{
    std::lock_guard lock(g_mapMutex);
    g_map[entityId] = {carrierId, 0.f};
    CarryBeginPacket pkt{carrierId, entityId};
    Net_Broadcast(EMsg::CarryBegin, &pkt, sizeof(pkt));
}

void CarryController_End(uint32_t entityId, const RED4ext::Vector3& pos, const RED4ext::Vector3& vel)
{
    CarryEndPacket pkt{entityId, pos, vel};
    Net_Broadcast(EMsg::CarryEnd, &pkt, sizeof(pkt));
    std::lock_guard lock(g_mapMutex);
    g_map.erase(entityId);
}

void CarryController_Tick(float dt)
{
    std::lock_guard lock(g_mapMutex);
    for (auto& kv : g_map)
    {
        kv.second.timer += dt;
        if (kv.second.timer >= 100.f)
        {
            kv.second.timer = 0.f;
            // No-op: do not broadcast empty/unknown position updates
        }
    }
}
} // namespace CoopNet

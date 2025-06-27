#include "CarryController.hpp"
#include "../net/Net.hpp"
#include <unordered_map>

namespace CoopNet
{
struct CarryState
{
    uint32_t carrier;
    float timer;
};
static std::unordered_map<uint32_t, CarryState> g_map;

void CarryController_Begin(uint32_t carrierId, uint32_t entityId)
{
    g_map[entityId] = {carrierId, 0.f};
    CarryBeginPacket pkt{carrierId, entityId};
    Net_Broadcast(EMsg::CarryBegin, &pkt, sizeof(pkt));
}

void CarryController_End(uint32_t entityId, const RED4ext::Vector3& pos, const RED4ext::Vector3& vel)
{
    CarryEndPacket pkt{entityId, pos, vel};
    Net_Broadcast(EMsg::CarryEnd, &pkt, sizeof(pkt));
    g_map.erase(entityId);
}

void CarryController_Tick(float dt)
{
    for (auto& kv : g_map)
    {
        kv.second.timer += dt;
        if (kv.second.timer >= 100.f)
        {
            kv.second.timer = 0.f;
            RED4ext::Vector3 pos{};
            RED4ext::Vector3 vel{};
            CarrySnapPacket pkt{kv.first, pos, vel};
            Net_Broadcast(EMsg::CarrySnap, &pkt, sizeof(pkt));
        }
    }
}
} // namespace CoopNet

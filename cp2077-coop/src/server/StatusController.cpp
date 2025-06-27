#include "StatusController.hpp"
#include "../net/Net.hpp"
#include "DamageValidator.hpp"
#include <vector>

namespace CoopNet
{
struct StatusEntry
{
    uint32_t targetId;
    uint8_t effectId;
    uint8_t amp;
    uint16_t remaining;
    uint16_t tickTimer;
};

static std::vector<StatusEntry> g_entries;

void StatusController_OnApply(Connection* src, const StatusApplyPacket& pkt)
{
    StatusEntry e{pkt.targetId, pkt.effectId, pkt.amp, pkt.durMs, 0};
    g_entries.push_back(e);
    Net_BroadcastStatusApply(pkt.targetId, pkt.effectId, pkt.durMs, pkt.amp);
}

void StatusController_Tick(float dt)
{
    for (auto it = g_entries.begin(); it != g_entries.end();)
    {
        it->remaining = (it->remaining > dt) ? static_cast<uint16_t>(it->remaining - dt) : 0;
        it->tickTimer += static_cast<uint16_t>(dt);
        if (it->tickTimer >= 500)
        {
            it->tickTimer -= 500;
            Net_BroadcastStatusTick(it->targetId, -static_cast<int16_t>(it->amp));
        }
        if (it->remaining == 0)
            it = g_entries.erase(it);
        else
            ++it;
    }
}

} // namespace CoopNet

#include "StatusController.hpp"
#include "../net/Net.hpp"
#include "DamageValidator.hpp"
#include <vector>
#include <mutex>

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
static std::mutex g_entriesMutex;

void StatusController_OnApply(Connection* src, const StatusApplyPacket& pkt)
{
    StatusEntry e{pkt.targetId, pkt.effectId, pkt.amp, pkt.durMs, 0};
    std::lock_guard lock(g_entriesMutex);
    g_entries.push_back(e);
    Net_BroadcastStatusApply(pkt.targetId, pkt.effectId, pkt.durMs, pkt.amp);
}

void StatusController_Tick(float dt)
{
    std::lock_guard lock(g_entriesMutex);
    for (auto it = g_entries.begin(); it != g_entries.end();)
    {
        uint16_t delta = static_cast<uint16_t>(dt > 65535.f ? 65535.f : (dt < 0.f ? 0.f : dt));
        it->remaining = (it->remaining > delta) ? static_cast<uint16_t>(it->remaining - delta) : 0;
        uint32_t tt = static_cast<uint32_t>(it->tickTimer) + delta;
        it->tickTimer = static_cast<uint16_t>(tt > 0xFFFF ? 0xFFFF : tt);
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

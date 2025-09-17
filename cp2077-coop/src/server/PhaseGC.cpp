#include "PhaseGC.hpp"
#include "../core/GameClock.hpp"
#include "../net/Connection.hpp"
#include "../net/Net.hpp"
#include "NpcController.hpp"
#include "PhaseTriggerController.hpp"
#include "SnapshotHeap.hpp"
#include <RED4ext/RED4ext.hpp>
#include <iostream>
#include <unordered_map>
#include <mutex>

namespace CoopNet
{

static std::unordered_map<uint32_t, uint64_t> g_lastActive;
static std::mutex g_gcMutex;

void PhaseGC_Touch(uint32_t phaseId)
{
    std::lock_guard lock(g_gcMutex);
    g_lastActive[phaseId] = GameClock::GetCurrentTick();
}

void PhaseGC_Tick(uint64_t nowTick)
{
    float tickMs = GameClock::GetTickMs();
    if (tickMs <= 0.f)
        return;
    const uint64_t timeout = static_cast<uint64_t>(600000.0f / tickMs);
    auto conns = Net_GetConnections();
    std::vector<uint32_t> toClear;
    {
        std::lock_guard lock(g_gcMutex);
        for (auto it = g_lastActive.begin(); it != g_lastActive.end();)
        {
            uint32_t id = it->first;
            if (id == 0)
            {
                ++it;
                continue;
            }
            bool hasPlayer = false;
            for (auto* c : conns)
            {
                if (c->peerId == id)
                {
                    hasPlayer = true;
                    break;
                }
            }
            if (!hasPlayer && nowTick - it->second > timeout)
            {
                toClear.push_back(id);
                it = g_lastActive.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    for (uint32_t id : toClear)
    {
        PhaseTrigger_Clear(id);
        NpcController_Despawn(id);
        SnapshotStore_PurgeOld(0.f);
        std::cout << "[PhaseGC] cleaned phase " << id << std::endl;
    }
}

} // namespace CoopNet

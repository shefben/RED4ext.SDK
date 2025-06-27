#include "PhaseGC.hpp"
#include "../core/GameClock.hpp"
#include "../net/Net.hpp"
#include <RED4ext/RED4ext.hpp>
#include <iostream>
#include <unordered_map>

namespace CoopNet
{

static std::unordered_map<uint32_t, uint64_t> g_lastActive;

void PhaseGC_Touch(uint32_t phaseId)
{
    g_lastActive[phaseId] = GameClock::GetCurrentTick();
}

void PhaseGC_Tick(uint64_t nowTick)
{
    const uint64_t timeout = static_cast<uint64_t>(600000.0f / GameClock::GetTickMs());
    auto conns = Net_GetConnections();
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
            RED4ext::ExecuteFunction("PhaseTrigger", "ClearPhaseTriggers", nullptr, id);
            std::cout << "[PhaseGC] cleaned phase " << id << std::endl;
            it = g_lastActive.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

} // namespace CoopNet

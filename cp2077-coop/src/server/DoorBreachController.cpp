#include "DoorBreachController.hpp"
#include "../core/GameClock.hpp"
#include "../net/Net.hpp"
#include "../core/Red4extUtils.hpp"
#include <RED4ext/RED4ext.hpp>
#include <unordered_map>

namespace CoopNet
{
struct BreachState
{
    uint32_t phase;
    uint32_t seed;
    float prog;
    float send;
    float duration;
};
static std::unordered_map<uint32_t, BreachState> g_map;

void DoorBreachController_Start(uint32_t doorId, uint32_t phaseId)
{
    uint32_t seed = static_cast<uint32_t>(GameClock::GetCurrentTick());
    bool hasPerk = false;
    int perks = 0;
    RED4ext::CName p1 = "FastBreach1";
    RED4EXT_EXECUTE("PerkSystem", "HasPerk", nullptr, &p1, &hasPerk);
    if (hasPerk)
        ++perks;
    RED4ext::CName p2 = "FastBreach2";
    RED4EXT_EXECUTE("PerkSystem", "HasPerk", nullptr, &p2, &hasPerk);
    if (hasPerk)
        ++perks;
    float dur = 1000.f - 100.f * static_cast<float>(perks);
    if (dur < 300.f)
        dur = 300.f;
    g_map[doorId] = {phaseId, seed, 0.f, 0.f, dur};
    DoorBreachStartPacket pkt{doorId, phaseId, seed};
    Net_Broadcast(EMsg::DoorBreachStart, &pkt, sizeof(pkt));
}

void DoorBreachController_Tick(float dt)
{
    for (auto it = g_map.begin(); it != g_map.end();)
    {
        bool combat = false;
        RED4EXT_EXECUTE("PlayerPuppet", "IsInCombat", nullptr, &combat);
        if (combat)
        {
            DoorBreachAbortPacket ab{it->first};
            Net_Broadcast(EMsg::DoorBreachAbort, &ab, sizeof(ab));
            it = g_map.erase(it);
            continue;
        }

        it->second.prog += dt / it->second.duration * 100.f;
        it->second.send += dt;
        if (it->second.send >= 250.f)
        {
            it->second.send = 0.f;
            uint8_t pct = static_cast<uint8_t>(it->second.prog);
            if (pct > 100)
                pct = 100;
            DoorBreachTickPacket pkt{it->first, pct, {0, 0, 0}};
            Net_Broadcast(EMsg::DoorBreachTick, &pkt, sizeof(pkt));
            if (pct >= 100)
            {
                DoorBreachSuccessPacket s{it->first};
                Net_Broadcast(EMsg::DoorBreachSuccess, &s, sizeof(s));
                it = g_map.erase(it);
                continue;
            }
        }
        ++it;
    }
}
} // namespace CoopNet

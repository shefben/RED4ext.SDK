#include "PerkController.hpp"
#include "../core/SessionState.hpp"
#include "../net/Net.hpp"
#include "LedgerService.hpp"
#include <iostream>
#include <unordered_map>

namespace CoopNet
{

struct PerkData
{
    uint8_t rank;
    float healthMult;
};
static std::unordered_map<uint32_t, std::unordered_map<uint32_t, PerkData>> g_perks;

float PerkController_GetHealthMult(uint32_t peerId)
{
    float mult = 1.f;
    auto itPeer = g_perks.find(peerId);
    if (itPeer != g_perks.end())
    {
        for (auto& kv : itPeer->second)
            mult *= kv.second.healthMult;
    }
    return mult;
}

void PerkController_HandleUnlock(Connection* conn, uint32_t perkId, uint8_t rank)
{
    float mult = 1.f + 0.05f * static_cast<float>(rank);
    g_perks[conn->peerId][perkId] = {rank, mult};
    SessionState_SetPerk(conn->peerId, perkId, rank);
    Net_BroadcastPerkUnlock(conn->peerId, perkId, rank);
    std::cout << "PerkUnlock peer=" << conn->peerId << " perk=" << perkId << " rank=" << static_cast<int>(rank)
              << std::endl;
}

void PerkController_HandleRespec(Connection* conn)
{
    uint64_t balance;
    if (!Ledger_Transfer(conn, -100000, 0, balance))
        return;
    g_perks[conn->peerId].clear();
    SessionState_ClearPerks(conn->peerId);
    Net_SendPerkRespecAck(conn, 0);
    std::cout << "PerkRespec peer=" << conn->peerId << std::endl;
}

} // namespace CoopNet

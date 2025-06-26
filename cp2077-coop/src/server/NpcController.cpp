#include "NpcController.hpp"
#include "../core/Hash.hpp"
#include "../core/SpatialGrid.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include <cmath>
#include <cstring>
#include <iostream>

namespace CoopNet
{

static uint32_t g_seed = 123456u;
static NpcSnap g_npc{
    1u,
    0u,
    0ull,
    RED4ext::Vector3{0.f, 0.f, 0.f},
    RED4ext::Quaternion{0.f, 0.f, 0.f, 1.f},
    NpcState::Idle,
    100u,
    0u
};
static bool g_alwaysRelevant = false;
static NpcSnap g_prevSnap = g_npc;
static SpatialGrid g_grid;
static bool g_gridInit = false;

void NpcController_ServerTick(float dt)
{
    if (!g_gridInit)
    {
        g_npc.sectorHash = Fnv1a64Pos(g_npc.pos.X, g_npc.pos.Y);
        g_grid.Insert(g_npc.npcId, g_npc.pos);
        g_gridInit = true;
    }
    // NR-2: deterministic RNG walk AI planned
    g_seed = g_seed * 1664525u + 1013904223u;
    float move = static_cast<float>(g_seed & 0xFF) / 255.0f - 0.5f;
    g_npc.pos.X += move * dt * 0.1f;
    g_grid.Move(g_npc.npcId, g_prevSnap.pos, g_npc.pos);

    std::cout << "[NPC] tick seed=" << g_seed << " pos=" << g_npc.pos.X << std::endl;

    bool changed = std::memcmp(&g_prevSnap, &g_npc, sizeof(NpcSnap)) != 0;

    auto conns = Net_GetConnections();
    std::vector<uint32_t> ids;
    for (auto* c : conns)
    {
        if (!c->sectorReady)
            continue;
        // Build interest set: nearby NPCs plus combatants.
        g_grid.QueryCircle(c->avatarPos, 80.f, ids);
        std::unordered_set<uint32_t> newSet(ids.begin(), ids.end());
        if (g_npc.state == NpcState::Combat)
            newSet.insert(g_npc.npcId);
        for (uint32_t id : newSet)
        {
            if (c->subscribedNpcs.insert(id).second)
            {
                InterestPacket pkt{id};
                Net_Send(c, EMsg::InterestAdd, &pkt, sizeof(pkt));
            }
        }
        for (auto it = c->subscribedNpcs.begin(); it != c->subscribedNpcs.end();)
        {
            if (newSet.count(*it) == 0)
            {
                InterestPacket pkt{*it};
                Net_Send(c, EMsg::InterestRemove, &pkt, sizeof(pkt));
                it = c->subscribedNpcs.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (c->subscribedNpcs.count(g_npc.npcId) && changed)
        {
            NpcSnapshotPacket pkt{g_npc};
            Net_Send(c, EMsg::NpcSnapshot, &pkt, sizeof(pkt));
        }
    }

    if (changed)
        g_prevSnap = g_npc;
    // Spatial grid reduces search complexity for interest checks
}

void NpcController_ClientApplySnap(const NpcSnap& snap)
{
    std::cout << "[NPC] apply snap id=" << snap.npcId << std::endl;
}

} // namespace CoopNet

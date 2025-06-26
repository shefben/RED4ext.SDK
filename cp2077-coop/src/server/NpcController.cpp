#include "NpcController.hpp"
#include "../core/Hash.hpp"
#include "../core/SpatialGrid.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include <RED4ext/RED4ext.hpp>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <iostream>

namespace CoopNet
{

static uint32_t g_seed = 123456u;
static std::unordered_map<uint64_t, uint32_t> g_sectorSeeds;
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
static float g_walkDir = 0.f;
static float g_dirTimer = 0.f;

static uint32_t GetSectorSeed(uint64_t hash)
{
    auto it = g_sectorSeeds.find(hash);
    if (it != g_sectorSeeds.end())
        return it->second;
    uint32_t seed = static_cast<uint32_t>(hash ^ 0xA5A5A5A5u);
    g_sectorSeeds[hash] = seed;
    return seed;
}

void NpcController_OnPlayerEnterSector(uint32_t peerId, uint64_t hash)
{
    uint32_t seed = GetSectorSeed(hash);
    CrowdSeedPacket pkt{hash, seed};
    Net_Broadcast(EMsg::CrowdSeed, &pkt, sizeof(pkt));
}

uint32_t NpcController_GetSectorSeed(uint64_t hash)
{
    return GetSectorSeed(hash);
}

void NpcController_ServerTick(float dt)
{
    if (!g_gridInit)
    {
        g_npc.sectorHash = Fnv1a64Pos(g_npc.pos.X, g_npc.pos.Y);
        g_grid.Insert(g_npc.npcId, g_npc.pos);
        g_gridInit = true;
    }
    // NR-2: deterministic AI walk routine
    g_dirTimer += dt;
    if (g_dirTimer >= 3.f)
    {
        g_seed = g_seed * 1664525u + 1013904223u;
        g_walkDir = static_cast<float>(g_seed & 0xFFFF) / 65535.f * 6.283185f;
        g_dirTimer = 0.f;
    }
    float speed = 0.5f; // m/s
    g_npc.pos.X += std::cos(g_walkDir) * speed * dt;
    g_npc.pos.Y += std::sin(g_walkDir) * speed * dt;
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
    RED4ext::ExecuteFunction("NpcController", "ClientApplySnap", nullptr, &snap);
}

void NpcController_ApplyCrowdSeed(uint64_t hash, uint32_t seed)
{
    RED4ext::ExecuteFunction("NpcController", "ApplyCrowdSeed", nullptr, hash, seed);
}

void NpcController_Despawn(uint32_t id)
{
    RED4ext::ExecuteFunction("NpcController", "DespawnNpc", nullptr, id);
}

} // namespace CoopNet

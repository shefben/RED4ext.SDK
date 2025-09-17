#include "NpcController.hpp"
#include "../core/Hash.hpp"
#include "../net/Connection.hpp"
#include "../net/InterestGrid.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include "../core/Red4extUtils.hpp"
#include <RED4ext/RED4ext.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <mutex>

namespace CoopNet
{

static uint32_t g_seed = 123456u;
static std::unordered_map<uint64_t, uint32_t> g_sectorSeeds;
static NpcSnap g_npc{1u,
                     0u,
                     0ull,
                     RED4ext::Vector3{0.f, 0.f, 0.f},
                     RED4ext::Quaternion{0.f, 0.f, 0.f, 1.f},
                     NpcState::Idle,
                     100u,
                     static_cast<uint8_t>(PoliceAIState::Idle),
                     0u,
                     {0, 0}};
static bool g_alwaysRelevant = false;
static NpcSnap g_prevSnap = g_npc;
static bool g_gridInit = false;
static float g_walkDir = 0.f;
static float g_dirTimer = 0.f;
static float g_healthMult = 1.f;
static float g_damageMult = 1.f;
static float g_waveTimer = 0.f;
static uint8_t g_waveCount = 0;
static uint32_t g_nextId = 2u;
static std::mutex g_npcMutex;

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
    std::lock_guard lock(g_npcMutex);
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
    auto conns = Net_GetConnections();
    size_t playerCount = conns.size();
    {
        std::lock_guard lock(g_npcMutex);
        g_healthMult = (std::min)(2.0f, 1.0f + 0.25f * (static_cast<float>(playerCount) - 1.f));
        g_damageMult = (std::min)(1.6f, 1.0f + 0.15f * (static_cast<float>(playerCount) - 1.f));
    }

    if (!g_gridInit)
    {
        std::lock_guard lock(g_npcMutex);
        g_npc.sectorHash = Fnv1a64Pos(g_npc.pos.X, g_npc.pos.Y);
        g_interestGrid.Insert(g_npc.npcId, g_npc.pos);
        g_gridInit = true;
        g_npc.health = static_cast<uint16_t>(100u * g_healthMult);
    }
    // NR-2: deterministic AI walk routine
    {
        std::lock_guard lock(g_npcMutex);
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
    }
    g_interestGrid.Move(g_npc.npcId, g_npc.pos);

    std::cout << "[NPC] tick seed=" << g_seed << " pos=" << g_npc.pos.X << std::endl;

    bool changed = false;
    {
        std::lock_guard lock(g_npcMutex);
        changed = std::memcmp(&g_prevSnap, &g_npc, sizeof(NpcSnap)) != 0;
    }

    for (auto* c : conns)
    {
        if (!c->sectorReady)
            continue;
        c->RefreshNpcInterest();
        if (c->subscribedNpcs.count(g_npc.npcId) && changed)
        {
            NpcSnapshotPacket pkt{g_npc};
            Net_Send(c, EMsg::NpcSnapshot, &pkt, sizeof(pkt));
            c->snapBytes += sizeof(pkt);
        }
    }

    if (changed)
    {
        std::lock_guard lock(g_npcMutex);
        g_prevSnap = g_npc;
    }

    if (playerCount > 2 && g_npc.state == NpcState::Combat)
    {
        g_waveTimer += dt;
        if (g_waveTimer >= 30000.f && g_waveCount < 3)
        {
            g_waveTimer = 0.f;
            g_waveCount++;
            for (int i = 0; i < 2; ++i)
            {
                NpcSnap s = g_npc;
                {
                    std::lock_guard lock(g_npcMutex);
                    s.npcId = g_nextId++;
                    s.health = static_cast<uint16_t>(100u * g_healthMult);
                }
                NpcSpawnPacket pkt{s};
                Net_Broadcast(EMsg::NpcSpawn, &pkt, sizeof(pkt));
                g_interestGrid.Insert(s.npcId, s.pos);
            }
        }
    }
    else
    {
        g_waveTimer = 0.f;
    }
    // Spatial grid reduces search complexity for interest checks
}

void NpcController_ClientApplySnap(const NpcSnap& snap)
{
    RED4EXT_EXECUTE("NpcController", "ClientApplySnap", nullptr, &snap);
}

void NpcController_ApplyCrowdSeed(uint64_t hash, uint32_t seed)
{
    RED4EXT_EXECUTE("NpcController", "ApplyCrowdSeed", nullptr, hash, seed);
}

void NpcController_Despawn(uint32_t id)
{
    RED4EXT_EXECUTE("NpcController", "DespawnNpc", nullptr, id);
    g_interestGrid.Remove(id);
}

const NpcSnap& NpcController_GetSnap()
{
    static thread_local NpcSnap s_copy;
    std::lock_guard lock(g_npcMutex);
    s_copy = g_npc;
    return s_copy;
}

} // namespace CoopNet

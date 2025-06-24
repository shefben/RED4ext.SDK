#include "NpcController.hpp"
#include <iostream>

namespace CoopNet {
static uint32_t g_seed = 123456u;
static NpcSnap g_npc {
    1u, // npcId
    0u, // templateId
    RED4ext::Vector3{0.f, 0.f, 0.f},
    RED4ext::Quaternion{0.f, 0.f, 0.f, 1.f},
    0u, // state idle
    100u, // health
    0u // appearance
};
static NpcSnap g_prevSnap = g_npc;

void NpcController_ServerTick(float dt)
{
    // Deterministic RNG walk placeholder
    g_seed = g_seed * 1664525u + 1013904223u;
    float move = static_cast<float>(g_seed & 0xFF) / 255.0f - 0.5f;
    g_npc.pos.X += move * dt * 0.1f;

    std::cout << "[NPC] tick seed=" << g_seed << " pos=" << g_npc.pos.X << std::endl;

    // Broadcast snapshot if changed (placeholder broadcast)
    if (std::memcmp(&g_prevSnap, &g_npc, sizeof(NpcSnap)) != 0)
    {
        std::cout << "[NPC] broadcast snapshot id=" << g_npc.npcId << std::endl;
        g_prevSnap = g_npc;
    }
}

void NpcController_ClientApplySnap(const NpcSnap& snap)
{
    std::cout << "[NPC] apply snap id=" << snap.npcId << std::endl;
}

} // namespace CoopNet

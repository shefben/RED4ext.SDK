#include "BreachController.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include <cstdlib>
#include <iostream>

namespace CoopNet {

static bool g_active = false;
static uint32_t g_seed = 0;
static uint8_t g_w = 0;
static uint8_t g_h = 0;
static float g_timer = 0.f;
static uint32_t g_peer = 0;

void BreachController_Start(uint32_t peerId, uint8_t w, uint8_t h)
{
    g_active = true;
    g_seed = static_cast<uint32_t>(std::rand());
    g_w = w;
    g_h = h;
    g_timer = 45.f;
    g_peer = peerId;
    BreachStartPacket pkt{peerId, g_seed, w, h, {0,0}};
    Net_Broadcast(EMsg::BreachStart, &pkt, sizeof(pkt));
    std::cout << "Breach start seed=" << g_seed << std::endl;
}

void BreachController_HandleInput(uint32_t peerId, uint8_t idx)
{
    if (!g_active)
        return;
    BreachInputPacket pkt{peerId, idx, {0,0,0}};
    Net_Broadcast(EMsg::BreachInput, &pkt, sizeof(pkt));
}

void BreachController_ServerTick(float dt)
{
    if (!g_active)
        return;
    g_timer -= dt / 1000.f;
    if (g_timer <= 0.f)
    {
        g_active = false;
        BreachResultPacket pkt{g_peer, 7u, {0,0,0}}; // all daemons active
        Net_Broadcast(EMsg::BreachResult, &pkt, sizeof(pkt));
        std::cout << "Breach result sent" << std::endl;
    }
}

} // namespace CoopNet

#include "ArcadeController.hpp"
#include "../core/GameClock.hpp"
#include "../core/SaveFork.hpp"
#include "../net/Net.hpp"
#include <unordered_map>
#include <mutex>

namespace CoopNet {
struct ArcadeState {
    uint32_t peerId = 0;
    uint32_t score = 0;
    uint32_t seed = 0;
    bool active = false;
};
static std::unordered_map<uint32_t, ArcadeState> g_games;
static std::mutex g_arcadeMutex;

void Arcade_Start(uint32_t cabId, uint32_t peerId, uint32_t seed)
{
    std::lock_guard lock(g_arcadeMutex);
    g_games[cabId] = {peerId, 0u, seed, true};
    ArcadeStartPacket pkt{cabId, peerId, seed};
    Net_Broadcast(EMsg::ArcadeStart, &pkt, sizeof(pkt));
    Arcade_BroadcastHighScore(cabId);
}

void Arcade_Input(uint32_t frame, uint8_t buttons)
{
    std::lock_guard lock(g_arcadeMutex);
    for (auto& kv : g_games) {
        if (kv.second.active)
        {
            uint8_t b = buttons;
            while (b) {
                kv.second.score += b & 1;
                b >>= 1;
            }
        }
    }
}

void Arcade_End(uint32_t peerId, uint32_t score)
{
    std::lock_guard lock(g_arcadeMutex);
    for (auto& kv : g_games) {
        if (kv.second.peerId == peerId && kv.second.active) {
            kv.second.active = false;
            kv.second.score = score;
            uint32_t hiPeer = 0, hiScore = 0;
            if (!LoadArcadeHighScore(kv.first, hiPeer, hiScore) || score > hiScore)
            {
                SaveArcadeHighScore(kv.first, peerId, score);
                Arcade_BroadcastHighScore(kv.first);
            }
            break;
        }
    }
    ArcadeScorePacket pkt{peerId, score};
    Net_Broadcast(EMsg::ArcadeScore, &pkt, sizeof(pkt));
}

void Arcade_Tick(float dt)
{
    static float accum = 0.f;
    accum += dt;
    if (accum < 1.0f)
        return;
    accum = 0.f;
    std::lock_guard lock(g_arcadeMutex);
    for (const auto& kv : g_games)
    {
        if (!kv.second.active)
            continue;
        ArcadeScorePacket pkt{kv.second.peerId, kv.second.score};
        Net_Broadcast(EMsg::ArcadeScore, &pkt, sizeof(pkt));
    }
}

void Arcade_BroadcastHighScore(uint32_t cabId)
{
    uint32_t peer = 0, score = 0;
    if (LoadArcadeHighScore(cabId, peer, score))
        Net_BroadcastArcadeHighScore(cabId, peer, score);
}

} // namespace CoopNet

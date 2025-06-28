#include "ArcadeController.hpp"
#include "../core/GameClock.hpp"
#include "../core/SaveFork.hpp"
#include "../net/Net.hpp"
#include <unordered_map>

namespace CoopNet {
struct ArcadeState {
    uint32_t peerId = 0;
    uint32_t score = 0;
    uint32_t seed = 0;
    bool active = false;
};
static std::unordered_map<uint32_t, ArcadeState> g_games;

void Arcade_Start(uint32_t cabId, uint32_t peerId, uint32_t seed)
{
    g_games[cabId] = {peerId, 0u, seed, true};
    ArcadeStartPacket pkt{cabId, peerId, seed};
    Net_Broadcast(EMsg::ArcadeStart, &pkt, sizeof(pkt));
}

void Arcade_Input(uint32_t frame, uint8_t buttons)
{
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
    for (auto& kv : g_games) {
        if (kv.second.peerId == peerId && kv.second.active) {
            kv.second.active = false;
            kv.second.score = score;
            uint32_t hiPeer = 0, hiScore = 0;
            if (!LoadArcadeHighScore(kv.first, hiPeer, hiScore) || score > hiScore)
                SaveArcadeHighScore(kv.first, peerId, score);
            break;
        }
    }
    ArcadeScorePacket pkt{peerId, score};
    Net_Broadcast(EMsg::ArcadeScore, &pkt, sizeof(pkt));
}

void Arcade_Tick(float /*dt*/) {}

} // namespace CoopNet

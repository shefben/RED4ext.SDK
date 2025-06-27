#include "BillboardController.hpp"
#include "../net/Net.hpp"
#include <unordered_map>

namespace CoopNet
{
struct BillboardState
{
    uint64_t seed;
    uint32_t ad;
    float timer;
};
static std::unordered_map<uint64_t, BillboardState> g_map;

void BillboardController_OnSectorLoad(uint32_t peerId, uint64_t hash)
{
    auto& st = g_map[hash];
    if (st.seed == 0)
        st.seed = hash ^ 0x5A5A5A5Au;
    HoloSeedPacket pkt{hash, st.seed};
    Net_Broadcast(EMsg::HoloSeed, &pkt, sizeof(pkt));
}

void BillboardController_Tick(float dt)
{
    for (auto& kv : g_map)
    {
        kv.second.timer += dt / 1000.f;
        if (kv.second.timer >= 120.f)
        {
            kv.second.timer = 0.f;
            kv.second.ad += 1;
            HoloNextAdPacket pkt{kv.first, kv.second.ad};
            Net_Broadcast(EMsg::HoloNextAd, &pkt, sizeof(pkt));
        }
    }
}
} // namespace CoopNet

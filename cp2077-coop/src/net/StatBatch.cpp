#include "StatBatch.hpp"
#include "Net.hpp"
#include "../../third_party/httplib.h"
#include <iostream>
#include <sstream>

// Batches player stats and posts to master server every 30 s.
namespace CoopNet
{
static BatchedStats g_stats;
static float g_timer = 0.f;

static void FlushStats()
{
    if (g_stats.peerId.empty())
        return;
    std::stringstream ss;
    ss << "{\"rows\":[";
    for (size_t i = 0; i < g_stats.peerId.size(); ++i)
    {
        Net_BroadcastScoreUpdate(g_stats.peerId[i], g_stats.k[i], g_stats.d[i]);
        if (i > 0) ss << ',';
        ss << "{\"id\":" << g_stats.peerId[i]
           << ",\"k\":" << g_stats.k[i]
           << ",\"d\":" << g_stats.d[i]
           << ",\"a\":" << g_stats.a[i]
           << ",\"dmg\":" << g_stats.dmg[i]
           << ",\"hs\":" << g_stats.hs[i] << '}';
    }
    ss << "]}";
    httplib::SSLClient cli("coop-master", 443);
    cli.Post("/api/stats", ss.str(), "application/json");
    g_stats = BatchedStats();
}

void StatBatch_Tick(float dt)
{
    g_timer += dt;
    if (g_timer >= 30.f)
    {
        g_timer = 0.f;
        FlushStats();
    }
}

void AddStats(uint32_t peerId, uint16_t k, uint16_t d, uint16_t a, uint32_t dmg, uint16_t hs)
{
    g_stats.peerId.push_back(peerId);
    g_stats.k.push_back(k);
    g_stats.d.push_back(d);
    g_stats.a.push_back(a);
    g_stats.dmg.push_back(dmg);
    g_stats.hs.push_back(hs);
}

} // namespace CoopNet

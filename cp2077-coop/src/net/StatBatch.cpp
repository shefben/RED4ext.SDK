#include "StatBatch.hpp"
#include <iostream>

// Batches score updates to reduce bandwidth. Sent once per tick.
namespace CoopNet
{
static BatchedStats g_stats;

void FlushStats()
{
    if (g_stats.peerId.empty())
        return;
    // P7-2: serialize and send ScoreUpdate packet in scoreboard update
    g_stats.peerId.clear();
    g_stats.k.clear();
    g_stats.d.clear();
    std::cout << "FlushStats" << std::endl;
}

void AddScore(uint32_t peerId, uint16_t k, uint16_t d)
{
    g_stats.peerId.push_back(peerId);
    g_stats.k.push_back(k);
    g_stats.d.push_back(d);
}

} // namespace CoopNet

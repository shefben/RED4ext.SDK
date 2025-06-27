#include "QuestWatchdog.hpp"
#include "../net/Net.hpp"
#include <unordered_map>

namespace CoopNet
{

static std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint16_t>> g_phaseStages; // PX-2
static std::unordered_map<uint32_t, float> g_diverge;
static float g_timer = 0.f;
static uint32_t g_resyncCount = 0;
static float g_window = 0.f;

void QuestWatchdog_Record(uint32_t phaseId, uint32_t questHash, uint16_t stage)
{
    g_phaseStages[phaseId][questHash] = stage;
}

void QuestWatchdog_BuildFullSync(uint32_t phaseId, QuestFullSyncPacket& outPkt)
{
    outPkt.count = 0;
    auto it = g_phaseStages.find(phaseId);
    if (it == g_phaseStages.end())
        return;
    for (auto& q : it->second)
    {
        if (outPkt.count >= 32)
            break;
        outPkt.entries[outPkt.count].nameHash = q.first;
        outPkt.entries[outPkt.count].stage = q.second;
        ++outPkt.count;
    }
}

void QuestWatchdog_Tick(float dt)
{
    g_timer += dt;
    g_window += dt;
    if (g_window >= 300.f)
    {
        g_window = 0.f;
        g_resyncCount = 0;
    }
    if (g_timer < 3.f)
        return;
    g_timer = 0.f;

    std::unordered_map<uint32_t, uint16_t> minStage;
    std::unordered_map<uint32_t, uint16_t> maxStage;
    for (auto& peer : g_phaseStages)
    {
        for (auto& q : peer.second)
        {
            uint16_t st = q.second;
            auto& mn = minStage[q.first];
            if (mn == 0 || st < mn)
                mn = st;
            auto& mx = maxStage[q.first];
            if (st > mx)
                mx = st;
        }
    }

    for (auto& kv : maxStage)
    {
        uint32_t hash = kv.first;
        uint16_t mx = kv.second;
        uint16_t mn = minStage[hash];
        if (mx > mn + 1)
        {
            g_diverge[hash] += 3.f;
            if (g_diverge[hash] > 15.f && g_resyncCount < 2)
            {
                g_resyncCount++;
                for (auto& peer : g_phaseStages)
                {
                    uint16_t st = peer.second[hash];
                    if (st != mx)
                    {
                        Connection* conn = Net_FindConnection(peer.first);
                        if (conn)
                        {
                            QuestFullSyncPacket pkt{};
                            QuestWatchdog_BuildFullSync(peer.first, pkt);
                            Net_SendQuestFullSync(conn, pkt);
                        }
                    }
                }
                g_diverge.erase(hash);
            }
        }
        else
        {
            g_diverge.erase(hash);
        }
    }
}

} // namespace CoopNet

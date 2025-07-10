#include "QuestWatchdog.hpp"
#include "../core/Hash.hpp"
#include "../net/Net.hpp"
#include "PhaseGC.hpp"
#include "PhaseTriggerController.hpp"
#include "NpcController.hpp"
#include "../core/SessionState.hpp"
#include <fstream>
#include <unordered_map>
#include <unordered_set>

namespace CoopNet
{

static std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint16_t>> g_phaseStages; // PX-2
static std::unordered_map<uint32_t, float> g_diverge;
static float g_timer = 0.f;
static uint32_t g_resyncCount = 0;
static float g_window = 0.f;
static std::unordered_map<uint32_t, uint16_t> g_critical;                              // PX-6
static std::unordered_map<uint32_t, std::unordered_map<uint16_t, uint32_t>> g_romance; // RM-1
static std::unordered_set<uint32_t> g_mainQuests; // QW-1
static std::unordered_set<uint32_t> g_sideQuests; // QW-1
static bool g_voteActive = false;
static uint32_t g_voteQuest = 0;
static uint32_t g_votePhase = 0;
static float g_voteTimer = 0.f;
static uint16_t g_voteStage = 0;
static bool g_branchVote = false;
static std::unordered_map<uint32_t, bool> g_voteCast;
static bool g_endVoteActive = false; // EG-1
static float g_endVoteTimer = 0.f;
static std::unordered_map<uint32_t, bool> g_endVoteCast;

void QuestWatchdog_Record(uint32_t phaseId, uint32_t questHash, uint16_t stage)
{
    auto& map = g_phaseStages[phaseId];
    if (map.empty())
    {
        for (uint32_t q : g_mainQuests)
            map[q] = 0;
        for (uint32_t q : g_sideQuests)
            map[q] = 0;
    }
    map[questHash] = stage;
    PhaseGC_Touch(phaseId);
    auto r = g_romance.find(questHash);
    if (r != g_romance.end())
    {
        auto stIt = r->second.find(stage);
        if (stIt != r->second.end())
            Net_BroadcastCineStart(stIt->second, 0u, phaseId, true);
    }
    auto crit = g_critical.find(questHash);
    if (crit != g_critical.end() && stage >= crit->second && !g_voteActive)
    {
        g_voteActive = true;
        g_voteQuest = questHash;
        g_votePhase = phaseId;
        g_voteTimer = 30.f;
        g_voteCast.clear();
        Net_BroadcastCriticalVoteStart(questHash);
    }
    if (questHash == 0xaa573886 && stage >= 1000 && !g_endVoteActive)
    {
        g_endVoteActive = true;
        g_endVoteTimer = 30.f;
        g_endVoteCast.clear();
        Net_BroadcastEndingVoteStart(questHash);
    }
}

void QuestWatchdog_BuildFullSync(uint32_t phaseId, QuestFullSyncPacket& outPkt)
{
    outPkt.questCount = 0;
    outPkt.npcCount = 0;
    outPkt.eventCount = 0;

    auto it = g_phaseStages.find(phaseId);
    if (it != g_phaseStages.end())
    {
        for (auto& q : it->second)
        {
            if (outPkt.questCount >= 32)
                break;
            outPkt.quests[outPkt.questCount].nameHash = q.first;
            outPkt.quests[outPkt.questCount].stage = q.second;
            ++outPkt.questCount;
        }
    }

    const NpcSnap& npc = NpcController_GetSnap();
    if (npc.phaseId == phaseId && outPkt.npcCount < 16)
    {
        outPkt.npcs[outPkt.npcCount] = npc;
        ++outPkt.npcCount;
    }

    for (const auto& e : SessionState_GetEvents())
    {
        if (e.phase != phaseId || !e.active || outPkt.eventCount >= 16)
            continue;
        outPkt.events[outPkt.eventCount].eventId = e.eventId;
        outPkt.events[outPkt.eventCount].phase = e.phase;
        outPkt.events[outPkt.eventCount].active = 1u;
        outPkt.events[outPkt.eventCount].seed = e.seed;
        ++outPkt.eventCount;
    }
}

uint16_t QuestWatchdog_GetStage(uint32_t phaseId, uint32_t questHash)
{
    auto it = g_phaseStages.find(phaseId);
    if (it == g_phaseStages.end())
        return 0u;
    auto qIt = it->second.find(questHash);
    if (qIt == it->second.end())
        return 0u;
    return qIt->second;
}

std::vector<uint32_t> QuestWatchdog_ListPhases()
{
    std::vector<uint32_t> ids;
    for (auto& kv : g_phaseStages)
        ids.push_back(kv.first);
    return ids;
}

void QuestWatchdog_HandleVote(uint32_t peerId, bool yes)
{
    if (!g_voteActive)
        return;
    g_voteCast[peerId] = yes;
}

void QuestWatchdog_HandleEndingVote(uint32_t peerId, bool yes)
{
    if (!g_endVoteActive)
        return;
    g_endVoteCast[peerId] = yes;
}

void QuestWatchdog_LoadCritical()
{
    std::ifstream in("CriticalQuests.json");
    if (!in.is_open())
        return;
    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    size_t pos = 0;
    while (true)
    {
        pos = json.find("\"questHash\"", pos);
        if (pos == std::string::npos)
            break;
        pos = json.find(':', pos);
        size_t s = json.find_first_of("0123456789", pos);
        size_t e = json.find_first_not_of("0123456789", s);
        uint32_t q = std::stoul(json.substr(s, e - s));
        pos = json.find("\"stage\"", pos);
        pos = json.find(':', pos);
        s = json.find_first_of("0123456789", pos);
        e = json.find_first_not_of("0123456789", s);
        uint16_t st = static_cast<uint16_t>(std::stoi(json.substr(s, e - s)));
        g_critical[q] = st;
    }
}

void QuestWatchdog_LoadRomance()
{
    std::ifstream in("RomanceScenes.json");
    if (!in.is_open())
        return;
    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    size_t pos = 0;
    while (true)
    {
        pos = json.find("\"questHash\"", pos);
        if (pos == std::string::npos)
            break;
        pos = json.find(':', pos);
        size_t s = json.find_first_of("0123456789", pos);
        size_t e = json.find_first_not_of("0123456789", s);
        uint32_t q = std::stoul(json.substr(s, e - s));
        pos = json.find("\"stage\"", pos);
        pos = json.find(':', pos);
        s = json.find_first_of("0123456789", pos);
        e = json.find_first_not_of("0123456789", s);
        uint16_t st = static_cast<uint16_t>(std::stoi(json.substr(s, e - s)));
        pos = json.find("\"sceneId\"", pos);
        pos = json.find(':', pos);
        s = json.find_first_of("0123456789", pos);
        e = json.find_first_not_of("0123456789", s);
        uint32_t sc = std::stoul(json.substr(s, e - s));
        g_romance[q][st] = sc;
    }
}

void QuestWatchdog_LoadMain()
{
    std::ifstream in("MainQuests.json");
    if (!in.is_open())
        return;
    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    size_t pos = 0;
    while (true)
    {
        pos = json.find("\"questHash\"", pos);
        if (pos == std::string::npos)
            break;
        pos = json.find(':', pos);
        size_t s = json.find_first_of("0123456789", pos);
        size_t e = json.find_first_not_of("0123456789", s);
        uint32_t q = std::stoul(json.substr(s, e - s));
        g_mainQuests.insert(q);
    }
}

void QuestWatchdog_LoadSide()
{
    std::ifstream in("SideQuests.json");
    if (!in.is_open())
        return;
    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    size_t pos = 0;
    while (true)
    {
        pos = json.find("\"questHash\"", pos);
        if (pos == std::string::npos)
            break;
        pos = json.find(':', pos);
        size_t s = json.find_first_of("0123456789", pos);
        size_t e = json.find_first_not_of("0123456789", s);
        uint32_t q = std::stoul(json.substr(s, e - s));
        g_sideQuests.insert(q);
    }
}

void QuestWatchdog_Tick(float dt)
{
    if (g_voteActive)
    {
        g_voteTimer -= dt / 1000.f;
        size_t total = Net_GetConnections().size();
        size_t yes = 0;
        for (auto& kv : g_voteCast)
            if (kv.second)
                ++yes;
        size_t uncast = total > g_voteCast.size() ? total - g_voteCast.size() : 0;
        bool majority = yes > total / 2 || (g_voteTimer <= 0.f && yes + uncast > total / 2);
        if (majority)
        {
            uint16_t stage = g_branchVote ? g_voteStage : g_phaseStages[g_votePhase][g_voteQuest];
            for (auto& peer : g_phaseStages)
            {
                peer.second[g_voteQuest] = stage;
                Connection* c = Net_FindConnection(peer.first);
                if (c)
                {
                    QuestStageP2PPacket pkt{peer.first, g_voteQuest, stage, 0};
                    Net_Send(c, EMsg::QuestStageP2P, &pkt, sizeof(pkt));
                }
            }
            PhaseTrigger_Clear(g_votePhase);
            g_voteActive = false;
            g_branchVote = false;
            g_voteCast.clear();
        }

        if (g_endVoteActive)
        {
            g_endVoteTimer -= dt / 1000.f;
            size_t total = Net_GetConnections().size();
            size_t yes = 0;
            for (auto& kv : g_endVoteCast)
                if (kv.second)
                    ++yes;
            size_t uncast = total > g_endVoteCast.size() ? total - g_endVoteCast.size() : 0;
            bool majority = yes > total / 2 || (g_endVoteTimer <= 0.f && yes + uncast > total / 2);
            if (majority)
            {
                Net_BroadcastCineStart(Fnv1a32("ending_roof"), 0u, 0u, false);
                g_phaseStages.clear();
                g_endVoteActive = false;
                g_endVoteCast.clear();
            }
            else if (g_endVoteTimer <= 0.f)
            {
                g_endVoteActive = false;
                g_endVoteCast.clear();
            }
        }
        else if (g_voteTimer <= 0.f)
        {
            g_voteActive = false;
            g_branchVote = false;
            g_voteCast.clear();
        }
    }

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
            if (g_diverge[hash] > 15.f && !g_voteActive)
            {
                std::unordered_map<uint16_t, size_t> count;
                for (auto& peer : g_phaseStages)
                    ++count[peer.second[hash]];
                uint16_t best = 0;
                size_t bestN = 0;
                for (auto& c : count)
                    if (c.second > bestN)
                    {
                        bestN = c.second;
                        best = c.first;
                    }
                g_voteActive = true;
                g_branchVote = true;
                g_voteQuest = hash;
                g_voteStage = best;
                g_voteTimer = 30.f;
                g_voteCast.clear();
                Net_BroadcastBranchVoteStart(hash, best);
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

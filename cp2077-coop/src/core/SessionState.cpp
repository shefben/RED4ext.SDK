#include "SessionState.hpp"
#include "SaveFork.hpp"
#include "SaveMigration.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace CoopNet
{

struct PartyMember
{
    uint32_t peerId;
    uint32_t xp;
    std::unordered_map<uint32_t, uint8_t> perks;
};

static std::vector<PartyMember> g_party;                            // PP-1: populated via lobby sync
static std::vector<std::pair<std::string, uint32_t>> g_questStages; // questName -> stage
static std::vector<ItemSnap> g_inventory;
static WorldStateSnap g_world{};
static std::vector<EventState> g_events;
static std::unordered_map<uint32_t, int16_t> g_reputation;
static uint32_t g_sessionId = 0;

uint32_t SessionState_SetParty(const std::vector<uint32_t>& peerIds)
{
    g_party.clear();
    std::vector<uint32_t> sorted = peerIds;
    std::sort(sorted.begin(), sorted.end());
    uint32_t hash = 2166136261u;
    for (uint32_t id : sorted)
    {
        const uint8_t* b = reinterpret_cast<const uint8_t*>(&id);
        for (size_t i = 0; i < sizeof(id); ++i)
        {
            hash ^= b[i];
            hash *= 16777619u;
        }
        g_party.push_back({id, 0, {}});
    }
    uint32_t prev = g_sessionId;
    g_sessionId = hash;
    if (g_sessionId != prev)
        LoadSessionState(g_sessionId);
    return g_sessionId;
}

void SaveSessionState(uint32_t sessionId)
{
    std::stringstream ss;
    ss << "{\n  \"party\": [";
    for (size_t i = 0; i < g_party.size(); ++i)
    {
        const auto& p = g_party[i];
        ss << "{\"peerId\":" << p.peerId << ",\"xp\":" << p.xp << ",\"perks\":{";
        size_t pc = 0;
        for (auto& kv : p.perks)
        {
            ss << "\"" << kv.first << "\":" << static_cast<int>(kv.second);
            if (++pc < p.perks.size())
                ss << ",";
        }
        ss << "}}";
        if (i + 1 < g_party.size())
            ss << ",";
    }
    ss << "],\n  \"quests\": {";
    for (size_t i = 0; i < g_questStages.size(); ++i)
    {
        const auto& q = g_questStages[i];
        ss << "\"" << q.first << "\":" << q.second;
        if (i + 1 < g_questStages.size())
            ss << ",";
    }
    ss << "},\n  \"inventory\": [";
    for (size_t i = 0; i < g_inventory.size(); ++i)
    {
        const auto& it = g_inventory[i];
        ss << "{\"itemId\":" << it.itemId << ",\"qty\":" << it.quantity << "}";
        if (i + 1 < g_inventory.size())
            ss << ",";
    }
    ss << "],\n  \"weather\":{\"sun\":" << g_world.sunDeg << ",\"id\":"
       << static_cast<int>(g_world.weatherId) << ",\"seed\":" << g_world.particleSeed
       << "},\n  \"events\":[";
    for (size_t i = 0; i < g_events.size(); ++i)
    {
        const auto& e = g_events[i];
        ss << "{\"id\":" << e.eventId << ",\"phase\":" << static_cast<int>(e.phase)
           << ",\"active\":" << (e.active ? "true" : "false") << ",\"seed\":" << e.seed
           << "}";
        if (i + 1 < g_events.size())
            ss << ",";
    }
    ss << "],\n  \"reputation\":{";
    size_t rc = 0;
    for (auto& kv : g_reputation)
    {
        ss << "\"" << kv.first << "\":" << kv.second;
        if (++rc < g_reputation.size())
            ss << ",";
    }
    ss << "}\n}\n";

    std::string blob = ss.str();
    SaveRollbackSnapshot(sessionId, blob);
    SaveSession(sessionId, blob);
}

void SaveMergeResolution(bool acceptAll)
{
    try
    {
        EnsureCoopSaveDirs();
        const std::filesystem::path file = std::filesystem::path(kCoopSavePath) / "merged.dat";
        std::ofstream out(file, std::ios::app);
        if (!out.is_open())
        {
            std::cerr << "Failed to open merged file" << std::endl;
            return;
        }
        out << "resolution=" << (acceptAll ? "acceptAll" : "skipEach") << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "SaveMergeResolution error: " << e.what() << std::endl;
    }
}

uint32_t SessionState_GetId()
{
    return g_sessionId;
}

void SessionState_SetPerk(uint32_t peerId, uint32_t perkId, uint8_t rank)
{
    for (auto& p : g_party)
    {
        if (p.peerId == peerId)
        {
            p.perks[perkId] = rank;
            return;
        }
    }
}

void SessionState_ClearPerks(uint32_t peerId)
{
    for (auto& p : g_party)
    {
        if (p.peerId == peerId)
        {
            p.perks.clear();
            return;
        }
    }
}

float SessionState_GetPerkHealthMult(uint32_t peerId)
{
    for (auto& p : g_party)
    {
        if (p.peerId == peerId)
        {
            float m = 1.f;
            for (auto& kv : p.perks)
                m *= 1.f + 0.05f * static_cast<float>(kv.second);
            return m;
        }
    }
    return 1.f;
}

const WorldStateSnap& SessionState_GetWorld()
{
    return g_world;
}

const std::vector<EventState>& SessionState_GetEvents()
{
    return g_events;
}

const std::unordered_map<uint32_t, int16_t>& SessionState_GetReputation()
{
    return g_reputation;
}

void SessionState_UpdateWeather(uint16_t sunDeg, uint8_t weatherId, uint16_t seed)
{
    g_world.sunDeg = sunDeg;
    g_world.weatherId = weatherId;
    g_world.particleSeed = seed;
}

void SessionState_RecordEvent(uint32_t eventId, uint8_t phase, bool active, uint32_t seed)
{
    for (auto& e : g_events)
    {
        if (e.eventId == eventId && e.phase == phase)
        {
            e.active = active;
            e.seed = seed;
            return;
        }
    }
    g_events.push_back({eventId, phase, active, seed});
}

void SessionState_SetReputation(uint32_t npcId, int16_t value)
{
    g_reputation[npcId] = value;
}

bool LoadSessionState(uint32_t sessionId)
{
    std::string json;
    if (!LoadSession(sessionId, json))
        return false;
    size_t pos = json.find("\"weather\"");
    if (pos != std::string::npos)
    {
        std::sscanf(json.c_str() + pos,
                    "\"weather\":{\"sun\":%hu,\"id\":%hhu,\"seed\":%hu",
                    &g_world.sunDeg, &g_world.weatherId, &g_world.particleSeed);
    }
    g_events.clear();
    pos = json.find("\"events\":[");
    if (pos != std::string::npos)
    {
        const char* p = json.c_str() + pos + 10;
        while (*p && *p != ']')
        {
            uint32_t id = 0, seed = 0;
            int ph = 0, act = 0;
            int consumed = 0;
            if (std::sscanf(p, "{\"id\":%u,\"phase\":%d,\"active\":%d,\"seed\":%u}%n",
                            &id, &ph, &act, &seed, &consumed) >= 4)
            {
                g_events.push_back({id, static_cast<uint8_t>(ph), act != 0, seed});
                p += consumed;
                if (*p == ',')
                    ++p;
            }
            else
                break;
        }
    }
    g_reputation.clear();
    pos = json.find("\"reputation\":{");
    if (pos != std::string::npos)
    {
        const char* p = json.c_str() + pos + 14;
        while (*p && *p != '}')
        {
            uint32_t npc = 0;
            int rep = 0;
            int consumed = 0;
            if (std::sscanf(p, "\"%u\":%d%n", &npc, &rep, &consumed) >= 2)
            {
                g_reputation[npc] = static_cast<int16_t>(rep);
                p += consumed;
                if (*p == ',')
                    ++p;
            }
            else
                break;
        }
    }
    return true;
}

} // namespace CoopNet

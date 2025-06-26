#include "SessionState.hpp"
#include "SaveFork.hpp"
#include "SaveMigration.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

namespace CoopNet
{

struct PartyMember
{
    uint32_t peerId;
    uint32_t xp;
};

static std::vector<PartyMember> g_party;                            // PP-1: populated via lobby sync
static std::vector<std::pair<std::string, uint32_t>> g_questStages; // questName -> stage
static std::vector<ItemSnap> g_inventory;
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
        g_party.push_back({id, 0});
    }
    g_sessionId = hash;
    return g_sessionId;
}

void SaveSessionState(uint32_t sessionId)
{
    std::stringstream ss;
    ss << "{\n  \"party\": [";
    for (size_t i = 0; i < g_party.size(); ++i)
    {
        const auto& p = g_party[i];
        ss << "{\"peerId\":" << p.peerId << ",\"xp\":" << p.xp << "}";
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
    ss << "]\n}\n";

    std::string blob = ss.str();
    SaveRollbackSnapshot(sessionId, blob);
    SaveSession(sessionId, blob);
}

void SaveMergeResolution(bool acceptAll)
{
    try {
        EnsureCoopSaveDirs();
        const std::filesystem::path file =
            std::filesystem::path(kCoopSavePath) / "merged.dat";
        std::ofstream out(file, std::ios::app);
        if (!out.is_open())
        {
            std::cerr << "Failed to open merged file" << std::endl;
            return;
        }
        out << "resolution=" << (acceptAll ? "acceptAll" : "skipEach") << "\n";
    } catch (const std::exception& e) {
        std::cerr << "SaveMergeResolution error: " << e.what() << std::endl;
    }
}

uint32_t SessionState_GetId()
{
    return g_sessionId;
}

} // namespace CoopNet

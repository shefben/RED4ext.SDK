#include "SessionState.hpp"
#include "SaveFork.hpp"
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

void SessionState_SetParty(const std::vector<uint32_t>& peerIds)
{
    g_party.clear();
    for (uint32_t id : peerIds)
    {
        g_party.push_back({id, 0});
    }
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

    SaveSession(sessionId, ss.str());
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

} // namespace CoopNet

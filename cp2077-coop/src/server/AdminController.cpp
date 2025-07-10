#include "AdminController.hpp"
#include "../core/GameClock.hpp"
#include "../core/ThreadSafeQueue.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include "WebDash.hpp"
#include "VehicleController.hpp"
#include <RED4ext/RED4ext.hpp>
#include <filesystem>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <unordered_map>

namespace CoopNet
{

static std::unordered_set<uint32_t> g_banList;
static std::unordered_map<uint32_t, std::unordered_set<uint32_t>> g_kickVotes;
static ThreadSafeQueue<std::string> g_cmdQueue;
static std::thread g_consoleThread;
static std::atomic<bool> g_consoleRunning{false};
static const char* g_banFile = "server/bans.json";
static const size_t VOTE_THRESHOLD = 3;

static void LoadBans()
{
    std::ifstream f(g_banFile);
    if (!f.is_open())
        return;
    std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    size_t pos = 0;
    while (true)
    {
        pos = data.find_first_of("0123456789", pos);
        if (pos == std::string::npos)
            break;
        size_t end = data.find_first_not_of("0123456789", pos);
        uint32_t id = std::stoul(data.substr(pos, end - pos));
        g_banList.insert(id);
        if (end == std::string::npos)
            break;
        pos = end;
    }
}

static void SaveBans()
{
    std::ofstream f(g_banFile);
    if (!f.is_open())
        return;
    f << '[';
    bool first = true;
    for (uint32_t id : g_banList)
    {
        if (!first)
            f << ',';
        f << id;
        first = false;
    }
    f << ']';
}

static size_t GetProcessRSS()
{
#ifdef __linux__
    std::ifstream f("/proc/self/statm");
    size_t pages = 0, rss = 0;
    if (f >> pages >> rss)
        return rss * static_cast<size_t>(sysconf(_SC_PAGESIZE));
#endif
    return 0;
}

static void GameModeManager_SetMode(uint32_t mode)
{
    RED4ext::ExecuteFunction("GameModeManager", "SetMode", nullptr, mode);
}

static void QuestSync_SetFreeze(bool freeze)
{
    RED4ext::ExecuteFunction("QuestSync", "SetFreeze", nullptr, freeze);
}

static Connection* FindConn(uint32_t peerId)
{
    auto conns = Net_GetConnections();
    for (auto* c : conns)
    {
        if (c->peerId == peerId)
            return c;
    }
    return nullptr;
}

static void DoKick(uint32_t peerId)
{
    if (Connection* c = FindConn(peerId))
    {
        Net_SendAdminCmd(c, static_cast<uint8_t>(AdminCmdType::Kick), 0);
        Net_Disconnect(c);
    }
}

static void DoBan(uint32_t peerId)
{
    g_banList.insert(peerId);
    SaveBans();
    DoKick(peerId);
}

static void DoMute(uint32_t peerId, uint32_t mins)
{
    if (Connection* c = FindConn(peerId))
    {
        c->voiceMuted = true;
        if (mins > 0)
            c->voiceMuteEndMs = GameClock::GetTimeMs() + static_cast<uint64_t>(mins) * 60000ull;
        else
            c->voiceMuteEndMs = 0;
        Net_SendAdminCmd(c, static_cast<uint8_t>(AdminCmdType::Mute), 1);
    }
}

static void DoUnmute(uint32_t peerId)
{
    if (Connection* c = FindConn(peerId))
    {
        c->voiceMuted = false;
        c->voiceMuteEndMs = 0;
        Net_SendAdminCmd(c, static_cast<uint8_t>(AdminCmdType::Mute), 0);
    }
}

static void ConsoleThread()
{
    while (g_consoleRunning)
    {
        std::string line;
        std::getline(std::cin, line);
        if (!line.empty())
            g_cmdQueue.Push(line);
    }
}

void AdminController_Start()
{
    if (g_consoleRunning)
        return;
    LoadBans();
    g_consoleRunning = true;
    g_consoleThread = std::thread(ConsoleThread);
}

void AdminController_Stop()
{
    if (!g_consoleRunning)
        return;
    SaveBans();
    g_consoleRunning = false;
    if (g_consoleThread.joinable())
        g_consoleThread.join();
}

void AdminController_PollCommands()
{
    std::string line;
    if (!g_cmdQueue.Pop(line))
        return;
    std::stringstream ss(line);
    std::string cmd;
    ss >> cmd;
    if (cmd == "kick")
    {
        uint32_t id;
        if (ss >> id)
        {
            DoKick(id);
            WebDash_PushEvent("{\"event\":\"kick\",\"id\":" + std::to_string(id) + "}");
        }
    }
    else if (cmd == "ban")
    {
        uint32_t id;
        if (ss >> id)
        {
            DoBan(id);
            WebDash_PushEvent("{\"event\":\"ban\",\"id\":" + std::to_string(id) + "}");
        }
    }
    else if (cmd == "mute")
    {
        uint32_t id = 0;
        uint32_t mins = 0;
        if (ss >> id)
            ss >> mins;
        DoMute(id, mins);
        WebDash_PushEvent("{\"event\":\"mute\",\"id\":" + std::to_string(id) + "}");
    }
    else if (cmd == "unmute")
    {
        uint32_t id;
        if (ss >> id)
        {
            DoUnmute(id);
            WebDash_PushEvent("{\"event\":\"unmute\",\"id\":" + std::to_string(id) + "}");
        }
    }
    else if (cmd == "unstuckcar")
    {
        uint32_t id;
        if (ss >> id)
        {
            if (Connection* c = Net_FindConnection(id))
                VehicleController_HandleTowRequest(c, c->avatarPos);
        }
    }
    else if (cmd == "purgecache")
    {
        namespace fs = std::filesystem;
        fs::remove_all("runtime_cache/plugins");
        fs::remove_all("cache/plugins");
        size_t rss = GetProcessRSS();
        std::cout << "[Admin] cache purged, RSS=" << rss / (1024 * 1024) << " MB" << std::endl;
    }
    else if (cmd == "sv_dm")
    {
        int flag = 0;
        if (ss >> flag)
        {
            GameModeManager_SetMode(flag ? 1u : 0u);
            QuestSync_SetFreeze(flag != 0);
        }
    }
}

bool AdminController_IsBanned(uint32_t peerId)
{
    return g_banList.count(peerId) != 0;
}

void AdminController_Ban(uint32_t peerId)
{
    DoBan(peerId);
}

void AdminController_AddKickVote(uint32_t voterId, uint32_t targetId)
{
    auto& set = g_kickVotes[targetId];
    set.insert(voterId);
    if (set.size() >= VOTE_THRESHOLD)
    {
        Net_BroadcastChat("VoteKick passed for " + std::to_string(targetId));
        DoKick(targetId);
        g_kickVotes.erase(targetId);
    }
}

} // namespace CoopNet

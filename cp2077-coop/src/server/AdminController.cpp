#include "AdminController.hpp"
#include "../core/GameClock.hpp"
#include "../core/ThreadSafeQueue.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include "WorldStateIO.hpp"
#include "../core/SessionState.hpp"
#include "WebDash.hpp"
#include "VehicleController.hpp"
#include "../core/Red4extUtils.hpp"
#include <RED4ext/RED4ext.hpp>
#include <filesystem>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace CoopNet
{

static std::unordered_set<uint32_t> g_banList;
static ThreadSafeQueue<std::string> g_cmdQueue;
static std::thread g_consoleThread;
static std::atomic<bool> g_consoleRunning{false};
struct VoteKickData
{
    bool active = false;
    uint32_t target = 0;
    float timer = 0.f;
    std::unordered_set<uint32_t> votes;
};
static VoteKickData g_voteKick;

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
    RED4EXT_EXECUTE("GameModeManager", "SetMode", nullptr, mode);
}

static void QuestSync_SetFreeze(bool freeze)
{
    RED4EXT_EXECUTE("QuestSync", "SetFreeze", nullptr, freeze);
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

static const char* GetBanPath()
{
    return "server/bans.json";
}

static void LoadBans()
{
    std::ifstream in(GetBanPath());
    if (!in.is_open())
        return;
    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    size_t pos = 0;
    while (true)
    {
        pos = json.find_first_of("0123456789", pos);
        if (pos == std::string::npos)
            break;
        size_t end = json.find_first_not_of("0123456789", pos);
        g_banList.insert(std::stoul(json.substr(pos, end - pos)));
        pos = end;
    }
}

static void SaveBans()
{
    namespace fs = std::filesystem;
    fs::create_directories("server");
    std::ofstream out(GetBanPath());
    if (!out.is_open())
        return;
    out << "[";
    bool first = true;
    for (uint32_t id : g_banList)
    {
        if (!first)
            out << ",";
        out << id;
        first = false;
    }
    out << "]";
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

static void CastVoteKick(uint32_t voter, uint32_t target)
{
    if (!g_voteKick.active || g_voteKick.target != target)
    {
        g_voteKick.active = true;
        g_voteKick.target = target;
        g_voteKick.timer = 30.f;
        g_voteKick.votes.clear();
    }
    g_voteKick.votes.insert(voter);
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
    else if (cmd == "snapshot")
    {
        WorldStatePacket pkt{};
        const auto& ws = SessionState_GetWorld();
        pkt.sunAngleDeg = ws.sunDeg;
        pkt.weatherId = ws.weatherId;
        pkt.particleSeed = ws.particleSeed;
        SaveWorldState(pkt);
        SaveSessionState(SessionState_GetId());
        std::cout << "[Admin] world snapshot saved" << std::endl;
    }
    else if (cmd == "reset")
    {
        WorldStatePacket pkt{};
        if (LoadWorldState(pkt))
        {
            SessionState_UpdateWeather(pkt.sunAngleDeg, pkt.weatherId, pkt.particleSeed);
            Net_BroadcastWorldState(pkt.sunAngleDeg, pkt.weatherId, pkt.particleSeed);
            std::cout << "[Admin] world reset" << std::endl;
        }
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

void AdminController_Kick(uint32_t peerId)
{
    DoKick(peerId);
}

void AdminController_Ban(uint32_t peerId)
{
    DoBan(peerId);
}

void AdminController_Mute(uint32_t peerId, uint32_t mins)
{
    DoMute(peerId, mins);
}

void AdminController_Unmute(uint32_t peerId)
{
    DoUnmute(peerId);
}

void AdminController_HandleVoteKick(uint32_t voterId, uint32_t targetId)
{
    CastVoteKick(voterId, targetId);
}

void AdminController_Tick(float dt)
{
    AdminController_PollCommands();
    if (g_voteKick.active)
    {
        g_voteKick.timer -= dt / 1000.f;
        size_t total = Net_GetConnections().size();
        size_t yes = g_voteKick.votes.size();
        if (yes > total / 2)
        {
            DoKick(g_voteKick.target);
            g_voteKick.active = false;
            g_voteKick.votes.clear();
        }
        else if (g_voteKick.timer <= 0.f)
        {
            g_voteKick.active = false;
            g_voteKick.votes.clear();
        }
    }
}

} // namespace CoopNet

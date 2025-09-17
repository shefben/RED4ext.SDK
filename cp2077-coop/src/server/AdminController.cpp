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
#ifndef _WIN32
#include <unistd.h>
#endif
#include <openssl/sha.h>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace CoopNet
{

static std::unordered_set<uint32_t> g_banList;
static std::mutex g_banMutex;
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

static std::string GetBanPath()
{
    // Use configurable ban file path with validation
    const char* configPath = std::getenv("COOP_BAN_FILE");
    if (configPath && strlen(configPath) > 0) {
        // Validate path to prevent directory traversal
        std::string path(configPath);
        if (path.find("..") != std::string::npos || path.find("//") != std::string::npos) {
            std::cerr << "Invalid ban file path detected, using default" << std::endl;
            return "server/bans.json";
        }
        return path;
    }
    return "server/bans.json";
}

static std::string GetBanSalt()
{
    const char* s = std::getenv("COOP_BAN_SALT");
    return s ? std::string(s) : std::string("coop-ban-salt");
}

static std::string ComputeBanChecksum(const std::vector<uint32_t>& ids, const std::string& salt)
{
    std::ostringstream data;
    for (size_t i = 0; i < ids.size(); ++i)
    {
        if (i) data << ',';
        data << ids[i];
    }
    data << '|' << salt;
    std::string s = data.str();
    unsigned char sha[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(s.data()), s.size(), sha);
    std::ostringstream hex;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        hex << std::hex << std::setw(2) << std::setfill('0') << (int)sha[i];
    return hex.str();
}

static void LoadBans()
{
    std::ifstream in(GetBanPath());
    if (!in.is_open())
        return;
    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    // Extract bans array
    size_t a = json.find("\"bans\":[");
    if (a == std::string::npos) return;
    size_t b = json.find(']', a);
    if (b == std::string::npos) return;
    std::string bans = json.substr(a + 8, b - (a + 8));
    std::vector<uint32_t> ids;
    size_t pos = 0;
    while (true)
    {
        pos = bans.find_first_of("0123456789", pos);
        if (pos == std::string::npos)
            break;
        size_t end = bans.find_first_not_of("0123456789", pos);
        try { ids.push_back(std::stoul(bans.substr(pos, end - pos))); } catch (...) {}
        pos = end;
    }
    // Verify checksum if present
    size_t cs = json.find("\"checksum\":\"");
    bool ok = true;
    if (cs != std::string::npos)
    {
        size_t cse = json.find('"', cs + 13);
        if (cse != std::string::npos)
        {
            std::string sum = json.substr(cs + 12, cse - (cs + 12));
            std::sort(ids.begin(), ids.end());
            ok = (sum == ComputeBanChecksum(ids, GetBanSalt()));
        }
    }
    if (!ok)
    {
        std::cerr << "[Admin] ban list checksum mismatch, ignoring file" << std::endl;
        return;
    }
    for (uint32_t id : ids)
        g_banList.insert(id);
}

static void SaveBans()
{
    namespace fs = std::filesystem;
    fs::create_directories("server");
    std::ofstream out(GetBanPath());
    if (!out.is_open())
        return;
    std::vector<uint32_t> ids;
    ids.reserve(g_banList.size());
    for (uint32_t id : g_banList) ids.push_back(id);
    std::sort(ids.begin(), ids.end());
    std::string sum = ComputeBanChecksum(ids, GetBanSalt());
    out << "{\"checksum\":\"" << sum << "\",\"bans\":[";
    for (size_t i = 0; i < ids.size(); ++i)
    {
        if (i) out << ',';
        out << ids[i];
    }
    out << "]}";
}

static void DoBan(uint32_t peerId)
{
    std::lock_guard lock(g_banMutex);
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
    std::lock_guard lock(g_banMutex);
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

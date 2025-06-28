#include "AdminController.hpp"
#include "../core/GameClock.hpp"
#include "../core/ThreadSafeQueue.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include "WebDash.hpp"
#include "VehicleController.hpp"
#include <RED4ext/RED4ext.hpp>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace CoopNet
{

static std::unordered_set<uint32_t> g_banList;
static ThreadSafeQueue<std::string> g_cmdQueue;
static std::thread g_consoleThread;
static std::atomic<bool> g_consoleRunning{false};

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
    g_consoleRunning = true;
    g_consoleThread = std::thread(ConsoleThread);
}

void AdminController_Stop()
{
    if (!g_consoleRunning)
        return;
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

} // namespace CoopNet

#include "AdminController.hpp"
#include "../core/GameClock.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace CoopNet {

static std::unordered_set<uint32_t> g_banList;

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

static void DoMute(uint32_t peerId, uint32_t secs)
{
    if (Connection* c = FindConn(peerId))
    {
        c->muteUntilMs = GameClock::GetTimeMs() + static_cast<uint64_t>(secs) * 1000ull;
        Net_SendAdminCmd(c, static_cast<uint8_t>(AdminCmdType::Mute), secs);
    }
}

void AdminController_PollConsole()
{
    if (!std::cin.rdbuf()->in_avail())
        return;
    std::string line;
    std::getline(std::cin, line);
    std::stringstream ss(line);
    std::string cmd;
    ss >> cmd;
    if (cmd == "kick")
    {
        uint32_t id;
        if (ss >> id)
            DoKick(id);
    }
    else if (cmd == "ban")
    {
        uint32_t id;
        if (ss >> id)
            DoBan(id);
    }
    else if (cmd == "mute")
    {
        uint32_t id, secs;
        if (ss >> id >> secs)
            DoMute(id, secs);
    }
}

bool AdminController_IsBanned(uint32_t peerId)
{
    return g_banList.count(peerId) != 0;
}

} // namespace CoopNet

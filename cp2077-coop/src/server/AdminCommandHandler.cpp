#include "AdminCommandHandler.hpp"
#include "AdminController.hpp"
#include "Journal.hpp"
#include "../core/GameClock.hpp"
#include "../core/SessionState.hpp"
#include "../net/Net.hpp"
#include "WorldStateIO.hpp"
#include <sstream>

namespace CoopNet {

bool AdminCommandHandler_Handle(uint32_t senderId, const std::string& text)
{
    if (text.empty() || text[0] != '/')
        return false;
    std::stringstream ss(text.substr(1));
    std::string cmd;
    ss >> cmd;
    if (cmd == "kick")
    {
        uint32_t id;
        if (ss >> id)
        {
            AdminController_Kick(id);
            Journal_Log(GameClock::GetCurrentTick(), senderId, "kick", id, 0);
        }
        return true;
    }
    else if (cmd == "ban")
    {
        uint32_t id;
        if (ss >> id)
        {
            AdminController_Ban(id);
            Journal_Log(GameClock::GetCurrentTick(), senderId, "ban", id, 0);
        }
        return true;
    }
    else if (cmd == "votekick")
    {
        uint32_t id;
        if (ss >> id)
        {
            AdminController_HandleVoteKick(senderId, id);
        }
        return true;
    }
    else if (cmd == "mute")
    {
        uint32_t id = 0;
        uint32_t mins = 0;
        if (ss >> id)
            ss >> mins;
        AdminController_Mute(id, mins);
        Journal_Log(GameClock::GetCurrentTick(), senderId, "mute", id, static_cast<int32_t>(mins));
        return true;
    }
    else if (cmd == "unmute")
    {
        uint32_t id;
        if (ss >> id)
        {
            AdminController_Unmute(id);
            Journal_Log(GameClock::GetCurrentTick(), senderId, "unmute", id, 0);
        }
        return true;
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
        return true;
    }
    else if (cmd == "reset")
    {
        WorldStatePacket pkt{};
        if (LoadWorldState(pkt))
        {
            SessionState_UpdateWeather(pkt.sunAngleDeg, pkt.weatherId, pkt.particleSeed);
            Net_BroadcastWorldState(pkt.sunAngleDeg, pkt.weatherId, pkt.particleSeed);
        }
        return true;
    }
    return false;
}

} // namespace CoopNet

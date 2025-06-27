#include "ApartmentController.hpp"
#include "../core/GameClock.hpp"
#include "../core/SaveFork.hpp"
#include "../core/SessionState.hpp"
#include "../net/Net.hpp"
#include "Journal.hpp"
#include "LedgerService.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace CoopNet
{
struct AptInfo
{
    float x;
    float y;
    float z;
    uint32_t price;
    std::string interiorScene;
    uint32_t extDoorId;
};
static std::unordered_map<uint32_t, AptInfo> g_info;
static std::unordered_map<uint32_t, std::unordered_set<uint32_t>> g_owned;
static std::unordered_map<uint32_t, bool> g_loaded;
struct PermInfo
{
    bool pub = false;
    std::unordered_set<uint32_t> peers;
};
static std::unordered_map<uint32_t, PermInfo> g_perms;

void ApartmentController_Load()
{
    std::ifstream in("Apartments.csv");
    if (!in.is_open())
        return;
    std::string line;
    std::getline(in, line); // header
    while (std::getline(in, line))
    {
        std::stringstream ss(line);
        std::string tok;
        AptInfo info{};
        uint32_t id = 0;
        for (int i = 0; std::getline(ss, tok, ','); ++i)
        {
            switch (i)
            {
            case 0:
                id = static_cast<uint32_t>(std::stoul(tok));
                break;
            case 1:
                info.x = std::stof(tok);
                break;
            case 2:
                info.y = std::stof(tok);
                break;
            case 3:
                info.z = std::stof(tok);
                break;
            case 4:
                info.price = static_cast<uint32_t>(std::stoul(tok));
                break;
            case 5:
                info.interiorScene = tok;
                break;
            case 6:
                info.extDoorId = static_cast<uint32_t>(std::stoul(tok));
                break;
            default:
                break;
            }
        }
        g_info[id] = info;
    }
    std::cout << "[Apt] loaded " << g_info.size() << " entries" << std::endl;
}

void ApartmentController_HandlePurchase(Connection* conn, uint32_t aptId)
{
    auto it = g_info.find(aptId);
    if (it == g_info.end() || !conn)
        return;
    uint64_t bal;
    if (!Ledger_Transfer(conn, -static_cast<int64_t>(it->second.price), 0, bal))
    {
        Net_SendAptPurchaseAck(conn, aptId, false, conn->balance);
        return;
    }
    g_owned[conn->peerId].insert(aptId);
    Journal_Log(GameClock::GetCurrentTick(), conn->peerId, "purchase", aptId, -static_cast<int32_t>(it->second.price));
    std::stringstream ss;
    ss << "{\"ApartmentOwnership\":[";
    bool first = true;
    for (auto id : g_owned[conn->peerId])
    {
        if (!first)
            ss << ',';
        ss << "{\"aptId\":" << id << ",\"owner\":" << conn->peerId << '}';
        first = false;
    }
    ss << "]}";
    SavePhase(SessionState_GetId(), conn->peerId, ss.str());
    Net_SendAptPurchaseAck(conn, aptId, true, bal);
}

const AptInfo* ApartmentController_GetInfo(uint32_t aptId)
{
    auto it = g_info.find(aptId);
    if (it == g_info.end())
        return nullptr;
    return &it->second;
}

bool ApartmentController_IsOwned(uint32_t peerId, uint32_t aptId)
{
    auto it = g_owned.find(peerId);
    if (it == g_owned.end())
        return false;
    return it->second.find(aptId) != it->second.end();
}

void ApartmentController_HandleEnter(Connection* conn, uint32_t aptId, uint32_t ownerPhaseId)
{
    if (!conn)
        return;
    auto infoIt = g_info.find(aptId);
    if (infoIt == g_info.end())
    {
        Net_SendAptEnterAck(conn, false, 0, 0);
        return;
    }
    if (!ApartmentController_IsOwned(ownerPhaseId, aptId))
    {
        Net_SendAptEnterAck(conn, false, 0, 0);
        return;
    }
    if (conn->peerId != ownerPhaseId)
    {
        auto permIt = g_perms.find(aptId);
        bool allowed = permIt != g_perms.end() && (permIt->second.pub || permIt->second.peers.count(conn->peerId));
        if (!allowed)
        {
            Net_SendAptEnterAck(conn, false, 0, 0);
            return;
        }
    }

    if (!g_loaded[ownerPhaseId])
    {
        // Interior scenes stream on demand per owner phase
        g_loaded[ownerPhaseId] = true;
    }

    uint32_t seed = static_cast<uint32_t>(std::hash<std::string>{}(infoIt->second.interiorScene) ^ ownerPhaseId);
    Net_SendAptEnterAck(conn, true, ownerPhaseId, seed);
}

void ApartmentController_HandlePermChange(Connection* conn, uint32_t aptId, uint32_t targetPeerId, bool allow)
{
    if (!conn || !ApartmentController_IsOwned(conn->peerId, aptId))
        return;
    PermInfo& p = g_perms[aptId];
    if (targetPeerId == 0)
    {
        p.pub = allow;
    }
    else if (allow)
    {
        p.peers.insert(targetPeerId);
    }
    else
    {
        p.peers.erase(targetPeerId);
    }
    AptPermChangePacket pkt{aptId, targetPeerId, static_cast<uint8_t>(allow), {0, 0, 0}};
    Net_Broadcast(EMsg::AptPermChange, &pkt, sizeof(pkt));
}

} // namespace CoopNet

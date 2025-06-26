#include "ElevatorController.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include <unordered_set>
#include <iostream>

namespace CoopNet {

struct ArrivalState {
    bool active = false;
    ElevatorArrivePacket pkt{};
    float timer = 0.f;
    int retries = 0;
    std::unordered_set<Connection*> acks;
};

static ArrivalState g_arrive;

void ElevatorController_OnCall(uint32_t peerId, uint32_t elevatorId, uint8_t floor)
{
    ElevatorCallPacket pkt{peerId, elevatorId, floor, {0,0,0}};
    Net_Broadcast(EMsg::ElevatorCall, &pkt, sizeof(pkt));
}

void ElevatorController_OnArrive(uint32_t elevatorId, uint64_t sectorHash, const RED4ext::Vector3& pos)
{
    g_arrive.active = true;
    g_arrive.pkt.elevatorId = elevatorId;
    g_arrive.pkt.sectorHash = sectorHash;
    g_arrive.pkt.pos = pos;
    g_arrive.timer = 8.f;
    g_arrive.retries = 0;
    g_arrive.acks.clear();
    Net_Broadcast(EMsg::ElevatorArrive, &g_arrive.pkt, sizeof(g_arrive.pkt));
}

void ElevatorController_OnAck(Connection* conn, uint32_t elevatorId)
{
    if (g_arrive.active && g_arrive.pkt.elevatorId == elevatorId)
        g_arrive.acks.insert(conn);
}

bool ElevatorController_IsPaused()
{
    return g_arrive.active && g_arrive.retries > 0 &&
           g_arrive.acks.size() < Net_GetConnections().size();
}

void ElevatorController_ServerTick(float dt)
{
    if (!g_arrive.active)
        return;
    if (g_arrive.acks.size() == Net_GetConnections().size())
    {
        g_arrive.active = false;
        g_arrive.acks.clear();
        return;
    }
    g_arrive.timer -= dt / 1000.f;
    if (g_arrive.timer <= 0.f)
    {
        if (g_arrive.retries >= 3)
        {
            g_arrive.active = false;
            g_arrive.acks.clear();
        }
        else
        {
            Net_Broadcast(EMsg::ElevatorArrive, &g_arrive.pkt, sizeof(g_arrive.pkt));
            g_arrive.retries++;
            g_arrive.timer = 8.f;
        }
    }
}

} // namespace CoopNet

#include "VehicleController.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include "../net/Connection.hpp"
#include "../core/Hash.hpp"
#include "../core/GameClock.hpp"
#include <cmath>
#include <iostream>

namespace CoopNet {
struct VehicleState {
    uint32_t id = 1;
    uint32_t archetype = 0;
    uint32_t paint = 0;
    TransformSnap snap{};
    uint16_t damage = 0;
    RED4ext::Vector3 prevVel{};
    bool destroyed = false;
    float despawn = 0.f;
    uint32_t seat[4] = {0,0,0,0};
    float lastHit = 0.f;
};

static VehicleState g_vehicle;

void VehicleController_Spawn(uint32_t archetype, uint32_t paint, const TransformSnap& t)
{
    g_vehicle.archetype = archetype;
    g_vehicle.paint = paint;
    g_vehicle.snap = t;
    g_vehicle.damage = 0;
    g_vehicle.destroyed = false;
    g_vehicle.despawn = 0.f;
    for (int i = 0; i < 4; ++i)
        g_vehicle.seat[i] = 0;
    VehicleSpawnPacket pkt{g_vehicle.id, archetype, paint, t};
    Net_Broadcast(EMsg::VehicleSpawn, &pkt, sizeof(pkt));
}

static float VecLen(const RED4ext::Vector3& v)
{
    return std::sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
}

void VehicleController_ApplyDamage(uint16_t dmg, bool side)
{
    g_vehicle.damage += dmg;
    if (side && dmg > 300u)
    {
        VehiclePartDetachPacket pkt{g_vehicle.id, 0, {0,0,0}};
        Net_Broadcast(EMsg::VehiclePartDetach, &pkt, sizeof(pkt));
    }
    if (!g_vehicle.destroyed && g_vehicle.damage >= 1000u)
    {
        uint32_t vfx = Fnv1a32("veh_explosion_big.ent");
        uint32_t seed = g_vehicle.damage * 1664525u + 1013904223u;
        VehicleExplodePacket pkt{g_vehicle.id, vfx, seed};
        Net_Broadcast(EMsg::VehicleExplode, &pkt, sizeof(pkt));
        g_vehicle.destroyed = true;
        g_vehicle.despawn = 10.f;
    }
}

void VehicleController_SetOccupant(uint32_t peerId)
{
    g_vehicle.seat[0] = peerId;
}

void VehicleController_HandleSeatRequest(CoopNet::Connection* c, uint32_t vehicleId, uint8_t seatIdx)
{
    if (vehicleId != g_vehicle.id || seatIdx >= 4)
        return;
    if (g_vehicle.seat[seatIdx] == 0)
    {
        g_vehicle.seat[seatIdx] = c->peerId;
        SeatAssignPacket pkt{c->peerId, vehicleId, seatIdx};
        Net_Broadcast(EMsg::SeatAssign, &pkt, sizeof(pkt));
    }
}

void VehicleController_HandleHit(uint32_t vehicleId, uint16_t dmg, bool side)
{
    if (vehicleId != g_vehicle.id)
        return;
    float now = GameClock::GetCurrentTick() * GameClock::GetTickMs();
    if (now - g_vehicle.lastHit < 200.f)
        return;
    g_vehicle.lastHit = now;
    VehicleController_ApplyDamage(dmg, side);
    VehicleHitPacket pkt{vehicleId, dmg, side ? 1 : 0, 0};
    Net_Broadcast(EMsg::VehicleHit, &pkt, sizeof(pkt));
}

void VehicleController_RemovePeer(uint32_t peerId)
{
    for (int i = 0; i < 4; ++i)
        if (g_vehicle.seat[i] == peerId)
            g_vehicle.seat[i] = 0;
}

void VehicleController_ServerTick(float dt)
{
    if (g_vehicle.destroyed)
    {
        g_vehicle.despawn -= dt / 1000.f;
        return;
    }
    float vPrev = VecLen(g_vehicle.prevVel);
    float vCur = VecLen(g_vehicle.snap.vel);
    float decel = (vPrev - vCur) / (dt / 1000.f);
    if (decel > 12.f && g_vehicle.seat[0] != 0)
    {
        RED4ext::Vector3 launch = g_vehicle.prevVel;
        EjectOccupantPacket pkt{g_vehicle.seat[0], launch};
        Net_Broadcast(EMsg::EjectOccupant, &pkt, sizeof(pkt));
        g_vehicle.seat[0] = 0;
    }
    g_vehicle.prevVel = g_vehicle.snap.vel;
}

} // namespace CoopNet

#include "VehicleController.hpp"
#include "../core/GameClock.hpp"
#include "../core/Hash.hpp"
#include "../core/SaveFork.hpp"
#include "../core/SessionState.hpp"
#include "../net/Connection.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include <cmath>
#include <iostream>

namespace CoopNet
{
struct VehicleState
{
    uint32_t id = 1;
    uint32_t archetype = 0;
    uint32_t paint = 0;
    TransformSnap snap{};
    uint16_t damage = 0;
    RED4ext::Vector3 prevVel{};
    bool destroyed = false;
    float despawn = 0.f;
    float idle = 0.f;
    uint32_t owner = 0;
    uint32_t phaseId = 0;
    uint32_t seat[4] = {0, 0, 0, 0};
    float lastHit = 0.f;
    float towTimer = 0.f;
};

static VehicleState g_vehicle;

void VehicleController_SpawnPhaseVehicle(uint32_t archetype, uint32_t paint, const TransformSnap& t, uint32_t phaseId)
{
    g_vehicle.phaseId = phaseId;
    g_vehicle.archetype = archetype;
    g_vehicle.paint = paint;
    g_vehicle.snap = t;
    g_vehicle.damage = 0;
    g_vehicle.destroyed = false;
    g_vehicle.despawn = 0.f;
    g_vehicle.idle = 0.f;
    g_vehicle.owner = 0;
    g_vehicle.towTimer = 0.f;
    for (int i = 0; i < 4; ++i)
        g_vehicle.seat[i] = 0;
    VehicleSpawnPacket pkt{g_vehicle.id, archetype, paint, phaseId, t};
    Net_Broadcast(EMsg::VehicleSpawn, &pkt, sizeof(pkt));
}

void VehicleController_Spawn(uint32_t archetype, uint32_t paint, const TransformSnap& t)
{
    VehicleController_SpawnPhaseVehicle(archetype, paint, t, 0u);
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
        VehiclePartDetachPacket pkt{g_vehicle.id, 0, {0, 0, 0}};
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
        g_vehicle.towTimer = 300.f;
        CarParking cp{};
        cp.vehTpl = g_vehicle.archetype;
        cp.pos = g_vehicle.snap.pos;
        cp.rot = g_vehicle.snap.rot;
        cp.health = 0;
        SaveCarParking(SessionState_GetId(), g_vehicle.owner, cp);
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

void VehicleController_HandleSummon(CoopNet::Connection* c, uint32_t vehId, const TransformSnap& t)
{
    if (g_vehicle.id == vehId && !g_vehicle.destroyed)
    {
        if (g_vehicle.damage >= 500u)
            return; // in combat or heavily damaged
        g_vehicle.snap = t;
    }
    else
    {
        g_vehicle.id = vehId;
        g_vehicle.snap = t;
        g_vehicle.damage = 0;
        g_vehicle.destroyed = false;
    }
    g_vehicle.owner = c->peerId;
    g_vehicle.idle = 0.f;
    VehicleSummonPacket pkt{vehId, c->peerId, t};
    Net_Broadcast(EMsg::VehicleSummon, &pkt, sizeof(pkt));
}

static RED4ext::Vector3 FindSafePos(const RED4ext::Vector3& pos)
{
    // Engine nav query will snap to nearest road; placeholder returns input
    return pos;
}

void VehicleController_HandleTowRequest(CoopNet::Connection* c, const RED4ext::Vector3& pos)
{
    if (!c || g_vehicle.owner != c->peerId)
        return;
    if (g_vehicle.destroyed)
    {
        g_vehicle.owner = 0;
        g_vehicle.towTimer = 0.f;
        if (Connection* target = Net_FindConnection(c->peerId))
            Net_SendVehicleTowAck(target, c->peerId, true);
        std::cout << "[Tow] Car returned" << std::endl;
    }
    else
    {
        g_vehicle.snap.pos = FindSafePos(pos);
        Net_SendVehicleTowAck(c, c->peerId, true);
    }
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
        if (g_vehicle.towTimer > 0.f)
        {
            g_vehicle.towTimer -= dt / 1000.f;
            if (g_vehicle.towTimer <= 0.f && g_vehicle.owner != 0)
            {
                if (Connection* c = Net_FindConnection(g_vehicle.owner))
                    Net_SendVehicleTowAck(c, g_vehicle.owner, true);
                std::cout << "[Tow] Car returned" << std::endl;
                g_vehicle.owner = 0;
            }
        }
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
    if (g_vehicle.seat[0] == 0 && vCur < 0.1f)
    {
        g_vehicle.idle += dt / 1000.f;
        if (g_vehicle.idle >= 10.f)
        {
            CarParking cp{};
            cp.vehTpl = g_vehicle.archetype;
            cp.pos = g_vehicle.snap.pos;
            cp.rot = g_vehicle.snap.rot;
            cp.health = static_cast<uint16_t>(1000u - g_vehicle.damage);
            SaveCarParking(SessionState_GetId(), g_vehicle.owner, cp);
            Net_BroadcastTrafficDespawn(g_vehicle.id);
            g_vehicle.idle = 0.f;
        }
    }
    else
    {
        g_vehicle.idle = 0.f;
    }
}

} // namespace CoopNet

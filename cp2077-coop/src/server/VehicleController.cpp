#include "VehicleController.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include "../core/Hash.hpp"
#include <cmath>
#include <iostream>

namespace CoopNet {
struct VehicleState {
    uint32_t id = 1;
    TransformSnap snap{};
    uint16_t damage = 0;
    RED4ext::Vector3 prevVel{};
    bool destroyed = false;
    float despawn = 0.f;
    uint32_t occupant = 0;
};

static VehicleState g_vehicle;

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
    g_vehicle.occupant = peerId;
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
    if (decel > 12.f && g_vehicle.occupant != 0)
    {
        EjectOccupantPacket pkt{g_vehicle.occupant};
        Net_Broadcast(EMsg::EjectOccupant, &pkt, sizeof(pkt));
        g_vehicle.occupant = 0;
    }
    g_vehicle.prevVel = g_vehicle.snap.vel;
}

} // namespace CoopNet

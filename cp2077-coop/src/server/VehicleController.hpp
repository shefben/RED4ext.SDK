#pragma once
#include "../net/Snapshot.hpp"

namespace CoopNet
{
class Connection;
void VehicleController_ServerTick(float dt);
void VehicleController_ApplyDamage(uint16_t dmg, bool side);
void VehicleController_SetOccupant(uint32_t peerId);
void VehicleController_Spawn(uint32_t archetype, uint32_t paint, const TransformSnap& t);
void VehicleController_SpawnPhaseVehicle(uint32_t archetype, uint32_t paint, const TransformSnap& t, uint32_t phaseId);
void VehicleController_HandleSeatRequest(CoopNet::Connection* c, uint32_t vehicleId, uint8_t seatIdx);
void VehicleController_HandleHit(uint32_t vehicleId, uint16_t dmg, bool side);
void VehicleController_RemovePeer(uint32_t peerId);
void VehicleController_HandleSummon(CoopNet::Connection* c, uint32_t vehId, const TransformSnap& t);
void VehicleController_HandleTowRequest(CoopNet::Connection* c, const RED4ext::Vector3& pos);
void VehicleController_PhysicsStep(float dt);

// Utility: query the current vehicle id owned or driven by a peer (0 if none)
uint32_t VehicleController_GetPeerVehicleId(uint32_t peerId);

// Future-proof validated hit application (includes attacker id for provenance)
void VehicleController_ApplyHitValidated(uint32_t attackerPeerId, uint32_t vehicleId, uint16_t dmg, bool side);
} // namespace CoopNet

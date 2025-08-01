#pragma once
#include "../net/Snapshot.hpp"

namespace CoopNet
{
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
} // namespace CoopNet

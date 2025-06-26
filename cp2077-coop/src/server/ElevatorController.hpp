#pragma once
#include "../net/Packets.hpp"
#include "../net/Connection.hpp"
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>
#include <cstdint>

namespace CoopNet {
void ElevatorController_OnCall(uint32_t peerId, uint32_t elevatorId, uint8_t floorIdx);
void ElevatorController_OnArrive(uint32_t elevatorId, uint64_t sectorHash, const RED4ext::Vector3& pos);
void ElevatorController_OnAck(Connection* conn, uint32_t elevatorId);
void ElevatorController_ServerTick(float dt);
bool ElevatorController_IsPaused();
}

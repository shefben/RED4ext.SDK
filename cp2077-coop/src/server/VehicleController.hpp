#pragma once
#include "../net/Snapshot.hpp"

namespace CoopNet {
void VehicleController_ServerTick(float dt);
void VehicleController_ApplyDamage(uint16_t dmg, bool side);
void VehicleController_SetOccupant(uint32_t peerId);
}

#pragma once
#include <RED4ext/Scripting/Natives/Vector3.hpp>
#include <cstdint>

namespace CoopNet
{
void GrenadeController_Prime(uint32_t entityId, uint32_t startTick); // GR-1
void GrenadeController_Remove(uint32_t entityId);                    // GR-1
void GrenadeController_Tick(float dt);                               // GR-1
} // namespace CoopNet

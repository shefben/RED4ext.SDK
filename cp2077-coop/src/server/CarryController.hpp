#pragma once
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>
#include <cstdint>

namespace CoopNet
{
void CarryController_Begin(uint32_t carrierId, uint32_t entityId);                                     // PC-1
void CarryController_End(uint32_t entityId, const RED4ext::Vector3& pos, const RED4ext::Vector3& vel); // PC-1
void CarryController_Tick(float dt);                                                                   // PC-1
} // namespace CoopNet

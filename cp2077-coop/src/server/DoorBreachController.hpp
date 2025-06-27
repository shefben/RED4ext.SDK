#pragma once
#include <cstdint>
namespace CoopNet
{
void DoorBreachController_Start(uint32_t doorId, uint32_t phaseId);
void DoorBreachController_Tick(float dt);
} // namespace CoopNet

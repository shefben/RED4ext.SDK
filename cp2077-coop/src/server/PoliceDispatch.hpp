#pragma once
#include <cstdint>

namespace CoopNet
{
void PoliceDispatch_OnHeatChange(uint8_t level);
void PoliceDispatch_Tick(float dt);
} // namespace CoopNet

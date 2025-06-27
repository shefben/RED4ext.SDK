#pragma once
#include <cstdint>

namespace CoopNet
{
void CamController_Hijack(uint32_t camId, uint32_t peerId); // SF-1
void CamController_Stop(uint32_t camId);                    // SF-1
void CamController_Tick(float dt);                          // SF-1
} // namespace CoopNet

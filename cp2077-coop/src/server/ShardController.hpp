#pragma once
#include <cstdint>

namespace CoopNet
{
void ShardController_Start(uint32_t phaseId);
void ShardController_HandleSelect(uint32_t peerId, uint8_t row, uint8_t col);
void ShardController_ServerTick(float dt);
} // namespace CoopNet

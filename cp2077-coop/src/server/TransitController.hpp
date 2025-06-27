#pragma once
#include <cstdint>

namespace CoopNet
{
void TransitController_Board(uint32_t peerId, uint32_t lineId, uint8_t carIdx);
void TransitController_Arrive(uint32_t peerId, uint32_t stationId);
} // namespace CoopNet

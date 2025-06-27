#pragma once
#include "../net/Connection.hpp"
#include <cstdint>

namespace CoopNet
{
void PerkController_HandleUnlock(Connection* conn, uint32_t perkId, uint8_t rank);
void PerkController_HandleRespec(Connection* conn);
float PerkController_GetHealthMult(uint32_t peerId);
bool PerkController_HasRelic(uint32_t peerId, uint32_t perkId); // SX-2
} // namespace CoopNet

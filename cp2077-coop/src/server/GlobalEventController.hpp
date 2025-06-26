#pragma once
#include <cstdint>

namespace CoopNet {
void GlobalEvent_Start(uint32_t eventId, uint8_t phase, uint32_t seed);
void GlobalEvent_Stop(uint32_t eventId, uint8_t phase);
}

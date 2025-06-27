#pragma once
#include <cstdint>

namespace CoopNet {
void Arcade_Start(uint32_t cabId, uint32_t peerId, uint32_t seed);
void Arcade_Input(uint32_t frame, uint8_t buttons);
void Arcade_End(uint32_t peerId, uint32_t score);
void Arcade_Tick(float dt);
}

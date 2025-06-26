#pragma once
#include <cstdint>

namespace CoopNet {
void BreachController_Start(uint32_t peerId, uint8_t w, uint8_t h);
void BreachController_HandleInput(uint32_t peerId, uint8_t idx);
void BreachController_ServerTick(float dt);
}

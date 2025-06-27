#pragma once
#include <cstdint>
namespace CoopNet
{
void BillboardController_OnSectorLoad(uint32_t peerId, uint64_t hash);
void BillboardController_Tick(float dt);
} // namespace CoopNet

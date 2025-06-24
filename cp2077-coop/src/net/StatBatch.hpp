#pragma once

#include <RED4ext/Scripting/Natives/Generated/uint16.hpp>
#include <RED4ext/Scripting/Natives/Generated/uint32.hpp>
#include <vector>

namespace CoopNet
{
struct BatchedStats
{
    std::vector<uint32_t> peerId;
    std::vector<uint16_t> k;
    std::vector<uint16_t> d;
};

void FlushStats();
void AddScore(uint32_t peerId, uint16_t k, uint16_t d);

} // namespace CoopNet

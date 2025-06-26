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
    std::vector<uint16_t> a;
    std::vector<uint32_t> dmg;
    std::vector<uint16_t> hs;
};

void StatBatch_Tick(float dt);
void AddStats(uint32_t peerId, uint16_t k, uint16_t d, uint16_t a, uint32_t dmg, uint16_t hs);

} // namespace CoopNet

#pragma once
#include <cstdint>
namespace CoopNet
{
uint16_t FilterDamage(uint32_t sourcePeer, uint32_t targetPeer, bool targetIsNpc, uint16_t rawDmg, uint16_t targetArmor,
                      bool invulnerable);
}

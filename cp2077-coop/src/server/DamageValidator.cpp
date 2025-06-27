#include "PerkController.hpp"
#include "ServerConfig.hpp"
#include <iostream>

namespace CoopNet
{
uint16_t FilterDamage(uint32_t sourcePeer, uint32_t targetPeer, bool targetIsNpc, uint16_t rawDmg, uint16_t targetArmor,
                      bool invulnerable)
{
    if (invulnerable)
        return 0;
    if (!g_cfgFriendlyFire && (sourcePeer == targetPeer || !targetIsNpc))
        return 0;

    float mult = PerkController_GetHealthMult(targetPeer);
    if (PerkController_HasRelic(sourcePeer, 1000)) // Data Tunneling
        rawDmg = static_cast<uint16_t>(static_cast<float>(rawDmg) * 1.1f);
    uint16_t maxAllowed = static_cast<uint16_t>((targetArmor * 4 + 200) * mult);
    if (rawDmg > maxAllowed)
    {
        std::cout << "cheat detected" << std::endl;
        return maxAllowed;
    }
    return rawDmg;
}
} // namespace CoopNet

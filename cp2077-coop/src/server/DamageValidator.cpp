#include <iostream>

namespace CoopNet
{
bool ValidateDamage(uint16_t rawDmg, uint16_t targetArmor)
{
    uint16_t maxAllowed = targetArmor * 4 + 200;
    if (rawDmg > maxAllowed)
    {
        std::cout << "cheat detected" << std::endl;
        return false;
    }
    return true;
}
} // namespace CoopNet

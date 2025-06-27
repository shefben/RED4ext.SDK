#include "SkillController.hpp"
#include "../net/Net.hpp"
#include "Connection.hpp"
#include <iostream>
#include <unordered_map>

namespace CoopNet
{
static std::unordered_map<uint32_t, std::unordered_map<uint16_t, int32_t>> g_skillTable;

void SkillController_HandleXP(Connection* conn, uint16_t skillId, int16_t delta)
{
    if (!conn)
        return;
    if (delta > 500)
        delta = 500;
    else if (delta < -500)
        delta = -500;
    int32_t& xp = g_skillTable[conn->peerId][skillId];
    xp += delta;
    Net_BroadcastSkillXP(conn->peerId, skillId, delta);
    std::cout << "SkillXP peer=" << conn->peerId << " skill=" << skillId << " delta=" << delta << std::endl;
}

int32_t SkillController_GetXP(uint32_t peerId, uint16_t skillId)
{
    auto pit = g_skillTable.find(peerId);
    if (pit != g_skillTable.end())
    {
        auto sit = pit->second.find(skillId);
        if (sit != pit->second.end())
            return sit->second;
    }
    return 0;
}

} // namespace CoopNet

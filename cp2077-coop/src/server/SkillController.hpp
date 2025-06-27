#pragma once
#include <cstdint>
namespace CoopNet
{
class Connection;
void SkillController_HandleXP(Connection* conn, uint16_t skillId, int16_t delta);
int32_t SkillController_GetXP(uint32_t peerId, uint16_t skillId);
} // namespace CoopNet

#ifndef COOP_QUEST_WATCHDOG_HPP
#define COOP_QUEST_WATCHDOG_HPP

#include "../net/Packets.hpp"
#include <cstdint>

namespace CoopNet
{

void QuestWatchdog_Record(uint32_t phaseId, uint32_t questHash, uint16_t stage); // PX-2
void QuestWatchdog_Tick(float dt);
void QuestWatchdog_BuildFullSync(uint32_t phaseId, QuestFullSyncPacket& outPkt); // PX-2

} // namespace CoopNet

#endif // COOP_QUEST_WATCHDOG_HPP

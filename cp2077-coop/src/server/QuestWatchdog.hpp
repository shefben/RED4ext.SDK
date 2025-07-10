#ifndef COOP_QUEST_WATCHDOG_HPP
#define COOP_QUEST_WATCHDOG_HPP

#include "../net/Packets.hpp"
#include <cstdint>
#include <vector>

namespace CoopNet
{

void QuestWatchdog_Record(uint32_t phaseId, uint32_t questHash, uint16_t stage); // PX-2
void QuestWatchdog_Tick(float dt);
void QuestWatchdog_BuildFullSync(uint32_t phaseId, QuestFullSyncPacket& outPkt); // PX-2
uint16_t QuestWatchdog_GetStage(uint32_t phaseId, uint32_t questHash);
std::vector<uint32_t> QuestWatchdog_ListPhases();               // PX-7
void QuestWatchdog_LoadCritical();                              // PX-6
void QuestWatchdog_LoadRomance();                               // RM-1
void QuestWatchdog_LoadMain();                                  // QW-1
void QuestWatchdog_LoadSide();                                  // QW-1
void QuestWatchdog_HandleVote(uint32_t peerId, bool yes);       // PX-6
void QuestWatchdog_HandleEndingVote(uint32_t peerId, bool yes); // EG-1

} // namespace CoopNet

#endif // COOP_QUEST_WATCHDOG_HPP

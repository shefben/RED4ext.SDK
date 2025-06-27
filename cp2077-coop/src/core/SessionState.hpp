#pragma once

#include <cstdint>
#include <vector>

namespace CoopNet
{

struct ItemSnap
{
    uint32_t itemId;
    uint16_t quantity;
};

void SaveSessionState(uint32_t sessionId);
void SaveMergeResolution(bool acceptAll);
// Returns derived session id from sorted peer list
uint32_t SessionState_SetParty(const std::vector<uint32_t>& peerIds);
uint32_t SessionState_GetId();
void SessionState_SetPerk(uint32_t peerId, uint32_t perkId, uint8_t rank);
void SessionState_ClearPerks(uint32_t peerId);
float SessionState_GetPerkHealthMult(uint32_t peerId);

} // namespace CoopNet

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
void SessionState_SetParty(const std::vector<uint32_t>& peerIds);

} // namespace CoopNet

#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>

namespace CoopNet
{

struct ItemSnap
{
    uint32_t itemId;
    uint16_t quantity;
};

struct WorldStateSnap
{
    uint16_t sunDeg;
    uint8_t weatherId;
    uint16_t particleSeed;
};

struct EventState
{
    uint32_t eventId;
    uint8_t phase;
    bool active;
    uint32_t seed;
};

struct ReputationEntry
{
    uint32_t npcId;
    int16_t value;
};

void SaveSessionState(uint32_t sessionId);
bool LoadSessionState(uint32_t sessionId);
void SaveMergeResolution(bool acceptAll);
// Returns derived session id from sorted peer list
uint32_t SessionState_SetParty(const std::vector<uint32_t>& peerIds);
uint32_t SessionState_GetId();
void SessionState_SetPerk(uint32_t peerId, uint32_t perkId, uint8_t rank);
void SessionState_ClearPerks(uint32_t peerId);
float SessionState_GetPerkHealthMult(uint32_t peerId);
const WorldStateSnap& SessionState_GetWorld();
const std::vector<EventState>& SessionState_GetEvents();
const std::unordered_map<uint32_t, int16_t>& SessionState_GetReputation();

// Returns number of active party members
uint32_t SessionState_GetActivePlayerCount();

void SessionState_UpdateWeather(uint16_t sunDeg, uint8_t weatherId, uint16_t seed);
void SessionState_RecordEvent(uint32_t eventId, uint8_t phase, bool active, uint32_t seed);
void SessionState_SetReputation(uint32_t npcId, int16_t value);

} // namespace CoopNet

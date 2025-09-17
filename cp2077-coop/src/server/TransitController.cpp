#include "TransitController.hpp"
#include "../net/Net.hpp"
#include <mutex>

namespace CoopNet
{
static std::mutex g_transitMutex;
void TransitController_Board(uint32_t peerId, uint32_t lineId, uint8_t carIdx)
{
    std::lock_guard lock(g_transitMutex);
    Net_BroadcastMetroBoard(peerId, lineId, carIdx);
}

void TransitController_Arrive(uint32_t peerId, uint32_t stationId)
{
    std::lock_guard lock(g_transitMutex);
    Net_BroadcastMetroArrive(peerId, stationId);
}

} // namespace CoopNet

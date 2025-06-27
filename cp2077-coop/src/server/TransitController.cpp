#include "TransitController.hpp"
#include "../net/Net.hpp"

namespace CoopNet
{

void TransitController_Board(uint32_t peerId, uint32_t lineId, uint8_t carIdx)
{
    Net_BroadcastMetroBoard(peerId, lineId, carIdx);
}

void TransitController_Arrive(uint32_t peerId, uint32_t stationId)
{
    Net_BroadcastMetroArrive(peerId, stationId);
}

} // namespace CoopNet

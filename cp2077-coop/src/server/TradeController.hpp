#pragma once
#include "../net/Connection.hpp"
#include "../net/Packets.hpp"

namespace CoopNet
{
void TradeController_Start(uint32_t fromId, uint32_t toId);
void TradeController_HandleOffer(Connection* conn, const TradeOfferPacket& pkt);
void TradeController_HandleAccept(Connection* conn, uint32_t peerId, bool accept);
} // namespace CoopNet

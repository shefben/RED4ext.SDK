#pragma once
#include "../net/Connection.hpp"
#include "../net/Packets.hpp"

namespace CoopNet
{
void CyberController_Equip(Connection* conn, uint8_t slotId, const ItemSnap& snap);
}

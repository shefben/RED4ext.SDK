#pragma once
#include "../net/Connection.hpp"
#include "../net/Packets.hpp"

namespace CoopNet
{
void QuestGadget_HandleFire(Connection* conn, const QuestGadgetFirePacket& pkt);
}

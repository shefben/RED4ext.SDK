#ifndef COOP_STATUS_CONTROLLER_HPP
#define COOP_STATUS_CONTROLLER_HPP

#include "../net/Connection.hpp"
#include "../net/Packets.hpp"
#include <cstdint>

namespace CoopNet
{

void StatusController_OnApply(Connection* src, const StatusApplyPacket& pkt);
void StatusController_Tick(float dt);

} // namespace CoopNet

#endif // COOP_STATUS_CONTROLLER_HPP

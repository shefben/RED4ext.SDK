#ifndef COOP_TRAFFIC_CONTROLLER_HPP
#define COOP_TRAFFIC_CONTROLLER_HPP

#include <cstdint>

namespace CoopNet
{
void TrafficController_Tick(float dtMs);
void TrafficController_OnDespawn(uint32_t vehId);
} // namespace CoopNet

#endif // COOP_TRAFFIC_CONTROLLER_HPP

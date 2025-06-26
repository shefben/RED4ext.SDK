#include "GlobalEventController.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include <iostream>

namespace CoopNet {

void GlobalEvent_Start(uint32_t eventId, uint8_t phase, uint32_t seed)
{
    GlobalEventPacket pkt{eventId, seed, phase, 1u, {0,0}};
    Net_Broadcast(EMsg::GlobalEvent, &pkt, sizeof(pkt));
    std::cout << "Global event start " << eventId << std::endl;
}

void GlobalEvent_Stop(uint32_t eventId, uint8_t phase)
{
    GlobalEventPacket pkt{eventId, 0u, phase, 0u, {0,0}};
    Net_Broadcast(EMsg::GlobalEvent, &pkt, sizeof(pkt));
    std::cout << "Global event stop " << eventId << std::endl;
}

} // namespace CoopNet

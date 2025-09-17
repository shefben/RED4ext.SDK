#include "GlobalEventController.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include "../core/SessionState.hpp"
#include <iostream>
#include <mutex>

namespace CoopNet {
static std::mutex g_eventMutex;

void GlobalEvent_Start(uint32_t eventId, uint8_t phase, uint32_t seed)
{
    std::lock_guard lock(g_eventMutex);
    GlobalEventPacket pkt{eventId, seed, phase, 1u, {0,0}};
    Net_Broadcast(EMsg::GlobalEvent, &pkt, sizeof(pkt));
    SessionState_RecordEvent(eventId, phase, true, seed);
    std::cout << "Global event start " << eventId << std::endl;
}

void GlobalEvent_Stop(uint32_t eventId, uint8_t phase)
{
    std::lock_guard lock(g_eventMutex);
    GlobalEventPacket pkt{eventId, 0u, phase, 0u, {0,0}};
    Net_Broadcast(EMsg::GlobalEvent, &pkt, sizeof(pkt));
    SessionState_RecordEvent(eventId, phase, false, 0u);
    std::cout << "Global event stop " << eventId << std::endl;
}

} // namespace CoopNet

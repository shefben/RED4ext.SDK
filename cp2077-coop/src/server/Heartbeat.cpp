#include <iostream>
#include <cstdint>

// Sends JSON heartbeat to master server every 30 seconds.
void SendHeartbeat(uint64_t sessionId)
{
    std::cout << "Heartbeat {\"id\":" << sessionId << ",\"ts\":...}" << std::endl;
    // Entry should be removed after 90 s of inactivity.
}

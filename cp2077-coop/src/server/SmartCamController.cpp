#include "SmartCamController.hpp"
#include "../net/Net.hpp"

namespace CoopNet
{
void SmartCam_Start(uint32_t projId)
{
    SmartCamStartPacket pkt{projId};
    Net_Broadcast(EMsg::SmartCamStart, &pkt, sizeof(pkt));
}

void SmartCam_End(uint32_t projId)
{
    SmartCamEndPacket pkt{projId};
    Net_Broadcast(EMsg::SmartCamEnd, &pkt, sizeof(pkt));
}
} // namespace CoopNet

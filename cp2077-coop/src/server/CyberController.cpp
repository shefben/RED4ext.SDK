#include "CyberController.hpp"
#include "../net/Net.hpp"
#include "../core/Red4extUtils.hpp"
#include <RED4ext/RED4ext.hpp>
#include <iostream>

namespace CoopNet
{

static bool CheckPrereqs(uint8_t slotId)
{
    uint32_t cred = 0;
    RED4EXT_EXECUTE("PlayerProgression", "GetStreetCredLevel", nullptr, &cred);
    if (cred < 10)
        return false;
    uint32_t capacity = 0;
    RED4EXT_EXECUTE("StatsSystem", "GetCyberCapacity", nullptr, &capacity);
    if (capacity < slotId)
        return false;
    return true;
}

void CyberController_Equip(Connection* conn, uint8_t slotId, const ItemSnap& snap)
{
    if (!CheckPrereqs(slotId))
    {
        std::cout << "Equip blocked: prereqs failed" << std::endl;
        return;
    }
    CyberEquipPacket pkt{};
    pkt.peerId = conn->peerId;
    pkt.slotId = slotId;
    pkt.snap = snap;
    Net_Broadcast(EMsg::CyberEquip, &pkt, sizeof(pkt));
    Net_BroadcastAppearance(conn->peerId, snap.tpl, 0u);
}

} // namespace CoopNet

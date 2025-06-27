#include "QuestGadgetController.hpp"
#include "../net/Net.hpp"
#include "QuestWatchdog.hpp"
#include <unordered_set>

namespace CoopNet
{
static std::unordered_set<uint32_t> g_allowed;

void QuestGadget_HandleFire(Connection* conn, const QuestGadgetFirePacket& pkt)
{
    auto it = g_allowed.find(pkt.questId);
    if (it == g_allowed.end())
        return;
    uint16_t stage = QuestWatchdog_GetStage(conn->peerId, pkt.questId);
    if (stage == 0)
        return;
    Net_BroadcastQuestGadgetFire(pkt.questId, static_cast<QuestGadgetType>(pkt.gadgetType), pkt.charge, pkt.targetId);
}
} // namespace CoopNet

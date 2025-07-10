#include "TradeController.hpp"
#include "../net/Net.hpp"
#include "InventoryController.hpp"
#include "LedgerService.hpp"
#include <cstring>
#include <unordered_map>
#include <vector>

namespace CoopNet
{
struct TradeState
{
    uint32_t a;
    uint32_t b;
    std::vector<ItemSnap> offerA;
    std::vector<ItemSnap> offerB;
    uint32_t eddiesA = 0;
    uint32_t eddiesB = 0;
    bool acceptA = false;
    bool acceptB = false;
};
static TradeState g_trade;
static bool g_active = false;

void TradeController_Start(uint32_t fromId, uint32_t toId)
{
    g_trade = TradeState{};
    g_trade.a = fromId;
    g_trade.b = toId;
    g_active = true;
    TradeInitPacket pkt{fromId, toId};
    Connection* a = Net_FindConnection(fromId);
    Connection* b = Net_FindConnection(toId);
    if (a)
        Net_Send(a, EMsg::TradeInit, &pkt, sizeof(pkt));
    if (b)
        Net_Send(b, EMsg::TradeInit, &pkt, sizeof(pkt));
}

static bool ValidateItems(uint32_t peerId, const TradeOfferPacket& pkt)
{
    for (uint8_t i = 0; i < pkt.count && i < 8; ++i)
    {
        auto it = g_items.find(pkt.items[i].itemId);
        if (it == g_items.end() || it->second.ownerId != peerId)
            return false;
    }
    return true;
}

static bool ValidateStored()
{
    for (auto& item : g_trade.offerA)
    {
        auto it = g_items.find(item.itemId);
        if (it == g_items.end() || it->second.ownerId != g_trade.a)
            return false;
    }
    for (auto& item : g_trade.offerB)
    {
        auto it = g_items.find(item.itemId);
        if (it == g_items.end() || it->second.ownerId != g_trade.b)
            return false;
    }
    return true;
}

void TradeController_HandleOffer(Connection* conn, const TradeOfferPacket& pkt)
{
    if (!g_active || !conn)
        return;
    if (pkt.fromId != conn->peerId)
        return;
    if (!ValidateItems(conn->peerId, pkt))
        return;
    std::vector<ItemSnap>& offer = (pkt.fromId == g_trade.a) ? g_trade.offerA : g_trade.offerB;
    offer.clear();
    for (uint8_t i = 0; i < pkt.count && i < 8; ++i)
        offer.push_back(pkt.items[i]);
    if (pkt.fromId == g_trade.a)
        g_trade.eddiesA = pkt.eddies;
    else
        g_trade.eddiesB = pkt.eddies;
    g_trade.acceptA = g_trade.acceptB = false;
    TradeOfferPacket fwd = pkt;
    Connection* other = Net_FindConnection(pkt.fromId == g_trade.a ? g_trade.b : g_trade.a);
    if (other)
        Net_Send(other, EMsg::TradeOffer, &fwd, sizeof(fwd));
}

static void Finalize()
{
    if (!ValidateStored())
    {
        Net_BroadcastTradeFinalize(false);
        g_active = false;
        return;
    }
    for (auto& item : g_trade.offerA)
    {
        ItemSnap& snap = g_items[item.itemId];
        snap.ownerId = g_trade.b;
        ItemSnapPacket pkt{snap};
        Net_Broadcast(EMsg::ItemSnap, &pkt, sizeof(pkt));
    }
    for (auto& item : g_trade.offerB)
    {
        ItemSnap& snap = g_items[item.itemId];
        snap.ownerId = g_trade.a;
        ItemSnapPacket pkt{snap};
        Net_Broadcast(EMsg::ItemSnap, &pkt, sizeof(pkt));
    }
    uint64_t bal;
    Ledger_Transfer(Net_FindConnection(g_trade.a), -static_cast<int64_t>(g_trade.eddiesA), 0, bal);
    Ledger_Transfer(Net_FindConnection(g_trade.b), -static_cast<int64_t>(g_trade.eddiesB), 0, bal);
    Ledger_Transfer(Net_FindConnection(g_trade.a), g_trade.eddiesB, 1, bal);
    Ledger_Transfer(Net_FindConnection(g_trade.b), g_trade.eddiesA, 1, bal);
    Net_BroadcastTradeFinalize(true);
    g_active = false;
}

void TradeController_HandleAccept(Connection* conn, uint32_t peerId, bool accept)
{
    if (!g_active || !conn)
        return;
    if (peerId == g_trade.a)
        g_trade.acceptA = accept;
    else if (peerId == g_trade.b)
        g_trade.acceptB = accept;
    TradeAcceptPacket pkt{peerId, static_cast<uint8_t>(accept), {0, 0, 0}};
    Connection* a = Net_FindConnection(g_trade.a);
    Connection* b = Net_FindConnection(g_trade.b);
    if (a)
        Net_Send(a, EMsg::TradeAccept, &pkt, sizeof(pkt));
    if (b && b != a)
        Net_Send(b, EMsg::TradeAccept, &pkt, sizeof(pkt));
    if (g_trade.acceptA && g_trade.acceptB)
        Finalize();
}

} // namespace CoopNet

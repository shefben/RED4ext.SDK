#include "VendorController.hpp"
#include "../core/GameClock.hpp"
#include "../net/Net.hpp"
#include "InventoryController.hpp"
#include "Journal.hpp"
#include "LedgerService.hpp"
#include "../core/Red4extUtils.hpp"
#include <algorithm>
#include <cstring>
#include <RED4ext/RED4ext.hpp>
#include <unordered_map>
#include <vector>

namespace CoopNet
{

struct VendorItem
{
    uint32_t price;
    uint16_t qty;
};
// vendorId -> phaseId -> itemId
static std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::unordered_map<uint32_t, VendorItem>>>
    g_stock; // PX-8
static std::unordered_map<uint32_t, uint64_t> g_lastDay;
void VendorController_Tick(float dt, uint64_t worldClock)
{
    uint64_t day = worldClock / 36000;
    uint32_t tod = static_cast<uint32_t>(worldClock % 36000);
    if (tod < 6000)
        return;
    for (auto& kv : g_stock)
    {
        if (g_lastDay[kv.first] >= day)
            continue;
        auto& phases = kv.second;
        for (auto& ph : phases)
        {
            ph.second.clear();
            VendorRefreshPacket ref{kv.first, ph.first};
            Net_BroadcastVendorRefresh(ref);
        }
        g_lastDay[kv.first] = day;
    }
}

static uint32_t CalculatePrice(uint32_t basePrice, Connection* conn)
{
    uint32_t cred = 0;
    RED4EXT_EXECUTE("PlayerProgression", "GetStreetCredLevel", nullptr, &cred);
    bool hasPerk = false;
    RED4ext::CName perk = "Wholesale";
    RED4EXT_EXECUTE("PerkSystem", "HasPerk", nullptr, &perk, &hasPerk);
    int32_t price = static_cast<int32_t>(basePrice);
    if (cred > 0 && cred <= 50)
        price = (price * static_cast<int32_t>(100 - cred)) / 100;
    if (hasPerk)
        price = (price * 90) / 100;
    if (price < 1)
        price = 1;
    return static_cast<uint32_t>(price);
}

void VendorController_HandlePurchase(Connection* conn, uint32_t vendorId, uint32_t itemId, uint64_t nonce)
{
    uint32_t phaseId = conn ? conn->peerId : 0u;
    auto vit = g_stock.find(vendorId);
    if (vit == g_stock.end())
        return;
    auto& phaseMap = vit->second[phaseId];
    auto itemIt = phaseMap.find(itemId);
    if (itemIt == phaseMap.end() || itemIt->second.qty == 0)
        return;
    uint64_t balance;
    uint32_t price = CalculatePrice(itemIt->second.price, conn);
    if (!Ledger_Transfer(conn, -static_cast<int64_t>(price), nonce, balance))
    {
        PurchaseResultPacket res{vendorId, itemId, conn->balance, 0, {0, 0, 0}};
        Net_Send(conn, EMsg::PurchaseResult, &res, sizeof(res));
        return;
    }
    ItemSnap snap = Inventory_CreateItem(static_cast<uint16_t>(itemIt->first), conn->peerId);
    ItemSnapPacket pkt{snap};
    Net_Send(conn, EMsg::ItemSnap, &pkt, sizeof(pkt));
    PurchaseResultPacket res{vendorId, itemId, balance, 1, {0, 0, 0}};
    Journal_Log(GameClock::GetCurrentTick(), conn->peerId, "purchase", itemId, -static_cast<int32_t>(price));
    Net_Send(conn, EMsg::PurchaseResult, &res, sizeof(res));
    if (itemIt->second.qty > 0)
        itemIt->second.qty -= 1;
    VendorStockUpdatePacket upd{vendorId, phaseId, itemId, itemIt->second.qty, 0};
    Net_BroadcastVendorStockUpdate(upd);
}

} // namespace CoopNet

#include "VendorController.hpp"
#include "../net/Net.hpp"
#include "InventoryController.hpp"
#include "LedgerService.hpp"
#include <algorithm>
#include <cstring>
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
static std::unordered_map<uint32_t, float> g_timer;

void VendorController_Tick(float dt)
{
    for (auto& kv : g_timer)
    {
        kv.second += dt;
        if (kv.second >= 3600.f)
        {
            kv.second = 0.f;
            auto& phases = g_stock[kv.first];
            for (auto& ph : phases)
            {
                ph.second.clear();
                VendorItem item{1000, 5};
                ph.second[kv.first * 10 + 1] = item;
                VendorStockPacket pkt{};
                pkt.vendorId = kv.first;
                pkt.phaseId = ph.first;
                pkt.count = 0;
                for (auto& it : ph.second)
                {
                    if (pkt.count >= 8)
                        break;
                    pkt.items[pkt.count].itemId = it.first;
                    pkt.items[pkt.count].price = it.second.price;
                    pkt.items[pkt.count].qty = it.second.qty;
                    ++pkt.count;
                }
                Net_BroadcastVendorStock(pkt);
            }
        }
    }
}

static uint32_t CalculatePrice(uint32_t basePrice, Connection* conn)
{
    uint32_t cred = 0;
    RED4ext::ExecuteFunction("PlayerProgression", "GetStreetCredLevel", nullptr, &cred);
    bool hasPerk = false;
    RED4ext::CName perk = "Wholesale";
    RED4ext::ExecuteFunction("PerkSystem", "HasPerk", nullptr, &perk, &hasPerk);
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
    Net_Send(conn, EMsg::PurchaseResult, &res, sizeof(res));
    if (itemIt->second.qty > 0)
        itemIt->second.qty -= 1;
    VendorStockUpdatePacket upd{vendorId, phaseId, itemId, itemIt->second.qty, 0};
    Net_BroadcastVendorStockUpdate(upd);
}

} // namespace CoopNet

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
    uint32_t id;
    uint32_t price;
};
static std::unordered_map<uint32_t, std::vector<VendorItem>> g_stock;
static std::unordered_map<uint32_t, float> g_timer;

void VendorController_Tick(float dt)
{
    for (auto& kv : g_timer)
    {
        kv.second += dt;
        if (kv.second >= 1800.f)
        {
            kv.second = 0.f;
            auto& list = g_stock[kv.first];
            list.clear();
            VendorItem item{kv.first * 10 + 1, 1000};
            list.push_back(item);
            VendorStockPacket pkt{};
            pkt.vendorId = kv.first;
            pkt.count = static_cast<uint8_t>(list.size());
            for (size_t i = 0; i < list.size() && i < 8; ++i)
            {
                pkt.items[i].itemId = list[i].id;
                pkt.items[i].price = list[i].price;
            }
            Net_BroadcastVendorStock(pkt);
        }
    }
}

static uint32_t CalculatePrice(const VendorItem& item, Connection* conn)
{
    // FIXME(next ticket): include street cred and perks
    (void)conn;
    return item.price;
}

void VendorController_HandlePurchase(Connection* conn, uint32_t vendorId, uint32_t itemId, uint64_t nonce)
{
    auto it = g_stock.find(vendorId);
    if (it == g_stock.end())
        return;
    auto& list = it->second;
    auto itemIt = std::find_if(list.begin(), list.end(), [&](const VendorItem& v) { return v.id == itemId; });
    if (itemIt == list.end())
        return;
    uint64_t balance;
    uint32_t price = CalculatePrice(*itemIt, conn);
    if (!Ledger_Transfer(conn, -static_cast<int64_t>(price), nonce, balance))
    {
        PurchaseResultPacket res{vendorId, itemId, conn->balance, 0, {0, 0, 0}};
        Net_Send(conn, EMsg::PurchaseResult, &res, sizeof(res));
        return;
    }
    ItemSnap snap = Inventory_CreateItem(static_cast<uint16_t>(itemIt->id), conn->peerId);
    ItemSnapPacket pkt{snap};
    Net_Send(conn, EMsg::ItemSnap, &pkt, sizeof(pkt));
    PurchaseResultPacket res{vendorId, itemId, balance, 1, {0, 0, 0}};
    Net_Send(conn, EMsg::PurchaseResult, &res, sizeof(res));
    list.erase(itemIt);
    VendorStockPacket stock{};
    stock.vendorId = vendorId;
    stock.count = static_cast<uint8_t>(list.size());
    for (size_t i = 0; i < list.size() && i < 8; ++i)
    {
        stock.items[i].itemId = list[i].id;
        stock.items[i].price = list[i].price;
    }
    Net_BroadcastVendorStock(stock);
}

} // namespace CoopNet

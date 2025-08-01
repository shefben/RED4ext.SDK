#include "InventoryController.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include <cstring>
#include <iostream>
#include <random>
#include <unordered_map>

namespace CoopNet
{

static uint64_t g_nextItemId = 1;
static std::unordered_map<uint64_t, ItemSnap> g_items;

static bool ValidateMaterials(uint32_t recipe)
{
    return recipe > 1000 && recipe < 100000;
}

static ItemSnap CraftItem(uint32_t recipe)
{
    ItemSnap snap{};
    snap.itemId = g_nextItemId++;
    snap.ownerId = 0;
    snap.tpl = static_cast<uint16_t>(recipe);
    snap.level = 1;
    snap.quality = 1;
    std::memset(snap.rolls, 0, sizeof(snap.rolls));
    snap.slotMask = 0;
    std::memset(snap.attachmentIds, 0, sizeof(snap.attachmentIds));
    g_items[snap.itemId] = snap;
    return snap;
}

ItemSnap Inventory_CreateItem(uint16_t tpl, uint32_t ownerId)
{
    ItemSnap snap{};
    snap.itemId = g_nextItemId++;
    snap.ownerId = ownerId;
    snap.tpl = tpl;
    snap.level = 1;
    snap.quality = 1;
    std::memset(snap.rolls, 0, sizeof(snap.rolls));
    snap.slotMask = 0;
    std::memset(snap.attachmentIds, 0, sizeof(snap.attachmentIds));
    g_items[snap.itemId] = snap;
    return snap;
}

static bool AttachMod(uint64_t itemId, uint8_t slot, uint64_t attachId, ItemSnap& out)
{
    auto it = g_items.find(itemId);
    if (it == g_items.end() || slot >= 4)
        return false;
    ItemSnap& item = it->second;
    if (item.slotMask & (1u << slot))
        return false;
    item.slotMask |= (1u << slot);
    item.attachmentIds[slot] = attachId;
    out = item;
    return true;
}

void Inventory_HandleCraftRequest(Connection* conn, uint32_t recipeId)
{
    if (!ValidateMaterials(recipeId))
        return;
    ItemSnap snap = CraftItem(recipeId);
    CraftResultPacket pkt{snap};
    Net_Send(conn, EMsg::CraftResult, &pkt, sizeof(pkt));
    ItemSnapPacket snapPkt{snap};
    Net_Broadcast(EMsg::ItemSnap, &snapPkt, sizeof(snapPkt));
    std::cout << "CraftRequest recipe=" << recipeId << " -> item " << snap.itemId << std::endl;
}

void Inventory_HandleAttachRequest(Connection* conn, uint64_t itemId, uint8_t slotIdx, uint64_t attachmentId)
{
    ItemSnap updated{};
    bool ok = AttachMod(itemId, slotIdx, attachmentId, updated);
    AttachModResultPacket pkt{updated, static_cast<uint8_t>(ok), {0, 0, 0}};
    Net_Send(conn, EMsg::AttachModResult, &pkt, sizeof(pkt));
    if (ok)
    {
        ItemSnapPacket snapPkt{updated};
        Net_Broadcast(EMsg::ItemSnap, &snapPkt, sizeof(snapPkt));
    }
    std::cout << "AttachRequest item=" << itemId << " slot=" << static_cast<int>(slotIdx) << " ok=" << ok << std::endl;
}

void Inventory_HandleReRollRequest(Connection* conn, uint64_t itemId, uint32_t seed)
{
    if (!conn)
        return;
    auto it = g_items.find(itemId);
    if (it == g_items.end() || !ValidateMaterials(it->second.tpl))
        return;
    std::mt19937 rng(seed);
    for (int i = 0; i < 4; ++i)
        it->second.rolls[i] = rng();
    ReRollResultPacket res{it->second};
    Net_Send(conn, EMsg::ReRollResult, &res, sizeof(res));
    ItemSnapPacket snapPkt{it->second};
    Net_Broadcast(EMsg::ItemSnap, &snapPkt, sizeof(snapPkt));
    std::cout << "ReRollRequest item=" << itemId << " seed=" << seed << std::endl;
}

} // namespace CoopNet

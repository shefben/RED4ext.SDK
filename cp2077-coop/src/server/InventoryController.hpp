#pragma once
#include "../net/Connection.hpp"
#include "../net/Snapshot.hpp"

namespace CoopNet
{

void Inventory_HandleCraftRequest(Connection* conn, uint32_t recipeId);
void Inventory_HandleAttachRequest(Connection* conn, uint64_t itemId, uint8_t slotIdx, uint64_t attachmentId);
void Inventory_HandleReRollRequest(Connection* conn, uint64_t itemId, uint32_t seed); // WM-1
ItemSnap Inventory_CreateItem(uint16_t tpl, uint32_t ownerId);

} // namespace CoopNet

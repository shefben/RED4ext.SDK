#pragma once
#include "../net/Snapshot.hpp"
#include "../net/Connection.hpp"

namespace CoopNet {

void Inventory_HandleCraftRequest(Connection* conn, uint32_t recipeId);
void Inventory_HandleAttachRequest(Connection* conn, uint64_t itemId, uint8_t slotIdx, uint64_t attachmentId);

}

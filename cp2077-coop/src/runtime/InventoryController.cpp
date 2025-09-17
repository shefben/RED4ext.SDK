#include "InventoryController.hpp"
#include "../core/Logger.hpp"
#include "../net/Snapshot.hpp"
#include <chrono>
#include <algorithm>
#include <cmath>

namespace CoopNet {

InventoryController& InventoryController::Instance() {
    static InventoryController instance;
    return instance;
}

bool InventoryController::UpdatePlayerInventory(const PlayerInventorySnap& snap) {
    if (!ValidatePlayerInventory(snap)) {
        Logger::Log(static_cast<LogLevel>(3), "Invalid inventory snapshot for peer " + std::to_string(snap.peerId));
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_inventoryMutex);
    
    auto it = m_playerInventories.find(snap.peerId);
    if (it != m_playerInventories.end()) {
        // Check version for conflict resolution
        if (snap.version <= it->second->version) {
            Logger::Log(static_cast<LogLevel>(2), "Received outdated inventory update for peer " + std::to_string(snap.peerId) + " (version " + std::to_string(snap.version) + " <= " + std::to_string(it->second->version) + ")");
            return false;
        }
        
        // Update existing inventory
        *it->second = snap;
        it->second->lastUpdate = GetCurrentTimestamp();
    } else {
        // Create new inventory entry
        auto newInventory = std::make_unique<PlayerInventorySnap>(snap);
        newInventory->lastUpdate = GetCurrentTimestamp();
        m_playerInventories[snap.peerId] = std::move(newInventory);
    }
    
    Logger::Log(static_cast<LogLevel>(1), "Updated inventory for peer " + std::to_string(snap.peerId) + " (" + std::to_string(snap.items.size()) + " items, version " + std::to_string(snap.version) + ")");
    return true;
}

PlayerInventorySnap* InventoryController::GetPlayerInventory(uint32_t peerId) {
    if (!ValidatePlayerId(peerId)) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(m_inventoryMutex);
    auto it = m_playerInventories.find(peerId);
    return (it != m_playerInventories.end()) ? it->second.get() : nullptr;
}

bool InventoryController::RemovePlayerInventory(uint32_t peerId) {
    std::lock_guard<std::mutex> lock(m_inventoryMutex);
    auto removed = m_playerInventories.erase(peerId);
    if (removed > 0) {
        Logger::Log(static_cast<LogLevel>(1), "Removed inventory for peer " + std::to_string(peerId));
    }
    return removed > 0;
}

uint32_t InventoryController::RequestItemTransfer(uint32_t fromPeerId, uint32_t toPeerId, 
                                                 uint64_t itemId, uint32_t quantity) {
    if (!ValidatePlayerId(fromPeerId) || !ValidatePlayerId(toPeerId) || 
        !ValidateItemId(itemId) || !ValidateQuantity(quantity)) {
        Logger::Log(static_cast<LogLevel>(3), "Invalid transfer request parameters");
        return 0;
    }
    
    if (fromPeerId == toPeerId) {
        Logger::Log(static_cast<LogLevel>(3), "Cannot transfer items to same player");
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(m_transferMutex);
    
    if (m_pendingTransfers.size() >= MAX_PENDING_TRANSFERS) {
        Logger::Log(static_cast<LogLevel>(3), "Too many pending transfers, rejecting request");
        return 0;
    }
    
    auto request = std::make_unique<ItemTransferRequest>();
    request->fromPeerId = fromPeerId;
    request->toPeerId = toPeerId;
    request->itemId = itemId;
    request->quantity = quantity;
    request->requestId = GenerateRequestId();
    request->timestamp = GetCurrentTimestamp();
    request->validated = false;
    
    uint32_t requestId = request->requestId;
    m_pendingTransfers[requestId] = std::move(request);
    
    Logger::Log(static_cast<LogLevel>(1), "Created item transfer request " + std::to_string(requestId) + " (item " + std::to_string(itemId) + " from " + std::to_string(fromPeerId) + " to " + std::to_string(toPeerId) + ")");
    
    // Validate the transfer asynchronously
    if (!ValidateItemTransfer(*m_pendingTransfers[requestId])) {
        m_pendingTransfers.erase(requestId);
        Logger::Log(static_cast<LogLevel>(2), "Transfer request " + std::to_string(requestId) + " failed validation");
        return 0;
    }
    
    m_pendingTransfers[requestId]->validated = true;
    return requestId;
}

bool InventoryController::ProcessTransferRequest(uint32_t requestId, bool approve, const std::string& reason) {
    std::lock_guard<std::mutex> transferLock(m_transferMutex);
    
    auto it = m_pendingTransfers.find(requestId);
    if (it == m_pendingTransfers.end()) {
        Logger::Log(static_cast<LogLevel>(2), "Transfer request " + std::to_string(requestId) + " not found");
        return false;
    }
    
    auto request = std::move(it->second);
    m_pendingTransfers.erase(it);
    
    if (!approve) {
        Logger::Log(static_cast<LogLevel>(1), "Transfer request " + std::to_string(requestId) + " denied: " + reason);
        return true;
    }
    
    if (!request->validated) {
        Logger::Log(static_cast<LogLevel>(3), "Attempting to approve unvalidated transfer request " + std::to_string(requestId));
        return false;
    }
    
    // Apply the transfer (this would integrate with game systems)
    std::lock_guard<std::mutex> inventoryLock(m_inventoryMutex);
    
    auto fromIt = m_playerInventories.find(request->fromPeerId);
    auto toIt = m_playerInventories.find(request->toPeerId);
    
    if (fromIt == m_playerInventories.end() || toIt == m_playerInventories.end()) {
        Logger::Log(static_cast<LogLevel>(3), "Player inventories not found for transfer " + std::to_string(requestId));
        return false;
    }
    
    // Find and remove item from sender
    auto& fromItems = fromIt->second->items;
    auto itemIt = std::find_if(fromItems.begin(), fromItems.end(),
        [&](const InventoryItemSnap& item) { return item.itemId == request->itemId; });
    
    if (itemIt == fromItems.end() || itemIt->quantity < request->quantity) {
        Logger::Log(static_cast<LogLevel>(3), "Sender doesn't have sufficient items for transfer " + std::to_string(requestId));
        return false;
    }
    
    // Update or remove item from sender
    if (itemIt->quantity == request->quantity) {
        fromItems.erase(itemIt);
    } else {
        itemIt->quantity -= request->quantity;
    }
    
    // Add item to receiver
    auto& toItems = toIt->second->items;
    auto receiverItemIt = std::find_if(toItems.begin(), toItems.end(),
        [&](const InventoryItemSnap& item) { return item.itemId == request->itemId; });
    
    if (receiverItemIt != toItems.end()) {
        // Stack with existing item
        receiverItemIt->quantity += request->quantity;
    } else {
        // Add new item
        InventoryItemSnap newItem;
        newItem.itemId = request->itemId;
        newItem.quantity = request->quantity;
        newItem.durability = itemIt != fromItems.end() ? itemIt->durability : 100;
        newItem.modData = itemIt != fromItems.end() ? itemIt->modData : std::vector<uint8_t>();
        toItems.push_back(newItem);
    }
    
    // Update inventory versions
    fromIt->second->version++;
    toIt->second->version++;
    fromIt->second->lastUpdate = GetCurrentTimestamp();
    toIt->second->lastUpdate = GetCurrentTimestamp();
    
    Logger::Log(static_cast<LogLevel>(1), "Transfer request " + std::to_string(requestId) + " completed successfully");
    return true;
}

bool InventoryController::CancelTransferRequest(uint32_t requestId) {
    std::lock_guard<std::mutex> lock(m_transferMutex);
    auto removed = m_pendingTransfers.erase(requestId);
    if (removed > 0) {
        Logger::Log(static_cast<LogLevel>(1), "Cancelled transfer request " + std::to_string(requestId));
    }
    return removed > 0;
}

bool InventoryController::RegisterWorldItemPickup(uint64_t itemId, const float worldPos[3], uint32_t playerId) {
    if (!ValidateItemId(itemId) || !ValidatePlayerId(playerId) || !worldPos) {
        Logger::Log(static_cast<LogLevel>(3), "Invalid world item pickup parameters");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_worldItemMutex);
    
    if (m_worldItems.find(itemId) != m_worldItems.end()) {
        Logger::Log(static_cast<LogLevel>(2), "World item " + std::to_string(itemId) + " already picked up");
        return false;
    }
    
    if (m_worldItems.size() >= MAX_WORLD_ITEMS) {
        Logger::Log(static_cast<LogLevel>(2), "Too many world items tracked, clearing old ones");
        ClearExpiredPickups(60000); // Clear items older than 1 minute
    }
    
    auto pickup = std::make_unique<ItemPickupEvent>();
    pickup->itemId = itemId;
    pickup->worldPosition[0] = worldPos[0];
    pickup->worldPosition[1] = worldPos[1];
    pickup->worldPosition[2] = worldPos[2];
    pickup->playerId = playerId;
    pickup->timestamp = static_cast<uint32_t>(GetCurrentTimestamp() & 0xFFFFFFFF);
    pickup->processed = false;
    
    m_worldItems[itemId] = std::move(pickup);
    
    Logger::Log(static_cast<LogLevel>(1), "Registered world item pickup " + std::to_string(itemId) + " by player " + std::to_string(playerId) + " at (" + std::to_string(worldPos[0]) + ", " + std::to_string(worldPos[1]) + ", " + std::to_string(worldPos[2]) + ")");
    return true;
}

bool InventoryController::IsWorldItemTaken(uint64_t itemId) {
    std::lock_guard<std::mutex> lock(m_worldItemMutex);
    return m_worldItems.find(itemId) != m_worldItems.end();
}

void InventoryController::ClearExpiredPickups(uint32_t maxAgeMs) {
    std::lock_guard<std::mutex> lock(m_worldItemMutex);
    
    uint32_t currentTime = static_cast<uint32_t>(GetCurrentTimestamp() & 0xFFFFFFFF);
    uint32_t cutoffTime = currentTime - maxAgeMs;
    
    auto it = m_worldItems.begin();
    size_t removed = 0;
    while (it != m_worldItems.end()) {
        if (it->second->timestamp < cutoffTime) {
            it = m_worldItems.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
    
    if (removed > 0) {
        Logger::Log(static_cast<LogLevel>(1), "Cleared " + std::to_string(removed) + " expired world item pickups");
    }
}

bool InventoryController::ValidatePlayerInventory(const PlayerInventorySnap& snap) {
    if (!ValidatePlayerId(snap.peerId)) {
        return false;
    }
    
    if (snap.items.size() > MAX_INVENTORY_ITEMS) {
        Logger::Log(static_cast<LogLevel>(3), "Player " + std::to_string(snap.peerId) + " inventory has too many items: " + std::to_string(snap.items.size()));
        return false;
    }
    
    if (snap.money > MAX_MONEY) {
        Logger::Log(static_cast<LogLevel>(3), "Player " + std::to_string(snap.peerId) + " has invalid money amount: " + std::to_string(snap.money));
        return false;
    }
    
    for (const auto& item : snap.items) {
        if (!ValidateItemId(item.itemId) || !ValidateQuantity(item.quantity)) {
            return false;
        }
        
        if (item.durability > 100) {
            Logger::Log(static_cast<LogLevel>(3), "Invalid item durability: " + std::to_string(item.durability));
            return false;
        }
        
        if (item.modData.size() > 1024) { // 1KB limit for mod data
            Logger::Log(static_cast<LogLevel>(3), "Item mod data too large: " + std::to_string(item.modData.size()) + " bytes");
            return false;
        }
    }
    
    return true;
}

bool InventoryController::ValidateItemTransfer(const ItemTransferRequest& request) {
    if (!ValidatePlayerId(request.fromPeerId) || !ValidatePlayerId(request.toPeerId) ||
        !ValidateItemId(request.itemId) || !ValidateQuantity(request.quantity)) {
        return false;
    }
    
    // Check if players are within transfer distance
    float distance = CalculatePlayerDistance(request.fromPeerId, request.toPeerId);
    if (distance > MAX_TRANSFER_DISTANCE) {
        Logger::Log(static_cast<LogLevel>(2), "Players too far apart for transfer: " + std::to_string(distance) + "m");
        return false;
    }
    
    // Check if sender has the item
    std::lock_guard<std::mutex> lock(m_inventoryMutex);
    auto fromIt = m_playerInventories.find(request.fromPeerId);
    if (fromIt == m_playerInventories.end()) {
        Logger::Log(static_cast<LogLevel>(3), "Sender inventory not found for transfer validation");
        return false;
    }
    
    auto& items = fromIt->second->items;
    auto itemIt = std::find_if(items.begin(), items.end(),
        [&](const InventoryItemSnap& item) { return item.itemId == request.itemId; });
    
    if (itemIt == items.end() || itemIt->quantity < request.quantity) {
        Logger::Log(static_cast<LogLevel>(2), "Sender doesn't have sufficient items for transfer");
        return false;
    }
    
    return true;
}

bool InventoryController::ResolveInventoryConflict(const PlayerInventorySnap& local, const PlayerInventorySnap& remote) {
    if (local.peerId != remote.peerId) {
        Logger::Log(static_cast<LogLevel>(3), "Cannot resolve conflict between different players");
        return false;
    }
    
    // Use timestamp-based resolution - newer wins
    if (remote.lastUpdate > local.lastUpdate) {
        Logger::Log(static_cast<LogLevel>(1), "Resolving inventory conflict for peer " + std::to_string(local.peerId) + " - using remote version");
        return UpdatePlayerInventory(remote);
    } else {
        Logger::Log(static_cast<LogLevel>(1), "Resolving inventory conflict for peer " + std::to_string(local.peerId) + " - keeping local version");
        return true;
    }
}

void InventoryController::CleanupExpiredRequests(uint32_t maxAgeMs) {
    std::lock_guard<std::mutex> lock(m_transferMutex);
    
    uint64_t currentTime = GetCurrentTimestamp();
    uint64_t cutoffTime = currentTime - maxAgeMs;
    
    auto it = m_pendingTransfers.begin();
    size_t removed = 0;
    while (it != m_pendingTransfers.end()) {
        if (it->second->timestamp < cutoffTime) {
            Logger::Log(static_cast<LogLevel>(1), "Removing expired transfer request " + std::to_string(it->first));
            it = m_pendingTransfers.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
    
    if (removed > 0) {
        Logger::Log(static_cast<LogLevel>(1), "Cleaned up " + std::to_string(removed) + " expired transfer requests");
    }
}

void InventoryController::ClearAllData() {
    {
        std::lock_guard<std::mutex> lock(m_inventoryMutex);
        m_playerInventories.clear();
    }
    {
        std::lock_guard<std::mutex> lock(m_transferMutex);
        m_pendingTransfers.clear();
    }
    {
        std::lock_guard<std::mutex> lock(m_worldItemMutex);
        m_worldItems.clear();
    }
    m_nextRequestId.store(1);
    Logger::Log(static_cast<LogLevel>(1), "Cleared all inventory data");
}

size_t InventoryController::GetPlayerCount() const {
    std::lock_guard<std::mutex> lock(m_inventoryMutex);
    return m_playerInventories.size();
}

size_t InventoryController::GetPendingTransferCount() const {
    std::lock_guard<std::mutex> lock(m_transferMutex);
    return m_pendingTransfers.size();
}

size_t InventoryController::GetWorldItemCount() const {
    std::lock_guard<std::mutex> lock(m_worldItemMutex);
    return m_worldItems.size();
}

std::vector<uint32_t> InventoryController::GetActivePlayers() const {
    std::lock_guard<std::mutex> lock(m_inventoryMutex);
    std::vector<uint32_t> players;
    players.reserve(m_playerInventories.size());
    
    for (const auto& pair : m_playerInventories) {
        players.push_back(pair.first);
    }
    
    return players;
}

// Private helper methods

bool InventoryController::ValidateItemId(uint64_t itemId) const {
    return itemId > 0 && itemId != UINT64_MAX;
}

bool InventoryController::ValidatePlayerId(uint32_t peerId) const {
    return peerId > 0 && peerId != UINT32_MAX;
}

bool InventoryController::ValidateQuantity(uint32_t quantity) const {
    return quantity > 0 && quantity <= 9999; // Reasonable upper limit
}

float InventoryController::CalculatePlayerDistance(uint32_t player1, uint32_t player2) const {
    // This would integrate with the game's player position system
    // For now, return a placeholder value
    // TODO: Implement actual player position lookup
    return 5.0f; // Assume players are always within range for now
}

uint64_t InventoryController::GetCurrentTimestamp() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

uint32_t InventoryController::GenerateRequestId() {
    return m_nextRequestId.fetch_add(1, std::memory_order_relaxed);
}

} // namespace CoopNet
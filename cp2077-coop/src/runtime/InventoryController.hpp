#pragma once

#include <RED4ext/RED4ext.hpp>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>

namespace CoopNet {

struct InventoryItemSnap {
    uint64_t itemId;
    uint32_t quantity;
    uint32_t durability;
    std::vector<uint8_t> modData; // Serialized mod/attachment data
};

struct PlayerInventorySnap {
    uint32_t peerId;
    std::vector<InventoryItemSnap> items;
    uint64_t money;
    uint32_t version;
    uint64_t lastUpdate; // Timestamp for conflict resolution
};

struct ItemTransferRequest {
    uint32_t fromPeerId;
    uint32_t toPeerId;
    uint64_t itemId;
    uint32_t quantity;
    uint32_t requestId;
    uint64_t timestamp;
    bool validated;
};

struct ItemPickupEvent {
    uint64_t itemId;
    float worldPosition[3]; // Vector3 as array for better alignment
    uint32_t playerId;
    uint32_t timestamp;
    bool processed;
};

class InventoryController {
public:
    static InventoryController& Instance();

    // System lifecycle
    bool Initialize();
    void Shutdown();

    // Player inventory management
    bool UpdatePlayerInventory(const PlayerInventorySnap& snap);
    PlayerInventorySnap* GetPlayerInventory(uint32_t peerId);
    bool RemovePlayerInventory(uint32_t peerId);
    
    // Item transfer system
    uint32_t RequestItemTransfer(uint32_t fromPeerId, uint32_t toPeerId, 
                                uint64_t itemId, uint32_t quantity);
    bool ProcessTransferRequest(uint32_t requestId, bool approve, const std::string& reason = "");
    bool CancelTransferRequest(uint32_t requestId);
    
    // World item management
    bool RegisterWorldItemPickup(uint64_t itemId, const float worldPos[3], uint32_t playerId);
    bool IsWorldItemTaken(uint64_t itemId);
    void ClearExpiredPickups(uint32_t maxAgeMs = 300000); // 5 minutes default
    
    // Validation and conflict resolution
    bool ValidatePlayerInventory(const PlayerInventorySnap& snap);
    bool ValidateItemTransfer(const ItemTransferRequest& request);
    bool ResolveInventoryConflict(const PlayerInventorySnap& local, const PlayerInventorySnap& remote);
    
    // Cleanup and maintenance
    void CleanupExpiredRequests(uint32_t maxAgeMs = 30000); // 30 seconds default
    void ClearAllData();
    
    // Statistics and debugging
    size_t GetPlayerCount() const;
    size_t GetPendingTransferCount() const;
    size_t GetWorldItemCount() const;
    std::vector<uint32_t> GetActivePlayers() const;

protected:
    InventoryController() = default;
    virtual ~InventoryController() = default;

private:
    InventoryController(const InventoryController&) = delete;
    InventoryController& operator=(const InventoryController&) = delete;
    
    // Thread-safe data structures
    mutable std::mutex m_inventoryMutex;
    mutable std::mutex m_transferMutex;
    mutable std::mutex m_worldItemMutex;
    
    std::unordered_map<uint32_t, std::unique_ptr<PlayerInventorySnap>> m_playerInventories;
    std::unordered_map<uint32_t, std::unique_ptr<ItemTransferRequest>> m_pendingTransfers;
    std::unordered_map<uint64_t, std::unique_ptr<ItemPickupEvent>> m_worldItems;
    
    std::atomic<uint32_t> m_nextRequestId{1};
    
    // Configuration constants
    static constexpr uint32_t MAX_PENDING_TRANSFERS = 100;
    static constexpr uint32_t MAX_WORLD_ITEMS = 1000;
    static constexpr uint32_t MAX_INVENTORY_ITEMS = 500;
    static constexpr float MAX_TRANSFER_DISTANCE = 10.0f;
    static constexpr uint64_t MAX_MONEY = 999999999ULL;
    
    // Helper methods
    bool ValidateItemId(uint64_t itemId) const;
    bool ValidatePlayerId(uint32_t peerId) const;
    bool ValidateQuantity(uint32_t quantity) const;
    float CalculatePlayerDistance(uint32_t player1, uint32_t player2) const;
    uint64_t GetCurrentTimestamp() const;
    uint32_t GenerateRequestId();
};

} // namespace CoopNet
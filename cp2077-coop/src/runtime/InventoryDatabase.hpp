#pragma once

#include <unordered_map>
#include <vector>
#include <mutex>
#include <string>
#include <memory>
#include <sqlite3.h>
#include "InventoryController.hpp"

namespace CoopNet {

// Database schema for persistent inventory storage
struct InventoryDBItem {
    uint64_t itemId;
    uint32_t peerId;
    uint32_t quantity;
    uint32_t durability;
    std::string modData; // JSON serialized mod data
    uint64_t lastModified;
    bool isDirty; // Needs sync
};

struct InventoryTransaction {
    uint64_t transactionId;
    uint32_t fromPeerId;
    uint32_t toPeerId;
    uint64_t itemId;
    uint32_t quantity;
    uint64_t timestamp;
    std::string status; // "pending", "completed", "failed", "cancelled"
    std::string reason;
};

class InventoryDatabase {
public:
    static InventoryDatabase& Instance();

    // Database lifecycle
    bool Initialize(const std::string& dbPath = "inventory.db");
    void Shutdown();
    bool IsInitialized() const { return m_initialized; }

    // Player inventory persistence
    bool SavePlayerInventory(uint32_t peerId, const PlayerInventorySnap& inventory);
    bool LoadPlayerInventory(uint32_t peerId, PlayerInventorySnap& inventory);
    bool DeletePlayerInventory(uint32_t peerId);

    // Item management
    bool AddItem(uint32_t peerId, uint64_t itemId, uint32_t quantity, uint32_t durability = 100);
    bool RemoveItem(uint32_t peerId, uint64_t itemId, uint32_t quantity);
    bool UpdateItemDurability(uint32_t peerId, uint64_t itemId, uint32_t durability);
    bool SetItemModData(uint32_t peerId, uint64_t itemId, const std::string& modData);

    // Transaction logging
    uint64_t LogTransaction(const ItemTransferRequest& request);
    bool UpdateTransactionStatus(uint64_t transactionId, const std::string& status, const std::string& reason = "");
    std::vector<InventoryTransaction> GetPendingTransactions();
    std::vector<InventoryTransaction> GetPlayerTransactionHistory(uint32_t peerId, uint32_t limit = 100);

    // Statistics and maintenance
    bool OptimizeDatabase();
    bool BackupDatabase(const std::string& backupPath);
    std::vector<uint32_t> GetActivePlayers();
    size_t GetTotalItems();
    size_t GetPlayerItemCount(uint32_t peerId);

    // Integrity checks
    bool VerifyInventoryIntegrity(uint32_t peerId);
    bool RepairCorruptedData();
    std::vector<std::string> RunIntegrityCheck();

private:
    InventoryDatabase() = default;
    ~InventoryDatabase();
    InventoryDatabase(const InventoryDatabase&) = delete;
    InventoryDatabase& operator=(const InventoryDatabase&) = delete;

    // Database operations
    bool CreateTables();
    bool ExecuteSQL(const std::string& sql);
    sqlite3_stmt* PrepareStatement(const std::string& sql);
    void FinalizeStatement(sqlite3_stmt* stmt);

    // Helper methods
    std::string SerializeInventoryItems(const std::vector<InventoryItemSnap>& items);
    std::vector<InventoryItemSnap> DeserializeInventoryItems(const std::string& data);
    uint64_t GetCurrentTimestamp();

    // Database connection
    sqlite3* m_db = nullptr;
    bool m_initialized = false;
    mutable std::mutex m_dbMutex;

    // Prepared statements for performance
    sqlite3_stmt* m_insertItemStmt = nullptr;
    sqlite3_stmt* m_updateItemStmt = nullptr;
    sqlite3_stmt* m_deleteItemStmt = nullptr;
    sqlite3_stmt* m_selectPlayerInventoryStmt = nullptr;
    sqlite3_stmt* m_insertTransactionStmt = nullptr;
    sqlite3_stmt* m_updateTransactionStmt = nullptr;

    // Cache for frequently accessed data
    std::unordered_map<uint32_t, uint64_t> m_playerLastSync;
    mutable std::mutex m_cacheMutex;
};

// Game integration adapter
class GameInventoryAdapter {
public:
    static GameInventoryAdapter& Instance();

    // Game integration
    bool SyncWithGameInventory(uint32_t peerId);
    bool ApplyInventoryToGame(uint32_t peerId, const PlayerInventorySnap& inventory);
    PlayerInventorySnap BuildInventoryFromGame(uint32_t peerId);

    // Item validation against game data
    bool IsValidItemId(uint64_t itemId);
    bool CanPlayerCarryItem(uint32_t peerId, uint64_t itemId, uint32_t quantity);
    bool IsItemStackable(uint64_t itemId);
    uint32_t GetMaxStackSize(uint64_t itemId);
    uint32_t GetItemWeight(uint64_t itemId);
    std::string GetItemName(uint64_t itemId);

    // Game state checking
    bool IsPlayerInGame(uint32_t peerId);
    bool CanModifyInventory(uint32_t peerId);
    bool IsPlayerInCombat(uint32_t peerId);
    bool IsPlayerInVehicle(uint32_t peerId);

    // Anti-cheat validation
    bool ValidateItemQuantity(uint64_t itemId, uint32_t quantity);
    bool ValidateItemModifications(uint64_t itemId, const std::string& modData);
    bool CheckDuplicationAttempt(uint32_t peerId, uint64_t itemId);

private:
    GameInventoryAdapter() = default;
    ~GameInventoryAdapter() = default;
    GameInventoryAdapter(const GameInventoryAdapter&) = delete;
    GameInventoryAdapter& operator=(const GameInventoryAdapter&) = delete;

    // Game system interfaces
    bool GetGamePlayerInventory(uint32_t peerId, std::vector<InventoryItemSnap>& items);
    bool SetGamePlayerInventory(uint32_t peerId, const std::vector<InventoryItemSnap>& items);
    uint64_t GetGamePlayerMoney(uint32_t peerId);
    bool SetGamePlayerMoney(uint32_t peerId, uint64_t money);

    // Item database cache
    std::unordered_map<uint64_t, std::string> m_itemNameCache;
    std::unordered_map<uint64_t, uint32_t> m_itemWeightCache;
    std::unordered_map<uint64_t, uint32_t> m_itemStackSizeCache;
    mutable std::mutex m_itemCacheMutex;
};

// Enhanced inventory controller with database integration
class EnhancedInventoryController : public InventoryController {
public:
    static EnhancedInventoryController& Instance();

    // Enhanced operations with database persistence
    bool UpdatePlayerInventoryPersistent(const PlayerInventorySnap& snap);
    bool TransferItemPersistent(uint32_t fromPeerId, uint32_t toPeerId, uint64_t itemId, uint32_t quantity);
    bool ValidateWithGameState(const PlayerInventorySnap& snap);

    // Conflict resolution with game state
    bool ResolveInventoryConflictWithGame(uint32_t peerId);
    bool SyncAllPlayersWithGame();

    // Transaction management
    bool ProcessPendingTransactions();
    bool RollbackTransaction(uint64_t transactionId);

    // Performance monitoring
    struct InventoryStats {
        uint32_t totalPlayers;
        uint32_t totalItems;
        uint32_t pendingTransfers;
        uint32_t failedTransfers;
        uint64_t lastSyncTime;
        float averageTransferTime;
    };

    InventoryStats GetInventoryStats();

public:
    virtual ~EnhancedInventoryController() = default;

private:
    EnhancedInventoryController() = default;

    uint64_t m_lastStatsUpdate = 0;
    InventoryStats m_cachedStats;
    mutable std::mutex m_statsMutex;
};

} // namespace CoopNet
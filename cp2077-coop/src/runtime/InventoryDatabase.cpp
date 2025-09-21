#include "InventoryDatabase.hpp"
#include "../core/Logger.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iomanip>

namespace CoopNet {

InventoryDatabase& InventoryDatabase::Instance() {
    static InventoryDatabase instance;
    return instance;
}

InventoryDatabase::~InventoryDatabase() {
    Shutdown();
}

bool InventoryDatabase::Initialize(const std::string& dbPath) {
    std::lock_guard<std::mutex> lock(m_dbMutex);

    if (m_initialized) {
        return true;
    }

    Logger::Log(LogLevel::INFO, "Initializing inventory database: " + dbPath);

    int rc = sqlite3_open(dbPath.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        Logger::Log(LogLevel::ERROR, "Failed to open inventory database: " + std::string(sqlite3_errmsg(m_db)));
        return false;
    }

    // Enable foreign keys and WAL mode for better performance
    ExecuteSQL("PRAGMA foreign_keys = ON;");
    ExecuteSQL("PRAGMA journal_mode = WAL;");
    ExecuteSQL("PRAGMA synchronous = NORMAL;");
    ExecuteSQL("PRAGMA cache_size = 10000;");

    if (!CreateTables()) {
        Logger::Log(LogLevel::ERROR, "Failed to create database tables");
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    // Prepare commonly used statements
    m_insertItemStmt = PrepareStatement(
        "INSERT OR REPLACE INTO inventory_items (item_id, peer_id, quantity, durability, mod_data, last_modified) "
        "VALUES (?, ?, ?, ?, ?, ?)"
    );

    m_updateItemStmt = PrepareStatement(
        "UPDATE inventory_items SET quantity = ?, durability = ?, mod_data = ?, last_modified = ? "
        "WHERE item_id = ? AND peer_id = ?"
    );

    m_deleteItemStmt = PrepareStatement(
        "DELETE FROM inventory_items WHERE item_id = ? AND peer_id = ?"
    );

    m_selectPlayerInventoryStmt = PrepareStatement(
        "SELECT item_id, quantity, durability, mod_data, last_modified FROM inventory_items WHERE peer_id = ?"
    );

    m_insertTransactionStmt = PrepareStatement(
        "INSERT INTO inventory_transactions (from_peer_id, to_peer_id, item_id, quantity, timestamp, status, reason) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"
    );

    m_updateTransactionStmt = PrepareStatement(
        "UPDATE inventory_transactions SET status = ?, reason = ? WHERE transaction_id = ?"
    );

    m_initialized = true;
    Logger::Log(LogLevel::INFO, "Inventory database initialized successfully");
    return true;
}

void InventoryDatabase::Shutdown() {
    std::lock_guard<std::mutex> lock(m_dbMutex);

    if (!m_initialized) {
        return;
    }

    Logger::Log(LogLevel::INFO, "Shutting down inventory database");

    // Finalize prepared statements
    FinalizeStatement(m_insertItemStmt);
    FinalizeStatement(m_updateItemStmt);
    FinalizeStatement(m_deleteItemStmt);
    FinalizeStatement(m_selectPlayerInventoryStmt);
    FinalizeStatement(m_insertTransactionStmt);
    FinalizeStatement(m_updateTransactionStmt);

    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }

    m_initialized = false;
    Logger::Log(LogLevel::INFO, "Inventory database shutdown complete");
}

bool InventoryDatabase::CreateTables() {
    std::vector<std::string> createStatements = {
        // Player inventories table
        "CREATE TABLE IF NOT EXISTS player_inventories ("
        "    peer_id INTEGER PRIMARY KEY,"
        "    money INTEGER NOT NULL DEFAULT 0,"
        "    version INTEGER NOT NULL DEFAULT 1,"
        "    last_update INTEGER NOT NULL,"
        "    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))"
        ");",

        // Individual inventory items
        "CREATE TABLE IF NOT EXISTS inventory_items ("
        "    item_id INTEGER NOT NULL,"
        "    peer_id INTEGER NOT NULL,"
        "    quantity INTEGER NOT NULL DEFAULT 1,"
        "    durability INTEGER NOT NULL DEFAULT 100,"
        "    mod_data TEXT DEFAULT '',"
        "    last_modified INTEGER NOT NULL,"
        "    PRIMARY KEY (item_id, peer_id),"
        "    FOREIGN KEY (peer_id) REFERENCES player_inventories(peer_id) ON DELETE CASCADE"
        ");",

        // Transaction log
        "CREATE TABLE IF NOT EXISTS inventory_transactions ("
        "    transaction_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    from_peer_id INTEGER NOT NULL,"
        "    to_peer_id INTEGER NOT NULL,"
        "    item_id INTEGER NOT NULL,"
        "    quantity INTEGER NOT NULL,"
        "    timestamp INTEGER NOT NULL,"
        "    status TEXT NOT NULL DEFAULT 'pending',"
        "    reason TEXT DEFAULT ''"
        ");",

        // World items (dropped items, loot, etc.)
        "CREATE TABLE IF NOT EXISTS world_items ("
        "    item_id INTEGER PRIMARY KEY,"
        "    world_x REAL NOT NULL,"
        "    world_y REAL NOT NULL,"
        "    world_z REAL NOT NULL,"
        "    quantity INTEGER NOT NULL DEFAULT 1,"
        "    durability INTEGER NOT NULL DEFAULT 100,"
        "    picked_up_by INTEGER DEFAULT NULL,"
        "    spawn_time INTEGER NOT NULL,"
        "    pickup_time INTEGER DEFAULT NULL"
        ");",

        // Indexes for performance
        "CREATE INDEX IF NOT EXISTS idx_inventory_items_peer ON inventory_items(peer_id);",
        "CREATE INDEX IF NOT EXISTS idx_inventory_items_modified ON inventory_items(last_modified);",
        "CREATE INDEX IF NOT EXISTS idx_transactions_status ON inventory_transactions(status);",
        "CREATE INDEX IF NOT EXISTS idx_transactions_timestamp ON inventory_transactions(timestamp);",
        "CREATE INDEX IF NOT EXISTS idx_world_items_position ON world_items(world_x, world_y, world_z);",
        "CREATE INDEX IF NOT EXISTS idx_world_items_spawn_time ON world_items(spawn_time);"
    };

    for (const auto& sql : createStatements) {
        if (!ExecuteSQL(sql)) {
            return false;
        }
    }

    return true;
}

bool InventoryDatabase::SavePlayerInventory(uint32_t peerId, const PlayerInventorySnap& inventory) {
    std::lock_guard<std::mutex> lock(m_dbMutex);

    if (!m_initialized) {
        return false;
    }

    // Begin transaction
    ExecuteSQL("BEGIN TRANSACTION;");

    // Update or insert player inventory record
    std::string sql = "INSERT OR REPLACE INTO player_inventories (peer_id, money, version, last_update) "
                     "VALUES (" + std::to_string(peerId) + ", " + std::to_string(inventory.money) +
                     ", " + std::to_string(inventory.version) + ", " + std::to_string(GetCurrentTimestamp()) + ");";

    if (!ExecuteSQL(sql)) {
        ExecuteSQL("ROLLBACK;");
        return false;
    }

    // Clear existing items for this player
    sql = "DELETE FROM inventory_items WHERE peer_id = " + std::to_string(peerId) + ";";
    if (!ExecuteSQL(sql)) {
        ExecuteSQL("ROLLBACK;");
        return false;
    }

    // Insert all items
    uint64_t timestamp = GetCurrentTimestamp();
    for (const auto& item : inventory.items) {
        if (!AddItem(peerId, item.itemId, item.quantity, item.durability)) {
            ExecuteSQL("ROLLBACK;");
            return false;
        }
    }

    ExecuteSQL("COMMIT;");

    Logger::Log(LogLevel::DEBUG, "Saved inventory for peer " + std::to_string(peerId) +
               " (" + std::to_string(inventory.items.size()) + " items)");
    return true;
}

bool InventoryDatabase::LoadPlayerInventory(uint32_t peerId, PlayerInventorySnap& inventory) {
    std::lock_guard<std::mutex> lock(m_dbMutex);

    if (!m_initialized) {
        return false;
    }

    // Load player inventory metadata
    std::string sql = "SELECT money, version, last_update FROM player_inventories WHERE peer_id = " + std::to_string(peerId);
    sqlite3_stmt* stmt = PrepareStatement(sql);

    if (!stmt) {
        return false;
    }

    inventory.peerId = peerId;
    inventory.items.clear();

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        inventory.money = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
        inventory.version = static_cast<uint32_t>(sqlite3_column_int(stmt, 1));
        inventory.lastUpdate = static_cast<uint64_t>(sqlite3_column_int64(stmt, 2));
    } else {
        // Player not found in database
        FinalizeStatement(stmt);
        inventory.money = 0;
        inventory.version = 1;
        inventory.lastUpdate = GetCurrentTimestamp();
        return true; // Not an error, just empty inventory
    }

    FinalizeStatement(stmt);

    // Load player items
    sqlite3_bind_int(m_selectPlayerInventoryStmt, 1, peerId);

    while (sqlite3_step(m_selectPlayerInventoryStmt) == SQLITE_ROW) {
        InventoryItemSnap item;
        item.itemId = static_cast<uint64_t>(sqlite3_column_int64(m_selectPlayerInventoryStmt, 0));
        item.quantity = static_cast<uint32_t>(sqlite3_column_int(m_selectPlayerInventoryStmt, 1));
        item.durability = static_cast<uint32_t>(sqlite3_column_int(m_selectPlayerInventoryStmt, 2));

        const char* modDataStr = reinterpret_cast<const char*>(sqlite3_column_text(m_selectPlayerInventoryStmt, 3));
        if (modDataStr) {
            // Deserialize mod data from JSON
            std::string modDataJson(modDataStr);
            // TODO: Proper JSON deserialization of mod data
            item.modData.clear(); // Placeholder
        }

        inventory.items.push_back(item);
    }

    sqlite3_reset(m_selectPlayerInventoryStmt);

    Logger::Log(LogLevel::DEBUG, "Loaded inventory for peer " + std::to_string(peerId) +
               " (" + std::to_string(inventory.items.size()) + " items)");
    return true;
}

bool InventoryDatabase::AddItem(uint32_t peerId, uint64_t itemId, uint32_t quantity, uint32_t durability) {
    if (!m_initialized || !m_insertItemStmt) {
        return false;
    }

    uint64_t timestamp = GetCurrentTimestamp();

    sqlite3_bind_int64(m_insertItemStmt, 1, itemId);
    sqlite3_bind_int(m_insertItemStmt, 2, peerId);
    sqlite3_bind_int(m_insertItemStmt, 3, quantity);
    sqlite3_bind_int(m_insertItemStmt, 4, durability);
    sqlite3_bind_text(m_insertItemStmt, 5, "", -1, SQLITE_STATIC); // Empty mod data for now
    sqlite3_bind_int64(m_insertItemStmt, 6, timestamp);

    int result = sqlite3_step(m_insertItemStmt);
    sqlite3_reset(m_insertItemStmt);

    bool success = (result == SQLITE_DONE);
    if (success) {
        Logger::Log(LogLevel::DEBUG, "Added item " + std::to_string(itemId) + " (qty: " +
                   std::to_string(quantity) + ") to peer " + std::to_string(peerId));
    }

    return success;
}

uint64_t InventoryDatabase::LogTransaction(const ItemTransferRequest& request) {
    std::lock_guard<std::mutex> lock(m_dbMutex);

    if (!m_initialized || !m_insertTransactionStmt) {
        return 0;
    }

    sqlite3_bind_int(m_insertTransactionStmt, 1, request.fromPeerId);
    sqlite3_bind_int(m_insertTransactionStmt, 2, request.toPeerId);
    sqlite3_bind_int64(m_insertTransactionStmt, 3, request.itemId);
    sqlite3_bind_int(m_insertTransactionStmt, 4, request.quantity);
    sqlite3_bind_int64(m_insertTransactionStmt, 5, request.timestamp);
    sqlite3_bind_text(m_insertTransactionStmt, 6, "pending", -1, SQLITE_STATIC);
    sqlite3_bind_text(m_insertTransactionStmt, 7, "", -1, SQLITE_STATIC);

    int result = sqlite3_step(m_insertTransactionStmt);
    sqlite3_reset(m_insertTransactionStmt);

    if (result == SQLITE_DONE) {
        uint64_t transactionId = sqlite3_last_insert_rowid(m_db);
        Logger::Log(LogLevel::INFO, "Logged transaction " + std::to_string(transactionId) +
                   " for item transfer");
        return transactionId;
    }

    return 0;
}

bool InventoryDatabase::UpdateTransactionStatus(uint64_t transactionId, const std::string& status, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_dbMutex);

    if (!m_initialized || !m_updateTransactionStmt) {
        return false;
    }

    sqlite3_bind_text(m_updateTransactionStmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(m_updateTransactionStmt, 2, reason.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(m_updateTransactionStmt, 3, transactionId);

    int result = sqlite3_step(m_updateTransactionStmt);
    sqlite3_reset(m_updateTransactionStmt);

    bool success = (result == SQLITE_DONE);
    if (success) {
        Logger::Log(LogLevel::DEBUG, "Updated transaction " + std::to_string(transactionId) +
                   " status to: " + status);
    }

    return success;
}

bool InventoryDatabase::ExecuteSQL(const std::string& sql) {
    if (!m_db) {
        return false;
    }

    char* errorMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errorMsg);

    if (rc != SQLITE_OK) {
        Logger::Log(LogLevel::ERROR, "SQL error: " + std::string(errorMsg ? errorMsg : "Unknown error"));
        if (errorMsg) {
            sqlite3_free(errorMsg);
        }
        return false;
    }

    return true;
}

sqlite3_stmt* InventoryDatabase::PrepareStatement(const std::string& sql) {
    if (!m_db) {
        return nullptr;
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        Logger::Log(LogLevel::ERROR, "Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db)));
        return nullptr;
    }

    return stmt;
}

void InventoryDatabase::FinalizeStatement(sqlite3_stmt* stmt) {
    if (stmt) {
        sqlite3_finalize(stmt);
    }
}

uint64_t InventoryDatabase::GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

std::vector<uint32_t> InventoryDatabase::GetActivePlayers() {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    std::vector<uint32_t> players;

    if (!m_initialized) {
        return players;
    }

    std::string sql = "SELECT DISTINCT peer_id FROM player_inventories";
    sqlite3_stmt* stmt = PrepareStatement(sql);

    if (stmt) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            players.push_back(static_cast<uint32_t>(sqlite3_column_int(stmt, 0)));
        }
        FinalizeStatement(stmt);
    }

    return players;
}

size_t InventoryDatabase::GetTotalItems() {
    std::lock_guard<std::mutex> lock(m_dbMutex);

    if (!m_initialized) {
        return 0;
    }

    std::string sql = "SELECT COUNT(*) FROM inventory_items";
    sqlite3_stmt* stmt = PrepareStatement(sql);

    size_t count = 0;
    if (stmt) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
        }
        FinalizeStatement(stmt);
    }

    return count;
}

bool InventoryDatabase::OptimizeDatabase() {
    std::lock_guard<std::mutex> lock(m_dbMutex);

    if (!m_initialized) {
        return false;
    }

    Logger::Log(LogLevel::INFO, "Optimizing inventory database");

    // Clean up old transactions (older than 30 days)
    uint64_t cutoffTime = GetCurrentTimestamp() - (30 * 24 * 60 * 60);
    std::string cleanupSql = "DELETE FROM inventory_transactions WHERE timestamp < " + std::to_string(cutoffTime) +
                            " AND status IN ('completed', 'failed', 'cancelled')";
    ExecuteSQL(cleanupSql);

    // Vacuum the database to reclaim space
    ExecuteSQL("VACUUM;");

    // Analyze tables for query optimization
    ExecuteSQL("ANALYZE;");

    Logger::Log(LogLevel::INFO, "Database optimization complete");
    return true;
}

// === Game Integration Adapter Implementation ===

GameInventoryAdapter& GameInventoryAdapter::Instance() {
    static GameInventoryAdapter instance;
    return instance;
}

bool GameInventoryAdapter::IsValidItemId(uint64_t itemId) {
    // TODO: Integrate with game's item database
    // For now, basic validation
    return itemId > 0 && itemId < 0xFFFFFFFFFFFFFFFFULL;
}

bool GameInventoryAdapter::CanPlayerCarryItem(uint32_t peerId, uint64_t itemId, uint32_t quantity) {
    // TODO: Check player's carry capacity, encumbrance, etc.
    // Basic implementation for now
    if (!IsValidItemId(itemId) || quantity == 0) {
        return false;
    }

    // Check if stackable
    if (IsItemStackable(itemId)) {
        return quantity <= GetMaxStackSize(itemId);
    }

    return quantity == 1;
}

bool GameInventoryAdapter::IsItemStackable(uint64_t itemId) {
    // TODO: Query game database for item stackability
    // For now, assume most items are stackable except weapons/armor
    std::lock_guard<std::mutex> lock(m_itemCacheMutex);

    // Simple heuristic based on item ID ranges (would be proper game integration)
    return (itemId % 100) > 50; // Placeholder logic
}

uint32_t GameInventoryAdapter::GetMaxStackSize(uint64_t itemId) {
    std::lock_guard<std::mutex> lock(m_itemCacheMutex);

    auto it = m_itemStackSizeCache.find(itemId);
    if (it != m_itemStackSizeCache.end()) {
        return it->second;
    }

    // TODO: Query game database
    uint32_t stackSize = IsItemStackable(itemId) ? 999 : 1;
    m_itemStackSizeCache[itemId] = stackSize;
    return stackSize;
}

std::string GameInventoryAdapter::GetItemName(uint64_t itemId) {
    std::lock_guard<std::mutex> lock(m_itemCacheMutex);

    auto it = m_itemNameCache.find(itemId);
    if (it != m_itemNameCache.end()) {
        return it->second;
    }

    // TODO: Query game database
    std::string name = "Item_" + std::to_string(itemId);
    m_itemNameCache[itemId] = name;
    return name;
}

bool GameInventoryAdapter::ValidateItemQuantity(uint64_t itemId, uint32_t quantity) {
    if (quantity == 0) {
        return false;
    }

    if (IsItemStackable(itemId)) {
        return quantity <= GetMaxStackSize(itemId);
    }

    return quantity == 1;
}

bool GameInventoryAdapter::CheckDuplicationAttempt(uint32_t peerId, uint64_t itemId) {
    // TODO: Implement duplication detection
    // - Check against recent transaction logs
    // - Verify item source legitimacy
    // - Cross-reference with game state
    return false; // No duplication detected (placeholder)
}

// Additional missing InventoryDatabase implementations
bool InventoryDatabase::DeletePlayerInventory(uint32_t peerId) {
    std::lock_guard<std::mutex> lock(m_dbMutex);

    if (!m_initialized) {
        return false;
    }

    ExecuteSQL("BEGIN TRANSACTION;");

    std::string sql = "DELETE FROM inventory_items WHERE peer_id = " + std::to_string(peerId);
    if (!ExecuteSQL(sql)) {
        ExecuteSQL("ROLLBACK;");
        return false;
    }

    sql = "DELETE FROM player_inventories WHERE peer_id = " + std::to_string(peerId);
    if (!ExecuteSQL(sql)) {
        ExecuteSQL("ROLLBACK;");
        return false;
    }

    ExecuteSQL("COMMIT;");
    Logger::Log(LogLevel::INFO, "Deleted inventory for peer " + std::to_string(peerId));
    return true;
}

bool InventoryDatabase::RemoveItem(uint32_t peerId, uint64_t itemId, uint32_t quantity) {
    if (!m_initialized || !m_deleteItemStmt) {
        return false;
    }

    sqlite3_bind_int64(m_deleteItemStmt, 1, itemId);
    sqlite3_bind_int(m_deleteItemStmt, 2, peerId);

    int result = sqlite3_step(m_deleteItemStmt);
    sqlite3_reset(m_deleteItemStmt);

    bool success = (result == SQLITE_DONE);
    if (success) {
        Logger::Log(LogLevel::DEBUG, "Removed item " + std::to_string(itemId) + " from peer " + std::to_string(peerId));
    }

    return success;
}

bool InventoryDatabase::UpdateItemDurability(uint32_t peerId, uint64_t itemId, uint32_t durability) {
    if (!m_initialized) {
        return false;
    }

    std::string sql = "UPDATE inventory_items SET durability = " + std::to_string(durability) +
                     ", last_modified = " + std::to_string(GetCurrentTimestamp()) +
                     " WHERE item_id = " + std::to_string(itemId) + " AND peer_id = " + std::to_string(peerId);

    return ExecuteSQL(sql);
}

bool InventoryDatabase::SetItemModData(uint32_t peerId, uint64_t itemId, const std::string& modData) {
    if (!m_initialized) {
        return false;
    }

    std::string sql = "UPDATE inventory_items SET mod_data = '" + modData +
                     "', last_modified = " + std::to_string(GetCurrentTimestamp()) +
                     " WHERE item_id = " + std::to_string(itemId) + " AND peer_id = " + std::to_string(peerId);

    return ExecuteSQL(sql);
}

std::vector<InventoryTransaction> InventoryDatabase::GetPendingTransactions() {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    std::vector<InventoryTransaction> transactions;

    if (!m_initialized) {
        return transactions;
    }

    std::string sql = "SELECT transaction_id, from_peer_id, to_peer_id, item_id, quantity, timestamp, status, reason "
                     "FROM inventory_transactions WHERE status = 'pending' ORDER BY timestamp";

    sqlite3_stmt* stmt = PrepareStatement(sql);
    if (stmt) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            InventoryTransaction transaction;
            transaction.transactionId = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
            transaction.fromPeerId = static_cast<uint32_t>(sqlite3_column_int(stmt, 1));
            transaction.toPeerId = static_cast<uint32_t>(sqlite3_column_int(stmt, 2));
            transaction.itemId = static_cast<uint64_t>(sqlite3_column_int64(stmt, 3));
            transaction.quantity = static_cast<uint32_t>(sqlite3_column_int(stmt, 4));
            transaction.timestamp = static_cast<uint64_t>(sqlite3_column_int64(stmt, 5));

            const char* statusStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            const char* reasonStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));

            transaction.status = statusStr ? statusStr : "";
            transaction.reason = reasonStr ? reasonStr : "";

            transactions.push_back(transaction);
        }
        FinalizeStatement(stmt);
    }

    return transactions;
}

std::vector<InventoryTransaction> InventoryDatabase::GetPlayerTransactionHistory(uint32_t peerId, uint32_t limit) {
    std::lock_guard<std::mutex> lock(m_dbMutex);
    std::vector<InventoryTransaction> transactions;

    if (!m_initialized) {
        return transactions;
    }

    std::string sql = "SELECT transaction_id, from_peer_id, to_peer_id, item_id, quantity, timestamp, status, reason "
                     "FROM inventory_transactions WHERE from_peer_id = " + std::to_string(peerId) +
                     " OR to_peer_id = " + std::to_string(peerId) +
                     " ORDER BY timestamp DESC LIMIT " + std::to_string(limit);

    sqlite3_stmt* stmt = PrepareStatement(sql);
    if (stmt) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            InventoryTransaction transaction;
            transaction.transactionId = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
            transaction.fromPeerId = static_cast<uint32_t>(sqlite3_column_int(stmt, 1));
            transaction.toPeerId = static_cast<uint32_t>(sqlite3_column_int(stmt, 2));
            transaction.itemId = static_cast<uint64_t>(sqlite3_column_int64(stmt, 3));
            transaction.quantity = static_cast<uint32_t>(sqlite3_column_int(stmt, 4));
            transaction.timestamp = static_cast<uint64_t>(sqlite3_column_int64(stmt, 5));

            const char* statusStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            const char* reasonStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));

            transaction.status = statusStr ? statusStr : "";
            transaction.reason = reasonStr ? reasonStr : "";

            transactions.push_back(transaction);
        }
        FinalizeStatement(stmt);
    }

    return transactions;
}

bool InventoryDatabase::BackupDatabase(const std::string& backupPath) {
    std::lock_guard<std::mutex> lock(m_dbMutex);

    if (!m_initialized) {
        return false;
    }

    Logger::Log(LogLevel::INFO, "Creating database backup: " + backupPath);

    sqlite3* backupDb;
    int rc = sqlite3_open(backupPath.c_str(), &backupDb);
    if (rc != SQLITE_OK) {
        Logger::Log(LogLevel::ERROR, "Failed to create backup database: " + std::string(sqlite3_errmsg(backupDb)));
        return false;
    }

    sqlite3_backup* backup = sqlite3_backup_init(backupDb, "main", m_db, "main");
    if (backup) {
        rc = sqlite3_backup_step(backup, -1);
        sqlite3_backup_finish(backup);
    }

    sqlite3_close(backupDb);

    bool success = (rc == SQLITE_DONE);
    if (success) {
        Logger::Log(LogLevel::INFO, "Database backup completed successfully");
    } else {
        Logger::Log(LogLevel::ERROR, "Database backup failed");
    }

    return success;
}

size_t InventoryDatabase::GetPlayerItemCount(uint32_t peerId) {
    std::lock_guard<std::mutex> lock(m_dbMutex);

    if (!m_initialized) {
        return 0;
    }

    std::string sql = "SELECT COUNT(*) FROM inventory_items WHERE peer_id = " + std::to_string(peerId);
    sqlite3_stmt* stmt = PrepareStatement(sql);

    size_t count = 0;
    if (stmt) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
        }
        FinalizeStatement(stmt);
    }

    return count;
}

bool InventoryDatabase::VerifyInventoryIntegrity(uint32_t peerId) {
    // Basic integrity check - verify all items have valid quantities and durabilities
    std::lock_guard<std::mutex> lock(m_dbMutex);

    if (!m_initialized) {
        return false;
    }

    std::string sql = "SELECT COUNT(*) FROM inventory_items WHERE peer_id = " + std::to_string(peerId) +
                     " AND (quantity <= 0 OR durability < 0 OR durability > 100)";

    sqlite3_stmt* stmt = PrepareStatement(sql);
    if (!stmt) {
        return false;
    }

    bool isValid = true;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int invalidItems = sqlite3_column_int(stmt, 0);
        isValid = (invalidItems == 0);
    }

    FinalizeStatement(stmt);
    return isValid;
}

bool InventoryDatabase::RepairCorruptedData() {
    std::lock_guard<std::mutex> lock(m_dbMutex);

    if (!m_initialized) {
        return false;
    }

    Logger::Log(LogLevel::INFO, "Repairing corrupted inventory data");

    ExecuteSQL("BEGIN TRANSACTION;");

    // Fix invalid quantities (set to 1 if <= 0)
    ExecuteSQL("UPDATE inventory_items SET quantity = 1 WHERE quantity <= 0;");

    // Fix invalid durabilities (clamp to 0-100 range)
    ExecuteSQL("UPDATE inventory_items SET durability = 0 WHERE durability < 0;");
    ExecuteSQL("UPDATE inventory_items SET durability = 100 WHERE durability > 100;");

    // Remove duplicate items (keep most recent)
    ExecuteSQL("DELETE FROM inventory_items WHERE rowid NOT IN ("
              "SELECT MIN(rowid) FROM inventory_items GROUP BY item_id, peer_id);");

    ExecuteSQL("COMMIT;");
    Logger::Log(LogLevel::INFO, "Corrupted data repair completed");
    return true;
}

std::vector<std::string> InventoryDatabase::RunIntegrityCheck() {
    std::vector<std::string> issues;

    if (!m_initialized) {
        issues.push_back("Database not initialized");
        return issues;
    }

    // Check for invalid quantities
    std::string sql = "SELECT COUNT(*) FROM inventory_items WHERE quantity <= 0";
    sqlite3_stmt* stmt = PrepareStatement(sql);
    if (stmt) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            if (count > 0) {
                issues.push_back("Found " + std::to_string(count) + " items with invalid quantities");
            }
        }
        FinalizeStatement(stmt);
    }

    // Check for invalid durabilities
    sql = "SELECT COUNT(*) FROM inventory_items WHERE durability < 0 OR durability > 100";
    stmt = PrepareStatement(sql);
    if (stmt) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            if (count > 0) {
                issues.push_back("Found " + std::to_string(count) + " items with invalid durabilities");
            }
        }
        FinalizeStatement(stmt);
    }

    return issues;
}

std::string InventoryDatabase::SerializeInventoryItems(const std::vector<InventoryItemSnap>& items) {
    nlohmann::json jsonItems = nlohmann::json::array();

    for (const auto& item : items) {
        nlohmann::json jsonItem;
        jsonItem["itemId"] = item.itemId;
        jsonItem["quantity"] = item.quantity;
        jsonItem["durability"] = item.durability;

        // Serialize mod data as hex string
        std::stringstream ss;
        ss << std::hex;
        for (uint8_t byte : item.modData) {
            ss << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        jsonItem["modData"] = ss.str();

        jsonItems.push_back(jsonItem);
    }

    return jsonItems.dump();
}

std::vector<InventoryItemSnap> InventoryDatabase::DeserializeInventoryItems(const std::string& data) {
    std::vector<InventoryItemSnap> items;

    try {
        nlohmann::json jsonItems = nlohmann::json::parse(data);

        for (const auto& jsonItem : jsonItems) {
            InventoryItemSnap item;
            item.itemId = jsonItem["itemId"];
            item.quantity = jsonItem["quantity"];
            item.durability = jsonItem["durability"];

            // Deserialize mod data from hex string
            if (jsonItem.contains("modData")) {
                std::string modDataHex = jsonItem["modData"];
                item.modData.clear();

                for (size_t i = 0; i < modDataHex.length(); i += 2) {
                    if (i + 1 < modDataHex.length()) {
                        std::string byteString = modDataHex.substr(i, 2);
                        uint8_t byte = static_cast<uint8_t>(std::stoul(byteString, nullptr, 16));
                        item.modData.push_back(byte);
                    }
                }
            }

            items.push_back(item);
        }
    } catch (const std::exception& e) {
        Logger::Log(LogLevel::ERROR, "Failed to deserialize inventory items: " + std::string(e.what()));
    }

    return items;
}

// Additional GameInventoryAdapter implementations
bool GameInventoryAdapter::SyncWithGameInventory(uint32_t peerId) {
    // TODO: Implement game inventory sync
    Logger::Log(LogLevel::DEBUG, "Syncing inventory with game for peer " + std::to_string(peerId));
    return true;
}

bool GameInventoryAdapter::ApplyInventoryToGame(uint32_t peerId, const PlayerInventorySnap& inventory) {
    // TODO: Apply inventory changes to game
    Logger::Log(LogLevel::DEBUG, "Applying inventory to game for peer " + std::to_string(peerId));
    return true;
}

PlayerInventorySnap GameInventoryAdapter::BuildInventoryFromGame(uint32_t peerId) {
    // TODO: Build inventory from current game state
    PlayerInventorySnap inventory;
    inventory.peerId = peerId;
    inventory.money = 0;
    inventory.version = 1;
    inventory.lastUpdate = std::chrono::system_clock::now().time_since_epoch().count();
    return inventory;
}

uint32_t GameInventoryAdapter::GetItemWeight(uint64_t itemId) {
    std::lock_guard<std::mutex> lock(m_itemCacheMutex);

    auto it = m_itemWeightCache.find(itemId);
    if (it != m_itemWeightCache.end()) {
        return it->second;
    }

    // TODO: Query game database for item weight
    uint32_t weight = 1; // Default weight
    m_itemWeightCache[itemId] = weight;
    return weight;
}

bool GameInventoryAdapter::IsPlayerInGame(uint32_t peerId) {
    // TODO: Check if player is currently in game
    return true; // Placeholder
}

bool GameInventoryAdapter::CanModifyInventory(uint32_t peerId) {
    // TODO: Check if player's inventory can be modified (not in cutscene, etc.)
    return true; // Placeholder
}

bool GameInventoryAdapter::IsPlayerInCombat(uint32_t peerId) {
    // TODO: Check if player is in combat
    return false; // Placeholder
}

bool GameInventoryAdapter::IsPlayerInVehicle(uint32_t peerId) {
    // TODO: Check if player is in a vehicle
    return false; // Placeholder
}

bool GameInventoryAdapter::ValidateItemModifications(uint64_t itemId, const std::string& modData) {
    // TODO: Validate item modifications against game rules
    return true; // Placeholder
}

bool GameInventoryAdapter::GetGamePlayerInventory(uint32_t peerId, std::vector<InventoryItemSnap>& items) {
    // TODO: Get inventory from game engine
    items.clear();
    return true; // Placeholder
}

bool GameInventoryAdapter::SetGamePlayerInventory(uint32_t peerId, const std::vector<InventoryItemSnap>& items) {
    // TODO: Set inventory in game engine
    return true; // Placeholder
}

uint64_t GameInventoryAdapter::GetGamePlayerMoney(uint32_t peerId) {
    // TODO: Get player money from game
    return 0; // Placeholder
}

bool GameInventoryAdapter::SetGamePlayerMoney(uint32_t peerId, uint64_t money) {
    // TODO: Set player money in game
    return true; // Placeholder
}

// === EnhancedInventoryController Implementation ===

EnhancedInventoryController& EnhancedInventoryController::Instance() {
    static EnhancedInventoryController instance;
    return instance;
}

bool EnhancedInventoryController::UpdatePlayerInventoryPersistent(const PlayerInventorySnap& snap) {
    // First update in memory
    if (!UpdatePlayerInventory(snap)) {
        return false;
    }

    // Then persist to database
    auto& db = InventoryDatabase::Instance();
    return db.SavePlayerInventory(snap.peerId, snap);
}

bool EnhancedInventoryController::TransferItemPersistent(uint32_t fromPeerId, uint32_t toPeerId, uint64_t itemId, uint32_t quantity) {
    auto& db = InventoryDatabase::Instance();

    // Create transaction record
    ItemTransferRequest request;
    request.fromPeerId = fromPeerId;
    request.toPeerId = toPeerId;
    request.itemId = itemId;
    request.quantity = quantity;
    request.timestamp = std::chrono::system_clock::now().time_since_epoch().count();

    uint64_t transactionId = db.LogTransaction(request);
    if (transactionId == 0) {
        return false;
    }

    // Perform the transfer in memory using parent class method
    bool success = (RequestItemTransfer(fromPeerId, toPeerId, itemId, quantity) != 0);

    // Update transaction status
    std::string status = success ? "completed" : "failed";
    std::string reason = success ? "" : "Transfer validation failed";
    db.UpdateTransactionStatus(transactionId, status, reason);

    if (success) {
        // Save updated inventories to database
        PlayerInventorySnap* fromInventory = GetPlayerInventory(fromPeerId);
        PlayerInventorySnap* toInventory = GetPlayerInventory(toPeerId);
        if (fromInventory) {
            db.SavePlayerInventory(fromPeerId, *fromInventory);
        }
        if (toInventory) {
            db.SavePlayerInventory(toPeerId, *toInventory);
        }
    }

    return success;
}

bool EnhancedInventoryController::ValidateWithGameState(const PlayerInventorySnap& snap) {
    auto& adapter = GameInventoryAdapter::Instance();

    // Check if player is in a valid state for inventory operations
    if (!adapter.IsPlayerInGame(snap.peerId) || !adapter.CanModifyInventory(snap.peerId)) {
        return false;
    }

    // Validate each item in the inventory
    for (const auto& item : snap.items) {
        if (!adapter.IsValidItemId(item.itemId)) {
            Logger::Log(LogLevel::WARNING, "Invalid item ID " + std::to_string(item.itemId) +
                       " in inventory for peer " + std::to_string(snap.peerId));
            return false;
        }

        if (!adapter.ValidateItemQuantity(item.itemId, item.quantity)) {
            Logger::Log(LogLevel::WARNING, "Invalid item quantity for item " + std::to_string(item.itemId) +
                       " in inventory for peer " + std::to_string(snap.peerId));
            return false;
        }
    }

    return true;
}

bool EnhancedInventoryController::ResolveInventoryConflictWithGame(uint32_t peerId) {
    auto& adapter = GameInventoryAdapter::Instance();
    auto& db = InventoryDatabase::Instance();

    // Get current game inventory
    PlayerInventorySnap gameInventory = adapter.BuildInventoryFromGame(peerId);

    // Get stored database inventory
    PlayerInventorySnap dbInventory;
    if (!db.LoadPlayerInventory(peerId, dbInventory)) {
        // No database inventory exists, use game state
        return db.SavePlayerInventory(peerId, gameInventory);
    }

    // Merge inventories (prefer most recent based on timestamps)
    if (gameInventory.lastUpdate > dbInventory.lastUpdate) {
        // Game state is newer, update database
        Logger::Log(LogLevel::INFO, "Game inventory is newer for peer " + std::to_string(peerId) + ", updating database");
        return db.SavePlayerInventory(peerId, gameInventory);
    } else {
        // Database is newer or equal, apply to game
        Logger::Log(LogLevel::INFO, "Database inventory is newer for peer " + std::to_string(peerId) + ", applying to game");
        bool success = adapter.ApplyInventoryToGame(peerId, dbInventory);
        if (success) {
            UpdatePlayerInventory(dbInventory);
        }
        return success;
    }
}

bool EnhancedInventoryController::SyncAllPlayersWithGame() {
    auto& db = InventoryDatabase::Instance();
    std::vector<uint32_t> activePlayers = db.GetActivePlayers();

    bool allSuccess = true;
    for (uint32_t peerId : activePlayers) {
        if (!ResolveInventoryConflictWithGame(peerId)) {
            Logger::Log(LogLevel::ERROR, "Failed to sync inventory for peer " + std::to_string(peerId));
            allSuccess = false;
        }
    }

    return allSuccess;
}

bool EnhancedInventoryController::ProcessPendingTransactions() {
    auto& db = InventoryDatabase::Instance();
    std::vector<InventoryTransaction> pendingTransactions = db.GetPendingTransactions();

    bool allSuccess = true;
    for (const auto& transaction : pendingTransactions) {
        bool success = (RequestItemTransfer(transaction.fromPeerId, transaction.toPeerId,
                                           transaction.itemId, transaction.quantity) != 0);

        std::string status = success ? "completed" : "failed";
        std::string reason = success ? "" : "Transfer processing failed";

        if (!db.UpdateTransactionStatus(transaction.transactionId, status, reason)) {
            allSuccess = false;
        }
    }

    return allSuccess;
}

bool EnhancedInventoryController::RollbackTransaction(uint64_t transactionId) {
    auto& db = InventoryDatabase::Instance();

    // Find the transaction
    std::vector<InventoryTransaction> transactions = db.GetPendingTransactions();
    for (const auto& transaction : transactions) {
        if (transaction.transactionId == transactionId) {
            // Perform reverse transfer
            bool success = (RequestItemTransfer(transaction.toPeerId, transaction.fromPeerId,
                                               transaction.itemId, transaction.quantity) != 0);

            std::string status = success ? "cancelled" : "rollback_failed";
            std::string reason = success ? "Manual rollback" : "Rollback operation failed";

            return db.UpdateTransactionStatus(transactionId, status, reason);
        }
    }

    return false; // Transaction not found
}

EnhancedInventoryController::InventoryStats EnhancedInventoryController::GetInventoryStats() {
    std::lock_guard<std::mutex> lock(m_statsMutex);

    uint64_t currentTime = std::chrono::system_clock::now().time_since_epoch().count();

    // Update stats if they're stale (older than 5 seconds)
    if (currentTime - m_lastStatsUpdate > 5000000000ULL) { // 5 seconds in nanoseconds
        auto& db = InventoryDatabase::Instance();

        m_cachedStats.totalPlayers = static_cast<uint32_t>(db.GetActivePlayers().size());
        m_cachedStats.totalItems = static_cast<uint32_t>(db.GetTotalItems());
        m_cachedStats.pendingTransfers = static_cast<uint32_t>(db.GetPendingTransactions().size());
        m_cachedStats.lastSyncTime = currentTime;

        // Calculate failed transfers (approximate)
        m_cachedStats.failedTransfers = 0; // Would need additional database query
        m_cachedStats.averageTransferTime = 0.0f; // Would need timing data

        m_lastStatsUpdate = currentTime;
    }

    return m_cachedStats;
}

} // namespace CoopNet
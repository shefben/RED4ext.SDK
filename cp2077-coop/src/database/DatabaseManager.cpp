// CRITICAL: Windows compatibility must be absolutely first before ANY includes
#ifdef _WIN32
// Prevent all possible unistd.h inclusion attempts
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// zlib configuration
#define HAVE_ZLIB_H 1
#define NO_UNISTD_H
#define ZLIB_CONST
// Comprehensive CURL/zlib Windows compatibility
#define HAVE_IO_H 1
#define HAVE_FCNTL_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_WINDOWS_H 1
// Define unistd.h replacement functions
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <direct.h>
// Prevent function name conflicts
#ifndef _CRT_INTERNAL_NONSTDC_NAMES
#define access _access
#define close _close
#define open _open
#define read _read
#define write _write
#define lseek _lseek
#define mkdir _mkdir
#define rmdir _rmdir
#define unlink _unlink
#define getcwd _getcwd
#define chdir _chdir
#endif
// Tell zlib/CURL we DON'T have unistd.h
#ifdef Z_HAVE_UNISTD_H
#undef Z_HAVE_UNISTD_H
#endif
#ifdef _LARGEFILE64_SOURCE
#undef _LARGEFILE64_SOURCE
#endif
#define CURL_DISABLE_UNISTD_H
#define Z_SOLO
#define NO_GZIP
#endif

#include "DatabaseManager.hpp"
#include "../core/ErrorManager.hpp"
#include <sqlite3.h>
// #include <spdlog/spdlog.h> // Replaced with CoopNet::Logger
#include "../core/Logger.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <regex>

#include "../core/ZlibWrapper.hpp"

namespace CoopNet {

// SQLite adapter implementation
class SQLiteAdapter : public IDatabaseAdapter {
public:
    SQLiteAdapter() = default;
    ~SQLiteAdapter() override { Disconnect(); }

    bool Connect(const DatabaseConfig& config) override {
        if (m_database) {
            sqlite3_close(m_database);
            m_database = nullptr;
        }

        int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
        if (config.enableSSL) {
            flags |= SQLITE_OPEN_FULLMUTEX;
        }

        int result = sqlite3_open_v2(config.database.c_str(), &m_database, flags, nullptr);
        if (result != SQLITE_OK) {
            m_lastError = sqlite3_errmsg(m_database);
            // spdlog::error("[DatabaseManager] Failed to open SQLite database: {}", m_lastError);
            if (m_database) {
                sqlite3_close(m_database);
                m_database = nullptr;
            }
            return false;
        }

        // Set pragmas for better performance
        ExecuteSimpleQuery("PRAGMA journal_mode=WAL");
        ExecuteSimpleQuery("PRAGMA synchronous=NORMAL");
        ExecuteSimpleQuery("PRAGMA cache_size=10000");
        ExecuteSimpleQuery("PRAGMA temp_store=MEMORY");
        ExecuteSimpleQuery("PRAGMA mmap_size=268435456"); // 256MB

        m_connected = true;
        // spdlog::info("[DatabaseManager] Connected to SQLite database: {}", config.database);
        return true;
    }

    void Disconnect() override {
        if (m_database) {
            sqlite3_close(m_database);
            m_database = nullptr;
        }
        m_connected = false;
    }

    bool IsConnected() const override {
        return m_connected && m_database != nullptr;
    }

    QueryResult ExecuteQuery(const QueryParams& params) override {
        QueryResult result;
        result.queryId = 0;
        result.query = params.query;
        result.status = QueryStatus::Failed;

        if (!IsConnected()) {
            result.errorMessage = "Database not connected";
            return result;
        }

        auto startTime = std::chrono::steady_clock::now();

        sqlite3_stmt* stmt = nullptr;
        int prepareResult = sqlite3_prepare_v2(m_database, params.query.c_str(), -1, &stmt, nullptr);

        if (prepareResult != SQLITE_OK) {
            result.errorMessage = sqlite3_errmsg(m_database);
            // spdlog::error("[DatabaseManager] Failed to prepare query: {}", result.errorMessage);
            return result;
        }

        // Bind parameters
        for (size_t i = 0; i < params.parameters.size(); ++i) {
            int bindResult = BindParameter(stmt, static_cast<int>(i + 1), params.parameters[i]);
            if (bindResult != SQLITE_OK) {
                result.errorMessage = sqlite3_errmsg(m_database);
                sqlite3_finalize(stmt);
                return result;
            }
        }

        // Execute query
        if (params.type == QueryType::Select) {
            ExecuteSelectQuery(stmt, result);
        } else {
            ExecuteModifyQuery(stmt, result);
        }

        sqlite3_finalize(stmt);

        auto endTime = std::chrono::steady_clock::now();
        result.executionTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

        if (result.status == QueryStatus::Success) {
            // spdlog::debug("[DatabaseManager] Query executed successfully in {} microseconds", result.executionTime.count());
        }

        return result;
    }

    std::future<QueryResult> ExecuteQueryAsync(const QueryParams& params) override {
        return std::async(std::launch::async, [this, params]() {
            return ExecuteQuery(params);
        });
    }

    bool BeginTransaction(Transaction& transaction) override {
        if (!IsConnected()) return false;

        std::string isolationQuery;
        switch (transaction.isolation) {
            case IsolationLevel::ReadUncommitted:
                isolationQuery = "PRAGMA read_uncommitted=true; BEGIN";
                break;
            case IsolationLevel::ReadCommitted:
                isolationQuery = "PRAGMA read_uncommitted=false; BEGIN";
                break;
            default:
                isolationQuery = "BEGIN";
                break;
        }

        QueryParams params;
        params.query = isolationQuery;
        params.type = QueryType::Transaction;

        QueryResult result = ExecuteQuery(params);
        return result.status == QueryStatus::Success;
    }

    bool CommitTransaction(const Transaction& transaction) override {
        if (!IsConnected()) return false;

        QueryParams params;
        params.query = "COMMIT";
        params.type = QueryType::Transaction;

        QueryResult result = ExecuteQuery(params);
        return result.status == QueryStatus::Success;
    }

    bool RollbackTransaction(const Transaction& transaction) override {
        if (!IsConnected()) return false;

        QueryParams params;
        params.query = "ROLLBACK";
        params.type = QueryType::Transaction;

        QueryResult result = ExecuteQuery(params);
        return result.status == QueryStatus::Success;
    }

    bool CreateSchema(const DatabaseSchema& schema) override {
        if (!IsConnected()) return false;

        // Create tables in dependency order
        for (const auto& table : schema.tables) {
            if (!CreateTable(table)) {
                return false;
            }
        }

        // Create indexes
        for (const auto& index : schema.indexes) {
            if (!CreateIndex(index)) {
                return false;
            }
        }

        return true;
    }

    bool DropSchema(const std::string& schemaName) override {
        if (!IsConnected()) return false;

        // SQLite doesn't have schemas, so we interpret this as dropping all tables
        QueryParams params;
        params.query = "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'";
        params.type = QueryType::Select;

        QueryResult result = ExecuteQuery(params);
        if (result.status != QueryStatus::Success) {
            return false;
        }

        for (const auto& row : result.rows) {
            auto it = row.find("name");
            if (it != row.end() && std::holds_alternative<std::string>(it->second)) {
                std::string tableName = std::get<std::string>(it->second);
                QueryParams dropParams;
                dropParams.query = "DROP TABLE IF EXISTS " + QuoteIdentifier(tableName);
                dropParams.type = QueryType::DropTable;
                ExecuteQuery(dropParams);
            }
        }

        return true;
    }

    DatabaseSchema GetSchema(const std::string& schemaName) override {
        DatabaseSchema schema;
        schema.name = schemaName.empty() ? "main" : schemaName;

        if (!IsConnected()) return schema;

        // Get tables
        QueryParams params;
        params.query = "SELECT name, sql FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'";
        params.type = QueryType::Select;

        QueryResult result = ExecuteQuery(params);
        if (result.status == QueryStatus::Success) {
            for (const auto& row : result.rows) {
                auto nameIt = row.find("name");
                if (nameIt != row.end() && std::holds_alternative<std::string>(nameIt->second)) {
                    TableDefinition table;
                    table.name = std::get<std::string>(nameIt->second);

                    // Get table info
                    GetTableInfo(table);
                    schema.tables.push_back(table);
                }
            }
        }

        // Get indexes
        params.query = "SELECT name, tbl_name, sql FROM sqlite_master WHERE type='index' AND name NOT LIKE 'sqlite_%'";
        result = ExecuteQuery(params);
        if (result.status == QueryStatus::Success) {
            for (const auto& row : result.rows) {
                auto nameIt = row.find("name");
                auto tableIt = row.find("tbl_name");
                if (nameIt != row.end() && tableIt != row.end() &&
                    std::holds_alternative<std::string>(nameIt->second) &&
                    std::holds_alternative<std::string>(tableIt->second)) {

                    IndexDefinition index;
                    index.name = std::get<std::string>(nameIt->second);
                    index.tableName = std::get<std::string>(tableIt->second);
                    schema.indexes.push_back(index);
                }
            }
        }

        return schema;
    }

    std::string GetLastError() const override {
        return m_lastError;
    }

private:
    sqlite3* m_database = nullptr;
    bool m_connected = false;
    std::string m_lastError;

    void ExecuteSimpleQuery(const std::string& query) {
        char* errorMsg = nullptr;
        int result = sqlite3_exec(m_database, query.c_str(), nullptr, nullptr, &errorMsg);
        if (result != SQLITE_OK && errorMsg) {
            // spdlog::warn("[DatabaseManager] Pragma warning: {}", errorMsg);
            sqlite3_free(errorMsg);
        }
    }

    int BindParameter(sqlite3_stmt* stmt, int index, const DatabaseValue& value) {
        return std::visit([stmt, index](const auto& v) -> int {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return sqlite3_bind_null(stmt, index);
            } else if constexpr (std::is_same_v<T, bool>) {
                return sqlite3_bind_int(stmt, index, v ? 1 : 0);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                return sqlite3_bind_int(stmt, index, v);
            } else if constexpr (std::is_same_v<T, uint32_t>) {
                return sqlite3_bind_int64(stmt, index, v);
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return sqlite3_bind_int64(stmt, index, v);
            } else if constexpr (std::is_same_v<T, uint64_t>) {
                return sqlite3_bind_int64(stmt, index, static_cast<int64_t>(v));
            } else if constexpr (std::is_same_v<T, float>) {
                return sqlite3_bind_double(stmt, index, v);
            } else if constexpr (std::is_same_v<T, double>) {
                return sqlite3_bind_double(stmt, index, v);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return sqlite3_bind_text(stmt, index, v.c_str(), -1, SQLITE_STATIC);
            } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                return sqlite3_bind_blob(stmt, index, v.data(), static_cast<int>(v.size()), SQLITE_STATIC);
            }
            return SQLITE_ERROR;
        }, value);
    }

    void ExecuteSelectQuery(sqlite3_stmt* stmt, QueryResult& result) {
        int columnCount = sqlite3_column_count(stmt);

        // Get column names and types
        for (int i = 0; i < columnCount; ++i) {
            result.columnNames.push_back(sqlite3_column_name(stmt, i));
            result.columnTypes.push_back(GetColumnTypeName(sqlite3_column_type(stmt, i)));
        }

        // Fetch rows
        while (true) {
            int stepResult = sqlite3_step(stmt);
            if (stepResult == SQLITE_ROW) {
                std::unordered_map<std::string, DatabaseValue> row;
                for (int i = 0; i < columnCount; ++i) {
                    row[result.columnNames[i]] = GetColumnValue(stmt, i);
                }
                result.rows.push_back(row);
            } else if (stepResult == SQLITE_DONE) {
                result.status = QueryStatus::Success;
                break;
            } else {
                result.status = QueryStatus::Failed;
                result.errorMessage = sqlite3_errmsg(m_database);
                break;
            }
        }
    }

    void ExecuteModifyQuery(sqlite3_stmt* stmt, QueryResult& result) {
        int stepResult = sqlite3_step(stmt);
        if (stepResult == SQLITE_DONE) {
            result.status = QueryStatus::Success;
            result.affectedRows = sqlite3_changes(m_database);
            result.insertId = sqlite3_last_insert_rowid(m_database);
        } else {
            result.status = QueryStatus::Failed;
            result.errorMessage = sqlite3_errmsg(m_database);
        }
    }

    DatabaseValue GetColumnValue(sqlite3_stmt* stmt, int column) {
        int type = sqlite3_column_type(stmt, column);
        switch (type) {
            case SQLITE_NULL:
                return nullptr;
            case SQLITE_INTEGER:
                return static_cast<int64_t>(sqlite3_column_int64(stmt, column));
            case SQLITE_FLOAT:
                return sqlite3_column_double(stmt, column);
            case SQLITE_TEXT: {
                const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, column));
                return std::string(text ? text : "");
            }
            case SQLITE_BLOB: {
                const void* blob = sqlite3_column_blob(stmt, column);
                int size = sqlite3_column_bytes(stmt, column);
                if (blob && size > 0) {
                    const uint8_t* data = static_cast<const uint8_t*>(blob);
                    return std::vector<uint8_t>(data, data + size);
                }
                return std::vector<uint8_t>();
            }
            default:
                return nullptr;
        }
    }

    std::string GetColumnTypeName(int sqliteType) {
        switch (sqliteType) {
            case SQLITE_NULL: return "NULL";
            case SQLITE_INTEGER: return "INTEGER";
            case SQLITE_FLOAT: return "REAL";
            case SQLITE_TEXT: return "TEXT";
            case SQLITE_BLOB: return "BLOB";
            default: return "UNKNOWN";
        }
    }

    bool CreateTable(const TableDefinition& table) {
        std::string sql = "CREATE TABLE IF NOT EXISTS " + QuoteIdentifier(table.name) + " (";

        for (size_t i = 0; i < table.columns.size(); ++i) {
            if (i > 0) sql += ", ";
            const auto& col = table.columns[i];
            sql += QuoteIdentifier(col.name) + " " + col.type;

            if (col.primaryKey) sql += " PRIMARY KEY";
            if (col.autoIncrement) sql += " AUTOINCREMENT";
            if (!col.nullable) sql += " NOT NULL";
            if (col.unique) sql += " UNIQUE";
            if (!col.defaultValue.empty()) sql += " DEFAULT " + col.defaultValue;
        }

        sql += ")";

        QueryParams params;
        params.query = sql;
        params.type = QueryType::CreateTable;

        QueryResult result = ExecuteQuery(params);
        return result.status == QueryStatus::Success;
    }

    bool CreateIndex(const IndexDefinition& index) {
        std::string sql = "CREATE ";
        if (index.unique) sql += "UNIQUE ";
        sql += "INDEX IF NOT EXISTS " + QuoteIdentifier(index.name);
        sql += " ON " + QuoteIdentifier(index.tableName) + " (";

        for (size_t i = 0; i < index.columns.size(); ++i) {
            if (i > 0) sql += ", ";
            sql += QuoteIdentifier(index.columns[i]);
        }
        sql += ")";

        QueryParams params;
        params.query = sql;
        params.type = QueryType::CreateIndex;

        QueryResult result = ExecuteQuery(params);
        return result.status == QueryStatus::Success;
    }

    void GetTableInfo(TableDefinition& table) {
        QueryParams params;
        params.query = "PRAGMA table_info(" + QuoteIdentifier(table.name) + ")";
        params.type = QueryType::Select;

        QueryResult result = ExecuteQuery(params);
        if (result.status == QueryStatus::Success) {
            for (const auto& row : result.rows) {
                ColumnDefinition column;

                auto nameIt = row.find("name");
                if (nameIt != row.end() && std::holds_alternative<std::string>(nameIt->second)) {
                    column.name = std::get<std::string>(nameIt->second);
                }

                auto typeIt = row.find("type");
                if (typeIt != row.end() && std::holds_alternative<std::string>(typeIt->second)) {
                    column.type = std::get<std::string>(typeIt->second);
                }

                auto notnullIt = row.find("notnull");
                if (notnullIt != row.end() && std::holds_alternative<int64_t>(notnullIt->second)) {
                    column.nullable = std::get<int64_t>(notnullIt->second) == 0;
                }

                auto pkIt = row.find("pk");
                if (pkIt != row.end() && std::holds_alternative<int64_t>(pkIt->second)) {
                    column.primaryKey = std::get<int64_t>(pkIt->second) != 0;
                }

                table.columns.push_back(column);
            }
        }
    }

    std::string QuoteIdentifier(const std::string& identifier) {
        return "\"" + identifier + "\"";
    }
};

// Simple connection pool implementation
class SimpleConnectionPool : public IConnectionPool {
public:
    SimpleConnectionPool(const DatabaseConfig& config) : m_config(config) {}

    std::shared_ptr<ConnectionInfo> AcquireConnection() override {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_availableConnections.empty()) {
            auto connection = m_availableConnections.front();
            m_availableConnections.pop();
            connection->lastUsed = std::chrono::steady_clock::now();
            return connection;
        }

        if (m_activeConnections.size() < m_config.maxConnections) {
            auto connection = CreateConnection();
            if (connection) {
                m_activeConnections.insert(connection);
                return connection;
            }
        }

        return nullptr;
    }

    void ReleaseConnection(std::shared_ptr<ConnectionInfo> connection) override {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (ValidateConnection(connection)) {
            connection->lastUsed = std::chrono::steady_clock::now();
            m_availableConnections.push(connection);
        } else {
            m_activeConnections.erase(connection);
        }
    }

    bool ValidateConnection(std::shared_ptr<ConnectionInfo> connection) override {
        if (!connection || connection->state != DatabaseConnectionState::Connected) {
            return false;
        }

        auto now = std::chrono::steady_clock::now();
        auto idleTime = std::chrono::duration_cast<std::chrono::seconds>(now - connection->lastUsed);

        return idleTime.count() < m_config.maxIdleTime;
    }

    void CloseAllConnections() override {
        std::lock_guard<std::mutex> lock(m_mutex);

        while (!m_availableConnections.empty()) {
            m_availableConnections.pop();
        }
        m_activeConnections.clear();
    }

    uint32_t GetActiveConnections() const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return static_cast<uint32_t>(m_activeConnections.size());
    }

    uint32_t GetIdleConnections() const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return static_cast<uint32_t>(m_availableConnections.size());
    }

private:
    DatabaseConfig m_config;
    std::set<std::shared_ptr<ConnectionInfo>> m_activeConnections;
    std::queue<std::shared_ptr<ConnectionInfo>> m_availableConnections;
    mutable std::mutex m_mutex;

    std::shared_ptr<ConnectionInfo> CreateConnection() {
        auto connection = std::make_shared<ConnectionInfo>();
        connection->connectionId = GenerateConnectionId();
        connection->type = m_config.type;
        connection->state = DatabaseConnectionState::Connecting;
        connection->createdAt = std::chrono::steady_clock::now();
        connection->lastUsed = connection->createdAt;
        connection->queriesExecuted = 0;
        connection->totalQueryTime = 0;
        connection->inTransaction = false;

        // Create adapter and connect
        auto adapter = std::make_shared<SQLiteAdapter>();
        if (adapter->Connect(m_config)) {
            connection->state = DatabaseConnectionState::Connected;
            connection->nativeHandle = adapter.get();
            return connection;
        }

        connection->state = DatabaseConnectionState::Error;
        return nullptr;
    }

    uint64_t GenerateConnectionId() {
        static std::atomic<uint64_t> counter{1};
        return counter++;
    }
};

// DatabaseManager implementation
DatabaseManager& DatabaseManager::Instance() {
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::Initialize(const DatabaseConfig& config) {
    std::lock_guard<std::recursive_mutex> lock(m_connectionMutex);

    if (m_initialized) {
        // spdlog::warn("[DatabaseManager] Already initialized");
        return true;
    }

    if (!ValidateConfig(config)) {
        // spdlog::error("[DatabaseManager] Invalid configuration");
        return false;
    }

    // Store default configuration
    m_configurations[m_defaultConnection] = config;

    // Create default connection
    if (!Connect(m_defaultConnection, config)) {
        // spdlog::error("[DatabaseManager] Failed to create default connection");
        return false;
    }

    // Start maintenance thread
    m_shouldStop = false;
    m_maintenanceThread = std::thread(&DatabaseManager::MaintenanceLoop, this);

    m_initialized = true;
    // spdlog::info("[DatabaseManager] Initialized successfully");
    return true;
}

void DatabaseManager::Shutdown() {
    std::lock_guard<std::recursive_mutex> lock(m_connectionMutex);

    if (!m_initialized) return;

    // Stop maintenance thread
    m_shouldStop = true;
    if (m_maintenanceThread.joinable()) {
        m_maintenanceThread.join();
    }

    // Close all connections
    for (const auto& [name, adapter] : m_adapters) {
        adapter->Disconnect();
    }
    m_adapters.clear();

    // Clean up connection pools
    for (const auto& [name, pool] : m_connectionPools) {
        pool->CloseAllConnections();
    }
    m_connectionPools.clear();

    m_configurations.clear();
    m_activeTransactions.clear();

    {
        std::lock_guard<std::mutex> cacheLock(m_cacheMutex);
        m_queryCache.clear();
        while (!m_cacheOrder.empty()) m_cacheOrder.pop();
    }

    m_initialized = false;
    // spdlog::info("[DatabaseManager] Shutdown completed");
}

bool DatabaseManager::Connect(const std::string& connectionName, const DatabaseConfig& config) {
    std::lock_guard<std::recursive_mutex> lock(m_connectionMutex);

    DatabaseConfig actualConfig = config;
    if (config.database.empty() && m_configurations.count(connectionName)) {
        actualConfig = m_configurations[connectionName];
    }

    if (!ValidateConfig(actualConfig)) {
        // spdlog::error("[DatabaseManager] Invalid configuration for connection: {}", connectionName);
        return false;
    }

    // Create adapter based on database type
    std::shared_ptr<IDatabaseAdapter> adapter;
    switch (actualConfig.type) {
        case DatabaseType::SQLite:
            adapter = std::make_shared<SQLiteAdapter>();
            break;
        default:
            // spdlog::error("[DatabaseManager] Unsupported database type: {}",
                         static_cast<int>(actualConfig.type));
            return false;
    }

    if (!adapter->Connect(actualConfig)) {
        // spdlog::error("[DatabaseManager] Failed to connect to database: {}", connectionName);
        return false;
    }

    m_adapters[connectionName] = adapter;
    m_configurations[connectionName] = actualConfig;

    // Create connection pool if enabled
    if (actualConfig.enableConnectionPooling) {
        m_connectionPools[connectionName] = CreateConnectionPool(actualConfig);
    }

    // spdlog::info("[DatabaseManager] Connected to database: {}", connectionName);

    // Notify event
    DatabaseEvent event;
    event.type = DatabaseEvent::Connected;
    event.connectionId = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.message = "Connection established: " + connectionName;
    NotifyEvent(event);

    return true;
}

bool DatabaseManager::Disconnect(const std::string& connectionName) {
    std::lock_guard<std::recursive_mutex> lock(m_connectionMutex);

    auto adapterIt = m_adapters.find(connectionName);
    if (adapterIt == m_adapters.end()) {
        // spdlog::warn("[DatabaseManager] Connection not found: {}", connectionName);
        return false;
    }

    // Close connection pool
    auto poolIt = m_connectionPools.find(connectionName);
    if (poolIt != m_connectionPools.end()) {
        poolIt->second->CloseAllConnections();
        m_connectionPools.erase(poolIt);
    }

    // Disconnect adapter
    adapterIt->second->Disconnect();
    m_adapters.erase(adapterIt);

    // Remove configuration
    m_configurations.erase(connectionName);

    // spdlog::info("[DatabaseManager] Disconnected from database: {}", connectionName);

    // Notify event
    DatabaseEvent event;
    event.type = DatabaseEvent::Disconnected;
    event.connectionId = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.message = "Connection closed: " + connectionName;
    NotifyEvent(event);

    return true;
}

bool DatabaseManager::IsConnected(const std::string& connectionName) const {
    std::lock_guard<std::recursive_mutex> lock(m_connectionMutex);

    auto it = m_adapters.find(connectionName);
    return it != m_adapters.end() && it->second->IsConnected();
}

QueryResult DatabaseManager::ExecuteQuery(const std::string& query, const std::vector<DatabaseValue>& params,
                                         const std::string& connectionName) {
    QueryParams queryParams;
    queryParams.query = query;
    queryParams.parameters = params;
    queryParams.type = DeduceQueryType(query);

    return ExecuteQuery(queryParams, connectionName);
}

QueryResult DatabaseManager::ExecuteQuery(const QueryParams& params, const std::string& connectionName) {
    auto adapter = GetAdapter(connectionName);
    if (!adapter) {
        QueryResult result;
        result.status = QueryStatus::Failed;
        result.errorMessage = "Connection not found: " + connectionName;
        return result;
    }

    // Check cache first
    if (m_queryCacheEnabled && params.cached) {
        std::string cacheKey = GenerateCacheKey(params);
        auto cachedResult = GetCachedResult(cacheKey);
        if (cachedResult) {
            // spdlog::debug("[DatabaseManager] Query result served from cache");
            return *cachedResult;
        }
    }

    // Execute query
    auto startTime = std::chrono::steady_clock::now();
    QueryResult result = adapter->ExecuteQuery(params);
    auto endTime = std::chrono::steady_clock::now();

    result.queryId = GenerateQueryId();

    // Update performance tracking
    if (m_performanceMonitoringEnabled) {
        RecordQueryPerformance(params.query, result.executionTime);
        UpdateConnectionStats(connectionName, result.executionTime.count());
    }

    // Cache successful results
    if (result.status == QueryStatus::Success && m_queryCacheEnabled && params.cached) {
        std::string cacheKey = GenerateCacheKey(params);
        CacheResult(cacheKey, result);
    }

    // Notify event
    DatabaseEvent event;
    event.type = DatabaseEvent::QueryExecuted;
    event.connectionId = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.message = "Query executed: " + params.query.substr(0, 50) + "...";
    event.data["execution_time"] = static_cast<int64_t>(result.executionTime.count());
    event.data["affected_rows"] = static_cast<int64_t>(result.affectedRows);
    NotifyEvent(event);

    return result;
}

bool DatabaseManager::BeginTransaction(const std::string& connectionName, IsolationLevel isolation) {
    auto adapter = GetAdapter(connectionName);
    if (!adapter) return false;

    Transaction transaction;
    transaction.transactionId = GenerateTransactionId();
    transaction.connectionId = 0;
    transaction.isolation = isolation;
    transaction.startTime = std::chrono::steady_clock::now();
    transaction.readOnly = false;
    transaction.committed = false;
    transaction.rolledBack = false;

    if (adapter->BeginTransaction(transaction)) {
        std::lock_guard<std::recursive_mutex> lock(m_connectionMutex);
        m_activeTransactions[connectionName] = transaction;

        DatabaseEvent event;
        event.type = DatabaseEvent::TransactionStarted;
        event.timestamp = std::chrono::steady_clock::now();
        event.message = "Transaction started: " + connectionName;
        NotifyEvent(event);

        return true;
    }

    return false;
}

bool DatabaseManager::CommitTransaction(const std::string& connectionName) {
    auto adapter = GetAdapter(connectionName);
    if (!adapter) return false;

    std::lock_guard<std::recursive_mutex> lock(m_connectionMutex);
    auto it = m_activeTransactions.find(connectionName);
    if (it == m_activeTransactions.end()) return false;

    Transaction& transaction = it->second;
    if (adapter->CommitTransaction(transaction)) {
        transaction.committed = true;
        m_activeTransactions.erase(it);

        DatabaseEvent event;
        event.type = DatabaseEvent::TransactionCommitted;
        event.timestamp = std::chrono::steady_clock::now();
        event.message = "Transaction committed: " + connectionName;
        NotifyEvent(event);

        return true;
    }

    return false;
}

bool DatabaseManager::RollbackTransaction(const std::string& connectionName) {
    auto adapter = GetAdapter(connectionName);
    if (!adapter) return false;

    std::lock_guard<std::recursive_mutex> lock(m_connectionMutex);
    auto it = m_activeTransactions.find(connectionName);
    if (it == m_activeTransactions.end()) return false;

    Transaction& transaction = it->second;
    if (adapter->RollbackTransaction(transaction)) {
        transaction.rolledBack = true;
        m_activeTransactions.erase(it);

        DatabaseEvent event;
        event.type = DatabaseEvent::TransactionRolledBack;
        event.timestamp = std::chrono::steady_clock::now();
        event.message = "Transaction rolled back: " + connectionName;
        NotifyEvent(event);

        return true;
    }

    return false;
}

// Helper methods implementation
std::shared_ptr<IDatabaseAdapter> DatabaseManager::GetAdapter(const std::string& connectionName) {
    std::lock_guard<std::recursive_mutex> lock(m_connectionMutex);
    auto it = m_adapters.find(connectionName);
    return it != m_adapters.end() ? it->second : nullptr;
}

bool DatabaseManager::ValidateConfig(const DatabaseConfig& config) {
    if (config.database.empty()) {
        // spdlog::error("[DatabaseManager] Database name cannot be empty");
        return false;
    }

    if (config.maxConnections == 0 || config.maxConnections > 1000) {
        // spdlog::error("[DatabaseManager] Invalid max connections: {}", config.maxConnections);
        return false;
    }

    if (config.minConnections > config.maxConnections) {
        // spdlog::error("[DatabaseManager] Min connections cannot exceed max connections");
        return false;
    }

    return true;
}

std::string DatabaseManager::GenerateCacheKey(const QueryParams& params) {
    if (!params.cacheKey.empty()) {
        return params.cacheKey;
    }

    std::string key = params.query;
    for (const auto& param : params.parameters) {
        key += "|" + DatabaseUtils::ValueToString(param);
    }

    return std::to_string(std::hash<std::string>{}(key));
}

std::optional<QueryResult> DatabaseManager::GetCachedResult(const std::string& cacheKey) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_queryCache.find(cacheKey);
    if (it != m_queryCache.end()) {
        return it->second;
    }
    return std::nullopt;
}

void DatabaseManager::CacheResult(const std::string& cacheKey, const QueryResult& result) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // Remove oldest entries if cache is full
    while (m_queryCache.size() >= m_maxCacheSize && !m_cacheOrder.empty()) {
        std::string oldKey = m_cacheOrder.front();
        m_cacheOrder.pop();
        m_queryCache.erase(oldKey);
    }

    m_queryCache[cacheKey] = result;
    m_cacheOrder.push(cacheKey);
}

std::shared_ptr<IConnectionPool> DatabaseManager::CreateConnectionPool(const DatabaseConfig& config) {
    return std::make_shared<SimpleConnectionPool>(config);
}

void DatabaseManager::NotifyEvent(const DatabaseEvent& event) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    if (m_eventCallback) {
        try {
            m_eventCallback(event);
        } catch (const std::exception& ex) {
            // spdlog::error("[DatabaseManager] Event callback exception: {}", ex.what());
        }
    }
}

uint64_t DatabaseManager::GenerateQueryId() {
    return m_nextQueryId++;
}

uint64_t DatabaseManager::GenerateTransactionId() {
    return m_nextTransactionId++;
}

void DatabaseManager::RecordQueryPerformance(const std::string& query, std::chrono::microseconds duration) {
    std::lock_guard<std::mutex> lock(m_statsMutex);

    // Extract query type for statistics
    std::string queryType = "UNKNOWN";
    std::string upperQuery = query;
    std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);

    if (upperQuery.find("SELECT") == 0) queryType = "SELECT";
    else if (upperQuery.find("INSERT") == 0) queryType = "INSERT";
    else if (upperQuery.find("UPDATE") == 0) queryType = "UPDATE";
    else if (upperQuery.find("DELETE") == 0) queryType = "DELETE";

    m_queryStats[queryType]++;

    // Track slow queries (> 1 second)
    if (duration.count() > 1000000) {
        QueryResult slowQuery;
        slowQuery.query = query;
        slowQuery.executionTime = duration;
        slowQuery.status = QueryStatus::Success;

        m_slowQueries.push_back(slowQuery);
        if (m_slowQueries.size() > 100) {
            m_slowQueries.erase(m_slowQueries.begin());
        }
    }
}

void DatabaseManager::MaintenanceLoop() {
    while (!m_shouldStop) {
        try {
            CleanupConnections();
            std::this_thread::sleep_for(std::chrono::seconds(60));
        } catch (const std::exception& ex) {
            // spdlog::error("[DatabaseManager] Maintenance loop error: {}", ex.what());
        }
    }
}

void DatabaseManager::CleanupConnections() {
    std::lock_guard<std::recursive_mutex> lock(m_connectionMutex);

    for (auto& [name, pool] : m_connectionPools) {
        // Connection pool cleanup is handled internally
        // This is where we could add additional cleanup logic
    }
}

QueryType DatabaseManager::DeduceQueryType(const std::string& query) {
    std::string upperQuery = query;
    std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);

    if (upperQuery.find("SELECT") == 0) return QueryType::Select;
    if (upperQuery.find("INSERT") == 0) return QueryType::Insert;
    if (upperQuery.find("UPDATE") == 0) return QueryType::Update;
    if (upperQuery.find("DELETE") == 0) return QueryType::Delete;
    if (upperQuery.find("CREATE TABLE") != std::string::npos) return QueryType::CreateTable;
    if (upperQuery.find("DROP TABLE") != std::string::npos) return QueryType::DropTable;
    if (upperQuery.find("CREATE INDEX") != std::string::npos) return QueryType::CreateIndex;
    if (upperQuery.find("DROP INDEX") != std::string::npos) return QueryType::DropIndex;
    if (upperQuery.find("BEGIN") == 0 || upperQuery.find("COMMIT") == 0 || upperQuery.find("ROLLBACK") == 0) {
        return QueryType::Transaction;
    }

    return QueryType::Custom;
}

// Utility functions implementation
namespace DatabaseUtils {
    std::string ValueToString(const DatabaseValue& value) {
        return std::visit([](const auto& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return "NULL";
            } else if constexpr (std::is_same_v<T, bool>) {
                return v ? "true" : "false";
            } else if constexpr (std::is_arithmetic_v<T>) {
                return std::to_string(v);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return v;
            } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                return "<BLOB:" + std::to_string(v.size()) + ">";
            }
            return "UNKNOWN";
        }, value);
    }
}

} // namespace CoopNet
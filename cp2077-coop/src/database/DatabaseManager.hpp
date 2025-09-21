#pragma once

#include <RED4ext/RED4ext.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include <string>
#include <variant>
#include <optional>
#include <future>

namespace CoopNet {

// Database value types
using DatabaseValue = std::variant<std::nullptr_t, bool, int32_t, uint32_t, int64_t, uint64_t, float, double, std::string, std::vector<uint8_t>>;

// Database backend types
enum class DatabaseType : uint8_t {
    SQLite = 0,         // Local SQLite database
    MySQL = 1,          // MySQL/MariaDB
    PostgreSQL = 2,     // PostgreSQL
    Redis = 3,          // Redis cache
    MongoDB = 4,        // MongoDB document store
    Memory = 5          // In-memory database for testing
};

// Query types
enum class QueryType : uint8_t {
    Select = 0,
    Insert = 1,
    Update = 2,
    Delete = 3,
    CreateTable = 4,
    DropTable = 5,
    CreateIndex = 6,
    DropIndex = 7,
    Transaction = 8,
    Custom = 9
};

// Transaction isolation levels
enum class IsolationLevel : uint8_t {
    ReadUncommitted = 0,
    ReadCommitted = 1,
    RepeatableRead = 2,
    Serializable = 3
};

// Database connection state (renamed to avoid conflict with net/Connection.hpp)
enum class DatabaseConnectionState : uint8_t {
    Disconnected = 0,
    Connecting = 1,
    Connected = 2,
    Error = 3,
    Timeout = 4
};

// Query result status
enum class QueryStatus : uint8_t {
    Success = 0,
    Failed = 1,
    Timeout = 2,
    Cancelled = 3,
    NoResults = 4,
    PartialResults = 5
};

// Forward declarations
struct DatabaseConfig;
struct ConnectionInfo;
struct QueryResult;
struct QueryParams;
struct Transaction;
struct DatabaseSchema;
struct TableDefinition;
struct IndexDefinition;

// Database configuration
struct DatabaseConfig {
    DatabaseType type = DatabaseType::SQLite;
    std::string host = "localhost";
    uint16_t port = 0;
    std::string database = "coopnet.db";
    std::string username;
    std::string password;
    std::string connectionString;

    // Connection pooling
    uint32_t minConnections = 1;
    uint32_t maxConnections = 10;
    uint32_t maxIdleTime = 300; // seconds
    uint32_t connectionTimeout = 30; // seconds
    uint32_t queryTimeout = 60; // seconds

    // Performance settings
    uint32_t maxRetries = 3;
    uint32_t retryDelay = 1000; // milliseconds
    bool enablePreparedStatements = true;
    bool enableConnectionPooling = true;
    bool enableQueryCaching = true;
    uint32_t queryCacheSize = 1000;

    // Security settings
    bool enableSSL = false;
    std::string sslCertPath;
    std::string sslKeyPath;
    std::string sslCaPath;
    bool verifyServerCert = true;

    // Backup settings
    bool enableAutoBackup = false;
    uint32_t backupInterval = 3600; // seconds
    std::string backupDirectory = "backups/";
    uint32_t maxBackups = 7;
    bool compressBackups = true;
};

// Connection information
struct ConnectionInfo {
    uint64_t connectionId;
    DatabaseType type;
    std::string identifier;
    DatabaseConnectionState state;
    std::chrono::steady_clock::time_point createdAt;
    std::chrono::steady_clock::time_point lastUsed;
    std::chrono::steady_clock::time_point lastError;
    uint64_t queriesExecuted;
    uint64_t totalQueryTime; // microseconds
    bool inTransaction;
    std::string currentDatabase;
    void* nativeHandle; // Database-specific connection handle
};

// Query parameters
struct QueryParams {
    std::string query;
    std::vector<DatabaseValue> parameters;
    QueryType type;
    uint32_t timeout = 0; // 0 = use default
    bool prepared = false;
    bool cached = false;
    std::string cacheKey;
    std::unordered_map<std::string, std::string> hints;
};

// Query result
struct QueryResult {
    QueryStatus status;
    uint64_t queryId;
    std::string query;
    std::vector<std::unordered_map<std::string, DatabaseValue>> rows;
    std::vector<std::string> columnNames;
    std::vector<std::string> columnTypes;
    uint64_t affectedRows;
    uint64_t insertId;
    std::chrono::microseconds executionTime;
    std::string errorMessage;
    std::string warningMessage;
    std::unordered_map<std::string, std::string> metadata;
};

// Transaction context
struct Transaction {
    uint64_t transactionId;
    uint64_t connectionId;
    IsolationLevel isolation;
    std::chrono::steady_clock::time_point startTime;
    std::vector<std::string> queries;
    bool readOnly;
    bool committed;
    bool rolledBack;
    std::string savepoint;
    std::unordered_map<std::string, DatabaseValue> context;
};

// Database schema definitions
struct ColumnDefinition {
    std::string name;
    std::string type;
    bool nullable = true;
    bool primaryKey = false;
    bool autoIncrement = false;
    bool unique = false;
    std::string defaultValue;
    std::string constraints;
    std::string comment;
};

struct TableDefinition {
    std::string name;
    std::vector<ColumnDefinition> columns;
    std::vector<std::string> primaryKeys;
    std::vector<std::string> foreignKeys;
    std::vector<std::string> uniqueConstraints;
    std::vector<std::string> checkConstraints;
    std::string engine;
    std::string charset;
    std::string comment;
    std::unordered_map<std::string, std::string> options;
};

struct IndexDefinition {
    std::string name;
    std::string tableName;
    std::vector<std::string> columns;
    bool unique = false;
    bool clustered = false;
    std::string type; // BTREE, HASH, etc.
    std::string comment;
    std::unordered_map<std::string, std::string> options;
};

struct DatabaseSchema {
    std::string name;
    std::string version;
    std::vector<TableDefinition> tables;
    std::vector<IndexDefinition> indexes;
    std::vector<std::string> views;
    std::vector<std::string> procedures;
    std::vector<std::string> functions;
    std::vector<std::string> triggers;
    std::unordered_map<std::string, std::string> metadata;
};

// Database event callbacks
struct DatabaseEvent {
    enum Type {
        Connected,
        Disconnected,
        QueryExecuted,
        TransactionStarted,
        TransactionCommitted,
        TransactionRolledBack,
        Error,
        Warning,
        SchemaChanged,
        BackupCompleted
    };

    Type type;
    uint64_t connectionId;
    std::chrono::steady_clock::time_point timestamp;
    std::string message;
    std::unordered_map<std::string, DatabaseValue> data;
};

// Connection pool interface
class IConnectionPool {
public:
    virtual ~IConnectionPool() = default;
    virtual std::shared_ptr<ConnectionInfo> AcquireConnection() = 0;
    virtual void ReleaseConnection(std::shared_ptr<ConnectionInfo> connection) = 0;
    virtual bool ValidateConnection(std::shared_ptr<ConnectionInfo> connection) = 0;
    virtual void CloseAllConnections() = 0;
    virtual uint32_t GetActiveConnections() const = 0;
    virtual uint32_t GetIdleConnections() const = 0;
};

// Database adapter interface
class IDatabaseAdapter {
public:
    virtual ~IDatabaseAdapter() = default;
    virtual bool Connect(const DatabaseConfig& config) = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;
    virtual QueryResult ExecuteQuery(const QueryParams& params) = 0;
    virtual std::future<QueryResult> ExecuteQueryAsync(const QueryParams& params) = 0;
    virtual bool BeginTransaction(Transaction& transaction) = 0;
    virtual bool CommitTransaction(const Transaction& transaction) = 0;
    virtual bool RollbackTransaction(const Transaction& transaction) = 0;
    virtual bool CreateSchema(const DatabaseSchema& schema) = 0;
    virtual bool DropSchema(const std::string& schemaName) = 0;
    virtual DatabaseSchema GetSchema(const std::string& schemaName = "") = 0;
    virtual std::string GetLastError() const = 0;
};

// Query builder helper
class QueryBuilder {
public:
    QueryBuilder(QueryType type);

    // SELECT operations
    QueryBuilder& Select(const std::vector<std::string>& columns = {"*"});
    QueryBuilder& From(const std::string& table);
    QueryBuilder& Join(const std::string& table, const std::string& condition, const std::string& type = "INNER");
    QueryBuilder& Where(const std::string& condition);
    QueryBuilder& WhereEquals(const std::string& column, const DatabaseValue& value);
    QueryBuilder& WhereIn(const std::string& column, const std::vector<DatabaseValue>& values);
    QueryBuilder& OrderBy(const std::string& column, bool ascending = true);
    QueryBuilder& GroupBy(const std::vector<std::string>& columns);
    QueryBuilder& Having(const std::string& condition);
    QueryBuilder& Limit(uint32_t count, uint32_t offset = 0);

    // INSERT operations
    QueryBuilder& InsertInto(const std::string& table);
    QueryBuilder& Values(const std::unordered_map<std::string, DatabaseValue>& values);
    QueryBuilder& OnDuplicateKeyUpdate(const std::unordered_map<std::string, DatabaseValue>& values);

    // UPDATE operations
    QueryBuilder& Update(const std::string& table);
    QueryBuilder& Set(const std::unordered_map<std::string, DatabaseValue>& values);

    // DELETE operations
    QueryBuilder& DeleteFrom(const std::string& table);

    // Build query
    QueryParams Build();
    std::string BuildSQL();

private:
    QueryType m_type;
    std::string m_sql;
    std::vector<DatabaseValue> m_parameters;
    std::unordered_map<std::string, std::string> m_hints;

    void AppendCondition(const std::string& condition, const std::string& operator_);
    std::string ValueToSQL(const DatabaseValue& value);
    std::string EscapeIdentifier(const std::string& identifier);
    std::string EscapeString(const std::string& str);
};

// Main database management system
class DatabaseManager {
public:
    static DatabaseManager& Instance();

    // System lifecycle
    bool Initialize(const DatabaseConfig& config);
    void Shutdown();

    // Connection management
    bool Connect(const std::string& connectionName = "default", const DatabaseConfig& config = {});
    bool Disconnect(const std::string& connectionName = "default");
    bool IsConnected(const std::string& connectionName = "default") const;
    std::vector<std::string> GetConnectionNames() const;

    // Query execution
    QueryResult ExecuteQuery(const std::string& query, const std::vector<DatabaseValue>& params = {},
                           const std::string& connectionName = "default");
    QueryResult ExecuteQuery(const QueryParams& params, const std::string& connectionName = "default");
    std::future<QueryResult> ExecuteQueryAsync(const QueryParams& params, const std::string& connectionName = "default");

    // Prepared statements
    bool PrepareStatement(const std::string& name, const std::string& query, const std::string& connectionName = "default");
    QueryResult ExecutePrepared(const std::string& name, const std::vector<DatabaseValue>& params = {},
                              const std::string& connectionName = "default");
    bool DropPreparedStatement(const std::string& name, const std::string& connectionName = "default");

    // Transaction management
    bool BeginTransaction(const std::string& connectionName = "default", IsolationLevel isolation = IsolationLevel::ReadCommitted);
    bool CommitTransaction(const std::string& connectionName = "default");
    bool RollbackTransaction(const std::string& connectionName = "default");
    bool IsInTransaction(const std::string& connectionName = "default") const;
    bool CreateSavepoint(const std::string& name, const std::string& connectionName = "default");
    bool RollbackToSavepoint(const std::string& name, const std::string& connectionName = "default");

    // Schema management
    bool CreateDatabase(const std::string& databaseName, const std::string& connectionName = "default");
    bool DropDatabase(const std::string& databaseName, const std::string& connectionName = "default");
    bool CreateTable(const TableDefinition& table, const std::string& connectionName = "default");
    bool DropTable(const std::string& tableName, const std::string& connectionName = "default");
    bool CreateIndex(const IndexDefinition& index, const std::string& connectionName = "default");
    bool DropIndex(const std::string& indexName, const std::string& connectionName = "default");

    // Schema introspection
    std::vector<std::string> GetTables(const std::string& connectionName = "default");
    std::vector<std::string> GetColumns(const std::string& tableName, const std::string& connectionName = "default");
    std::vector<std::string> GetIndexes(const std::string& tableName, const std::string& connectionName = "default");
    TableDefinition GetTableDefinition(const std::string& tableName, const std::string& connectionName = "default");
    DatabaseSchema GetSchema(const std::string& connectionName = "default");

    // Data operations helpers
    QueryResult Insert(const std::string& tableName, const std::unordered_map<std::string, DatabaseValue>& data,
                     const std::string& connectionName = "default");
    QueryResult Update(const std::string& tableName, const std::unordered_map<std::string, DatabaseValue>& data,
                     const std::string& whereClause, const std::vector<DatabaseValue>& whereParams = {},
                     const std::string& connectionName = "default");
    QueryResult Delete(const std::string& tableName, const std::string& whereClause,
                     const std::vector<DatabaseValue>& whereParams = {}, const std::string& connectionName = "default");
    QueryResult Select(const std::string& tableName, const std::vector<std::string>& columns = {"*"},
                     const std::string& whereClause = "", const std::vector<DatabaseValue>& whereParams = {},
                     const std::string& orderBy = "", uint32_t limit = 0, uint32_t offset = 0,
                     const std::string& connectionName = "default");

    // Batch operations
    bool ExecuteBatch(const std::vector<QueryParams>& queries, const std::string& connectionName = "default");
    bool InsertBatch(const std::string& tableName, const std::vector<std::unordered_map<std::string, DatabaseValue>>& data,
                   const std::string& connectionName = "default");

    // Query caching
    void EnableQueryCache(bool enabled, uint32_t maxSize = 1000);
    void ClearQueryCache();
    void InvalidateCache(const std::string& pattern = "");

    // Backup and restore
    bool CreateBackup(const std::string& backupPath, const std::string& connectionName = "default");
    bool RestoreBackup(const std::string& backupPath, const std::string& connectionName = "default");
    std::vector<std::string> GetBackupList() const;
    bool DeleteBackup(const std::string& backupName);

    // Performance monitoring
    void EnablePerformanceMonitoring(bool enabled);
    std::unordered_map<std::string, uint64_t> GetQueryStatistics() const;
    std::vector<QueryResult> GetSlowQueries(uint32_t count = 10) const;
    void ResetStatistics();

    // Event handling
    using DatabaseEventCallback = std::function<void(const DatabaseEvent&)>;
    void RegisterEventCallback(DatabaseEventCallback callback);
    void UnregisterEventCallback();

    // Utility functions
    std::string EscapeString(const std::string& str, const std::string& connectionName = "default");
    std::string QuoteIdentifier(const std::string& identifier, const std::string& connectionName = "default");
    bool TestConnection(const DatabaseConfig& config);
    std::string GetDatabaseVersion(const std::string& connectionName = "default");

    // Migration system
    bool ExecuteMigration(const std::string& migrationScript, const std::string& connectionName = "default");
    bool RollbackMigration(const std::string& migrationScript, const std::string& connectionName = "default");
    std::vector<std::string> GetMigrationHistory(const std::string& connectionName = "default");

private:
    DatabaseManager() = default;
    ~DatabaseManager() = default;
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // Core operations
    std::shared_ptr<IDatabaseAdapter> GetAdapter(const std::string& connectionName);
    std::shared_ptr<ConnectionInfo> GetConnection(const std::string& connectionName);
    bool ValidateConfig(const DatabaseConfig& config);

    // Cache management
    std::string GenerateCacheKey(const QueryParams& params);
    std::optional<QueryResult> GetCachedResult(const std::string& cacheKey);
    void CacheResult(const std::string& cacheKey, const QueryResult& result);

    // Connection pooling
    std::shared_ptr<IConnectionPool> CreateConnectionPool(const DatabaseConfig& config);
    void CleanupConnections();

    // Event processing
    void NotifyEvent(const DatabaseEvent& event);
    uint64_t GenerateQueryId();
    uint64_t GenerateTransactionId();

    // Performance tracking
    void RecordQueryPerformance(const std::string& query, std::chrono::microseconds duration);
    void UpdateConnectionStats(const std::string& connectionName, uint64_t queryTime);

    // Backup utilities
    std::string GenerateBackupName() const;
    bool CompressBackup(const std::string& sourcePath, const std::string& destPath);
    bool DecompressBackup(const std::string& sourcePath, const std::string& destPath);

    // Data storage
    std::unordered_map<std::string, std::shared_ptr<IDatabaseAdapter>> m_adapters;
    std::unordered_map<std::string, std::shared_ptr<IConnectionPool>> m_connectionPools;
    std::unordered_map<std::string, DatabaseConfig> m_configurations;
    std::unordered_map<std::string, Transaction> m_activeTransactions;

    // Query cache
    std::unordered_map<std::string, QueryResult> m_queryCache;
    std::queue<std::string> m_cacheOrder;
    uint32_t m_maxCacheSize = 1000;
    bool m_queryCacheEnabled = true;

    // Performance tracking
    std::unordered_map<std::string, uint64_t> m_queryStats;
    std::vector<QueryResult> m_slowQueries;
    bool m_performanceMonitoringEnabled = false;

    // Event handling
    DatabaseEventCallback m_eventCallback;

    // Synchronization
    mutable std::recursive_mutex m_connectionMutex;
    mutable std::mutex m_cacheMutex;
    mutable std::mutex m_statsMutex;
    mutable std::mutex m_eventMutex;

    // System state
    bool m_initialized = false;
    std::string m_defaultConnection = "default";

    // ID generation
    std::atomic<uint64_t> m_nextQueryId{1};
    std::atomic<uint64_t> m_nextTransactionId{1};

    // Prepared statements
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_preparedStatements;

    // Background tasks
    std::thread m_maintenanceThread;
    std::atomic<bool> m_shouldStop{false};
    void MaintenanceLoop();
};

// Database connection RAII helper
class DatabaseConnection {
public:
    DatabaseConnection(const std::string& connectionName = "default");
    ~DatabaseConnection();

    bool IsValid() const;
    QueryResult Execute(const std::string& query, const std::vector<DatabaseValue>& params = {});
    QueryResult Execute(const QueryParams& params);

    bool BeginTransaction(IsolationLevel isolation = IsolationLevel::ReadCommitted);
    bool Commit();
    bool Rollback();

private:
    std::string m_connectionName;
    bool m_ownsTransaction = false;
    bool m_valid = false;
};

// Utility functions for database operations
namespace DatabaseUtils {
    std::string GetTypeName(DatabaseType type);
    std::string GetQueryTypeName(QueryType type);
    std::string GetIsolationLevelName(IsolationLevel level);
    std::string GetConnectionStateName(DatabaseConnectionState state);

    // Value conversion utilities
    template<typename T>
    std::optional<T> ConvertValue(const DatabaseValue& value);

    std::string ValueToString(const DatabaseValue& value);
    DatabaseValue StringToValue(const std::string& str, const std::string& type);

    // SQL utilities
    bool IsValidIdentifier(const std::string& identifier);
    std::string SanitizeIdentifier(const std::string& identifier);
    std::vector<std::string> ParseSQL(const std::string& sql);

    // Schema validation
    bool ValidateTableDefinition(const TableDefinition& table);
    bool ValidateIndexDefinition(const IndexDefinition& index);
    std::vector<std::string> GetTableDependencies(const DatabaseSchema& schema);
}

} // namespace CoopNet
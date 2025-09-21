#pragma once

#include <RED4ext/RED4ext.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <memory>
#include <chrono>
#include <queue>
#include <functional>
#include <thread>
#include <atomic>

namespace CoopNet {

// Asset types for streaming prioritization
enum class AssetType : uint8_t {
    Texture = 0,
    Mesh = 1,
    Audio = 2,
    Animation = 3,
    Material = 4,
    Script = 5,
    World = 6,
    Character = 7,
    Vehicle = 8,
    Weapon = 9,
    Effect = 10,
    UI = 11,
    Config = 12,
    Custom = 13,
    Unknown = 255
};

// Asset priority levels for streaming
enum class AssetPriority : uint8_t {
    Critical = 0,    // Player models, current area
    High = 1,        // Nearby objects, weapons
    Medium = 2,      // Background objects, distant NPCs
    Low = 3,         // Far objects, ambient effects
    Background = 4,  // Preload for next areas
    Disabled = 255   // Not loaded
};

// Asset streaming state
enum class AssetStreamState : uint8_t {
    Unloaded = 0,
    Requested = 1,
    Downloading = 2,
    Loading = 3,
    Loaded = 4,
    Failed = 5,
    Evicted = 6
};

// Asset compression methods
enum class CompressionType : uint8_t {
    None = 0,
    LZ4 = 1,
    ZSTD = 2,
    Custom = 3
};

// Asset sync mode between players
enum class AssetSyncMode : uint8_t {
    Mandatory = 0,   // All players must have this asset
    Optional = 1,    // Asset is optional for gameplay
    Conditional = 2, // Required only in certain conditions
    ClientSide = 3   // Local client decides
};

// Forward declarations
struct AssetInfo;
struct AssetChunk;
struct AssetRequest;
struct AssetCacheEntry;
struct AssetStreamingStats;

// Asset information structure
struct AssetInfo {
    uint64_t assetId;
    std::string assetPath;
    AssetType type;
    AssetPriority priority;
    AssetSyncMode syncMode;
    uint64_t fileSize;
    uint64_t compressedSize;
    CompressionType compression;
    uint32_t version;
    uint64_t lastModified;
    uint32_t chunkCount;
    std::vector<uint64_t> chunkHashes;
    uint32_t refCount;
    bool isCustomAsset;
    std::string metadata;
};

// Asset chunk for streaming
struct AssetChunk {
    uint64_t assetId;
    uint32_t chunkIndex;
    uint32_t chunkSize;
    uint32_t compressedSize;
    uint64_t chunkHash;
    std::vector<uint8_t> data;
    bool isCompressed;
    std::chrono::steady_clock::time_point requestTime;
};

// Asset request tracking
struct AssetRequest {
    uint64_t requestId;
    uint64_t assetId;
    uint32_t playerId;
    AssetPriority priority;
    std::chrono::steady_clock::time_point requestTime;
    std::chrono::steady_clock::time_point deadline;
    uint32_t retryCount;
    bool isActive;
    std::function<void(bool)> callback;
};

// Cache entry for loaded assets
struct AssetCacheEntry {
    uint64_t assetId;
    AssetStreamState state;
    std::shared_ptr<AssetInfo> info;
    std::vector<std::shared_ptr<AssetChunk>> chunks;
    uint64_t memoryUsage;
    std::chrono::steady_clock::time_point lastAccess;
    std::chrono::steady_clock::time_point loadTime;
    uint32_t accessCount;
    bool isPinned;
    std::mutex chunkMutex;
};

// Streaming performance statistics
struct AssetStreamingStats {
    uint64_t totalAssetsRegistered = 0;
    uint64_t totalAssetsLoaded = 0;
    uint64_t totalAssetsFailed = 0;
    uint64_t totalBytesStreamed = 0;
    uint64_t totalBytesCompressed = 0;
    uint64_t currentMemoryUsage = 0;
    uint64_t peakMemoryUsage = 0;
    uint32_t activeRequests = 0;
    uint32_t activeDownloads = 0;
    float averageDownloadSpeed = 0.0f;
    float cacheHitRatio = 0.0f;
    uint32_t evictionsPerMinute = 0;
    std::chrono::steady_clock::time_point lastStatsUpdate;
};

// Asset streaming bandwidth management
struct BandwidthManager {
    uint64_t maxBandwidthBytesPerSecond = 10 * 1024 * 1024; // 10 MB/s default
    uint64_t currentBandwidthUsage = 0;
    std::chrono::steady_clock::time_point lastBandwidthReset;
    std::queue<std::pair<std::chrono::steady_clock::time_point, uint64_t>> bandwidthHistory;
    uint32_t priorityWeights[5] = {100, 75, 50, 25, 10}; // Critical to Background
};

// Asset streaming system
class AssetSyncManager {
public:
    static AssetSyncManager& Instance();

    // System lifecycle
    bool Initialize();
    void Shutdown();
    void Tick(float deltaTime);

    // Asset registration and management
    bool RegisterAsset(const AssetInfo& assetInfo);
    bool UnregisterAsset(uint64_t assetId);
    std::shared_ptr<AssetInfo> GetAssetInfo(uint64_t assetId) const;
    std::vector<uint64_t> GetRegisteredAssets() const;

    // Asset streaming requests
    bool RequestAsset(uint64_t assetId, uint32_t playerId, AssetPriority priority,
                     std::function<void(bool)> callback = nullptr);
    bool CancelAssetRequest(uint64_t requestId);
    bool IsAssetLoaded(uint64_t assetId) const;
    AssetStreamState GetAssetState(uint64_t assetId) const;

    // Asset data access
    std::shared_ptr<AssetCacheEntry> GetAsset(uint64_t assetId);
    bool GetAssetData(uint64_t assetId, std::vector<uint8_t>& outData);
    bool GetAssetChunk(uint64_t assetId, uint32_t chunkIndex, std::vector<uint8_t>& outData);

    // Priority and memory management
    bool SetAssetPriority(uint64_t assetId, AssetPriority priority);
    bool PinAsset(uint64_t assetId, bool pin = true);
    void EvictLowPriorityAssets(uint64_t targetMemoryReduction);
    void SetMemoryLimit(uint64_t maxMemoryBytes);

    // Custom asset support
    bool RegisterCustomAsset(const std::string& modName, const std::string& assetPath,
                           const std::vector<uint8_t>& assetData, AssetType type);
    bool LoadCustomAssetsFromMod(const std::string& modPath);
    std::vector<std::string> GetLoadedMods() const;

    // Bandwidth and performance
    void SetBandwidthLimit(uint64_t bytesPerSecond);
    uint64_t GetCurrentBandwidthUsage() const;
    AssetStreamingStats GetStreamingStats() const;
    void ResetStats();

    // Player synchronization
    bool SynchronizeAssetWithPlayers(uint64_t assetId, const std::vector<uint32_t>& playerIds);
    bool BroadcastAssetUpdate(uint64_t assetId);
    void ProcessPlayerAssetRequests(uint32_t playerId);

    // Area-based preloading
    bool PreloadAssetsForArea(const std::string& areaName, AssetPriority priority);
    bool UnloadAssetsForArea(const std::string& areaName);
    void RegisterAreaAssets(const std::string& areaName, const std::vector<uint64_t>& assetIds);

    // Asset validation and integrity
    bool ValidateAsset(uint64_t assetId);
    bool RepairCorruptedAsset(uint64_t assetId);
    uint32_t GetAssetChecksum(uint64_t assetId);

    // Event callbacks
    using AssetEventCallback = std::function<void(uint64_t assetId, AssetStreamState state, const std::string& eventData)>;
    void RegisterEventCallback(const std::string& eventType, AssetEventCallback callback);
    void UnregisterEventCallback(const std::string& eventType);

private:
    AssetSyncManager() = default;
    ~AssetSyncManager() = default;
    AssetSyncManager(const AssetSyncManager&) = delete;
    AssetSyncManager& operator=(const AssetSyncManager&) = delete;

    // Core processing methods
    void ProcessAssetRequests();
    void ProcessDownloads();
    void ProcessCacheManagement();
    void ProcessBandwidthManagement();
    void UpdateStatistics();

    // Asset loading and streaming
    bool LoadAssetFromDisk(uint64_t assetId);
    bool LoadAssetFromNetwork(uint64_t assetId, uint32_t playerId);
    bool LoadAssetChunk(uint64_t assetId, uint32_t chunkIndex);
    bool DecompressAssetData(const std::vector<uint8_t>& compressedData,
                           std::vector<uint8_t>& decompressedData, CompressionType compression);

    // Cache management
    bool AddToCache(uint64_t assetId, std::shared_ptr<AssetCacheEntry> entry);
    bool RemoveFromCache(uint64_t assetId);
    void EvictLeastRecentlyUsed();
    void EvictByPriority(AssetPriority maxPriority);
    uint64_t CalculateMemoryUsage() const;

    // Request management
    uint64_t GenerateRequestId();
    bool AddAssetRequest(const AssetRequest& request);
    bool RemoveAssetRequest(uint64_t requestId);
    std::vector<AssetRequest> GetPendingRequests(AssetPriority minPriority) const;

    // Network communication
    bool SendAssetRequest(uint64_t assetId, uint32_t targetPlayerId, AssetPriority priority);
    bool SendAssetChunk(const AssetChunk& chunk, uint32_t targetPlayerId);
    bool BroadcastAssetAvailable(uint64_t assetId);

    // Asset file I/O
    bool ReadAssetFromFile(const std::string& filePath, std::vector<uint8_t>& data);
    bool WriteAssetToFile(const std::string& filePath, const std::vector<uint8_t>& data);
    bool CreateAssetChunks(uint64_t assetId, const std::vector<uint8_t>& data,
                          uint32_t chunkSize = 64 * 1024); // 64KB default chunks

    // Utility methods
    AssetType DetermineAssetType(const std::string& filePath);
    uint64_t GenerateAssetId(const std::string& assetPath);
    uint64_t CalculateDataHash(const std::vector<uint8_t>& data);
    std::string GetAssetCachePath(uint64_t assetId);
    bool IsMemoryLimitExceeded() const;
    void TriggerEvent(const std::string& eventType, uint64_t assetId, AssetStreamState state, const std::string& data);
    uint64_t GetCurrentTimestamp() const;

    // Data storage
    std::unordered_map<uint64_t, std::shared_ptr<AssetInfo>> m_assetRegistry;
    std::unordered_map<uint64_t, std::shared_ptr<AssetCacheEntry>> m_assetCache;
    std::unordered_map<uint64_t, AssetRequest> m_pendingRequests;
    std::unordered_map<std::string, std::vector<uint64_t>> m_areaAssets;
    std::unordered_map<std::string, std::string> m_loadedMods; // modName -> modPath

    // Synchronization
    mutable std::mutex m_registryMutex;
    mutable std::mutex m_cacheMutex;
    mutable std::mutex m_requestMutex;
    mutable std::mutex m_statsMutex;

    // System state
    bool m_initialized = false;
    AssetStreamingStats m_stats;
    BandwidthManager m_bandwidthManager;

    // Configuration
    struct Config {
        uint64_t maxMemoryBytes = 2ULL * 1024 * 1024 * 1024; // 2GB default
        uint64_t maxCacheEntries = 10000;
        uint32_t maxConcurrentDownloads = 8;
        uint32_t chunkSize = 64 * 1024; // 64KB
        uint32_t maxRetries = 3;
        uint32_t requestTimeoutSeconds = 30;
        float evictionThreshold = 0.85f; // Start evicting at 85% memory usage
        bool enableCompression = true;
        bool enableCustomAssets = true;
        bool enableDiskCache = true;
        std::string cachePath = "cache/assets/";
        std::string customAssetsPath = "mods/";
    } m_config;

    // Background processing
    std::thread m_processingThread;
    std::atomic<bool> m_shouldStop{false};

    // Timing
    float m_requestProcessTimer = 0.0f;
    float m_downloadProcessTimer = 0.0f;
    float m_cacheManagementTimer = 0.0f;
    float m_bandwidthTimer = 0.0f;
    float m_statsTimer = 0.0f;

    // Request ID generation
    std::atomic<uint64_t> m_nextRequestId{1};

    // Event system
    std::unordered_map<std::string, std::vector<AssetEventCallback>> m_eventCallbacks;
    std::mutex m_callbackMutex;
};

// Asset streaming network packets
struct AssetRequestPacket {
    uint64_t requestId;
    uint64_t assetId;
    uint32_t playerId;
    uint8_t priority;
    uint64_t timestamp;
};

struct AssetResponsePacket {
    uint64_t requestId;
    uint64_t assetId;
    uint8_t responseCode; // 0=success, 1=not_found, 2=denied, 3=error
    uint32_t totalChunks;
    uint64_t totalSize;
    uint64_t timestamp;
};

struct AssetChunkPacket {
    uint64_t assetId;
    uint32_t chunkIndex;
    uint32_t chunkSize;
    uint32_t compressedSize;
    uint64_t chunkHash;
    uint8_t compressionType;
    // Followed by chunk data
};

struct AssetAvailablePacket {
    uint64_t assetId;
    uint8_t assetType;
    uint8_t priority;
    uint64_t fileSize;
    uint32_t version;
    uint64_t timestamp;
};

// Utility functions for asset management
namespace AssetUtils {
    std::string GetAssetTypeName(AssetType type);
    std::string GetAssetPriorityName(AssetPriority priority);
    std::string GetAssetStateName(AssetStreamState state);
    bool IsAssetPathValid(const std::string& path);
    AssetType GetAssetTypeFromExtension(const std::string& extension);
    uint32_t CalculateChunkCount(uint64_t fileSize, uint32_t chunkSize);
    bool IsAssetSizeReasonable(uint64_t size, AssetType type);
}

} // namespace CoopNet
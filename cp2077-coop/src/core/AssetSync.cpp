#include "AssetSync.hpp"
#include "Logger.hpp"
#include "../net/Net.hpp"
#include "../../third_party/zstd/zstd.h"
#include <chrono>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <random>

namespace CoopNet {

AssetSyncManager& AssetSyncManager::Instance() {
    static AssetSyncManager instance;
    return instance;
}

bool AssetSyncManager::Initialize() {
    if (m_initialized) {
        return true;
    }

    Logger::Log(LogLevel::INFO, "[AssetSyncManager] Initializing asset streaming system");

    // Initialize data structures
    m_assetRegistry.clear();
    m_assetCache.clear();
    m_pendingRequests.clear();
    m_areaAssets.clear();
    m_loadedMods.clear();

    // Reset statistics
    m_stats = AssetStreamingStats{};
    m_stats.lastStatsUpdate = std::chrono::steady_clock::now();

    // Initialize bandwidth manager
    m_bandwidthManager.lastBandwidthReset = std::chrono::steady_clock::now();

    // Create cache directory if it doesn't exist
    try {
        std::filesystem::create_directories(m_config.cachePath);
        std::filesystem::create_directories(m_config.customAssetsPath);
    } catch (const std::exception& e) {
        Logger::Log(LogLevel::WARNING, "[AssetSyncManager] Failed to create directories: " + std::string(e.what()));
    }

    // Start background processing thread
    m_shouldStop = false;
    m_processingThread = std::thread([this]() {
        while (!m_shouldStop) {
            try {
                ProcessAssetRequests();
                ProcessDownloads();
                ProcessCacheManagement();
                ProcessBandwidthManagement();
                UpdateStatistics();

                std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 20 FPS processing
            } catch (const std::exception& e) {
                Logger::Log(LogLevel::ERROR, "[AssetSyncManager] Background processing error: " + std::string(e.what()));
            }
        }
    });

    m_initialized = true;
    Logger::Log(LogLevel::INFO, "[AssetSyncManager] Asset streaming system initialized successfully");
    return true;
}

void AssetSyncManager::Shutdown() {
    if (!m_initialized) {
        return;
    }

    Logger::Log(LogLevel::INFO, "[AssetSyncManager] Shutting down asset streaming system");

    // Stop background processing
    m_shouldStop = true;
    if (m_processingThread.joinable()) {
        m_processingThread.join();
    }

    // Clear all data structures
    {
        std::lock_guard<std::mutex> registryLock(m_registryMutex);
        std::lock_guard<std::mutex> cacheLock(m_cacheMutex);
        std::lock_guard<std::mutex> requestLock(m_requestMutex);

        m_assetRegistry.clear();
        m_assetCache.clear();
        m_pendingRequests.clear();
        m_areaAssets.clear();
        m_loadedMods.clear();
    }

    m_initialized = false;
    Logger::Log(LogLevel::INFO, "[AssetSyncManager] Asset streaming system shutdown complete");
}

void AssetSyncManager::Tick(float deltaTime) {
    if (!m_initialized) {
        return;
    }

    // Update timers
    m_requestProcessTimer += deltaTime;
    m_downloadProcessTimer += deltaTime;
    m_cacheManagementTimer += deltaTime;
    m_bandwidthTimer += deltaTime;
    m_statsTimer += deltaTime;

    // Periodic cleanup and maintenance (every 5 seconds)
    if (m_cacheManagementTimer >= 5.0f) {
        // Check if memory limit is exceeded
        if (IsMemoryLimitExceeded()) {
            EvictLowPriorityAssets(m_config.maxMemoryBytes * 0.1f); // Free 10% of memory
        }

        m_cacheManagementTimer = 0.0f;
    }

    // Statistics update (every 10 seconds)
    if (m_statsTimer >= 10.0f) {
        UpdateStatistics();
        m_statsTimer = 0.0f;
    }
}

bool AssetSyncManager::RegisterAsset(const AssetInfo& assetInfo) {
    if (assetInfo.assetId == 0 || assetInfo.assetPath.empty()) {
        Logger::Log(LogLevel::ERROR, "[AssetSyncManager] Invalid asset info provided");
        return false;
    }

    std::lock_guard<std::mutex> lock(m_registryMutex);

    // Check if asset already exists
    auto it = m_assetRegistry.find(assetInfo.assetId);
    if (it != m_assetRegistry.end()) {
        // Update if this is a newer version
        if (assetInfo.version > it->second->version) {
            it->second = std::make_shared<AssetInfo>(assetInfo);
            Logger::Log(LogLevel::INFO, "[AssetSyncManager] Updated asset " + std::to_string(assetInfo.assetId) +
                       " to version " + std::to_string(assetInfo.version));
        } else {
            Logger::Log(LogLevel::WARNING, "[AssetSyncManager] Attempted to register older version of asset " +
                       std::to_string(assetInfo.assetId));
            return false;
        }
    } else {
        m_assetRegistry[assetInfo.assetId] = std::make_shared<AssetInfo>(assetInfo);
        m_stats.totalAssetsRegistered++;
        Logger::Log(LogLevel::INFO, "[AssetSyncManager] Registered new asset " +
                   std::to_string(assetInfo.assetId) + " (" + assetInfo.assetPath + ")");
    }

    // Trigger event
    TriggerEvent("asset_registered", assetInfo.assetId, AssetStreamState::Unloaded,
                "type:" + AssetUtils::GetAssetTypeName(assetInfo.type));

    return true;
}

bool AssetSyncManager::UnregisterAsset(uint64_t assetId) {
    std::lock_guard<std::mutex> registryLock(m_registryMutex);
    std::lock_guard<std::mutex> cacheLock(m_cacheMutex);

    // Remove from registry
    auto registryIt = m_assetRegistry.find(assetId);
    if (registryIt == m_assetRegistry.end()) {
        return false;
    }

    // Remove from cache if loaded
    auto cacheIt = m_assetCache.find(assetId);
    if (cacheIt != m_assetCache.end()) {
        m_assetCache.erase(cacheIt);
    }

    m_assetRegistry.erase(registryIt);

    Logger::Log(LogLevel::INFO, "[AssetSyncManager] Unregistered asset " + std::to_string(assetId));
    TriggerEvent("asset_unregistered", assetId, AssetStreamState::Unloaded, "");
    return true;
}

std::shared_ptr<AssetInfo> AssetSyncManager::GetAssetInfo(uint64_t assetId) const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    auto it = m_assetRegistry.find(assetId);
    return (it != m_assetRegistry.end()) ? it->second : nullptr;
}

std::vector<uint64_t> AssetSyncManager::GetRegisteredAssets() const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    std::vector<uint64_t> assetIds;
    assetIds.reserve(m_assetRegistry.size());

    for (const auto& [assetId, info] : m_assetRegistry) {
        assetIds.push_back(assetId);
    }

    return assetIds;
}

bool AssetSyncManager::RequestAsset(uint64_t assetId, uint32_t playerId, AssetPriority priority,
                                   std::function<void(bool)> callback) {
    if (assetId == 0) {
        Logger::Log(LogLevel::ERROR, "[AssetSyncManager] Invalid asset ID in request");
        if (callback) callback(false);
        return false;
    }

    // Check if asset is already loaded
    {
        std::lock_guard<std::mutex> cacheLock(m_cacheMutex);
        auto cacheIt = m_assetCache.find(assetId);
        if (cacheIt != m_assetCache.end() && cacheIt->second->state == AssetStreamState::Loaded) {
            // Update access time
            cacheIt->second->lastAccess = std::chrono::steady_clock::now();
            cacheIt->second->accessCount++;

            if (callback) callback(true);
            Logger::Log(LogLevel::DEBUG, "[AssetSyncManager] Asset " + std::to_string(assetId) + " served from cache");
            return true;
        }
    }

    // Check if asset exists in registry
    auto assetInfo = GetAssetInfo(assetId);
    if (!assetInfo) {
        Logger::Log(LogLevel::ERROR, "[AssetSyncManager] Requested asset " + std::to_string(assetId) + " not found in registry");
        if (callback) callback(false);
        return false;
    }

    // Create request
    AssetRequest request;
    request.requestId = GenerateRequestId();
    request.assetId = assetId;
    request.playerId = playerId;
    request.priority = priority;
    request.requestTime = std::chrono::steady_clock::now();
    request.deadline = request.requestTime + std::chrono::seconds(m_config.requestTimeoutSeconds);
    request.retryCount = 0;
    request.isActive = true;
    request.callback = callback;

    // Add to pending requests
    bool added = AddAssetRequest(request);
    if (added) {
        Logger::Log(LogLevel::DEBUG, "[AssetSyncManager] Added asset request " + std::to_string(request.requestId) +
                   " for asset " + std::to_string(assetId));
        m_stats.activeRequests++;
    }

    return added;
}

bool AssetSyncManager::CancelAssetRequest(uint64_t requestId) {
    std::lock_guard<std::mutex> lock(m_requestMutex);

    auto it = m_pendingRequests.find(requestId);
    if (it != m_pendingRequests.end()) {
        it->second.isActive = false;
        if (it->second.callback) {
            it->second.callback(false);
        }
        m_pendingRequests.erase(it);
        m_stats.activeRequests--;

        Logger::Log(LogLevel::DEBUG, "[AssetSyncManager] Cancelled asset request " + std::to_string(requestId));
        return true;
    }

    return false;
}

bool AssetSyncManager::IsAssetLoaded(uint64_t assetId) const {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_assetCache.find(assetId);
    return (it != m_assetCache.end() && it->second->state == AssetStreamState::Loaded);
}

AssetStreamState AssetSyncManager::GetAssetState(uint64_t assetId) const {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_assetCache.find(assetId);
    return (it != m_assetCache.end()) ? it->second->state : AssetStreamState::Unloaded;
}

std::shared_ptr<AssetCacheEntry> AssetSyncManager::GetAsset(uint64_t assetId) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_assetCache.find(assetId);
    if (it != m_assetCache.end() && it->second->state == AssetStreamState::Loaded) {
        // Update access statistics
        it->second->lastAccess = std::chrono::steady_clock::now();
        it->second->accessCount++;
        return it->second;
    }
    return nullptr;
}

bool AssetSyncManager::GetAssetData(uint64_t assetId, std::vector<uint8_t>& outData) {
    auto cacheEntry = GetAsset(assetId);
    if (!cacheEntry || cacheEntry->chunks.empty()) {
        return false;
    }

    // Reconstruct data from chunks
    outData.clear();
    for (const auto& chunk : cacheEntry->chunks) {
        if (chunk && !chunk->data.empty()) {
            outData.insert(outData.end(), chunk->data.begin(), chunk->data.end());
        }
    }

    return !outData.empty();
}

bool AssetSyncManager::GetAssetChunk(uint64_t assetId, uint32_t chunkIndex, std::vector<uint8_t>& outData) {
    auto cacheEntry = GetAsset(assetId);
    if (!cacheEntry || chunkIndex >= cacheEntry->chunks.size()) {
        return false;
    }

    auto chunk = cacheEntry->chunks[chunkIndex];
    if (!chunk || chunk->data.empty()) {
        return false;
    }

    outData = chunk->data;
    return true;
}

bool AssetSyncManager::SetAssetPriority(uint64_t assetId, AssetPriority priority) {
    auto assetInfo = GetAssetInfo(assetId);
    if (!assetInfo) {
        return false;
    }

    // Update priority in registry
    {
        std::lock_guard<std::mutex> lock(m_registryMutex);
        assetInfo->priority = priority;
    }

    // Update priority in cache if loaded
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto cacheIt = m_assetCache.find(assetId);
        if (cacheIt != m_assetCache.end()) {
            cacheIt->second->info->priority = priority;
        }
    }

    Logger::Log(LogLevel::DEBUG, "[AssetSyncManager] Updated priority for asset " + std::to_string(assetId) +
               " to " + AssetUtils::GetAssetPriorityName(priority));
    return true;
}

bool AssetSyncManager::PinAsset(uint64_t assetId, bool pin) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_assetCache.find(assetId);
    if (it != m_assetCache.end()) {
        it->second->isPinned = pin;
        Logger::Log(LogLevel::DEBUG, "[AssetSyncManager] " + std::string(pin ? "Pinned" : "Unpinned") +
                   " asset " + std::to_string(assetId));
        return true;
    }
    return false;
}

void AssetSyncManager::EvictLowPriorityAssets(uint64_t targetMemoryReduction) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    uint64_t memoryFreed = 0;
    auto it = m_assetCache.begin();

    while (it != m_assetCache.end() && memoryFreed < targetMemoryReduction) {
        auto& entry = it->second;

        // Don't evict pinned assets or critical/high priority assets
        if (!entry->isPinned &&
            entry->info->priority >= AssetPriority::Medium &&
            entry->state == AssetStreamState::Loaded) {

            memoryFreed += entry->memoryUsage;
            Logger::Log(LogLevel::DEBUG, "[AssetSyncManager] Evicting asset " + std::to_string(it->first) +
                       " (freed " + std::to_string(entry->memoryUsage) + " bytes)");

            // Update state and clear chunks
            entry->state = AssetStreamState::Evicted;
            entry->chunks.clear();

            TriggerEvent("asset_evicted", it->first, AssetStreamState::Evicted,
                        "memory_freed:" + std::to_string(entry->memoryUsage));
        }
        ++it;
    }

    Logger::Log(LogLevel::INFO, "[AssetSyncManager] Evicted " + std::to_string(memoryFreed) + " bytes of asset memory");
}

void AssetSyncManager::SetMemoryLimit(uint64_t maxMemoryBytes) {
    m_config.maxMemoryBytes = maxMemoryBytes;
    Logger::Log(LogLevel::INFO, "[AssetSyncManager] Set memory limit to " + std::to_string(maxMemoryBytes) + " bytes");

    // Trigger eviction if we're over the new limit
    if (IsMemoryLimitExceeded()) {
        uint64_t currentUsage = CalculateMemoryUsage();
        uint64_t excess = currentUsage - maxMemoryBytes;
        EvictLowPriorityAssets(excess);
    }
}

bool AssetSyncManager::RegisterCustomAsset(const std::string& modName, const std::string& assetPath,
                                          const std::vector<uint8_t>& assetData, AssetType type) {
    if (!m_config.enableCustomAssets) {
        Logger::Log(LogLevel::WARNING, "[AssetSyncManager] Custom assets are disabled");
        return false;
    }

    // Generate asset ID
    uint64_t assetId = GenerateAssetId(modName + "/" + assetPath);

    // Create asset info
    AssetInfo assetInfo;
    assetInfo.assetId = assetId;
    assetInfo.assetPath = modName + "/" + assetPath;
    assetInfo.type = type;
    assetInfo.priority = AssetPriority::Medium; // Custom assets default to medium priority
    assetInfo.syncMode = AssetSyncMode::Optional;
    assetInfo.fileSize = assetData.size();
    assetInfo.compressedSize = assetData.size(); // Will be updated if compressed
    assetInfo.compression = CompressionType::None;
    assetInfo.version = 1;
    assetInfo.lastModified = GetCurrentTimestamp();
    assetInfo.chunkCount = 1; // Single chunk for custom assets
    assetInfo.chunkHashes = {CalculateDataHash(assetData)};
    assetInfo.refCount = 0;
    assetInfo.isCustomAsset = true;
    assetInfo.metadata = "mod:" + modName;

    // Register the asset
    bool registered = RegisterAsset(assetInfo);
    if (!registered) {
        return false;
    }

    // Create cache entry with the data
    auto cacheEntry = std::make_shared<AssetCacheEntry>();
    cacheEntry->assetId = assetId;
    cacheEntry->state = AssetStreamState::Loaded;
    cacheEntry->info = std::make_shared<AssetInfo>(assetInfo);
    cacheEntry->memoryUsage = assetData.size();
    cacheEntry->lastAccess = std::chrono::steady_clock::now();
    cacheEntry->loadTime = cacheEntry->lastAccess;
    cacheEntry->accessCount = 0;
    cacheEntry->isPinned = false;

    // Create single chunk
    auto chunk = std::make_shared<AssetChunk>();
    chunk->assetId = assetId;
    chunk->chunkIndex = 0;
    chunk->chunkSize = assetData.size();
    chunk->compressedSize = assetData.size();
    chunk->chunkHash = assetInfo.chunkHashes[0];
    chunk->data = assetData;
    chunk->isCompressed = false;
    chunk->requestTime = std::chrono::steady_clock::now();

    cacheEntry->chunks.push_back(chunk);

    // Add to cache
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_assetCache[assetId] = cacheEntry;
    }

    // Track the mod
    m_loadedMods[modName] = m_config.customAssetsPath + modName;

    Logger::Log(LogLevel::INFO, "[AssetSyncManager] Registered custom asset " + std::to_string(assetId) +
               " from mod " + modName + " (" + std::to_string(assetData.size()) + " bytes)");

    TriggerEvent("custom_asset_registered", assetId, AssetStreamState::Loaded,
                "mod:" + modName + ",type:" + AssetUtils::GetAssetTypeName(type));

    return true;
}

bool AssetSyncManager::LoadCustomAssetsFromMod(const std::string& modPath) {
    if (!m_config.enableCustomAssets) {
        return false;
    }

    if (!std::filesystem::exists(modPath) || !std::filesystem::is_directory(modPath)) {
        Logger::Log(LogLevel::ERROR, "[AssetSyncManager] Mod path does not exist: " + modPath);
        return false;
    }

    std::string modName = std::filesystem::path(modPath).filename().string();
    uint32_t assetsLoaded = 0;

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(modPath)) {
            if (entry.is_regular_file()) {
                std::string relativePath = std::filesystem::relative(entry.path(), modPath).string();
                AssetType assetType = DetermineAssetType(entry.path().string());

                if (assetType != AssetType::Unknown) {
                    std::vector<uint8_t> assetData;
                    if (ReadAssetFromFile(entry.path().string(), assetData)) {
                        if (RegisterCustomAsset(modName, relativePath, assetData, assetType)) {
                            assetsLoaded++;
                        }
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        Logger::Log(LogLevel::ERROR, "[AssetSyncManager] Error loading mod " + modName + ": " + e.what());
        return false;
    }

    if (assetsLoaded > 0) {
        Logger::Log(LogLevel::INFO, "[AssetSyncManager] Loaded " + std::to_string(assetsLoaded) +
                   " assets from mod " + modName);
        return true;
    }

    return false;
}

std::vector<std::string> AssetSyncManager::GetLoadedMods() const {
    std::vector<std::string> modNames;
    for (const auto& [modName, modPath] : m_loadedMods) {
        modNames.push_back(modName);
    }
    return modNames;
}

void AssetSyncManager::SetBandwidthLimit(uint64_t bytesPerSecond) {
    m_bandwidthManager.maxBandwidthBytesPerSecond = bytesPerSecond;
    Logger::Log(LogLevel::INFO, "[AssetSyncManager] Set bandwidth limit to " +
               std::to_string(bytesPerSecond) + " bytes/second");
}

uint64_t AssetSyncManager::GetCurrentBandwidthUsage() const {
    return m_bandwidthManager.currentBandwidthUsage;
}

AssetStreamingStats AssetSyncManager::GetStreamingStats() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    AssetStreamingStats stats = m_stats;

    // Update current memory usage
    stats.currentMemoryUsage = CalculateMemoryUsage();

    return stats;
}

void AssetSyncManager::ResetStats() {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    m_stats = AssetStreamingStats{};
    m_stats.lastStatsUpdate = std::chrono::steady_clock::now();
    Logger::Log(LogLevel::INFO, "[AssetSyncManager] Reset streaming statistics");
}

// Process asset requests in background thread
void AssetSyncManager::ProcessAssetRequests() {
    std::vector<AssetRequest> requestsToProcess;

    // Get pending requests with timeout check
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        auto now = std::chrono::steady_clock::now();

        for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end();) {
            if (!it->second.isActive || now > it->second.deadline) {
                // Request timed out or cancelled
                if (it->second.callback) {
                    it->second.callback(false);
                }
                it = m_pendingRequests.erase(it);
                m_stats.activeRequests--;
            } else {
                requestsToProcess.push_back(it->second);
                ++it;
            }
        }
    }

    // Process requests by priority
    std::sort(requestsToProcess.begin(), requestsToProcess.end(),
              [](const AssetRequest& a, const AssetRequest& b) {
                  return a.priority < b.priority; // Lower enum value = higher priority
              });

    for (const auto& request : requestsToProcess) {
        // Try to load asset from disk first
        if (LoadAssetFromDisk(request.assetId)) {
            // Asset loaded successfully
            if (request.callback) {
                request.callback(true);
            }
            RemoveAssetRequest(request.requestId);
        } else {
            // Try to load from network
            LoadAssetFromNetwork(request.assetId, request.playerId);
        }
    }
}

void AssetSyncManager::ProcessDownloads() {
    // This would handle incoming asset chunks from network
    // For now, it's a placeholder that could be implemented with network integration
}

void AssetSyncManager::ProcessCacheManagement() {
    // Periodic cache cleanup and optimization
    EvictLeastRecentlyUsed();
}

void AssetSyncManager::ProcessBandwidthManagement() {
    auto now = std::chrono::steady_clock::now();
    auto timeSinceReset = std::chrono::duration_cast<std::chrono::seconds>(
        now - m_bandwidthManager.lastBandwidthReset).count();

    if (timeSinceReset >= 1) {
        // Reset bandwidth usage every second
        m_bandwidthManager.currentBandwidthUsage = 0;
        m_bandwidthManager.lastBandwidthReset = now;

        // Clean old bandwidth history
        while (!m_bandwidthManager.bandwidthHistory.empty() &&
               std::chrono::duration_cast<std::chrono::seconds>(
                   now - m_bandwidthManager.bandwidthHistory.front().first).count() > 60) {
            m_bandwidthManager.bandwidthHistory.pop();
        }
    }
}

void AssetSyncManager::UpdateStatistics() {
    std::lock_guard<std::mutex> lock(m_statsMutex);

    m_stats.currentMemoryUsage = CalculateMemoryUsage();
    m_stats.peakMemoryUsage = std::max(m_stats.peakMemoryUsage, m_stats.currentMemoryUsage);

    // Calculate cache hit ratio
    uint64_t totalAccesses = m_stats.totalAssetsLoaded + m_stats.totalAssetsFailed;
    if (totalAccesses > 0) {
        m_stats.cacheHitRatio = static_cast<float>(m_stats.totalAssetsLoaded) / totalAccesses;
    }

    m_stats.lastStatsUpdate = std::chrono::steady_clock::now();
}

// Utility method implementations
AssetType AssetSyncManager::DetermineAssetType(const std::string& filePath) {
    std::string extension = std::filesystem::path(filePath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == ".dds" || extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
        return AssetType::Texture;
    } else if (extension == ".mesh" || extension == ".obj" || extension == ".fbx") {
        return AssetType::Mesh;
    } else if (extension == ".wav" || extension == ".mp3" || extension == ".ogg") {
        return AssetType::Audio;
    } else if (extension == ".anim" || extension == ".fbx") {
        return AssetType::Animation;
    } else if (extension == ".mat" || extension == ".material") {
        return AssetType::Material;
    } else if (extension == ".reds" || extension == ".lua" || extension == ".js") {
        return AssetType::Script;
    } else if (extension == ".json" || extension == ".xml" || extension == ".cfg") {
        return AssetType::Config;
    }

    return AssetType::Unknown;
}

uint64_t AssetSyncManager::GenerateAssetId(const std::string& assetPath) {
    return CalculateDataHash(std::vector<uint8_t>(assetPath.begin(), assetPath.end()));
}

uint64_t AssetSyncManager::CalculateDataHash(const std::vector<uint8_t>& data) {
    // FNV-1a hash
    uint64_t hash = 14695981039346656037ULL; // FNV offset basis
    for (uint8_t byte : data) {
        hash ^= byte;
        hash *= 1099511628211ULL; // FNV prime
    }
    return hash;
}

std::string AssetSyncManager::GetAssetCachePath(uint64_t assetId) {
    return m_config.cachePath + std::to_string(assetId) + ".cache";
}

bool AssetSyncManager::IsMemoryLimitExceeded() const {
    uint64_t currentUsage = CalculateMemoryUsage();
    return currentUsage > (m_config.maxMemoryBytes * m_config.evictionThreshold);
}

void AssetSyncManager::TriggerEvent(const std::string& eventType, uint64_t assetId,
                                   AssetStreamState state, const std::string& data) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    auto it = m_eventCallbacks.find(eventType);
    if (it != m_eventCallbacks.end()) {
        for (const auto& callback : it->second) {
            try {
                callback(assetId, state, data);
            } catch (const std::exception& e) {
                Logger::Log(LogLevel::ERROR, "[AssetSyncManager] Event callback error: " + std::string(e.what()));
            }
        }
    }
}

uint64_t AssetSyncManager::GetCurrentTimestamp() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

bool AssetSyncManager::LoadAssetFromDisk(uint64_t assetId) {
    auto assetInfo = GetAssetInfo(assetId);
    if (!assetInfo) {
        return false;
    }

    // Try to load from cache file
    std::string cachePath = GetAssetCachePath(assetId);
    std::vector<uint8_t> assetData;

    if (ReadAssetFromFile(cachePath, assetData)) {
        // Create cache entry
        auto cacheEntry = std::make_shared<AssetCacheEntry>();
        cacheEntry->assetId = assetId;
        cacheEntry->state = AssetStreamState::Loaded;
        cacheEntry->info = assetInfo;
        cacheEntry->memoryUsage = assetData.size();
        cacheEntry->lastAccess = std::chrono::steady_clock::now();
        cacheEntry->loadTime = cacheEntry->lastAccess;
        cacheEntry->accessCount = 0;
        cacheEntry->isPinned = false;

        // Create chunk
        auto chunk = std::make_shared<AssetChunk>();
        chunk->assetId = assetId;
        chunk->chunkIndex = 0;
        chunk->chunkSize = assetData.size();
        chunk->compressedSize = assetData.size();
        chunk->chunkHash = CalculateDataHash(assetData);
        chunk->data = assetData;
        chunk->isCompressed = false;
        chunk->requestTime = std::chrono::steady_clock::now();

        cacheEntry->chunks.push_back(chunk);

        // Add to cache
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            m_assetCache[assetId] = cacheEntry;
        }

        m_stats.totalAssetsLoaded++;
        Logger::Log(LogLevel::DEBUG, "[AssetSyncManager] Loaded asset " + std::to_string(assetId) + " from disk cache");
        return true;
    }

    return false;
}

bool AssetSyncManager::LoadAssetFromNetwork(uint64_t assetId, uint32_t playerId) {
    // Send asset request packet to other players
    AssetRequestPacket packet;
    packet.requestId = GenerateRequestId();
    packet.assetId = assetId;
    packet.playerId = playerId;
    packet.priority = static_cast<uint8_t>(AssetPriority::Medium);
    packet.timestamp = GetCurrentTimestamp();

    // Broadcast request (implementation would depend on network layer)
    Logger::Log(LogLevel::DEBUG, "[AssetSyncManager] Requesting asset " + std::to_string(assetId) + " from network");

    // In a real implementation, this would send the packet through the network layer
    // For now, we'll just log the attempt
    return false;
}

bool AssetSyncManager::ReadAssetFromFile(const std::string& filePath, std::vector<uint8_t>& data) {
    try {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return false;
        }

        size_t fileSize = file.tellg();
        file.seekg(0);

        data.resize(fileSize);
        file.read(reinterpret_cast<char*>(data.data()), fileSize);

        return file.good();
    } catch (const std::exception& e) {
        Logger::Log(LogLevel::ERROR, "[AssetSyncManager] Failed to read file " + filePath + ": " + e.what());
        return false;
    }
}

bool AssetSyncManager::WriteAssetToFile(const std::string& filePath, const std::vector<uint8_t>& data) {
    try {
        std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());

        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        return file.good();
    } catch (const std::exception& e) {
        Logger::Log(LogLevel::ERROR, "[AssetSyncManager] Failed to write file " + filePath + ": " + e.what());
        return false;
    }
}

uint64_t AssetSyncManager::GenerateRequestId() {
    return m_nextRequestId.fetch_add(1);
}

bool AssetSyncManager::AddAssetRequest(const AssetRequest& request) {
    std::lock_guard<std::mutex> lock(m_requestMutex);
    m_pendingRequests[request.requestId] = request;
    return true;
}

bool AssetSyncManager::RemoveAssetRequest(uint64_t requestId) {
    std::lock_guard<std::mutex> lock(m_requestMutex);
    auto it = m_pendingRequests.find(requestId);
    if (it != m_pendingRequests.end()) {
        m_pendingRequests.erase(it);
        m_stats.activeRequests--;
        return true;
    }
    return false;
}

uint64_t AssetSyncManager::CalculateMemoryUsage() const {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    uint64_t totalMemory = 0;

    for (const auto& [assetId, cacheEntry] : m_assetCache) {
        if (cacheEntry->state == AssetStreamState::Loaded) {
            totalMemory += cacheEntry->memoryUsage;
        }
    }

    return totalMemory;
}

void AssetSyncManager::EvictLeastRecentlyUsed() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    if (m_assetCache.empty()) return;

    auto oldestIt = m_assetCache.end();
    auto oldestTime = std::chrono::steady_clock::now();

    for (auto it = m_assetCache.begin(); it != m_assetCache.end(); ++it) {
        if (!it->second->isPinned &&
            it->second->info->priority >= AssetPriority::Medium &&
            it->second->lastAccess < oldestTime) {
            oldestTime = it->second->lastAccess;
            oldestIt = it;
        }
    }

    if (oldestIt != m_assetCache.end()) {
        Logger::Log(LogLevel::DEBUG, "[AssetSyncManager] Evicting LRU asset " + std::to_string(oldestIt->first));
        oldestIt->second->state = AssetStreamState::Evicted;
        oldestIt->second->chunks.clear();
        TriggerEvent("asset_evicted", oldestIt->first, AssetStreamState::Evicted, "reason:lru");
    }
}

// AssetUtils namespace implementation
namespace AssetUtils {
    std::string GetAssetTypeName(AssetType type) {
        switch (type) {
            case AssetType::Texture: return "Texture";
            case AssetType::Mesh: return "Mesh";
            case AssetType::Audio: return "Audio";
            case AssetType::Animation: return "Animation";
            case AssetType::Material: return "Material";
            case AssetType::Script: return "Script";
            case AssetType::World: return "World";
            case AssetType::Character: return "Character";
            case AssetType::Vehicle: return "Vehicle";
            case AssetType::Weapon: return "Weapon";
            case AssetType::Effect: return "Effect";
            case AssetType::UI: return "UI";
            case AssetType::Config: return "Config";
            case AssetType::Custom: return "Custom";
            default: return "Unknown";
        }
    }

    std::string GetAssetPriorityName(AssetPriority priority) {
        switch (priority) {
            case AssetPriority::Critical: return "Critical";
            case AssetPriority::High: return "High";
            case AssetPriority::Medium: return "Medium";
            case AssetPriority::Low: return "Low";
            case AssetPriority::Background: return "Background";
            default: return "Disabled";
        }
    }

    std::string GetAssetStateName(AssetStreamState state) {
        switch (state) {
            case AssetStreamState::Unloaded: return "Unloaded";
            case AssetStreamState::Requested: return "Requested";
            case AssetStreamState::Downloading: return "Downloading";
            case AssetStreamState::Loading: return "Loading";
            case AssetStreamState::Loaded: return "Loaded";
            case AssetStreamState::Failed: return "Failed";
            case AssetStreamState::Evicted: return "Evicted";
            default: return "Unknown";
        }
    }

    bool IsAssetPathValid(const std::string& path) {
        // Basic validation for asset paths
        if (path.empty() || path.size() > 1024) {
            return false;
        }

        // Check for invalid characters
        const std::string invalidChars = "<>:\"|?*";
        for (char c : invalidChars) {
            if (path.find(c) != std::string::npos) {
                return false;
            }
        }

        return true;
    }

    AssetType GetAssetTypeFromExtension(const std::string& extension) {
        std::string ext = extension;
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".dds" || ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
            return AssetType::Texture;
        } else if (ext == ".mesh" || ext == ".obj" || ext == ".fbx") {
            return AssetType::Mesh;
        } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
            return AssetType::Audio;
        } else if (ext == ".anim") {
            return AssetType::Animation;
        } else if (ext == ".mat" || ext == ".material") {
            return AssetType::Material;
        } else if (ext == ".reds" || ext == ".lua" || ext == ".js") {
            return AssetType::Script;
        } else if (ext == ".json" || ext == ".xml" || ext == ".cfg") {
            return AssetType::Config;
        }

        return AssetType::Unknown;
    }

    uint32_t CalculateChunkCount(uint64_t fileSize, uint32_t chunkSize) {
        if (chunkSize == 0) return 0;
        return static_cast<uint32_t>((fileSize + chunkSize - 1) / chunkSize);
    }

    bool IsAssetSizeReasonable(uint64_t size, AssetType type) {
        // Define reasonable size limits for different asset types
        const uint64_t MB = 1024 * 1024;

        switch (type) {
            case AssetType::Texture:
                return size <= 128 * MB; // 128MB max for textures
            case AssetType::Mesh:
                return size <= 64 * MB;  // 64MB max for meshes
            case AssetType::Audio:
                return size <= 32 * MB;  // 32MB max for audio
            case AssetType::Animation:
                return size <= 16 * MB;  // 16MB max for animations
            case AssetType::Script:
                return size <= 1 * MB;   // 1MB max for scripts
            case AssetType::Config:
                return size <= 1 * MB;   // 1MB max for config files
            default:
                return size <= 256 * MB; // 256MB general limit
        }
    }
}

} // namespace CoopNet
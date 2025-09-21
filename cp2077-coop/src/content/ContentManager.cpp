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

#include "ContentManager.hpp"
#include "../core/ErrorManager.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <iomanip>
#include <sstream>

#include "../core/ZlibWrapper.hpp"
#include <openssl/md5.h>
#include <openssl/sha.h>

namespace CoopNet {

// File system content storage implementation
class FileSystemStorage : public IContentStorage {
public:
    FileSystemStorage(const std::string& basePath) : m_basePath(basePath) {
        std::filesystem::create_directories(m_basePath);
    }

    bool Store(const std::string& key, const void* data, uint64_t size) override {
        try {
            std::string filePath = m_basePath + "/" + SanitizeKey(key);
            std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());

            std::ofstream file(filePath, std::ios::binary);
            if (!file.is_open()) {
                spdlog::error("[ContentManager] Failed to open file for writing: {}", filePath);
                return false;
            }

            file.write(static_cast<const char*>(data), size);
            file.close();

            return file.good();
        } catch (const std::exception& ex) {
            spdlog::error("[ContentManager] Failed to store content: {}", ex.what());
            return false;
        }
    }

    bool Retrieve(const std::string& key, void*& data, uint64_t& size) override {
        try {
            std::string filePath = m_basePath + "/" + SanitizeKey(key);
            if (!std::filesystem::exists(filePath)) {
                return false;
            }

            std::ifstream file(filePath, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                spdlog::error("[ContentManager] Failed to open file for reading: {}", filePath);
                return false;
            }

            size = file.tellg();
            file.seekg(0, std::ios::beg);

            data = std::malloc(size);
            if (!data) {
                spdlog::error("[ContentManager] Failed to allocate memory for content: {} bytes", size);
                return false;
            }

            file.read(static_cast<char*>(data), size);
            file.close();

            return file.good();
        } catch (const std::exception& ex) {
            spdlog::error("[ContentManager] Failed to retrieve content: {}", ex.what());
            return false;
        }
    }

    bool Remove(const std::string& key) override {
        try {
            std::string filePath = m_basePath + "/" + SanitizeKey(key);
            return std::filesystem::remove(filePath);
        } catch (const std::exception& ex) {
            spdlog::error("[ContentManager] Failed to remove content: {}", ex.what());
            return false;
        }
    }

    bool Exists(const std::string& key) const override {
        std::string filePath = m_basePath + "/" + SanitizeKey(key);
        return std::filesystem::exists(filePath);
    }

    uint64_t GetSize(const std::string& key) const override {
        try {
            std::string filePath = m_basePath + "/" + SanitizeKey(key);
            if (!std::filesystem::exists(filePath)) {
                return 0;
            }
            return std::filesystem::file_size(filePath);
        } catch (const std::exception&) {
            return 0;
        }
    }

    std::vector<std::string> ListKeys() const override {
        std::vector<std::string> keys;
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(m_basePath)) {
                if (entry.is_regular_file()) {
                    std::string relativePath = std::filesystem::relative(entry.path(), m_basePath).string();
                    keys.push_back(relativePath);
                }
            }
        } catch (const std::exception& ex) {
            spdlog::error("[ContentManager] Failed to list keys: {}", ex.what());
        }
        return keys;
    }

    void Clear() override {
        try {
            std::filesystem::remove_all(m_basePath);
            std::filesystem::create_directories(m_basePath);
        } catch (const std::exception& ex) {
            spdlog::error("[ContentManager] Failed to clear storage: {}", ex.what());
        }
    }

private:
    std::string m_basePath;

    std::string SanitizeKey(const std::string& key) const {
        std::string sanitized = key;
        std::replace(sanitized.begin(), sanitized.end(), '\\', '/');

        // Remove any dangerous characters
        std::regex dangerousChars(R"([<>:"|?*])");
        sanitized = std::regex_replace(sanitized, dangerousChars, "_");

        return sanitized;
    }
};

// Basic content loader implementation
class BasicContentLoader : public IContentLoader {
public:
    bool CanLoad(ContentType type) const override {
        return type == ContentType::Config ||
               type == ContentType::Script ||
               type == ContentType::Custom;
    }

    bool LoadContent(ContentInfo& content) override {
        if (!std::filesystem::exists(content.filePath)) {
            m_lastError = "File does not exist: " + content.filePath;
            return false;
        }

        try {
            auto startTime = std::chrono::steady_clock::now();

            std::ifstream file(content.filePath, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                m_lastError = "Failed to open file: " + content.filePath;
                return false;
            }

            uint64_t fileSize = file.tellg();
            file.seekg(0, std::ios::beg);

            void* data = std::malloc(fileSize);
            if (!data) {
                m_lastError = "Failed to allocate memory for content";
                return false;
            }

            file.read(static_cast<char*>(data), fileSize);
            file.close();

            if (!file.good()) {
                std::free(data);
                m_lastError = "Failed to read file data";
                return false;
            }

            content.data = data;
            content.memorySize = fileSize;
            content.state = ContentState::Ready;

            auto endTime = std::chrono::steady_clock::now();
            content.loadTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

            spdlog::debug("[ContentManager] Loaded content: {} ({} bytes)", content.contentId, fileSize);
            return true;

        } catch (const std::exception& ex) {
            m_lastError = "Exception while loading content: " + std::string(ex.what());
            return false;
        }
    }

    bool UnloadContent(ContentInfo& content) override {
        if (content.data) {
            if (!content.isMemoryMapped) {
                std::free(content.data);
            }
            content.data = nullptr;
            content.memorySize = 0;
            content.state = ContentState::Unloaded;

            spdlog::debug("[ContentManager] Unloaded content: {}", content.contentId);
            return true;
        }
        return false;
    }

    bool ValidateContent(const ContentInfo& content) override {
        if (!std::filesystem::exists(content.filePath)) {
            return false;
        }

        if (!content.checksum.empty()) {
            std::string actualChecksum = ContentUtils::CalculateSHA256(content.data, content.memorySize);
            return actualChecksum == content.checksum;
        }

        return true;
    }

    std::string GetLastError() const override {
        return m_lastError;
    }

private:
    std::string m_lastError;
};

// ContentManager implementation
ContentManager& ContentManager::Instance() {
    static ContentManager instance;
    return instance;
}

bool ContentManager::Initialize(const std::string& contentDirectory) {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    if (m_initialized) {
        spdlog::warn("[ContentManager] Already initialized");
        return true;
    }

    m_contentDirectory = contentDirectory;

    // Create content directory if it doesn't exist
    if (!CreateDirectoryIfNotExists(m_contentDirectory)) {
        spdlog::error("[ContentManager] Failed to create content directory: {}", m_contentDirectory);
        return false;
    }

    // Initialize default storage
    m_defaultStorage = std::make_shared<FileSystemStorage>(m_contentDirectory + "/storage");

    // Register default content loader
    RegisterLoader(ContentType::Config, std::make_shared<BasicContentLoader>());
    RegisterLoader(ContentType::Script, std::make_shared<BasicContentLoader>());
    RegisterLoader(ContentType::Custom, std::make_shared<BasicContentLoader>());

    // Start worker threads
    m_shouldStop = false;
    for (uint32_t i = 0; i < m_maxConcurrentLoads; ++i) {
        m_loadingWorkers.emplace_back(&ContentManager::LoadingWorkerLoop, this);
    }
    m_streamingWorker = std::thread(&ContentManager::StreamingWorkerLoop, this);
    m_maintenanceWorker = std::thread(&ContentManager::MaintenanceLoop, this);

    if (m_fileWatchingEnabled) {
        m_fileWatchingWorker = std::thread(&ContentManager::FileWatchingLoop, this);
    }

    // Scan initial content
    ScanContentDirectory();

    m_initialized = true;
    spdlog::info("[ContentManager] Initialized successfully");
    return true;
}

void ContentManager::Shutdown() {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    if (!m_initialized) return;

    // Stop worker threads
    m_shouldStop = true;

    for (auto& worker : m_loadingWorkers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_loadingWorkers.clear();

    if (m_streamingWorker.joinable()) {
        m_streamingWorker.join();
    }

    if (m_fileWatchingWorker.joinable()) {
        m_fileWatchingWorker.join();
    }

    if (m_maintenanceWorker.joinable()) {
        m_maintenanceWorker.join();
    }

    // Unload all content
    std::vector<std::string> loadedContent = GetLoadedContent();
    for (const std::string& contentId : loadedContent) {
        UnloadContent(contentId);
    }

    // Clear all data
    m_content.clear();
    m_packages.clear();
    m_cache.clear();
    m_streamingRequests.clear();
    m_providers.clear();
    m_loaders.clear();
    m_storages.clear();
    m_defaultStorage.reset();

    {
        std::lock_guard<std::mutex> queueLock(m_loadingMutex);
        while (!m_loadingQueue.empty()) m_loadingQueue.pop();
        m_currentlyLoading.clear();
    }

    {
        std::lock_guard<std::mutex> streamLock(m_streamingMutex);
        while (!m_streamingQueue.empty()) m_streamingQueue.pop();
    }

    m_initialized = false;
    spdlog::info("[ContentManager] Shutdown completed");
}

bool ContentManager::LoadContent(const std::string& contentId, ContentPriority priority) {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    auto it = m_content.find(contentId);
    if (it == m_content.end()) {
        spdlog::error("[ContentManager] Content not found: {}", contentId);
        return false;
    }

    ContentInfo& content = it->second;
    if (content.state == ContentState::Ready) {
        spdlog::debug("[ContentManager] Content already loaded: {}", contentId);
        return true;
    }

    content.priority = priority;
    return LoadContentInternal(content);
}

bool ContentManager::LoadContentAsync(const std::string& contentId, ContentPriority priority) {
    std::lock_guard<std::mutex> lock(m_loadingMutex);

    auto contentIt = m_content.find(contentId);
    if (contentIt == m_content.end()) {
        spdlog::error("[ContentManager] Content not found for async load: {}", contentId);
        return false;
    }

    if (m_currentlyLoading.count(contentId)) {
        spdlog::debug("[ContentManager] Content already queued for loading: {}", contentId);
        return true;
    }

    contentIt->second.priority = priority;
    m_loadingQueue.push(contentId);
    m_currentlyLoading.insert(contentId);

    spdlog::debug("[ContentManager] Queued content for async loading: {}", contentId);
    return true;
}

bool ContentManager::UnloadContent(const std::string& contentId) {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    auto it = m_content.find(contentId);
    if (it == m_content.end()) {
        spdlog::error("[ContentManager] Content not found: {}", contentId);
        return false;
    }

    ContentInfo& content = it->second;
    if (content.state != ContentState::Ready) {
        spdlog::debug("[ContentManager] Content not loaded: {}", contentId);
        return true;
    }

    return UnloadContentInternal(content);
}

bool ContentManager::RegisterContent(const ContentInfo& content) {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    if (content.contentId.empty()) {
        spdlog::error("[ContentManager] Content ID cannot be empty");
        return false;
    }

    m_content[content.contentId] = content;
    spdlog::info("[ContentManager] Registered content: {}", content.contentId);

    NotifyStateChanged(content.contentId, content.state);
    return true;
}

std::optional<ContentInfo> ContentManager::GetContentInfo(const std::string& contentId) const {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    auto it = m_content.find(contentId);
    if (it != m_content.end()) {
        return it->second;
    }
    return std::nullopt;
}

ContentState ContentManager::GetContentState(const std::string& contentId) const {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    auto it = m_content.find(contentId);
    if (it != m_content.end()) {
        return it->second.state;
    }
    return ContentState::Unknown;
}

std::vector<std::string> ContentManager::GetLoadedContent() const {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    std::vector<std::string> loaded;
    for (const auto& [contentId, content] : m_content) {
        if (content.state == ContentState::Ready) {
            loaded.push_back(contentId);
        }
    }
    return loaded;
}

std::vector<ContentInfo> ContentManager::SearchContent(const std::string& query) const {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    std::vector<ContentInfo> results;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    for (const auto& [contentId, content] : m_content) {
        std::string lowerName = content.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

        std::string lowerDesc = content.description;
        std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);

        if (lowerName.find(lowerQuery) != std::string::npos ||
            lowerDesc.find(lowerQuery) != std::string::npos ||
            contentId.find(query) != std::string::npos) {
            results.push_back(content);
        }

        // Check tags
        for (const std::string& tag : content.tags) {
            std::string lowerTag = tag;
            std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
            if (lowerTag.find(lowerQuery) != std::string::npos) {
                results.push_back(content);
                break;
            }
        }
    }

    return results;
}

std::vector<ContentInfo> ContentManager::GetContentByType(ContentType type) const {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    std::vector<ContentInfo> results;
    for (const auto& [contentId, content] : m_content) {
        if (content.type == type) {
            results.push_back(content);
        }
    }
    return results;
}

void ContentManager::ScanContentDirectory(const std::string& directory) {
    std::string scanDir = directory.empty() ? m_contentDirectory : directory;

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(scanDir)) {
            if (entry.is_regular_file()) {
                std::string filePath = entry.path().string();
                std::string contentId = GenerateContentId(filePath);

                // Check if content is already registered
                if (m_content.count(contentId)) {
                    continue;
                }

                ContentInfo content;
                content.contentId = contentId;
                content.name = entry.path().filename().string();
                content.filePath = filePath;
                content.fileSize = entry.file_size();
                content.type = DetectContentType(filePath);
                content.state = ContentState::Unloaded;
                content.priority = ContentPriority::Medium;
                content.storageType = StorageType::File;
                content.createdAt = std::chrono::steady_clock::now();
                content.modifiedAt = content.createdAt;
                content.accessedAt = content.createdAt;
                content.accessCount = 0;
                content.downloadCount = 0;
                content.isPersistent = true;
                content.isCompressed = false;
                content.isEncrypted = false;

                RegisterContent(content);
            }
        }

        spdlog::info("[ContentManager] Scanned directory: {} - found {} content items",
                     scanDir, m_content.size());

    } catch (const std::exception& ex) {
        spdlog::error("[ContentManager] Failed to scan directory {}: {}", scanDir, ex.what());
    }
}

void ContentManager::RegisterLoader(ContentType type, std::shared_ptr<IContentLoader> loader) {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);
    m_loaders[type] = loader;
    spdlog::info("[ContentManager] Registered loader for content type: {}",
                 static_cast<int>(type));
}

void ContentManager::SetCacheSize(uint64_t maxSize) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_maxCacheSize = maxSize;
    EvictCache(); // Evict if current cache exceeds new limit
}

uint64_t ContentManager::GetCacheSize() const {
    return m_maxCacheSize;
}

uint64_t ContentManager::GetCacheUsage() const {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    uint64_t usage = 0;
    for (const auto& [key, cache] : m_cache) {
        usage += cache.size;
    }
    return usage;
}

void ContentManager::ClearCache() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    for (auto& [key, cache] : m_cache) {
        if (cache.destructor) {
            cache.destructor();
        } else if (cache.data) {
            std::free(cache.data);
        }
    }

    m_cache.clear();
    spdlog::info("[ContentManager] Cache cleared");
}

// Private methods implementation
bool ContentManager::LoadContentInternal(ContentInfo& content) {
    if (content.state == ContentState::Ready) {
        return true;
    }

    NotifyStateChanged(content.contentId, ContentState::Loading);

    auto loader = GetLoader(content.type);
    if (!loader) {
        std::string error = "No loader available for content type: " + std::to_string(static_cast<int>(content.type));
        spdlog::error("[ContentManager] {}", error);
        NotifyError(content.contentId, error);
        content.state = ContentState::Error;
        return false;
    }

    auto startTime = std::chrono::steady_clock::now();

    if (loader->LoadContent(content)) {
        content.accessCount++;
        content.accessedAt = std::chrono::steady_clock::now();

        auto endTime = std::chrono::steady_clock::now();
        content.loadTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        // Cache the content if caching is enabled
        if (content.memorySize > 0 && content.memorySize < m_maxCacheSize / 10) {
            CacheContent(content.contentId, content.data, content.memorySize);
        }

        // Update statistics
        if (m_performanceMonitoringEnabled) {
            std::lock_guard<std::mutex> statsLock(m_statsMutex);
            m_loadingStats[ContentUtils::GetTypeName(content.type)]++;
            m_lastAccess[content.contentId] = std::chrono::steady_clock::now();
        }

        NotifyLoaded(content.contentId);
        spdlog::info("[ContentManager] Loaded content: {} ({} microseconds)",
                     content.contentId, content.loadTime);
        return true;
    } else {
        content.state = ContentState::Error;
        std::string error = loader->GetLastError();
        spdlog::error("[ContentManager] Failed to load content {}: {}", content.contentId, error);
        NotifyError(content.contentId, error);
        return false;
    }
}

bool ContentManager::UnloadContentInternal(ContentInfo& content) {
    auto loader = GetLoader(content.type);
    if (!loader) {
        spdlog::error("[ContentManager] No loader available for unloading content type: {}",
                     static_cast<int>(content.type));
        return false;
    }

    if (loader->UnloadContent(content)) {
        NotifyUnloaded(content.contentId);
        spdlog::debug("[ContentManager] Unloaded content: {}", content.contentId);
        return true;
    } else {
        spdlog::error("[ContentManager] Failed to unload content: {}", content.contentId);
        return false;
    }
}

std::shared_ptr<IContentLoader> ContentManager::GetLoader(ContentType type) {
    auto it = m_loaders.find(type);
    return it != m_loaders.end() ? it->second : nullptr;
}

bool ContentManager::CacheContent(const std::string& contentId, const void* data, uint64_t size) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // Check if cache would exceed limit
    uint64_t currentUsage = GetCacheUsage();
    if (currentUsage + size > m_maxCacheSize) {
        EvictCache();
        currentUsage = GetCacheUsage();
        if (currentUsage + size > m_maxCacheSize) {
            return false; // Content too large for cache
        }
    }

    std::string cacheKey = GenerateCacheKey(contentId);

    ContentCache cache;
    cache.cacheKey = cacheKey;
    cache.contentId = contentId;
    cache.size = size;
    cache.createdAt = std::chrono::steady_clock::now();
    cache.lastAccessed = cache.createdAt;
    cache.accessCount = 0;
    cache.hitCount = 0;
    cache.isPinned = false;
    cache.isCompressed = false;
    cache.compressionRatio = 100;

    // Allocate and copy data
    cache.data = std::malloc(size);
    if (!cache.data) {
        return false;
    }
    std::memcpy(cache.data, data, size);

    m_cache[cacheKey] = cache;
    return true;
}

void ContentManager::EvictCache() {
    if (m_cache.empty()) return;

    uint64_t currentUsage = GetCacheUsage();
    uint64_t targetUsage = m_maxCacheSize * 0.8; // Evict to 80% capacity

    // Sort cache entries by last access time (LRU)
    std::vector<std::pair<std::chrono::steady_clock::time_point, std::string>> sortedEntries;
    for (const auto& [key, cache] : m_cache) {
        if (!cache.isPinned) {
            sortedEntries.push_back({cache.lastAccessed, key});
        }
    }

    std::sort(sortedEntries.begin(), sortedEntries.end());

    // Evict oldest entries
    for (const auto& [accessTime, key] : sortedEntries) {
        if (currentUsage <= targetUsage) break;

        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            if (it->second.destructor) {
                it->second.destructor();
            } else if (it->second.data) {
                std::free(it->second.data);
            }
            currentUsage -= it->second.size;
            m_cache.erase(it);
        }
    }

    spdlog::debug("[ContentManager] Cache evicted, usage: {} -> {}",
                  GetCacheUsage() + currentUsage, GetCacheUsage());
}

std::string ContentManager::GenerateCacheKey(const std::string& contentId) const {
    return "content_" + contentId;
}

void ContentManager::LoadingWorkerLoop() {
    while (!m_shouldStop) {
        std::string contentId;

        {
            std::lock_guard<std::mutex> lock(m_loadingMutex);
            if (!m_loadingQueue.empty()) {
                contentId = m_loadingQueue.front();
                m_loadingQueue.pop();
            }
        }

        if (!contentId.empty()) {
            auto contentIt = m_content.find(contentId);
            if (contentIt != m_content.end()) {
                LoadContentInternal(contentIt->second);
            }

            {
                std::lock_guard<std::mutex> lock(m_loadingMutex);
                m_currentlyLoading.erase(contentId);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void ContentManager::StreamingWorkerLoop() {
    while (!m_shouldStop) {
        StreamingRequest request;
        bool hasRequest = false;

        {
            std::lock_guard<std::mutex> lock(m_streamingMutex);
            if (!m_streamingQueue.empty()) {
                request = m_streamingQueue.front();
                m_streamingQueue.pop();
                hasRequest = true;
            }
        }

        if (hasRequest) {
            // Implement streaming logic here
            // This would involve downloading content from URLs
            spdlog::debug("[ContentManager] Processing streaming request: {}", request.contentId);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void ContentManager::FileWatchingLoop() {
    while (!m_shouldStop && m_fileWatchingEnabled) {
        try {
            // Check for file modifications
            for (auto& [contentId, content] : m_content) {
                if (content.storageType == StorageType::File &&
                    std::filesystem::exists(content.filePath)) {

                    auto lastWrite = std::filesystem::last_write_time(content.filePath);
                    auto it = m_fileTimestamps.find(content.filePath);

                    if (it == m_fileTimestamps.end()) {
                        m_fileTimestamps[content.filePath] = lastWrite;
                    } else if (it->second != lastWrite) {
                        // File modified, reload if necessary
                        if (content.state == ContentState::Ready) {
                            spdlog::info("[ContentManager] File modified, reloading: {}", content.contentId);
                            ReloadContent(content.contentId);
                        }
                        it->second = lastWrite;
                    }
                }
            }
        } catch (const std::exception& ex) {
            spdlog::error("[ContentManager] File watching error: {}", ex.what());
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void ContentManager::MaintenanceLoop() {
    while (!m_shouldStop) {
        try {
            CleanupExpiredContent();
            CleanupCache();

            std::this_thread::sleep_for(std::chrono::minutes(5));
        } catch (const std::exception& ex) {
            spdlog::error("[ContentManager] Maintenance error: {}", ex.what());
        }
    }
}

void ContentManager::CleanupExpiredContent() {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> toRemove;

    for (const auto& [contentId, content] : m_content) {
        if (content.expiresAt != std::chrono::steady_clock::time_point{} &&
            now > content.expiresAt) {
            toRemove.push_back(contentId);
        }
    }

    for (const std::string& contentId : toRemove) {
        UnloadContent(contentId);
        m_content.erase(contentId);
        spdlog::debug("[ContentManager] Removed expired content: {}", contentId);
    }
}

void ContentManager::CleanupCache() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> toRemove;

    for (const auto& [key, cache] : m_cache) {
        if (cache.expiresAt != std::chrono::steady_clock::time_point{} &&
            now > cache.expiresAt && !cache.isPinned) {
            toRemove.push_back(key);
        }
    }

    for (const std::string& key : toRemove) {
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            if (it->second.destructor) {
                it->second.destructor();
            } else if (it->second.data) {
                std::free(it->second.data);
            }
            m_cache.erase(it);
        }
    }

    if (!toRemove.empty()) {
        spdlog::debug("[ContentManager] Cleaned up {} expired cache entries", toRemove.size());
    }
}

// Event notification methods
void ContentManager::NotifyStateChanged(const std::string& contentId, ContentState newState) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_callbacks.onStateChanged) {
        try {
            m_callbacks.onStateChanged(contentId, newState);
        } catch (const std::exception& ex) {
            spdlog::error("[ContentManager] State change callback error: {}", ex.what());
        }
    }
}

void ContentManager::NotifyProgress(const std::string& contentId, float progress) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_callbacks.onLoadProgress) {
        try {
            m_callbacks.onLoadProgress(contentId, progress);
        } catch (const std::exception& ex) {
            spdlog::error("[ContentManager] Progress callback error: {}", ex.what());
        }
    }
}

void ContentManager::NotifyError(const std::string& contentId, const std::string& error) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_callbacks.onError) {
        try {
            m_callbacks.onError(contentId, error);
        } catch (const std::exception& ex) {
            spdlog::error("[ContentManager] Error callback error: {}", ex.what());
        }
    }
}

void ContentManager::NotifyLoaded(const std::string& contentId) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_callbacks.onLoaded) {
        try {
            m_callbacks.onLoaded(contentId);
        } catch (const std::exception& ex) {
            spdlog::error("[ContentManager] Loaded callback error: {}", ex.what());
        }
    }
}

void ContentManager::NotifyUnloaded(const std::string& contentId) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_callbacks.onUnloaded) {
        try {
            m_callbacks.onUnloaded(contentId);
        } catch (const std::exception& ex) {
            spdlog::error("[ContentManager] Unloaded callback error: {}", ex.what());
        }
    }
}

// Utility method implementations
std::string ContentManager::GenerateContentId(const std::string& filePath) const {
    // Generate a hash-based ID from the file path
    std::hash<std::string> hasher;
    return "content_" + std::to_string(hasher(filePath));
}

ContentType ContentManager::DetectContentType(const std::string& filePath) const {
    std::string extension = ContentUtils::GetFileExtension(filePath);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    static const std::unordered_map<std::string, ContentType> extensionMap = {
        {".png", ContentType::Texture}, {".jpg", ContentType::Texture}, {".jpeg", ContentType::Texture},
        {".dds", ContentType::Texture}, {".tga", ContentType::Texture},
        {".fbx", ContentType::Model}, {".obj", ContentType::Model}, {".dae", ContentType::Model},
        {".wav", ContentType::Audio}, {".mp3", ContentType::Audio}, {".ogg", ContentType::Audio},
        {".mp4", ContentType::Video}, {".avi", ContentType::Video}, {".webm", ContentType::Video},
        {".reds", ContentType::Script}, {".red", ContentType::Script},
        {".archive", ContentType::Archive}, {".pak", ContentType::Archive},
        {".json", ContentType::Config}, {".yaml", ContentType::Config}, {".ini", ContentType::Config},
        {".hlsl", ContentType::Shader}, {".glsl", ContentType::Shader},
        {".anims", ContentType::Animation},
        {".sav", ContentType::Save}, {".save", ContentType::Save}
    };

    auto it = extensionMap.find(extension);
    return it != extensionMap.end() ? it->second : ContentType::Unknown;
}

bool ContentManager::CreateDirectoryIfNotExists(const std::string& path) {
    try {
        return std::filesystem::create_directories(path);
    } catch (const std::exception& ex) {
        spdlog::error("[ContentManager] Failed to create directory {}: {}", path, ex.what());
        return false;
    }
}

// Utility functions implementation
namespace ContentUtils {
    std::string GetTypeName(ContentType type) {
        static const std::unordered_map<ContentType, std::string> typeNames = {
            {ContentType::Unknown, "Unknown"},
            {ContentType::Texture, "Texture"},
            {ContentType::Model, "Model"},
            {ContentType::Audio, "Audio"},
            {ContentType::Video, "Video"},
            {ContentType::Script, "Script"},
            {ContentType::Archive, "Archive"},
            {ContentType::Config, "Config"},
            {ContentType::Shader, "Shader"},
            {ContentType::Animation, "Animation"},
            {ContentType::Level, "Level"},
            {ContentType::UI, "UI"},
            {ContentType::Mod, "Mod"},
            {ContentType::Localization, "Localization"},
            {ContentType::Save, "Save"},
            {ContentType::Custom, "Custom"}
        };

        auto it = typeNames.find(type);
        return it != typeNames.end() ? it->second : "Unknown";
    }

    std::string GetFileExtension(const std::string& filePath) {
        size_t dotPos = filePath.find_last_of('.');
        if (dotPos != std::string::npos && dotPos < filePath.length() - 1) {
            return filePath.substr(dotPos);
        }
        return "";
    }

    std::string CalculateSHA256(const void* data, uint64_t size) {
        if (!data || size == 0) return "";

        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, data, size);
        SHA256_Final(hash, &sha256);

        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }
}

// Missing ContentManager method implementations
std::vector<ContentInfo> ContentManager::GetAllContent() const {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    std::vector<ContentInfo> allContent;
    allContent.reserve(m_content.size());

    for (const auto& [contentId, content] : m_content) {
        allContent.push_back(content);
    }

    return allContent;
}

bool ContentManager::ReloadContent(const std::string& contentId) {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    auto it = m_content.find(contentId);
    if (it == m_content.end()) {
        spdlog::error("[ContentManager] Content not found for reload: {}", contentId);
        return false;
    }

    ContentInfo& content = it->second;

    // Unload first if loaded
    if (content.state == ContentState::Ready) {
        if (!UnloadContentInternal(content)) {
            spdlog::error("[ContentManager] Failed to unload content for reload: {}", contentId);
            return false;
        }
    }

    // Reload
    return LoadContentInternal(content);
}

bool ContentManager::UnregisterContent(const std::string& contentId) {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    auto it = m_content.find(contentId);
    if (it == m_content.end()) {
        return false;
    }

    // Unload if loaded
    if (it->second.state == ContentState::Ready) {
        UnloadContentInternal(it->second);
    }

    m_content.erase(it);
    spdlog::info("[ContentManager] Unregistered content: {}", contentId);
    return true;
}

bool ContentManager::UpdateContentInfo(const ContentInfo& content) {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    auto it = m_content.find(content.contentId);
    if (it == m_content.end()) {
        return false;
    }

    // Preserve current state and runtime data
    ContentState currentState = it->second.state;
    void* currentData = it->second.data;
    uint64_t currentMemorySize = it->second.memorySize;

    it->second = content;
    it->second.state = currentState;
    it->second.data = currentData;
    it->second.memorySize = currentMemorySize;

    spdlog::debug("[ContentManager] Updated content info: {}", content.contentId);
    return true;
}

std::vector<ContentInfo> ContentManager::GetContentByTag(const std::string& tag) const {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    std::vector<ContentInfo> results;
    for (const auto& [contentId, content] : m_content) {
        auto it = std::find(content.tags.begin(), content.tags.end(), tag);
        if (it != content.tags.end()) {
            results.push_back(content);
        }
    }
    return results;
}

std::vector<ContentInfo> ContentManager::GetContentByCategory(const std::string& category) const {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    std::vector<ContentInfo> results;
    for (const auto& [contentId, content] : m_content) {
        auto it = content.attributes.find("category");
        if (it != content.attributes.end() && it->second == category) {
            results.push_back(content);
        }
    }
    return results;
}

void ContentManager::SetCallbacks(const ContentCallbacks& callbacks) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_callbacks = callbacks;
}

void ContentManager::ClearCallbacks() {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_callbacks = {};
}

std::string ContentManager::CalculateChecksum(const std::string& filePath) const {
    try {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return "";
        }

        file.seekg(0, std::ios::end);
        uint64_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        file.read(buffer.data(), size);

        return ContentUtils::CalculateSHA256(buffer.data(), size);
    } catch (const std::exception&) {
        return "";
    }
}

std::string ContentManager::GetContentTypeName(ContentType type) const {
    return ContentUtils::GetTypeName(type);
}

bool ContentManager::IsContentTypeSupported(ContentType type) const {
    return m_loaders.count(type) > 0;
}

void ContentManager::SetMaxConcurrentLoads(uint32_t maxLoads) {
    m_maxConcurrentLoads = maxLoads;
}

uint32_t ContentManager::GetMaxConcurrentLoads() const {
    return m_maxConcurrentLoads;
}

void ContentManager::SetLoadTimeout(std::chrono::milliseconds timeout) {
    m_loadTimeout = timeout;
}

std::chrono::milliseconds ContentManager::GetLoadTimeout() const {
    return m_loadTimeout;
}

void ContentManager::EnableCompression(bool enabled) {
    m_compressionEnabled = enabled;
}

bool ContentManager::IsCompressionEnabled() const {
    return m_compressionEnabled;
}

void ContentManager::EnableFileWatching(bool enabled) {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    if (m_fileWatchingEnabled == enabled) return;

    m_fileWatchingEnabled = enabled;

    if (enabled && !m_fileWatchingWorker.joinable()) {
        m_fileWatchingWorker = std::thread(&ContentManager::FileWatchingLoop, this);
    } else if (!enabled && m_fileWatchingWorker.joinable()) {
        // Stop will be handled by the loop checking m_fileWatchingEnabled
    }
}

bool ContentManager::IsFileWatchingEnabled() const {
    return m_fileWatchingEnabled;
}

void ContentManager::RefreshContent() {
    ScanContentDirectory();
}

void ContentManager::EnablePerformanceMonitoring(bool enabled) {
    m_performanceMonitoringEnabled = enabled;
}

std::unordered_map<std::string, uint64_t> ContentManager::GetLoadingStatistics() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_loadingStats;
}

std::vector<ContentInfo> ContentManager::GetMostAccessedContent(uint32_t count) const {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    std::vector<ContentInfo> contentList;
    for (const auto& [contentId, content] : m_content) {
        contentList.push_back(content);
    }

    // Sort by access count (descending)
    std::sort(contentList.begin(), contentList.end(),
              [](const ContentInfo& a, const ContentInfo& b) {
                  return a.accessCount > b.accessCount;
              });

    if (contentList.size() > count) {
        contentList.resize(count);
    }

    return contentList;
}

std::vector<ContentInfo> ContentManager::GetLargestContent(uint32_t count) const {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    std::vector<ContentInfo> contentList;
    for (const auto& [contentId, content] : m_content) {
        contentList.push_back(content);
    }

    // Sort by file size (descending)
    std::sort(contentList.begin(), contentList.end(),
              [](const ContentInfo& a, const ContentInfo& b) {
                  return a.fileSize > b.fileSize;
              });

    if (contentList.size() > count) {
        contentList.resize(count);
    }

    return contentList;
}

void ContentManager::ResetStatistics() {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    m_loadingStats.clear();
    m_lastAccess.clear();
}

void ContentManager::CleanupUnusedContent() {
    std::lock_guard<std::recursive_mutex> lock(m_contentMutex);

    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> toRemove;

    for (const auto& [contentId, content] : m_content) {
        // Consider content unused if not accessed for 24 hours and not persistent
        if (!content.isPersistent &&
            content.accessCount == 0 &&
            std::chrono::duration_cast<std::chrono::hours>(now - content.accessedAt).count() >= 24) {
            toRemove.push_back(contentId);
        }
    }

    for (const std::string& contentId : toRemove) {
        UnregisterContent(contentId);
    }

    if (!toRemove.empty()) {
        spdlog::info("[ContentManager] Cleaned up {} unused content items", toRemove.size());
    }
}

void ContentManager::OptimizeStorage() {
    // Clear cache and reload frequently accessed content
    ClearCache();

    auto mostAccessed = GetMostAccessedContent(10);
    for (const auto& content : mostAccessed) {
        if (content.state != ContentState::Ready) {
            LoadContentAsync(content.contentId, ContentPriority::High);
        }
    }

    spdlog::info("[ContentManager] Storage optimization completed");
}

} // namespace CoopNet
#pragma once

#include <RED4ext/RED4ext.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include <string>
#include <variant>
#include <optional>
#include <fstream>
#include <filesystem>

namespace CoopNet {

// Content types
enum class ContentType : uint8_t {
    Unknown = 0,
    Texture = 1,        // Image files (.png, .jpg, .dds, etc.)
    Model = 2,          // 3D models (.fbx, .obj, .dae, etc.)
    Audio = 3,          // Sound files (.wav, .mp3, .ogg, etc.)
    Video = 4,          // Video files (.mp4, .avi, .webm, etc.)
    Script = 5,         // REDscript files (.reds)
    Archive = 6,        // Game archives (.archive)
    Config = 7,         // Configuration files (.json, .yaml, .ini)
    Shader = 8,         // Shader files (.hlsl, .glsl)
    Animation = 9,      // Animation files (.anims)
    Level = 10,         // Level/world data
    UI = 11,            // UI layouts and resources
    Mod = 12,           // Mod packages
    Localization = 13,  // Language files
    Save = 14,          // Save game data
    Custom = 255        // Custom content types
};

// Content states
enum class ContentState : uint8_t {
    Unknown = 0,
    Loading = 1,
    Ready = 2,
    Error = 3,
    Unloaded = 4,
    Streaming = 5,
    Cached = 6,
    Expired = 7
};

// Content priorities
enum class ContentPriority : uint8_t {
    Critical = 0,       // Essential game content
    High = 1,           // Important content
    Medium = 2,         // Regular content
    Low = 3,            // Optional content
    Background = 4      // Preload content
};

// Content storage types
enum class StorageType : uint8_t {
    File = 0,           // File system storage
    Memory = 1,         // In-memory storage
    Network = 2,        // Network/remote storage
    Database = 3,       // Database storage
    Cache = 4,          // Cache storage
    Archive = 5         // Compressed archive storage
};

// Content validation types
enum class ContentValidationType : uint8_t {
    None = 0,
    Checksum = 1,       // Hash-based validation
    Signature = 2,      // Digital signature validation
    Version = 3,        // Version-based validation
    Dependency = 4,     // Dependency validation
    Schema = 5,         // Schema validation
    Custom = 6          // Custom validation
};

// Forward declarations
struct ContentInfo;
struct ContentMetadata;
struct ContentDependency;
struct ContentManifest;
struct ContentPackage;
struct ContentCache;
struct StreamingRequest;

// Content validation
struct ContentValidation {
    ContentValidationType type;
    std::string expectedChecksum;
    std::string actualChecksum;
    std::string signature;
    std::string publicKey;
    std::string schema;
    std::function<bool(const ContentInfo&)> customValidator;

    bool isValid;
    std::string errorMessage;
    std::chrono::steady_clock::time_point lastValidated;
};

// Content information
struct ContentInfo {
    std::string contentId;
    std::string name;
    std::string description;
    std::string version;
    ContentType type;
    ContentState state;
    ContentPriority priority;
    StorageType storageType;

    // File information
    std::string filePath;
    std::string url;
    uint64_t fileSize;
    std::string checksum;
    std::string mimeType;

    // Timestamps
    std::chrono::steady_clock::time_point createdAt;
    std::chrono::steady_clock::time_point modifiedAt;
    std::chrono::steady_clock::time_point accessedAt;
    std::chrono::steady_clock::time_point expiresAt;

    // Usage tracking
    uint64_t accessCount;
    uint64_t downloadCount;
    uint64_t loadTime; // microseconds
    bool isPersistent;
    bool isCompressed;
    bool isEncrypted;

    // Relationships
    std::vector<ContentDependency> dependencies;
    std::vector<std::string> tags;
    std::unordered_map<std::string, std::string> attributes;

    // Memory information
    void* data = nullptr;
    uint64_t memorySize = 0;
    bool isMemoryMapped = false;
};

// Content metadata
struct ContentMetadata {
    std::string author;
    std::string license;
    std::string copyright;
    std::string website;
    std::string category;
    std::string language;
    std::vector<std::string> keywords;
    std::unordered_map<std::string, std::string> customFields;

    // Technical metadata
    std::string engine;
    std::string targetPlatform;
    std::string minimumVersion;
    std::string maximumVersion;
    std::vector<std::string> requiredFeatures;

    // Rating and quality
    float rating = 0.0f;
    uint32_t downloadCount = 0;
    uint32_t ratingCount = 0;
    std::string qualityLevel; // "low", "medium", "high", "ultra"
};

// Content dependency
struct ContentDependency {
    std::string contentId;
    std::string name;
    std::string version;
    std::string minimumVersion;
    std::string maximumVersion;
    bool isOptional;
    bool isLoaded;
    std::string loadOrder; // "before", "after", "any"
    std::unordered_map<std::string, std::string> conditions;
};

// Content manifest for packages
struct ContentManifest {
    std::string manifestId;
    std::string name;
    std::string version;
    std::string description;
    ContentMetadata metadata;

    std::vector<ContentInfo> contents;
    std::vector<ContentDependency> dependencies;
    std::vector<std::string> installScript;
    std::vector<std::string> uninstallScript;

    // Installation information
    std::string installPath;
    std::vector<std::string> backupFiles;
    std::unordered_map<std::string, std::string> environment;

    // Validation
    ContentValidation validation;
    std::chrono::steady_clock::time_point lastValidated;
    bool isValid;
};

// Content package
struct ContentPackage {
    std::string packageId;
    std::string name;
    std::string version;
    std::string filePath;
    ContentManifest manifest;

    ContentState state;
    uint64_t totalSize;
    uint64_t installedSize;
    float installProgress;

    std::chrono::steady_clock::time_point installTime;
    std::chrono::steady_clock::time_point lastUpdateCheck;
    bool autoUpdate;
    bool isActive;

    std::vector<std::string> conflictingPackages;
    std::unordered_map<std::string, std::string> settings;
};

// Content cache entry
struct ContentCache {
    std::string cacheKey;
    std::string contentId;
    void* data;
    uint64_t size;
    ContentType type;

    std::chrono::steady_clock::time_point createdAt;
    std::chrono::steady_clock::time_point lastAccessed;
    std::chrono::steady_clock::time_point expiresAt;

    uint32_t accessCount;
    uint32_t hitCount;
    bool isPinned;
    bool isCompressed;
    uint32_t compressionRatio;

    std::function<void()> destructor;
};

// Streaming request
struct StreamingRequest {
    std::string requestId;
    std::string contentId;
    ContentPriority priority;
    std::string url;
    std::string localPath;

    std::chrono::steady_clock::time_point requestTime;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;

    uint64_t totalBytes;
    uint64_t downloadedBytes;
    float progress;
    float downloadSpeed; // bytes per second

    bool isActive;
    bool isPaused;
    bool isCancelled;
    std::string errorMessage;

    std::function<void(float)> progressCallback;
    std::function<void(bool, const std::string&)> completionCallback;
};

// Content loading callbacks
struct ContentCallbacks {
    std::function<void(const std::string&, ContentState)> onStateChanged;
    std::function<void(const std::string&, float)> onLoadProgress;
    std::function<void(const std::string&, const std::string&)> onError;
    std::function<void(const std::string&)> onLoaded;
    std::function<void(const std::string&)> onUnloaded;
};

// Content loader interface
class IContentLoader {
public:
    virtual ~IContentLoader() = default;
    virtual bool CanLoad(ContentType type) const = 0;
    virtual bool LoadContent(ContentInfo& content) = 0;
    virtual bool UnloadContent(ContentInfo& content) = 0;
    virtual bool ValidateContent(const ContentInfo& content) = 0;
    virtual std::string GetLastError() const = 0;
};

// Content provider interface
class IContentProvider {
public:
    virtual ~IContentProvider() = default;
    virtual bool CanProvide(const std::string& contentId) const = 0;
    virtual std::optional<ContentInfo> GetContentInfo(const std::string& contentId) = 0;
    virtual bool DownloadContent(const std::string& contentId, const std::string& localPath) = 0;
    virtual std::vector<ContentInfo> SearchContent(const std::string& query) = 0;
    virtual bool IsAvailable() const = 0;
};

// Content storage interface
class IContentStorage {
public:
    virtual ~IContentStorage() = default;
    virtual bool Store(const std::string& key, const void* data, uint64_t size) = 0;
    virtual bool Retrieve(const std::string& key, void*& data, uint64_t& size) = 0;
    virtual bool Remove(const std::string& key) = 0;
    virtual bool Exists(const std::string& key) const = 0;
    virtual uint64_t GetSize(const std::string& key) const = 0;
    virtual std::vector<std::string> ListKeys() const = 0;
    virtual void Clear() = 0;
};

// Main content management system
class ContentManager {
public:
    static ContentManager& Instance();

    // System lifecycle
    bool Initialize(const std::string& contentDirectory = "content/");
    void Shutdown();

    // Content loading
    bool LoadContent(const std::string& contentId, ContentPriority priority = ContentPriority::Medium);
    bool LoadContentAsync(const std::string& contentId, ContentPriority priority = ContentPriority::Medium);
    bool UnloadContent(const std::string& contentId);
    bool ReloadContent(const std::string& contentId);

    // Content information
    std::optional<ContentInfo> GetContentInfo(const std::string& contentId) const;
    ContentState GetContentState(const std::string& contentId) const;
    std::vector<std::string> GetLoadedContent() const;
    std::vector<ContentInfo> GetAllContent() const;

    // Content registration
    bool RegisterContent(const ContentInfo& content);
    bool UnregisterContent(const std::string& contentId);
    bool UpdateContentInfo(const ContentInfo& content);

    // Content search and discovery
    std::vector<ContentInfo> SearchContent(const std::string& query) const;
    std::vector<ContentInfo> GetContentByType(ContentType type) const;
    std::vector<ContentInfo> GetContentByTag(const std::string& tag) const;
    std::vector<ContentInfo> GetContentByCategory(const std::string& category) const;

    // Package management
    bool InstallPackage(const std::string& packagePath);
    bool UninstallPackage(const std::string& packageId);
    bool UpdatePackage(const std::string& packageId);
    std::vector<ContentPackage> GetInstalledPackages() const;
    std::optional<ContentPackage> GetPackageInfo(const std::string& packageId) const;

    // Manifest operations
    bool LoadManifest(const std::string& manifestPath);
    bool SaveManifest(const std::string& manifestPath, const ContentManifest& manifest);
    bool ValidateManifest(const ContentManifest& manifest);
    ContentManifest CreateManifest(const std::vector<std::string>& contentIds);

    // Content validation
    bool ValidateContent(const std::string& contentId);
    bool ValidateAllContent();
    std::vector<std::string> GetInvalidContent() const;
    bool RepairContent(const std::string& contentId);

    // Content caching
    void SetCacheSize(uint64_t maxSize);
    uint64_t GetCacheSize() const;
    uint64_t GetCacheUsage() const;
    void ClearCache();
    void PinContentInCache(const std::string& contentId);
    void UnpinContentFromCache(const std::string& contentId);

    // Content streaming
    std::string StartContentStream(const std::string& contentId, const std::string& url);
    bool PauseContentStream(const std::string& requestId);
    bool ResumeContentStream(const std::string& requestId);
    bool CancelContentStream(const std::string& requestId);
    float GetStreamProgress(const std::string& requestId) const;

    // Content providers
    void RegisterProvider(const std::string& name, std::shared_ptr<IContentProvider> provider);
    void UnregisterProvider(const std::string& name);
    std::vector<std::string> GetProviderNames() const;
    bool RefreshProvidersContent();

    // Content loaders
    void RegisterLoader(ContentType type, std::shared_ptr<IContentLoader> loader);
    void UnregisterLoader(ContentType type);
    std::vector<ContentType> GetSupportedTypes() const;

    // Storage management
    void SetDefaultStorage(std::shared_ptr<IContentStorage> storage);
    void RegisterStorage(const std::string& name, std::shared_ptr<IContentStorage> storage);
    std::shared_ptr<IContentStorage> GetStorage(const std::string& name) const;

    // Dependencies
    bool ResolveDependencies(const std::string& contentId);
    std::vector<std::string> GetDependencies(const std::string& contentId) const;
    std::vector<std::string> GetDependents(const std::string& contentId) const;
    bool CheckDependencyLoop(const std::string& contentId) const;

    // Content monitoring
    void EnableFileWatching(bool enabled);
    bool IsFileWatchingEnabled() const;
    void RefreshContent();
    void ScanContentDirectory(const std::string& directory = "");

    // Performance and statistics
    void EnablePerformanceMonitoring(bool enabled);
    std::unordered_map<std::string, uint64_t> GetLoadingStatistics() const;
    std::vector<ContentInfo> GetMostAccessedContent(uint32_t count = 10) const;
    std::vector<ContentInfo> GetLargestContent(uint32_t count = 10) const;
    void ResetStatistics();

    // Events and callbacks
    void SetCallbacks(const ContentCallbacks& callbacks);
    void ClearCallbacks();

    // Utility functions
    std::string GenerateContentId(const std::string& filePath) const;
    std::string CalculateChecksum(const std::string& filePath) const;
    ContentType DetectContentType(const std::string& filePath) const;
    std::string GetContentTypeName(ContentType type) const;
    bool IsContentTypeSupported(ContentType type) const;

    // Configuration
    void SetMaxConcurrentLoads(uint32_t maxLoads);
    uint32_t GetMaxConcurrentLoads() const;
    void SetLoadTimeout(std::chrono::milliseconds timeout);
    std::chrono::milliseconds GetLoadTimeout() const;
    void EnableCompression(bool enabled);
    bool IsCompressionEnabled() const;

    // Import/Export
    bool ExportContent(const std::string& contentId, const std::string& exportPath);
    bool ImportContent(const std::string& importPath, const std::string& contentId = "");
    bool ExportPackage(const std::string& packageId, const std::string& exportPath);
    bool ImportPackage(const std::string& importPath);

    // Content cleanup
    void CleanupUnusedContent();
    void CleanupExpiredContent();
    void CleanupCache();
    void OptimizeStorage();

private:
    ContentManager() = default;
    ~ContentManager() = default;
    ContentManager(const ContentManager&) = delete;
    ContentManager& operator=(const ContentManager&) = delete;

    // Core operations
    bool LoadContentInternal(ContentInfo& content);
    bool UnloadContentInternal(ContentInfo& content);
    std::shared_ptr<IContentLoader> GetLoader(ContentType type);

    // Cache management
    bool CacheContent(const std::string& contentId, const void* data, uint64_t size);
    bool GetCachedContent(const std::string& contentId, void*& data, uint64_t& size);
    void EvictCache();
    std::string GenerateCacheKey(const std::string& contentId) const;

    // File operations
    bool ValidateFileIntegrity(const ContentInfo& content);
    bool CompressContent(const std::string& sourcePath, const std::string& destPath);
    bool DecompressContent(const std::string& sourcePath, const std::string& destPath);

    // Dependency resolution
    std::vector<std::string> TopologicalSort(const std::vector<std::string>& contentIds);
    bool HasCircularDependency(const std::string& contentId, std::unordered_set<std::string>& visited) const;

    // Threading
    void LoadingWorkerLoop();
    void StreamingWorkerLoop();
    void FileWatchingLoop();
    void MaintenanceLoop();

    // Event handling
    void NotifyStateChanged(const std::string& contentId, ContentState newState);
    void NotifyProgress(const std::string& contentId, float progress);
    void NotifyError(const std::string& contentId, const std::string& error);
    void NotifyLoaded(const std::string& contentId);
    void NotifyUnloaded(const std::string& contentId);

    // Utility methods
    std::string GetMimeType(const std::string& filePath) const;
    bool CreateDirectoryIfNotExists(const std::string& path);
    uint64_t GetFileSize(const std::string& filePath) const;
    std::string GetFileChecksum(const std::string& filePath) const;

    // Data storage
    std::unordered_map<std::string, ContentInfo> m_content;
    std::unordered_map<std::string, ContentPackage> m_packages;
    std::unordered_map<std::string, ContentCache> m_cache;
    std::unordered_map<std::string, StreamingRequest> m_streamingRequests;

    // Content providers and loaders
    std::unordered_map<std::string, std::shared_ptr<IContentProvider>> m_providers;
    std::unordered_map<ContentType, std::shared_ptr<IContentLoader>> m_loaders;
    std::unordered_map<std::string, std::shared_ptr<IContentStorage>> m_storages;
    std::shared_ptr<IContentStorage> m_defaultStorage;

    // Loading management
    std::queue<std::string> m_loadingQueue;
    std::queue<StreamingRequest> m_streamingQueue;
    std::unordered_set<std::string> m_currentlyLoading;

    // Configuration
    std::string m_contentDirectory = "content/";
    uint64_t m_maxCacheSize = 512 * 1024 * 1024; // 512MB
    uint32_t m_maxConcurrentLoads = 4;
    std::chrono::milliseconds m_loadTimeout{30000}; // 30 seconds
    bool m_compressionEnabled = true;
    bool m_fileWatchingEnabled = false;
    bool m_performanceMonitoringEnabled = false;

    // Statistics
    std::unordered_map<std::string, uint64_t> m_loadingStats;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_lastAccess;

    // Callbacks
    ContentCallbacks m_callbacks;

    // Threading
    std::vector<std::thread> m_loadingWorkers;
    std::thread m_streamingWorker;
    std::thread m_fileWatchingWorker;
    std::thread m_maintenanceWorker;

    // Synchronization
    mutable std::recursive_mutex m_contentMutex;
    mutable std::mutex m_cacheMutex;
    mutable std::mutex m_loadingMutex;
    mutable std::mutex m_streamingMutex;
    mutable std::mutex m_statsMutex;
    mutable std::mutex m_callbackMutex;

    // System state
    bool m_initialized = false;
    std::atomic<bool> m_shouldStop{false};

    // ID generation
    std::atomic<uint64_t> m_nextRequestId{1};

    // File watching
    std::unordered_map<std::string, std::filesystem::file_time_type> m_fileTimestamps;
};

// Content utilities
namespace ContentUtils {
    std::string GetTypeName(ContentType type);
    std::string GetStateName(ContentState state);
    std::string GetPriorityName(ContentPriority priority);
    std::string GetStorageTypeName(StorageType type);

    // File utilities
    bool IsValidContentFile(const std::string& filePath);
    std::string GetFileExtension(const std::string& filePath);
    ContentType GetTypeFromExtension(const std::string& extension);
    std::vector<std::string> GetSupportedExtensions(ContentType type);

    // Compression utilities
    bool CompressData(const void* input, uint64_t inputSize, std::vector<uint8_t>& output);
    bool DecompressData(const void* input, uint64_t inputSize, std::vector<uint8_t>& output);
    uint32_t CalculateCompressionRatio(uint64_t originalSize, uint64_t compressedSize);

    // Checksum utilities
    std::string CalculateMD5(const void* data, uint64_t size);
    std::string CalculateSHA256(const void* data, uint64_t size);
    bool VerifyChecksum(const std::string& filePath, const std::string& expectedChecksum, const std::string& algorithm = "SHA256");

    // Validation utilities
    bool ValidateContentFormat(const std::string& filePath, ContentType type);
    bool ValidateManifestFormat(const std::string& manifestPath);
    std::vector<std::string> ValidatePackage(const std::string& packagePath);

    // Dependency utilities
    std::vector<std::string> ExtractDependencies(const std::string& filePath);
    bool CheckVersionCompatibility(const std::string& required, const std::string& available);
    std::string ResolveVersion(const std::string& versionSpec);
}

} // namespace CoopNet
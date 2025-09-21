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
#include <algorithm>

namespace CoopNet {

// Network optimization strategies
enum class OptimizationStrategy : uint8_t {
    Aggressive = 0,     // Maximum compression, minimum latency
    Balanced = 1,       // Balance between compression and CPU usage
    Conservative = 2,   // Minimal CPU usage, reliability focused
    Adaptive = 3        // Automatically adjust based on conditions
};

// Packet priority levels
enum class PacketPriority : uint8_t {
    Critical = 0,       // Player movement, combat actions
    High = 1,           // Voice chat, important game events
    Medium = 2,         // Inventory updates, chat messages
    Low = 3,            // Statistics, background sync
    Background = 4      // Telemetry, analytics
};

// Compression algorithms
enum class CompressionType : uint8_t {
    None = 0,
    LZ4 = 1,
    ZSTD = 2,
    Custom = 3
};

// Network adaptation modes
enum class AdaptationMode : uint8_t {
    Manual = 0,         // Fixed settings
    Bandwidth = 1,      // Adapt to bandwidth changes
    Latency = 2,        // Adapt to latency changes
    Quality = 3,        // Adapt to connection quality
    Full = 4            // Adapt to all conditions
};

// Forward declarations
struct NetworkPacket;
struct NetworkMetrics;
struct OptimizationProfile;
struct BandwidthManager;
struct PacketScheduler;

// Enhanced network packet structure
struct NetworkPacket {
    uint32_t packetId;
    uint32_t sourcePlayerId;
    uint32_t targetPlayerId;
    PacketPriority priority;
    CompressionType compression;
    uint32_t originalSize;
    uint32_t compressedSize;
    std::vector<uint8_t> data;
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point deadline;
    uint32_t retryCount;
    bool requiresAck;
    bool isReliable;
    uint32_t sequenceNumber;
    uint32_t channelId;
    std::string packetType;
};

// Network performance metrics
struct NetworkMetrics {
    // Latency metrics
    float currentLatency = 0.0f;
    float averageLatency = 0.0f;
    float minLatency = 0.0f;
    float maxLatency = 0.0f;
    float jitter = 0.0f;

    // Bandwidth metrics
    uint64_t currentBandwidthUp = 0;
    uint64_t currentBandwidthDown = 0;
    uint64_t averageBandwidthUp = 0;
    uint64_t averageBandwidthDown = 0;
    uint64_t peakBandwidthUp = 0;
    uint64_t peakBandwidthDown = 0;

    // Packet metrics
    uint64_t packetsSent = 0;
    uint64_t packetsReceived = 0;
    uint64_t packetsLost = 0;
    uint64_t packetsRetransmitted = 0;
    uint64_t bytesSent = 0;
    uint64_t bytesReceived = 0;
    uint64_t bytesCompressed = 0;
    uint64_t bytesDecompressed = 0;

    // Quality metrics
    float packetLossRate = 0.0f;
    float compressionRatio = 0.0f;
    float connectionQuality = 1.0f;
    uint32_t congestionEvents = 0;

    std::chrono::steady_clock::time_point lastUpdate;
};

// Optimization profile configuration
struct OptimizationProfile {
    std::string profileName;
    OptimizationStrategy strategy;
    AdaptationMode adaptationMode;

    // Compression settings
    CompressionType defaultCompression;
    std::unordered_map<PacketPriority, CompressionType> compressionByPriority;
    uint32_t compressionThreshold; // Minimum packet size for compression

    // Bandwidth management
    uint64_t maxBandwidthUp;   // bytes per second
    uint64_t maxBandwidthDown; // bytes per second
    float bandwidthUtilization; // Target utilization (0.0-1.0)

    // Packet scheduling
    uint32_t maxPacketsPerFrame;
    uint32_t maxRetries;
    std::chrono::milliseconds retryTimeout;
    std::chrono::milliseconds maxPacketAge;

    // Quality adaptation
    float latencyThreshold;     // ms
    float packetLossThreshold; // percentage
    float jitterThreshold;     // ms
    bool enableCongestionControl;
    bool enableAdaptiveCompression;
    bool enablePacketAggregation;

    // Performance settings
    uint32_t processingThreads;
    uint32_t bufferSize;
    bool enableZeroCopy;
    bool enableBatching;
};

// Bandwidth manager for traffic shaping
struct BandwidthManager {
    uint64_t allocatedBandwidthUp = 0;
    uint64_t allocatedBandwidthDown = 0;
    uint64_t usedBandwidthUp = 0;
    uint64_t usedBandwidthDown = 0;
    std::chrono::steady_clock::time_point lastReset;

    // Token bucket for rate limiting
    struct TokenBucket {
        uint64_t capacity;
        uint64_t tokens;
        uint64_t refillRate; // tokens per second
        std::chrono::steady_clock::time_point lastRefill;
    } upstreamBucket, downstreamBucket;

    // Priority-based allocation
    std::unordered_map<PacketPriority, float> priorityWeights;
    std::unordered_map<PacketPriority, uint64_t> priorityQuotas;
};

// Packet scheduler for prioritization and batching
struct PacketScheduler {
    // Priority queues
    std::unordered_map<PacketPriority, std::queue<NetworkPacket>> priorityQueues;
    std::mutex queueMutex;

    // Scheduling configuration
    uint32_t maxBatchSize = 10;
    std::chrono::milliseconds batchTimeout{5};
    std::chrono::milliseconds schedulingInterval{1};

    // Batching support
    std::vector<NetworkPacket> pendingBatch;
    std::chrono::steady_clock::time_point batchStartTime;

    // Statistics
    uint64_t totalScheduled = 0;
    uint64_t totalBatched = 0;
    uint32_t currentQueueSize = 0;
};

// Main network optimization system
class NetworkOptimizer {
public:
    static NetworkOptimizer& Instance();

    // System lifecycle
    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);

    // Profile management
    bool LoadProfile(const std::string& profileName);
    bool SaveProfile(const std::string& profileName, const OptimizationProfile& profile);
    void SetProfile(const OptimizationProfile& profile);
    OptimizationProfile GetCurrentProfile() const;
    std::vector<std::string> GetAvailableProfiles() const;

    // Packet processing
    bool OptimizePacket(NetworkPacket& packet);
    bool CompressPacket(NetworkPacket& packet, CompressionType compression);
    bool DecompressPacket(NetworkPacket& packet);
    PacketPriority DeterminePacketPriority(const NetworkPacket& packet);

    // Bandwidth management
    bool AllocateBandwidth(PacketPriority priority, uint64_t bytes);
    void UpdateBandwidthUsage(uint64_t bytesSent, uint64_t bytesReceived);
    uint64_t GetAvailableBandwidth(bool upstream = true) const;
    float GetBandwidthUtilization(bool upstream = true) const;

    // Packet scheduling
    bool SchedulePacket(const NetworkPacket& packet);
    std::vector<NetworkPacket> GetScheduledPackets(uint32_t maxCount = 0);
    bool HasPendingPackets() const;
    uint32_t GetQueueSize(PacketPriority priority = PacketPriority::Critical) const;

    // Adaptive optimization
    void EnableAdaptation(bool enabled);
    bool IsAdaptationEnabled() const;
    void UpdateNetworkConditions(float latency, float packetLoss, uint64_t bandwidth);
    void TriggerAdaptation();

    // Compression management
    bool SetCompressionForPriority(PacketPriority priority, CompressionType compression);
    CompressionType GetCompressionForPriority(PacketPriority priority) const;
    float GetCompressionRatio() const;
    bool IsCompressionBeneficial(const NetworkPacket& packet, CompressionType compression);

    // Congestion control
    void DetectCongestion();
    void HandleCongestion();
    bool IsCongestionDetected() const;
    void SetCongestionThresholds(float latency, float packetLoss);

    // Metrics and monitoring
    NetworkMetrics GetMetrics() const;
    void ResetMetrics();
    void UpdateMetrics();

    // Quality of Service (QoS)
    void SetPriorityWeights(const std::unordered_map<PacketPriority, float>& weights);
    std::unordered_map<PacketPriority, float> GetPriorityWeights() const;
    void EnableTrafficShaping(bool enabled);
    bool IsTrafficShapingEnabled() const;

    // Advanced features
    bool EnablePacketAggregation(bool enabled);
    bool IsPacketAggregationEnabled() const;
    void SetMaxRetries(uint32_t maxRetries);
    uint32_t GetMaxRetries() const;
    void EnableReliableTransmission(bool enabled);
    bool IsReliableTransmissionEnabled() const;

    // Optimization control
    bool IsOptimizationActive() const;
    void SetPacketPriority(const std::string& packetType, PacketPriority priority);

    // Performance optimization
    void OptimizeForLatency();
    void OptimizeForBandwidth();
    void OptimizeForReliability();
    void ApplyOptimizationStrategy(OptimizationStrategy strategy);

    // Event callbacks
    using OptimizationEventCallback = std::function<void(const std::string& event, const std::string& data)>;
    void RegisterEventCallback(const std::string& eventType, OptimizationEventCallback callback);
    void UnregisterEventCallback(const std::string& eventType);

private:
    NetworkOptimizer() = default;
    ~NetworkOptimizer() = default;
    NetworkOptimizer(const NetworkOptimizer&) = delete;
    NetworkOptimizer& operator=(const NetworkOptimizer&) = delete;

    // Core optimization processing
    void ProcessingLoop();
    void ProcessPacketQueue();
    void ProcessBandwidthManagement();
    void ProcessAdaptation();
    void ProcessCongestionControl();

    // Compression algorithms
    std::vector<uint8_t> CompressLZ4(const std::vector<uint8_t>& data);
    std::vector<uint8_t> DecompressLZ4(const std::vector<uint8_t>& data);
    std::vector<uint8_t> CompressZSTD(const std::vector<uint8_t>& data);
    std::vector<uint8_t> DecompressZSTD(const std::vector<uint8_t>& data);

    // Bandwidth management
    void RefillTokenBucket(BandwidthManager::TokenBucket& bucket);
    bool ConsumeTokens(BandwidthManager::TokenBucket& bucket, uint64_t tokens);
    void UpdateBandwidthQuotas();

    // Packet scheduling algorithms
    void ScheduleByPriority();
    void ScheduleRoundRobin();
    void ScheduleWeightedFair();
    std::vector<NetworkPacket> CreatePacketBatch();

    // Adaptive algorithms
    void AdaptToLatency(float latency);
    void AdaptToBandwidth(uint64_t bandwidth);
    void AdaptToPacketLoss(float packetLoss);
    void AdaptCompressionSettings();
    void AdaptSchedulingSettings();

    // Congestion detection algorithms
    bool DetectLatencyBasedCongestion();
    bool DetectLossBasedCongestion();
    bool DetectBandwidthBasedCongestion();
    void ApplyCongestionResponse();

    // Utility methods
    uint32_t GeneratePacketId();
    std::chrono::steady_clock::time_point GetCurrentTime() const;
    void TriggerEvent(const std::string& eventType, const std::string& data);
    std::string SerializeProfile(const OptimizationProfile& profile) const;
    OptimizationProfile DeserializeProfile(const std::string& data) const;

    // Data storage
    OptimizationProfile m_currentProfile;
    NetworkMetrics m_metrics;
    BandwidthManager m_bandwidthManager;
    PacketScheduler m_scheduler;

    // Adaptation state
    bool m_adaptationEnabled = true;
    bool m_congestionDetected = false;
    std::chrono::steady_clock::time_point m_lastAdaptation;

    // Processing threads
    std::thread m_processingThread;
    std::atomic<bool> m_shouldStop{false};

    // Synchronization
    mutable std::mutex m_profileMutex;
    mutable std::mutex m_metricsMutex;
    mutable std::mutex m_bandwidthMutex;

    // System state
    bool m_initialized = false;
    bool m_trafficShapingEnabled = true;
    bool m_packetAggregationEnabled = true;
    bool m_reliableTransmissionEnabled = true;

    // Event system
    std::unordered_map<std::string, std::vector<OptimizationEventCallback>> m_eventCallbacks;
    std::mutex m_callbackMutex;

    // Packet type priority mapping
    std::unordered_map<std::string, PacketPriority> m_packetTypePriorities;

    // Performance tracking
    std::queue<float> m_latencyHistory;
    std::queue<float> m_packetLossHistory;
    std::queue<uint64_t> m_bandwidthHistory;

    // ID generation
    std::atomic<uint32_t> m_nextPacketId{1};

    // Congestion control state
    struct CongestionState {
        float slowStartThreshold = 65536.0f;
        float congestionWindow = 1.0f;
        uint32_t duplicateAcks = 0;
        bool fastRecovery = false;
        std::chrono::steady_clock::time_point lastCongestionEvent;
    } m_congestionState;
};

// Network optimization presets
namespace OptimizationPresets {
    OptimizationProfile GetLowLatencyProfile();
    OptimizationProfile GetHighBandwidthProfile();
    OptimizationProfile GetReliabilityProfile();
    OptimizationProfile GetBalancedProfile();
    OptimizationProfile GetMobileProfile();
}

// Utility functions for network optimization
namespace NetworkUtils {
    std::string GetStrategyName(OptimizationStrategy strategy);
    std::string GetPriorityName(PacketPriority priority);
    std::string GetCompressionName(CompressionType compression);
    std::string GetAdaptationModeName(AdaptationMode mode);

    // Network calculations
    float CalculatePacketLoss(uint64_t sent, uint64_t received);
    float CalculateJitter(const std::vector<float>& latencies);
    uint64_t EstimateOptimalBandwidth(const NetworkMetrics& metrics);
    float CalculateConnectionQuality(const NetworkMetrics& metrics);

    // Compression utilities
    uint32_t EstimateCompressionSavings(const std::vector<uint8_t>& data, CompressionType compression);
    bool ShouldCompressPacket(const NetworkPacket& packet, float compressionThreshold = 0.1f);

    // Scheduling utilities
    PacketPriority GetOptimalPriority(const std::string& packetType);
    std::chrono::milliseconds CalculateOptimalTimeout(PacketPriority priority, float latency);
}

} // namespace CoopNet
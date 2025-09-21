#include "NetworkOptimizer.hpp"
#include "../core/Logger.hpp"
#include <lz4.h>
#include <zstd.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>

namespace CoopNet {

NetworkOptimizer& NetworkOptimizer::Instance() {
    static NetworkOptimizer instance;
    return instance;
}

bool NetworkOptimizer::Initialize() {
    if (m_initialized) {
        return true;
    }

    spdlog::info("[NetworkOptimizer] Initializing network protocol optimization system");

    // Load default profile
    m_currentProfile = OptimizationPresets::GetBalancedProfile();

    // Initialize bandwidth manager
    m_bandwidthManager.allocatedBandwidthUp = m_currentProfile.maxBandwidthUp;
    m_bandwidthManager.allocatedBandwidthDown = m_currentProfile.maxBandwidthDown;

    // Initialize token buckets
    m_bandwidthManager.upstreamBucket.capacity = m_currentProfile.maxBandwidthUp;
    m_bandwidthManager.upstreamBucket.tokens = m_currentProfile.maxBandwidthUp;
    m_bandwidthManager.upstreamBucket.refillRate = m_currentProfile.maxBandwidthUp;

    m_bandwidthManager.downstreamBucket.capacity = m_currentProfile.maxBandwidthDown;
    m_bandwidthManager.downstreamBucket.tokens = m_currentProfile.maxBandwidthDown;
    m_bandwidthManager.downstreamBucket.refillRate = m_currentProfile.maxBandwidthDown;

    // Set priority weights
    m_bandwidthManager.priorityWeights[PacketPriority::Critical] = 0.4f;
    m_bandwidthManager.priorityWeights[PacketPriority::High] = 0.3f;
    m_bandwidthManager.priorityWeights[PacketPriority::Medium] = 0.2f;
    m_bandwidthManager.priorityWeights[PacketPriority::Low] = 0.08f;
    m_bandwidthManager.priorityWeights[PacketPriority::Background] = 0.02f;

    // Initialize packet scheduler
    m_scheduler.maxBatchSize = 10;
    m_scheduler.batchTimeout = std::chrono::milliseconds(5);

    // Reset metrics
    ResetMetrics();

    // Start processing thread
    m_shouldStop.store(false);
    m_processingThread = std::thread(&NetworkOptimizer::ProcessingLoop, this);

    m_initialized = true;
    spdlog::info("[NetworkOptimizer] Network optimization system initialized with profile: {}",
                 m_currentProfile.profileName);

    TriggerEvent("optimizer_initialized", m_currentProfile.profileName);
    return true;
}

void NetworkOptimizer::Shutdown() {
    if (!m_initialized) {
        return;
    }

    spdlog::info("[NetworkOptimizer] Shutting down network optimization system");

    // Stop processing thread
    m_shouldStop.store(true);
    if (m_processingThread.joinable()) {
        m_processingThread.join();
    }

    // Clear queues
    {
        std::lock_guard<std::mutex> lock(m_scheduler.queueMutex);
        for (auto& [priority, queue] : m_scheduler.priorityQueues) {
            while (!queue.empty()) {
                queue.pop();
            }
        }
    }

    m_initialized = false;
    TriggerEvent("optimizer_shutdown", "");
}

void NetworkOptimizer::ProcessingLoop() {
    spdlog::debug("[NetworkOptimizer] Processing thread started");

    auto lastProcessTime = std::chrono::steady_clock::now();

    while (!m_shouldStop.load()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastProcessTime);

        if (elapsed >= m_scheduler.schedulingInterval) {
            // Process packet queue
            ProcessPacketQueue();

            // Process bandwidth management
            ProcessBandwidthManagement();

            // Process adaptation if enabled
            if (m_adaptationEnabled) {
                ProcessAdaptation();
            }

            // Process congestion control
            ProcessCongestionControl();

            // Update metrics
            UpdateMetrics();

            lastProcessTime = now;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    spdlog::debug("[NetworkOptimizer] Processing thread stopped");
}

bool NetworkOptimizer::OptimizePacket(NetworkPacket& packet) {
    // Determine packet priority
    packet.priority = DeterminePacketPriority(packet);

    // Apply compression if beneficial
    CompressionType compression = GetCompressionForPriority(packet.priority);
    if (compression != CompressionType::None &&
        packet.data.size() >= m_currentProfile.compressionThreshold) {

        if (IsCompressionBeneficial(packet, compression)) {
            if (!CompressPacket(packet, compression)) {
                spdlog::warn("[NetworkOptimizer] Failed to compress packet {}", packet.packetId);
            }
        }
    }

    // Set deadline based on priority
    auto timeout = NetworkUtils::CalculateOptimalTimeout(packet.priority, m_metrics.currentLatency);
    packet.deadline = packet.timestamp + timeout;

    // Update packet metrics
    {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        m_metrics.packetsSent++;
        m_metrics.bytesSent += packet.data.size();
    }

    return true;
}

bool NetworkOptimizer::CompressPacket(NetworkPacket& packet, CompressionType compression) {
    if (packet.data.empty()) {
        return false;
    }

    packet.originalSize = static_cast<uint32_t>(packet.data.size());
    std::vector<uint8_t> compressedData;

    switch (compression) {
        case CompressionType::LZ4:
            compressedData = CompressLZ4(packet.data);
            break;
        case CompressionType::ZSTD:
            compressedData = CompressZSTD(packet.data);
            break;
        default:
            return false;
    }

    if (compressedData.empty() || compressedData.size() >= packet.data.size()) {
        // Compression not beneficial
        return false;
    }

    packet.data = std::move(compressedData);
    packet.compressedSize = static_cast<uint32_t>(packet.data.size());
    packet.compression = compression;

    // Update compression metrics
    {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        m_metrics.bytesCompressed += packet.originalSize;

        if (packet.originalSize > 0) {
            float ratio = static_cast<float>(packet.compressedSize) / packet.originalSize;
            m_metrics.compressionRatio = (m_metrics.compressionRatio * 0.9f) + (ratio * 0.1f);
        }
    }

    spdlog::debug("[NetworkOptimizer] Compressed packet {} from {} to {} bytes (ratio: {:.2f})",
                  packet.packetId, packet.originalSize, packet.compressedSize,
                  static_cast<float>(packet.compressedSize) / packet.originalSize);

    return true;
}

std::vector<uint8_t> NetworkOptimizer::CompressLZ4(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return {};
    }

    int maxCompressedSize = LZ4_compressBound(static_cast<int>(data.size()));
    std::vector<uint8_t> compressed(maxCompressedSize);

    int compressedSize = LZ4_compress_default(
        reinterpret_cast<const char*>(data.data()),
        reinterpret_cast<char*>(compressed.data()),
        static_cast<int>(data.size()),
        maxCompressedSize
    );

    if (compressedSize <= 0) {
        return {};
    }

    compressed.resize(compressedSize);
    return compressed;
}

std::vector<uint8_t> NetworkOptimizer::CompressZSTD(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return {};
    }

    size_t maxCompressedSize = ZSTD_compressBound(data.size());
    std::vector<uint8_t> compressed(maxCompressedSize);

    size_t compressedSize = ZSTD_compress(
        compressed.data(), maxCompressedSize,
        data.data(), data.size(),
        ZSTD_CLEVEL_DEFAULT
    );

    if (ZSTD_isError(compressedSize)) {
        return {};
    }

    compressed.resize(compressedSize);
    return compressed;
}

bool NetworkOptimizer::DecompressPacket(NetworkPacket& packet) {
    if (packet.compression == CompressionType::None || packet.data.empty()) {
        return true;
    }

    std::vector<uint8_t> decompressedData;

    switch (packet.compression) {
        case CompressionType::LZ4:
            decompressedData = DecompressLZ4(packet.data);
            break;
        case CompressionType::ZSTD:
            decompressedData = DecompressZSTD(packet.data);
            break;
        default:
            return false;
    }

    if (decompressedData.empty()) {
        spdlog::error("[NetworkOptimizer] Failed to decompress packet {}", packet.packetId);
        return false;
    }

    packet.data = std::move(decompressedData);
    packet.compression = CompressionType::None;

    // Update decompression metrics
    {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        m_metrics.bytesDecompressed += packet.data.size();
    }

    return true;
}

std::vector<uint8_t> NetworkOptimizer::DecompressLZ4(const std::vector<uint8_t>& data) {
    // For LZ4, we need to know the original size, which should be stored separately
    // This is a simplified implementation
    std::vector<uint8_t> decompressed(65536); // Assume max size

    int decompressedSize = LZ4_decompress_safe(
        reinterpret_cast<const char*>(data.data()),
        reinterpret_cast<char*>(decompressed.data()),
        static_cast<int>(data.size()),
        static_cast<int>(decompressed.size())
    );

    if (decompressedSize <= 0) {
        return {};
    }

    decompressed.resize(decompressedSize);
    return decompressed;
}

std::vector<uint8_t> NetworkOptimizer::DecompressZSTD(const std::vector<uint8_t>& data) {
    unsigned long long decompressedSize = ZSTD_getFrameContentSize(data.data(), data.size());

    if (decompressedSize == ZSTD_CONTENTSIZE_ERROR || decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        decompressedSize = 65536; // Fallback size
    }

    std::vector<uint8_t> decompressed(decompressedSize);

    size_t actualSize = ZSTD_decompress(
        decompressed.data(), decompressed.size(),
        data.data(), data.size()
    );

    if (ZSTD_isError(actualSize)) {
        return {};
    }

    decompressed.resize(actualSize);
    return decompressed;
}

PacketPriority NetworkOptimizer::DeterminePacketPriority(const NetworkPacket& packet) {
    // Determine priority based on packet type
    if (packet.packetType == "player_movement" || packet.packetType == "combat_action") {
        return PacketPriority::Critical;
    } else if (packet.packetType == "voice_data" || packet.packetType == "game_event") {
        return PacketPriority::High;
    } else if (packet.packetType == "inventory_update" || packet.packetType == "chat_message") {
        return PacketPriority::Medium;
    } else if (packet.packetType == "statistics" || packet.packetType == "background_sync") {
        return PacketPriority::Low;
    } else {
        return PacketPriority::Background;
    }
}

bool NetworkOptimizer::SchedulePacket(const NetworkPacket& packet) {
    std::lock_guard<std::mutex> lock(m_scheduler.queueMutex);

    // Check bandwidth availability
    if (!AllocateBandwidth(packet.priority, packet.data.size())) {
        spdlog::debug("[NetworkOptimizer] Insufficient bandwidth for packet {} (priority: {})",
                     packet.packetId, static_cast<int>(packet.priority));
        return false;
    }

    // Add to appropriate priority queue
    m_scheduler.priorityQueues[packet.priority].push(packet);
    m_scheduler.currentQueueSize++;
    m_scheduler.totalScheduled++;

    return true;
}

std::vector<NetworkPacket> NetworkOptimizer::GetScheduledPackets(uint32_t maxCount) {
    std::lock_guard<std::mutex> lock(m_scheduler.queueMutex);

    std::vector<NetworkPacket> packets;
    uint32_t count = 0;
    uint32_t maxPackets = (maxCount == 0) ? m_currentProfile.maxPacketsPerFrame : maxCount;

    // Process packets by priority
    for (int priority = static_cast<int>(PacketPriority::Critical);
         priority <= static_cast<int>(PacketPriority::Background) && count < maxPackets;
         ++priority) {

        auto priorityEnum = static_cast<PacketPriority>(priority);
        auto& queue = m_scheduler.priorityQueues[priorityEnum];

        while (!queue.empty() && count < maxPackets) {
            packets.push_back(queue.front());
            queue.pop();
            count++;
            m_scheduler.currentQueueSize--;
        }
    }

    return packets;
}

bool NetworkOptimizer::AllocateBandwidth(PacketPriority priority, uint64_t bytes) {
    std::lock_guard<std::mutex> lock(m_bandwidthMutex);

    // Check if we have enough tokens in the bucket
    RefillTokenBucket(m_bandwidthManager.upstreamBucket);

    if (m_bandwidthManager.upstreamBucket.tokens >= bytes) {
        m_bandwidthManager.upstreamBucket.tokens -= bytes;
        m_bandwidthManager.usedBandwidthUp += bytes;
        return true;
    }

    return false;
}

void NetworkOptimizer::RefillTokenBucket(BandwidthManager::TokenBucket& bucket) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - bucket.lastRefill);

    if (elapsed.count() > 0) {
        uint64_t tokensToAdd = (bucket.refillRate * elapsed.count()) / 1000;
        bucket.tokens = std::min(bucket.capacity, bucket.tokens + tokensToAdd);
        bucket.lastRefill = now;
    }
}

void NetworkOptimizer::UpdateNetworkConditions(float latency, float packetLoss, uint64_t bandwidth) {
    {
        std::lock_guard<std::mutex> lock(m_metricsMutex);

        // Update current metrics
        m_metrics.currentLatency = latency;
        m_metrics.packetLossRate = packetLoss;
        m_metrics.currentBandwidthUp = bandwidth;

        // Update averages
        m_metrics.averageLatency = (m_metrics.averageLatency * 0.9f) + (latency * 0.1f);

        // Update min/max
        m_metrics.minLatency = std::min(m_metrics.minLatency, latency);
        m_metrics.maxLatency = std::max(m_metrics.maxLatency, latency);

        // Calculate jitter
        m_latencyHistory.push(latency);
        while (m_latencyHistory.size() > 100) {
            m_latencyHistory.pop();
        }

        if (m_latencyHistory.size() >= 2) {
            std::vector<float> latencies;
            auto tempQueue = m_latencyHistory;
            while (!tempQueue.empty()) {
                latencies.push_back(tempQueue.front());
                tempQueue.pop();
            }
            m_metrics.jitter = NetworkUtils::CalculateJitter(latencies);
        }

        // Update connection quality
        m_metrics.connectionQuality = NetworkUtils::CalculateConnectionQuality(m_metrics);
    }

    // Trigger adaptation if conditions have changed significantly
    if (m_adaptationEnabled) {
        static float lastLatency = 0.0f;
        static float lastPacketLoss = 0.0f;

        if (std::abs(latency - lastLatency) > 10.0f ||
            std::abs(packetLoss - lastPacketLoss) > 0.01f) {
            TriggerAdaptation();
            lastLatency = latency;
            lastPacketLoss = packetLoss;
        }
    }
}

void NetworkOptimizer::TriggerAdaptation() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastAdaptation);

    // Don't adapt too frequently
    if (elapsed.count() < 1000) {
        return;
    }

    spdlog::debug("[NetworkOptimizer] Triggering network adaptation");

    AdaptToLatency(m_metrics.currentLatency);
    AdaptToPacketLoss(m_metrics.packetLossRate);
    AdaptToBandwidth(m_metrics.currentBandwidthUp);

    m_lastAdaptation = now;
    TriggerEvent("adaptation_triggered", "");
}

void NetworkOptimizer::AdaptToLatency(float latency) {
    if (latency > m_currentProfile.latencyThreshold) {
        // High latency - prioritize compression and reduce packet size
        spdlog::info("[NetworkOptimizer] Adapting to high latency: {:.1f}ms", latency);

        // Increase compression for non-critical packets
        SetCompressionForPriority(PacketPriority::Medium, CompressionType::ZSTD);
        SetCompressionForPriority(PacketPriority::Low, CompressionType::ZSTD);
        SetCompressionForPriority(PacketPriority::Background, CompressionType::ZSTD);

        // Reduce batch size to send packets faster
        m_scheduler.maxBatchSize = std::max(1u, m_scheduler.maxBatchSize / 2);

        TriggerEvent("adapted_to_latency", std::to_string(latency));
    }
}

void NetworkOptimizer::AdaptToPacketLoss(float packetLoss) {
    if (packetLoss > m_currentProfile.packetLossThreshold) {
        // High packet loss - enable more aggressive retransmission
        spdlog::info("[NetworkOptimizer] Adapting to high packet loss: {:.2f}%", packetLoss * 100);

        // Increase retries for important packets
        // This would be implemented in the reliable transmission system

        // Reduce bandwidth utilization to avoid congestion
        m_currentProfile.bandwidthUtilization = std::max(0.5f, m_currentProfile.bandwidthUtilization * 0.8f);

        TriggerEvent("adapted_to_packet_loss", std::to_string(packetLoss));
    }
}

void NetworkOptimizer::AdaptToBandwidth(uint64_t bandwidth) {
    // Adapt compression and scheduling based on available bandwidth
    if (bandwidth < 1024 * 1024) { // Less than 1 MB/s
        spdlog::info("[NetworkOptimizer] Adapting to low bandwidth: {} KB/s", bandwidth / 1024);

        // Enable aggressive compression
        SetCompressionForPriority(PacketPriority::High, CompressionType::ZSTD);
        SetCompressionForPriority(PacketPriority::Medium, CompressionType::ZSTD);
        SetCompressionForPriority(PacketPriority::Low, CompressionType::ZSTD);

        // Reduce maximum packets per frame
        m_currentProfile.maxPacketsPerFrame = std::max(1u, m_currentProfile.maxPacketsPerFrame / 2);

        TriggerEvent("adapted_to_bandwidth", std::to_string(bandwidth));
    }
}

NetworkMetrics NetworkOptimizer::GetMetrics() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    return m_metrics;
}

void NetworkOptimizer::UpdateMetrics() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_metrics.lastUpdate = std::chrono::steady_clock::now();

    // Calculate packet loss rate
    if (m_metrics.packetsSent > 0) {
        m_metrics.packetLossRate = static_cast<float>(m_metrics.packetsLost) / m_metrics.packetsSent;
    }
}

void NetworkOptimizer::TriggerEvent(const std::string& eventType, const std::string& data) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    auto it = m_eventCallbacks.find(eventType);
    if (it != m_eventCallbacks.end()) {
        for (const auto& callback : it->second) {
            try {
                callback(eventType, data);
            } catch (const std::exception& e) {
                spdlog::error("[NetworkOptimizer] Exception in event callback for '{}': {}", eventType, e.what());
            }
        }
    }
}

uint32_t NetworkOptimizer::GeneratePacketId() {
    return m_nextPacketId.fetch_add(1);
}

// Optimization presets implementation
namespace OptimizationPresets {
    OptimizationProfile GetBalancedProfile() {
        OptimizationProfile profile;
        profile.profileName = "Balanced";
        profile.strategy = OptimizationStrategy::Balanced;
        profile.adaptationMode = AdaptationMode::Full;

        // Compression settings
        profile.defaultCompression = CompressionType::LZ4;
        profile.compressionByPriority[PacketPriority::Critical] = CompressionType::None;
        profile.compressionByPriority[PacketPriority::High] = CompressionType::LZ4;
        profile.compressionByPriority[PacketPriority::Medium] = CompressionType::LZ4;
        profile.compressionByPriority[PacketPriority::Low] = CompressionType::ZSTD;
        profile.compressionByPriority[PacketPriority::Background] = CompressionType::ZSTD;
        profile.compressionThreshold = 128; // bytes

        // Bandwidth settings
        profile.maxBandwidthUp = 1024 * 1024; // 1 MB/s
        profile.maxBandwidthDown = 2 * 1024 * 1024; // 2 MB/s
        profile.bandwidthUtilization = 0.8f;

        // Packet scheduling
        profile.maxPacketsPerFrame = 10;
        profile.maxRetries = 3;
        profile.retryTimeout = std::chrono::milliseconds(100);
        profile.maxPacketAge = std::chrono::milliseconds(1000);

        // Quality thresholds
        profile.latencyThreshold = 100.0f; // ms
        profile.packetLossThreshold = 0.02f; // 2%
        profile.jitterThreshold = 20.0f; // ms
        profile.enableCongestionControl = true;
        profile.enableAdaptiveCompression = true;
        profile.enablePacketAggregation = true;

        // Performance settings
        profile.processingThreads = 2;
        profile.bufferSize = 65536;
        profile.enableZeroCopy = false;
        profile.enableBatching = true;

        return profile;
    }

    OptimizationProfile GetLowLatencyProfile() {
        OptimizationProfile profile = GetBalancedProfile();
        profile.profileName = "Low Latency";
        profile.strategy = OptimizationStrategy::Aggressive;

        // Minimize compression for speed
        profile.compressionByPriority[PacketPriority::Critical] = CompressionType::None;
        profile.compressionByPriority[PacketPriority::High] = CompressionType::None;
        profile.compressionByPriority[PacketPriority::Medium] = CompressionType::LZ4;

        // Faster processing
        profile.maxPacketsPerFrame = 20;
        profile.retryTimeout = std::chrono::milliseconds(50);
        profile.processingThreads = 4;

        // Stricter latency requirements
        profile.latencyThreshold = 50.0f;
        profile.jitterThreshold = 10.0f;

        return profile;
    }
}

// Utility functions implementation
namespace NetworkUtils {
    float CalculateJitter(const std::vector<float>& latencies) {
        if (latencies.size() < 2) {
            return 0.0f;
        }

        float sumSquaredDiff = 0.0f;
        float mean = 0.0f;

        // Calculate mean
        for (float latency : latencies) {
            mean += latency;
        }
        mean /= latencies.size();

        // Calculate variance
        for (float latency : latencies) {
            float diff = latency - mean;
            sumSquaredDiff += diff * diff;
        }

        return std::sqrt(sumSquaredDiff / latencies.size());
    }

    float CalculateConnectionQuality(const NetworkMetrics& metrics) {
        // Calculate quality score based on latency, packet loss, and jitter
        float latencyScore = std::max(0.0f, 1.0f - (metrics.currentLatency / 200.0f));
        float lossScore = std::max(0.0f, 1.0f - (metrics.packetLossRate * 50.0f));
        float jitterScore = std::max(0.0f, 1.0f - (metrics.jitter / 50.0f));

        return (latencyScore * 0.4f) + (lossScore * 0.4f) + (jitterScore * 0.2f);
    }

    std::string GetStrategyName(OptimizationStrategy strategy) {
        switch (strategy) {
            case OptimizationStrategy::Aggressive: return "Aggressive";
            case OptimizationStrategy::Balanced: return "Balanced";
            case OptimizationStrategy::Conservative: return "Conservative";
            case OptimizationStrategy::Adaptive: return "Adaptive";
            default: return "Unknown";
        }
    }

    std::string GetPriorityName(PacketPriority priority) {
        switch (priority) {
            case PacketPriority::Critical: return "Critical";
            case PacketPriority::High: return "High";
            case PacketPriority::Medium: return "Medium";
            case PacketPriority::Low: return "Low";
            case PacketPriority::Background: return "Background";
            default: return "Unknown";
        }
    }
}

    // Missing NetworkOptimizer private method implementations
    void NetworkOptimizer::ProcessPacketQueue() {
        std::lock_guard<std::mutex> lock(m_scheduler.queueMutex);

        auto now = std::chrono::steady_clock::now();

        // Process each priority queue
        for (int priority = static_cast<int>(PacketPriority::Critical);
             priority <= static_cast<int>(PacketPriority::Background); ++priority) {

            auto priorityEnum = static_cast<PacketPriority>(priority);
            auto& queue = m_scheduler.priorityQueues[priorityEnum];

            // Remove expired packets
            std::queue<NetworkPacket> tempQueue;
            while (!queue.empty()) {
                auto packet = queue.front();
                queue.pop();

                if (now < packet.deadline) {
                    tempQueue.push(packet);
                } else {
                    // Packet expired - using existing metrics field
                    m_metrics.packetsLost++;
                    spdlog::debug("[NetworkOptimizer] Packet {} expired", packet.packetId);
                }
            }
            queue = std::move(tempQueue);
        }

        // Update queue statistics - removed lastProcessTime as it's not in header
    }

    void NetworkOptimizer::ProcessBandwidthManagement() {
        std::lock_guard<std::mutex> lock(m_bandwidthMutex);

        auto now = std::chrono::steady_clock::now();

        // Refill token buckets
        RefillTokenBucket(m_bandwidthManager.upstreamBucket);
        RefillTokenBucket(m_bandwidthManager.downstreamBucket);

        // Calculate current usage rates using lastReset which exists in header
        auto timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_bandwidthManager.lastReset).count();

        if (timeSinceLastUpdate > 0) {
            // Calculate rates (bytes per second)
            uint64_t currentUpstreamRate = (m_bandwidthManager.usedBandwidthUp * 1000) / timeSinceLastUpdate;
            uint64_t currentDownstreamRate = (m_bandwidthManager.usedBandwidthDown * 1000) / timeSinceLastUpdate;

            // Reset counters
            m_bandwidthManager.usedBandwidthUp = 0;
            m_bandwidthManager.usedBandwidthDown = 0;
            m_bandwidthManager.lastReset = now;

            // Check for bandwidth threshold violations
            if (currentUpstreamRate > m_currentProfile.maxBandwidthUp * 0.9f) {
                spdlog::warn("[NetworkOptimizer] Upstream bandwidth usage high: {} KB/s",
                            currentUpstreamRate / 1024);
            }
        }
    }

    void NetworkOptimizer::ProcessAdaptation() {
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastAdaptation = std::chrono::duration_cast<std::chrono::seconds>(
            now - m_lastAdaptation).count();

        // Only adapt every few seconds to avoid oscillation
        if (timeSinceLastAdaptation < 5) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_metricsMutex);

        // Analyze current network conditions
        bool shouldAdapt = false;

        // Check if latency has increased significantly
        if (m_metrics.currentLatency > m_metrics.averageLatency * 1.5f) {
            spdlog::info("[NetworkOptimizer] Latency spike detected: {:.1f}ms vs {:.1f}ms avg",
                        m_metrics.currentLatency, m_metrics.averageLatency);
            shouldAdapt = true;
        }

        // Check packet loss rate
        if (m_metrics.packetLossRate > m_currentProfile.packetLossThreshold) {
            spdlog::info("[NetworkOptimizer] High packet loss detected: {:.2f}%",
                        m_metrics.packetLossRate * 100);
            shouldAdapt = true;
        }

        // Check jitter
        if (m_metrics.jitter > m_currentProfile.jitterThreshold) {
            spdlog::info("[NetworkOptimizer] High jitter detected: {:.1f}ms", m_metrics.jitter);
            shouldAdapt = true;
        }

        if (shouldAdapt) {
            // Trigger adaptation based on current profile settings
            if (m_currentProfile.enableAdaptiveCompression) {
                AdaptCompressionSettings();
            }

            if (m_currentProfile.enablePacketAggregation) {
                AdaptSchedulingSettings();
            }

            m_lastAdaptation = now;
            TriggerEvent("network_adapted", "conditions_changed");
        }
    }

    void NetworkOptimizer::ProcessCongestionControl() {
        if (!m_currentProfile.enableCongestionControl) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_metricsMutex);

        // Simple congestion control based on packet loss and latency
        float congestionScore = 0.0f;

        // Factor in packet loss (0-1 scale)
        congestionScore += m_metrics.packetLossRate * 2.0f;

        // Factor in latency increase (normalized)
        if (m_metrics.averageLatency > 0) {
            float latencyIncrease = (m_metrics.currentLatency - m_metrics.averageLatency) / m_metrics.averageLatency;
            congestionScore += std::max(0.0f, latencyIncrease);
        }

        // Factor in jitter
        congestionScore += (m_metrics.jitter / 100.0f);

        // Clamp score
        congestionScore = std::clamp(congestionScore, 0.0f, 1.0f);

        if (congestionScore > 0.5f) {
            // High congestion detected - reduce transmission rate
            float reductionFactor = 0.8f - (congestionScore * 0.3f);

            // Reduce bandwidth allocation
            m_bandwidthManager.allocatedBandwidthUp = static_cast<uint64_t>(
                m_currentProfile.maxBandwidthUp * reductionFactor);

            // Reduce packet scheduling rate
            m_scheduler.maxBatchSize = std::max(1u,
                static_cast<uint32_t>(m_currentProfile.maxPacketsPerFrame * reductionFactor));

            spdlog::debug("[NetworkOptimizer] Congestion control active: score={:.2f}, reduction={:.2f}",
                         congestionScore, reductionFactor);

            TriggerEvent("congestion_detected", std::to_string(congestionScore));
        } else {
            // Low congestion - gradually restore normal rates
            float restoreFactor = 0.05f; // 5% increase per update

            m_bandwidthManager.allocatedBandwidthUp = std::min(
                m_currentProfile.maxBandwidthUp,
                static_cast<uint64_t>(m_bandwidthManager.allocatedBandwidthUp * (1.0f + restoreFactor)));

            m_scheduler.maxBatchSize = std::min(
                m_currentProfile.maxPacketsPerFrame,
                m_scheduler.maxBatchSize + 1u);
        }
    }

    void NetworkOptimizer::AdaptCompressionSettings() {
        // Increase compression for high latency/low bandwidth conditions
        if (m_metrics.currentLatency > m_currentProfile.latencyThreshold) {
            // Enable more aggressive compression for medium and low priority packets
            SetCompressionForPriority(PacketPriority::Medium, CompressionType::ZSTD);
            SetCompressionForPriority(PacketPriority::Low, CompressionType::ZSTD);
            SetCompressionForPriority(PacketPriority::Background, CompressionType::ZSTD);

            // Lower compression threshold
            m_currentProfile.compressionThreshold = std::max(64u, m_currentProfile.compressionThreshold / 2);

            spdlog::debug("[NetworkOptimizer] Adapted compression for high latency");
        } else if (m_metrics.currentLatency < m_currentProfile.latencyThreshold * 0.5f) {
            // Good conditions - reduce compression to save CPU
            SetCompressionForPriority(PacketPriority::Medium, CompressionType::LZ4);
            SetCompressionForPriority(PacketPriority::Low, CompressionType::LZ4);

            // Raise compression threshold
            m_currentProfile.compressionThreshold = std::min(512u, m_currentProfile.compressionThreshold * 2);

            spdlog::debug("[NetworkOptimizer] Reduced compression for good conditions");
        }
    }

    void NetworkOptimizer::AdaptSchedulingSettings() {
        // Adjust batching based on current conditions
        if (m_metrics.currentLatency > m_currentProfile.latencyThreshold) {
            // High latency - reduce batch size for faster transmission
            m_scheduler.maxBatchSize = std::max(1u, m_scheduler.maxBatchSize / 2);
            m_scheduler.batchTimeout = std::chrono::milliseconds(2); // Shorter timeout
        } else {
            // Good latency - increase batch size for efficiency
            m_scheduler.maxBatchSize = std::min(20u, m_scheduler.maxBatchSize + 1);
            m_scheduler.batchTimeout = std::chrono::milliseconds(10); // Longer timeout
        }
    }

    void NetworkOptimizer::ResetMetrics() {
        std::lock_guard<std::mutex> lock(m_metricsMutex);

        m_metrics = NetworkMetrics{};
        m_metrics.lastUpdate = std::chrono::steady_clock::now();
        m_metrics.minLatency = std::numeric_limits<float>::max();
        m_metrics.maxLatency = 0.0f;
        m_metrics.connectionQuality = 1.0f;

        // Clear latency history
        while (!m_latencyHistory.empty()) {
            m_latencyHistory.pop();
        }
    }

    bool NetworkOptimizer::IsCompressionBeneficial(const NetworkPacket& packet, CompressionType compression) {
        // Simple heuristic: compression is beneficial for packets larger than threshold
        // and when not under severe latency constraints
        if (packet.data.size() < m_currentProfile.compressionThreshold) {
            return false;
        }

        // Don't compress if latency is extremely high (compression adds delay)
        if (m_metrics.currentLatency > m_currentProfile.latencyThreshold * 2.0f) {
            return false;
        }

        // Don't compress critical packets in poor conditions
        if (packet.priority == PacketPriority::Critical &&
            m_metrics.packetLossRate > m_currentProfile.packetLossThreshold) {
            return false;
        }

        return true;
    }

    CompressionType NetworkOptimizer::GetCompressionForPriority(PacketPriority priority) const {
        auto it = m_currentProfile.compressionByPriority.find(priority);
        if (it != m_currentProfile.compressionByPriority.end()) {
            return it->second;
        }
        return m_currentProfile.defaultCompression;
    }

    bool NetworkOptimizer::SetCompressionForPriority(PacketPriority priority, CompressionType compression) {
        m_currentProfile.compressionByPriority[priority] = compression;
        return true;
    }

    void NetworkOptimizer::Update(float deltaTime) {
        // Basic update that calls ProcessingLoop functionality
        if (!m_initialized) {
            return;
        }

        auto currentTime = std::chrono::steady_clock::now();

        // Process packet queue
        ProcessPacketQueue();

        // Process bandwidth management
        ProcessBandwidthManagement();

        // Process adaptation if enabled
        if (m_adaptationEnabled) {
            ProcessAdaptation();
        }

        // Process congestion control
        ProcessCongestionControl();

        // Update metrics
        UpdateMetrics();
    }

    float NetworkOptimizer::GetBandwidthUtilization(bool upstream) const {
        std::lock_guard<std::mutex> lock(m_bandwidthMutex);

        if (upstream) {
            if (m_bandwidthManager.allocatedBandwidthUp == 0) {
                return 0.0f;
            }
            return static_cast<float>(m_bandwidthManager.usedBandwidthUp) /
                   static_cast<float>(m_bandwidthManager.allocatedBandwidthUp);
        } else {
            if (m_bandwidthManager.allocatedBandwidthDown == 0) {
                return 0.0f;
            }
            return static_cast<float>(m_bandwidthManager.usedBandwidthDown) /
                   static_cast<float>(m_bandwidthManager.allocatedBandwidthDown);
        }
    }

    bool NetworkOptimizer::IsAdaptationEnabled() const {
        return m_adaptationEnabled;
    }

    float NetworkOptimizer::GetCompressionRatio() const {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        return m_metrics.compressionRatio;
    }

    bool NetworkOptimizer::IsOptimizationActive() const {
        return m_initialized && m_adaptationEnabled;
    }

    void NetworkOptimizer::SetPacketPriority(const std::string& packetType, PacketPriority priority) {
        std::lock_guard<std::mutex> lock(m_profileMutex);
        m_packetTypePriorities[packetType] = priority;
    }

    // Missing NetworkUtils function implementation
    std::chrono::milliseconds NetworkUtils::CalculateOptimalTimeout(PacketPriority priority, float currentLatency) {
        // Base timeout multiplier based on priority
        float multiplier = 1.0f;
        switch (priority) {
            case PacketPriority::Critical:
                multiplier = 0.5f;  // Fastest timeout
                break;
            case PacketPriority::High:
                multiplier = 1.0f;
                break;
            case PacketPriority::Medium:
                multiplier = 2.0f;
                break;
            case PacketPriority::Low:
                multiplier = 4.0f;
                break;
            case PacketPriority::Background:
                multiplier = 8.0f;  // Longest timeout
                break;
        }

        // Calculate timeout based on current latency with some safety margin
        float timeoutMs = std::max(10.0f, currentLatency * multiplier * 2.0f);

        // Clamp to reasonable bounds
        timeoutMs = std::clamp(timeoutMs, 10.0f, 5000.0f);

        return std::chrono::milliseconds(static_cast<int>(timeoutMs));
    }

} // namespace CoopNet
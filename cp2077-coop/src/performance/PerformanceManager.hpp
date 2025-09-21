#pragma once

#include <RED4ext/RED4ext.hpp>
#include <chrono>
#include <vector>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <thread>
#include <deque>

namespace CoopNet {

// Performance monitoring and optimization manager
class PerformanceManager {
public:
    static PerformanceManager& Instance();

    // System lifecycle
    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);

    // Performance metrics
    float GetCurrentFPS() const { return m_currentFPS; }
    float GetFrameTime() const { return m_frameTime; }
    float GetAverageFrameTime() const { return m_averageFrameTime; }
    uint64_t GetFrameCount() const { return m_frameCount; }

    // Memory management
    size_t GetUsedMemoryMB() const;
    size_t GetAvailableMemoryMB() const;
    void RequestGarbageCollection();
    void SetMemoryThreshold(size_t thresholdMB);

    // CPU performance
    float GetCPUUsage() const { return m_cpuUsage; }
    uint32_t GetActiveThreadCount() const;
    void SetCPUThrottling(bool enabled);

    // GPU performance
    float GetGPUUsage() const;
    float GetGPUMemoryUsage() const;
    void SetDynamicResolution(bool enabled);
    void SetTargetFrameRate(float fps);

    // Network performance
    uint32_t GetNetworkLatency() const { return m_networkLatency; }
    float GetNetworkBandwidthUsage() const { return m_networkBandwidth; }
    uint32_t GetPacketLoss() const { return m_packetLoss; }

    // Optimization controls
    void EnableAdaptiveQuality(bool enabled);
    void SetQualityLevel(uint32_t level); // 0=low, 1=medium, 2=high, 3=ultra
    uint32_t GetCurrentQualityLevel() const { return m_currentQualityLevel; }

    // Profiling
    void StartProfiling(const std::string& name);
    void EndProfiling(const std::string& name);
    std::vector<std::pair<std::string, float>> GetProfilingResults() const;

    // Resource management
    void RegisterResourcePool(const std::string& name, size_t maxSize);
    void* GetPooledResource(const std::string& poolName);
    void ReturnPooledResource(const std::string& poolName, void* resource);

private:
    PerformanceManager();
    ~PerformanceManager();
    PerformanceManager(const PerformanceManager&) = delete;
    PerformanceManager& operator=(const PerformanceManager&) = delete;

    // Internal methods
    void UpdateFrameMetrics(float deltaTime);
    void UpdateSystemMetrics();
    void CheckAndApplyOptimizations();
    void OptimizeMemoryUsage();
    void OptimizeCPUUsage();
    void OptimizeGPUUsage();
    void OptimizeNetworkUsage();

    // Performance monitoring thread
    void MonitoringThread();
    std::atomic<bool> m_monitoringActive{true};
    std::thread m_monitoringThread;

    // Frame metrics
    std::atomic<float> m_currentFPS{60.0f};
    std::atomic<float> m_frameTime{16.67f};
    std::atomic<float> m_averageFrameTime{16.67f};
    std::atomic<uint64_t> m_frameCount{0};
    std::deque<float> m_frameTimeHistory;
    mutable std::mutex m_frameMetricsMutex;

    // System metrics
    std::atomic<float> m_cpuUsage{0.0f};
    std::atomic<size_t> m_memoryUsageMB{0};
    std::atomic<uint32_t> m_networkLatency{0};
    std::atomic<float> m_networkBandwidth{0.0f};
    std::atomic<uint32_t> m_packetLoss{0};

    // Quality settings
    std::atomic<uint32_t> m_currentQualityLevel{2}; // Default to high
    std::atomic<bool> m_adaptiveQualityEnabled{true};
    std::atomic<bool> m_dynamicResolutionEnabled{false};
    std::atomic<float> m_targetFrameRate{60.0f};
    std::atomic<bool> m_cpuThrottlingEnabled{false};

    // Profiling data
    struct ProfilingEntry {
        std::string name;
        std::chrono::high_resolution_clock::time_point startTime;
        float duration;
        bool active;
    };
    std::unordered_map<std::string, ProfilingEntry> m_profilingData;
    mutable std::mutex m_profilingMutex;

    // Resource pools
    struct ResourcePool {
        std::string name;
        std::vector<void*> available;
        std::vector<void*> used;
        size_t maxSize;
        std::mutex poolMutex;
    };
    std::unordered_map<std::string, std::unique_ptr<ResourcePool>> m_resourcePools;
    mutable std::mutex m_poolsMutex;

    // Configuration
    size_t m_memoryThresholdMB = 2048; // 2GB default
    float m_qualityAdjustmentCooldown = 0.0f;
    uint64_t m_lastGCRequest = 0;
    uint64_t m_gcInterval = 30000; // 30 seconds
};

// CPU utilization monitor
class CPUMonitor {
public:
    static CPUMonitor& Instance();

    void Update();
    float GetCPUUsage() const;
    float GetPerCoreUsage(uint32_t core) const;
    uint32_t GetCoreCount() const;
    void SetAffinityMask(uint64_t mask);

private:
    CPUMonitor() = default;
    ~CPUMonitor() = default;
    CPUMonitor(const CPUMonitor&) = delete;
    CPUMonitor& operator=(const CPUMonitor&) = delete;

    struct CPUInfo {
        float usage;
        uint64_t idleTime;
        uint64_t totalTime;
    };

    std::vector<CPUInfo> m_coreInfo;
    float m_overallUsage = 0.0f;
    mutable std::mutex m_cpuMutex;
    std::chrono::steady_clock::time_point m_lastUpdate;
};

// Memory usage monitor and optimizer
class MemoryMonitor {
public:
    static MemoryMonitor& Instance();

    void Update();
    size_t GetTotalMemoryMB() const;
    size_t GetUsedMemoryMB() const;
    size_t GetAvailableMemoryMB() const;
    float GetMemoryUsagePercentage() const;

    // Memory optimization
    void TrimWorkingSet();
    void CompactHeaps();
    void FlushUnusedPages();

    // Memory leak detection
    void StartMemoryTracking();
    void StopMemoryTracking();
    std::vector<std::string> GetMemoryLeaks() const;

private:
    MemoryMonitor() = default;
    ~MemoryMonitor() = default;
    MemoryMonitor(const MemoryMonitor&) = delete;
    MemoryMonitor& operator=(const MemoryMonitor&) = delete;

    std::atomic<size_t> m_totalMemoryMB{0};
    std::atomic<size_t> m_usedMemoryMB{0};
    std::atomic<float> m_usagePercentage{0.0f};

    bool m_trackingEnabled = false;
    std::unordered_map<void*, size_t> m_allocations;
    mutable std::mutex m_allocationsMutex;
};

// GPU performance monitor
class GPUMonitor {
public:
    static GPUMonitor& Instance();

    void Update();
    float GetGPUUsage() const;
    float GetGPUMemoryUsage() const;
    float GetGPUTemperature() const;
    uint32_t GetGPUClockSpeed() const;

    // Dynamic resolution scaling
    void SetDynamicResolution(bool enabled);
    float GetCurrentRenderScale() const;
    void AdjustRenderScale(float delta);

private:
    GPUMonitor() = default;
    ~GPUMonitor() = default;
    GPUMonitor(const GPUMonitor&) = delete;
    GPUMonitor& operator=(const GPUMonitor&) = delete;

    std::atomic<float> m_gpuUsage{0.0f};
    std::atomic<float> m_gpuMemoryUsage{0.0f};
    std::atomic<float> m_gpuTemperature{0.0f};
    std::atomic<uint32_t> m_gpuClockSpeed{0};

    bool m_dynamicResolutionEnabled = false;
    float m_currentRenderScale = 1.0f;
    float m_targetFrameTime = 16.67f; // 60 FPS
    mutable std::mutex m_gpuMutex;
};

// Network performance monitor
class NetworkMonitor {
public:
    static NetworkMonitor& Instance();

    void Update();
    uint32_t GetLatency() const;
    float GetBandwidthUsage() const;
    uint32_t GetPacketLoss() const;
    uint32_t GetPacketsPerSecond() const;

    // Network optimization
    void SetCompressionEnabled(bool enabled);
    void SetPacketBatching(bool enabled);
    void SetAdaptiveBitrate(bool enabled);

    // Statistics
    void RecordPacketSent(size_t size);
    void RecordPacketReceived(size_t size);
    void RecordPacketLoss();
    void RecordLatencyMeasurement(uint32_t latencyMs);

private:
    NetworkMonitor() = default;
    ~NetworkMonitor() = default;
    NetworkMonitor(const NetworkMonitor&) = delete;
    NetworkMonitor& operator=(const NetworkMonitor&) = delete;

    std::atomic<uint32_t> m_latency{0};
    std::atomic<float> m_bandwidthUsage{0.0f};
    std::atomic<uint32_t> m_packetLoss{0};
    std::atomic<uint32_t> m_packetsPerSecond{0};

    // Statistics tracking
    std::atomic<uint64_t> m_bytesSent{0};
    std::atomic<uint64_t> m_bytesReceived{0};
    std::atomic<uint32_t> m_packetsLost{0};
    std::atomic<uint32_t> m_totalPackets{0};

    std::deque<uint32_t> m_latencyHistory;
    mutable std::mutex m_networkMutex;

    bool m_compressionEnabled = true;
    bool m_packetBatchingEnabled = true;
    bool m_adaptiveBitrateEnabled = true;
};

// Automatic performance optimization system
class AutoOptimizer {
public:
    static AutoOptimizer& Instance();

    void SetEnabled(bool enabled);
    void Update();

    // Optimization strategies
    void OptimizeForFramerate();
    void OptimizeForQuality();
    void OptimizeForNetwork();
    void OptimizeForMemory();

    // Adaptive optimization
    void SetOptimizationTarget(float targetFPS);
    void SetOptimizationMode(uint32_t mode); // 0=performance, 1=balanced, 2=quality

private:
    AutoOptimizer() = default;
    ~AutoOptimizer() = default;
    AutoOptimizer(const AutoOptimizer&) = delete;
    AutoOptimizer& operator=(const AutoOptimizer&) = delete;

    bool m_enabled = true;
    float m_targetFPS = 60.0f;
    uint32_t m_optimizationMode = 1; // Balanced by default

    float m_lastOptimizationTime = 0.0f;
    float m_optimizationCooldown = 5.0f; // 5 second cooldown

    void ApplyFramerateOptimizations();
    void ApplyQualityOptimizations();
    void ApplyNetworkOptimizations();
    void ApplyMemoryOptimizations();

    bool ShouldOptimize() const;
};

} // namespace CoopNet
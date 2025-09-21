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

namespace CoopNet {

// Performance metric types
enum class MetricType : uint8_t {
    FPS = 0,
    FrameTime = 1,
    CPUUsage = 2,
    MemoryUsage = 3,
    NetworkLatency = 4,
    NetworkBandwidth = 5,
    DiskIO = 6,
    GPUUsage = 7,
    AudioLatency = 8,
    VoiceLatency = 9,
    Custom = 255
};

// Performance severity levels
enum class PerformanceSeverity : uint8_t {
    Optimal = 0,
    Good = 1,
    Warning = 2,
    Critical = 3,
    Severe = 4
};

// Performance alert types
enum class AlertType : uint8_t {
    Threshold = 0,      // Value crosses threshold
    Trend = 1,          // Performance trend detected
    Spike = 2,          // Sudden performance spike
    Degradation = 3,    // Gradual performance loss
    Recovery = 4        // Performance recovery
};

// Forward declarations
struct PerformanceMetric;
struct PerformanceAlert;
struct PerformanceProfile;
struct HardwareInfo;

// Individual performance metric
struct PerformanceMetric {
    MetricType type;
    std::string name;
    float currentValue;
    float averageValue;
    float minValue;
    float maxValue;
    std::string unit;
    std::chrono::steady_clock::time_point timestamp;
    std::queue<float> history;
    uint32_t sampleCount;
    bool isActive;
};

// Performance alert
struct PerformanceAlert {
    uint64_t alertId;
    AlertType type;
    MetricType metricType;
    std::string metricName;
    PerformanceSeverity severity;
    float triggerValue;
    float threshold;
    std::string message;
    std::string recommendation;
    std::chrono::steady_clock::time_point timestamp;
    bool isActive;
    uint32_t occurrenceCount;
};

// Performance profile for different scenarios
struct PerformanceProfile {
    std::string profileName;
    std::string description;
    std::unordered_map<MetricType, float> thresholds;
    std::unordered_map<MetricType, bool> enabledMetrics;
    uint32_t samplingInterval; // milliseconds
    uint32_t historySize;
    bool enablePredictiveAnalysis;
    bool enableAutomaticOptimization;
};

// Hardware/system information (renamed to avoid conflict with SystemManager.hpp)
struct HardwareInfo {
    std::string cpuName;
    uint32_t cpuCores;
    uint32_t cpuThreads;
    uint64_t totalMemory;
    std::string gpuName;
    uint64_t gpuMemory;
    std::string osVersion;
    std::string gameVersion;
    std::string modVersion;
    bool isDebugBuild;
    std::chrono::steady_clock::time_point bootTime;
};

// Performance statistics
struct PerformanceStats {
    uint64_t totalSamples = 0;
    uint64_t totalAlerts = 0;
    uint32_t activeAlerts = 0;
    float averageFPS = 0.0f;
    float averageFrameTime = 0.0f;
    float averageCPUUsage = 0.0f;
    float averageMemoryUsage = 0.0f;
    float averageNetworkLatency = 0.0f;
    std::chrono::steady_clock::time_point sessionStart;
    std::chrono::milliseconds totalSessionTime{0};
};

// Main performance monitoring system
class PerformanceMonitor {
public:
    static PerformanceMonitor& Instance();

    // System lifecycle
    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);

    // Profile management
    bool LoadProfile(const std::string& profileName);
    bool SaveProfile(const std::string& profileName, const PerformanceProfile& profile);
    std::vector<std::string> GetAvailableProfiles() const;
    PerformanceProfile GetCurrentProfile() const;
    void SetProfile(const PerformanceProfile& profile);

    // Metric management
    bool RegisterMetric(MetricType type, const std::string& name, const std::string& unit = "");
    bool UpdateMetric(MetricType type, float value);
    bool UpdateMetric(const std::string& name, float value);
    PerformanceMetric GetMetric(MetricType type) const;
    PerformanceMetric GetMetric(const std::string& name) const;
    std::vector<PerformanceMetric> GetAllMetrics() const;

    // FPS and frame time monitoring
    void RecordFrameTime(float frameTimeMs);
    float GetCurrentFPS() const;
    float GetAverageFrameTime() const;
    float GetFrameTimePercentile(float percentile) const;

    // CPU monitoring
    void UpdateCPUUsage(float cpuPercent);
    float GetCurrentCPUUsage() const;
    float GetAverageCPUUsage() const;

    // Memory monitoring
    void UpdateMemoryUsage(uint64_t usedBytes, uint64_t totalBytes);
    float GetCurrentMemoryUsage() const; // Returns percentage
    uint64_t GetCurrentMemoryBytes() const;
    float GetAverageMemoryUsage() const;

    // GPU monitoring
    void UpdateGPUUsage(float gpuPercent, uint64_t gpuMemoryUsed = 0);
    float GetCurrentGPUUsage() const;
    uint64_t GetCurrentGPUMemory() const;

    // Network monitoring
    void UpdateNetworkMetrics(float latencyMs, uint64_t bytesSent, uint64_t bytesReceived);
    float GetCurrentNetworkLatency() const;
    float GetAverageNetworkLatency() const;
    uint64_t GetNetworkBandwidthUp() const;
    uint64_t GetNetworkBandwidthDown() const;

    // Audio/Voice monitoring
    void UpdateAudioLatency(float latencyMs);
    void UpdateVoiceLatency(float latencyMs);
    float GetCurrentAudioLatency() const;
    float GetCurrentVoiceLatency() const;

    // Alert system
    bool SetThreshold(MetricType type, float threshold, PerformanceSeverity severity = PerformanceSeverity::Warning);
    bool SetThreshold(const std::string& metricName, float threshold, PerformanceSeverity severity = PerformanceSeverity::Warning);
    std::vector<PerformanceAlert> GetActiveAlerts() const;
    std::vector<PerformanceAlert> GetAlertHistory() const;
    bool DismissAlert(uint64_t alertId);
    void ClearAllAlerts();

    // Performance analysis
    bool AnalyzePerformanceTrend(MetricType type, uint32_t sampleCount = 100);
    std::string GetPerformanceReport() const;
    std::string GetOptimizationRecommendations() const;
    bool PredictPerformanceIssues();

    // System information
    HardwareInfo GetSystemInfo() const;
    void RefreshSystemInfo();

    // Statistics
    PerformanceStats GetStatistics() const;
    void ResetStatistics();

    // Export and logging
    bool ExportMetricsToCSV(const std::string& filename) const;
    bool ExportMetricsToJSON(const std::string& filename) const;
    void EnableLogging(bool enabled, const std::string& logFile = "");

    // Event callbacks
    using PerformanceEventCallback = std::function<void(const PerformanceAlert& alert)>;
    void RegisterAlertCallback(PerformanceEventCallback callback);
    void UnregisterAlertCallback();

    // Real-time monitoring
    void StartRealTimeMonitoring();
    void StopRealTimeMonitoring();
    bool IsRealTimeMonitoringActive() const;

    // Performance optimization hints
    void EnableAutomaticOptimization(bool enabled);
    bool IsAutomaticOptimizationEnabled() const;
    void TriggerOptimization();

    // Custom metrics
    bool RegisterCustomMetric(const std::string& name, const std::string& unit = "");
    bool UpdateCustomMetric(const std::string& name, float value);

    // Profiling integration
    void BeginProfileScope(const std::string& scopeName);
    void EndProfileScope(const std::string& scopeName);
    std::unordered_map<std::string, float> GetProfileScopeStats() const;

private:
    PerformanceMonitor() = default;
    ~PerformanceMonitor() = default;
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;

    // Core monitoring
    void MonitoringLoop();
    void CollectSystemMetrics();
    void ProcessMetricUpdates();
    void CheckThresholds();
    void AnalyzeTrends();
    void UpdateStatistics();

    // Metric processing
    void ProcessMetric(PerformanceMetric& metric, float newValue);
    void UpdateMetricHistory(PerformanceMetric& metric, float value);
    float CalculateMovingAverage(const std::queue<float>& history, uint32_t samples = 0);

    // Alert management
    uint64_t GenerateAlertId();
    void TriggerAlert(AlertType type, MetricType metricType, const std::string& metricName,
                     PerformanceSeverity severity, float value, float threshold,
                     const std::string& message = "");
    void ProcessAlerts();
    std::string GenerateRecommendation(MetricType type, float value, float threshold);

    // System monitoring
    void CollectCPUMetrics();
    void CollectMemoryMetrics();
    void CollectGPUMetrics();
    void CollectNetworkMetrics();
    void CollectDiskMetrics();

    // Performance analysis
    bool DetectPerformanceSpike(const PerformanceMetric& metric);
    bool DetectPerformanceDegradation(const PerformanceMetric& metric);
    float CalculateTrendSlope(const std::queue<float>& history);

    // Optimization
    void ApplyAutomaticOptimizations();
    void OptimizeForLowFPS();
    void OptimizeForHighMemory();
    void OptimizeForHighCPU();

    // Data persistence
    bool SaveMetricsToFile();
    bool LoadMetricsFromFile();
    std::string GetMetricsFilePath() const;

    // Utility methods
    uint64_t GetCurrentTimestamp() const;
    std::string FormatTimestamp(std::chrono::steady_clock::time_point timestamp) const;
    std::string FormatValue(float value, const std::string& unit) const;

    // Data storage
    std::unordered_map<MetricType, PerformanceMetric> m_metrics;
    std::unordered_map<std::string, PerformanceMetric> m_customMetrics;
    std::vector<PerformanceAlert> m_activeAlerts;
    std::vector<PerformanceAlert> m_alertHistory;
    std::unordered_map<MetricType, float> m_thresholds;
    std::unordered_map<std::string, float> m_customThresholds;

    // Profiling scopes
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_profileScopes;
    std::unordered_map<std::string, float> m_profileStats;

    // Configuration
    PerformanceProfile m_currentProfile;
    HardwareInfo m_systemInfo;
    PerformanceStats m_statistics;

    // Threading
    std::thread m_monitoringThread;
    std::atomic<bool> m_shouldStop{false};
    std::atomic<bool> m_realTimeMonitoring{false};

    // Synchronization
    mutable std::mutex m_metricsMutex;
    mutable std::mutex m_alertsMutex;
    mutable std::mutex m_statsMutex;
    mutable std::mutex m_profileMutex;

    // System state
    bool m_initialized = false;
    bool m_loggingEnabled = false;
    std::string m_logFile;
    bool m_automaticOptimization = false;

    // Event system
    PerformanceEventCallback m_alertCallback;
    std::mutex m_callbackMutex;

    // Timing
    std::chrono::steady_clock::time_point m_lastUpdate;
    std::chrono::steady_clock::time_point m_sessionStart;

    // FPS calculation
    std::queue<float> m_frameTimeHistory;
    float m_currentFPS = 0.0f;
    std::chrono::steady_clock::time_point m_lastFPSUpdate;

    // Network tracking
    uint64_t m_lastNetworkBytesSent = 0;
    uint64_t m_lastNetworkBytesReceived = 0;
    std::chrono::steady_clock::time_point m_lastNetworkUpdate;

    // Alert ID generation
    std::atomic<uint64_t> m_nextAlertId{1};
};

// Performance profiler helper class
class PerformanceProfiler {
public:
    PerformanceProfiler(const std::string& scopeName);
    ~PerformanceProfiler();

private:
    std::string m_scopeName;
    std::chrono::steady_clock::time_point m_startTime;
};

// Macro for easy profiling
#define PERF_PROFILE(name) CoopNet::PerformanceProfiler _perf_prof(name)

// Performance configuration presets
namespace PerformancePresets {
    PerformanceProfile GetLowEndProfile();
    PerformanceProfile GetMidRangeProfile();
    PerformanceProfile GetHighEndProfile();
    PerformanceProfile GetDevelopmentProfile();
    PerformanceProfile GetServerProfile();
}

// Utility functions for performance monitoring
namespace PerformanceUtils {
    std::string GetMetricTypeName(MetricType type);
    std::string GetSeverityName(PerformanceSeverity severity);
    std::string GetAlertTypeName(AlertType type);

    // System resource utilities
    float GetCurrentCPUUsage();
    uint64_t GetCurrentMemoryUsage();
    float GetCurrentGPUUsage();
    uint64_t GetCurrentGPUMemory();

    // Performance calculation utilities
    float CalculatePercentile(const std::vector<float>& values, float percentile);
    float CalculateStandardDeviation(const std::vector<float>& values);
    bool IsPerformanceAcceptable(MetricType type, float value);

    // System information utilities
    std::string GetCPUName();
    uint64_t GetTotalSystemMemory();
    std::string GetGPUName();
    std::string GetOSVersion();
}

} // namespace CoopNet
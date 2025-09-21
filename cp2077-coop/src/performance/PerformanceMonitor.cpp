#include "PerformanceMonitor.hpp"
#include "../core/Logger.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <spdlog/spdlog.h>

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#endif

namespace CoopNet {

PerformanceMonitor& PerformanceMonitor::Instance() {
    static PerformanceMonitor instance;
    return instance;
}

bool PerformanceMonitor::Initialize() {
    if (m_initialized) {
        return true;
    }

    spdlog::info("[PerformanceMonitor] Initializing performance monitoring system");

    // Initialize system information
    RefreshSystemInfo();

    // Set up default profile
    m_currentProfile = PerformancePresets::GetMidRangeProfile();

    // Register core metrics
    RegisterMetric(MetricType::FPS, "FPS", "fps");
    RegisterMetric(MetricType::FrameTime, "Frame Time", "ms");
    RegisterMetric(MetricType::CPUUsage, "CPU Usage", "%");
    RegisterMetric(MetricType::MemoryUsage, "Memory Usage", "%");
    RegisterMetric(MetricType::NetworkLatency, "Network Latency", "ms");
    RegisterMetric(MetricType::NetworkBandwidth, "Network Bandwidth", "KB/s");
    RegisterMetric(MetricType::GPUUsage, "GPU Usage", "%");
    RegisterMetric(MetricType::AudioLatency, "Audio Latency", "ms");
    RegisterMetric(MetricType::VoiceLatency, "Voice Latency", "ms");

    // Set default thresholds
    SetThreshold(MetricType::FPS, 30.0f, PerformanceSeverity::Warning);
    SetThreshold(MetricType::FPS, 15.0f, PerformanceSeverity::Critical);
    SetThreshold(MetricType::FrameTime, 33.33f, PerformanceSeverity::Warning);
    SetThreshold(MetricType::CPUUsage, 80.0f, PerformanceSeverity::Warning);
    SetThreshold(MetricType::MemoryUsage, 85.0f, PerformanceSeverity::Warning);
    SetThreshold(MetricType::NetworkLatency, 100.0f, PerformanceSeverity::Warning);

    // Initialize statistics
    ResetStatistics();

    // Start monitoring thread
    m_shouldStop.store(false);
    m_monitoringThread = std::thread(&PerformanceMonitor::MonitoringLoop, this);

    m_initialized = true;
    m_sessionStart = std::chrono::steady_clock::now();

    spdlog::info("[PerformanceMonitor] Performance monitoring system initialized");
    return true;
}

void PerformanceMonitor::Shutdown() {
    if (!m_initialized) {
        return;
    }

    spdlog::info("[PerformanceMonitor] Shutting down performance monitoring system");

    // Stop monitoring thread
    m_shouldStop.store(true);
    if (m_monitoringThread.joinable()) {
        m_monitoringThread.join();
    }

    // Save final metrics
    if (m_loggingEnabled) {
        SaveMetricsToFile();
    }

    // Clear all data
    {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        m_metrics.clear();
        m_customMetrics.clear();
    }

    {
        std::lock_guard<std::mutex> lock(m_alertsMutex);
        m_activeAlerts.clear();
        m_alertHistory.clear();
    }

    m_initialized = false;
}

void PerformanceMonitor::Update(float deltaTime) {
    if (!m_initialized) {
        return;
    }

    auto currentTime = std::chrono::steady_clock::now();

    // Calculate FPS from deltaTime
    if (deltaTime > 0.0f) {
        float fps = 1.0f / deltaTime;
        UpdateMetric(MetricType::FPS, fps);
        UpdateMetric(MetricType::FrameTime, deltaTime * 1000.0f); // Convert to milliseconds
    }

    // Update system metrics periodically (every 100ms)
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - m_lastUpdate).count();

    if (elapsed >= 100) {
        // Update CPU usage
        UpdateMetric(MetricType::CPUUsage, GetCurrentCPUUsage());

        // Update memory usage
        UpdateMetric(MetricType::MemoryUsage, GetCurrentMemoryUsage());

        // Update GPU usage
        UpdateMetric(MetricType::GPUUsage, GetCurrentGPUUsage());

        m_lastUpdate = currentTime;
    }

    // Update statistics
    UpdateStatistics();

    // Check for performance alerts
    CheckThresholds();
}

void PerformanceMonitor::MonitoringLoop() {
    spdlog::debug("[PerformanceMonitor] Monitoring thread started");

    auto lastCollectionTime = std::chrono::steady_clock::now();

    while (!m_shouldStop.load()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCollectionTime);

        if (elapsed.count() >= m_currentProfile.samplingInterval) {
            // Collect system metrics
            CollectSystemMetrics();

            // Process metrics and check thresholds
            ProcessMetricUpdates();
            CheckThresholds();

            // Analyze trends
            AnalyzeTrends();

            // Update statistics
            UpdateStatistics();

            // Apply automatic optimizations if enabled
            if (m_automaticOptimization) {
                ApplyAutomaticOptimizations();
            }

            lastCollectionTime = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    spdlog::debug("[PerformanceMonitor] Monitoring thread stopped");
}

void PerformanceMonitor::CollectSystemMetrics() {
    if (!m_realTimeMonitoring.load()) {
        return;
    }

    CollectCPUMetrics();
    CollectMemoryMetrics();
    CollectGPUMetrics();
    CollectNetworkMetrics();
}

void PerformanceMonitor::CollectCPUMetrics() {
    float cpuUsage = PerformanceUtils::GetCurrentCPUUsage();
    UpdateMetric(MetricType::CPUUsage, cpuUsage);
}

void PerformanceMonitor::CollectMemoryMetrics() {
    uint64_t memoryUsage = PerformanceUtils::GetCurrentMemoryUsage();
    uint64_t totalMemory = m_systemInfo.totalMemory;

    if (totalMemory > 0) {
        float memoryPercent = (static_cast<float>(memoryUsage) / totalMemory) * 100.0f;
        UpdateMetric(MetricType::MemoryUsage, memoryPercent);
    }
}

void PerformanceMonitor::CollectGPUMetrics() {
    float gpuUsage = PerformanceUtils::GetCurrentGPUUsage();
    UpdateMetric(MetricType::GPUUsage, gpuUsage);
}

void PerformanceMonitor::CollectNetworkMetrics() {
    // Network metrics are updated externally through UpdateNetworkMetrics
}

bool PerformanceMonitor::RegisterMetric(MetricType type, const std::string& name, const std::string& unit) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);

    PerformanceMetric metric;
    metric.type = type;
    metric.name = name;
    metric.currentValue = 0.0f;
    metric.averageValue = 0.0f;
    metric.minValue = std::numeric_limits<float>::max();
    metric.maxValue = std::numeric_limits<float>::lowest();
    metric.unit = unit;
    metric.timestamp = std::chrono::steady_clock::now();
    metric.sampleCount = 0;
    metric.isActive = true;

    m_metrics[type] = metric;

    spdlog::debug("[PerformanceMonitor] Registered metric: {} ({})", name, unit);
    return true;
}

bool PerformanceMonitor::UpdateMetric(MetricType type, float value) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);

    auto it = m_metrics.find(type);
    if (it == m_metrics.end()) {
        return false;
    }

    ProcessMetric(it->second, value);
    return true;
}

void PerformanceMonitor::ProcessMetric(PerformanceMetric& metric, float newValue) {
    metric.currentValue = newValue;
    metric.timestamp = std::chrono::steady_clock::now();

    // Update min/max
    metric.minValue = std::min(metric.minValue, newValue);
    metric.maxValue = std::max(metric.maxValue, newValue);

    // Update history
    UpdateMetricHistory(metric, newValue);

    // Calculate moving average
    metric.averageValue = CalculateMovingAverage(metric.history);
    metric.sampleCount++;
}

void PerformanceMonitor::UpdateMetricHistory(PerformanceMetric& metric, float value) {
    metric.history.push(value);

    // Limit history size
    while (metric.history.size() > m_currentProfile.historySize) {
        metric.history.pop();
    }
}

float PerformanceMonitor::CalculateMovingAverage(const std::queue<float>& history, uint32_t samples) {
    if (history.empty()) {
        return 0.0f;
    }

    uint32_t sampleCount = (samples == 0) ? static_cast<uint32_t>(history.size()) : std::min(samples, static_cast<uint32_t>(history.size()));

    float sum = 0.0f;
    auto tempQueue = history;

    // Skip to the last 'sampleCount' samples
    while (tempQueue.size() > sampleCount) {
        tempQueue.pop();
    }

    while (!tempQueue.empty()) {
        sum += tempQueue.front();
        tempQueue.pop();
    }

    return sum / sampleCount;
}

void PerformanceMonitor::RecordFrameTime(float frameTimeMs) {
    UpdateMetric(MetricType::FrameTime, frameTimeMs);

    // Calculate FPS
    float fps = (frameTimeMs > 0.0f) ? (1000.0f / frameTimeMs) : 0.0f;
    UpdateMetric(MetricType::FPS, fps);

    // Update FPS history for more accurate calculations
    {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        m_frameTimeHistory.push(frameTimeMs);

        while (m_frameTimeHistory.size() > 60) { // Keep last 60 frames
            m_frameTimeHistory.pop();
        }

        // Calculate smoothed FPS
        if (!m_frameTimeHistory.empty()) {
            float avgFrameTime = CalculateMovingAverage(m_frameTimeHistory, 30);
            m_currentFPS = (avgFrameTime > 0.0f) ? (1000.0f / avgFrameTime) : 0.0f;
        }
    }
}

void PerformanceMonitor::CheckThresholds() {
    std::lock_guard<std::mutex> metricsLock(m_metricsMutex);

    for (const auto& [type, metric] : m_metrics) {
        auto thresholdIt = m_thresholds.find(type);
        if (thresholdIt == m_thresholds.end()) {
            continue;
        }

        float threshold = thresholdIt->second;
        bool shouldAlert = false;
        PerformanceSeverity severity = PerformanceSeverity::Warning;

        // Check if metric crosses threshold
        switch (type) {
            case MetricType::FPS:
                shouldAlert = metric.currentValue < threshold;
                severity = (metric.currentValue < 15.0f) ? PerformanceSeverity::Critical : PerformanceSeverity::Warning;
                break;
            case MetricType::FrameTime:
            case MetricType::CPUUsage:
            case MetricType::MemoryUsage:
            case MetricType::NetworkLatency:
            case MetricType::AudioLatency:
            case MetricType::VoiceLatency:
                shouldAlert = metric.currentValue > threshold;
                severity = (metric.currentValue > threshold * 1.5f) ? PerformanceSeverity::Critical : PerformanceSeverity::Warning;
                break;
            default:
                break;
        }

        if (shouldAlert) {
            std::string message = metric.name + " is " +
                ((type == MetricType::FPS) ? "below" : "above") + " threshold";

            TriggerAlert(AlertType::Threshold, type, metric.name, severity,
                        metric.currentValue, threshold, message);
        }
    }
}

void PerformanceMonitor::TriggerAlert(AlertType type, MetricType metricType, const std::string& metricName,
                                     PerformanceSeverity severity, float value, float threshold,
                                     const std::string& message) {
    std::lock_guard<std::mutex> lock(m_alertsMutex);

    // Check if similar alert is already active
    for (const auto& alert : m_activeAlerts) {
        if (alert.metricType == metricType && alert.type == type && alert.isActive) {
            return; // Don't create duplicate alerts
        }
    }

    PerformanceAlert alert;
    alert.alertId = GenerateAlertId();
    alert.type = type;
    alert.metricType = metricType;
    alert.metricName = metricName;
    alert.severity = severity;
    alert.triggerValue = value;
    alert.threshold = threshold;
    alert.message = message.empty() ? (metricName + " threshold exceeded") : message;
    alert.recommendation = GenerateRecommendation(metricType, value, threshold);
    alert.timestamp = std::chrono::steady_clock::now();
    alert.isActive = true;
    alert.occurrenceCount = 1;

    m_activeAlerts.push_back(alert);
    m_alertHistory.push_back(alert);

    // Trigger callback if registered
    {
        std::lock_guard<std::mutex> callbackLock(m_callbackMutex);
        if (m_alertCallback) {
            try {
                m_alertCallback(alert);
            } catch (const std::exception& e) {
                spdlog::error("[PerformanceMonitor] Exception in alert callback: {}", e.what());
            }
        }
    }

    spdlog::warn("[PerformanceMonitor] Performance alert: {} - {}", alert.message, alert.recommendation);
}

std::string PerformanceMonitor::GenerateRecommendation(MetricType type, float value, float threshold) {
    switch (type) {
        case MetricType::FPS:
            if (value < 30.0f) {
                return "Consider lowering graphics settings or reducing multiplayer players";
            }
            break;
        case MetricType::FrameTime:
            return "Frame time is high - check for performance bottlenecks";
        case MetricType::CPUUsage:
            if (value > 90.0f) {
                return "CPU usage is critical - close other applications or reduce game settings";
            }
            return "CPU usage is high - consider optimizing background processes";
        case MetricType::MemoryUsage:
            if (value > 90.0f) {
                return "Memory usage is critical - restart the game or close other applications";
            }
            return "Memory usage is high - consider reducing texture quality";
        case MetricType::NetworkLatency:
            return "High network latency detected - check internet connection";
        case MetricType::AudioLatency:
            return "Audio latency is high - check audio driver settings";
        case MetricType::VoiceLatency:
            return "Voice chat latency is high - consider changing voice quality";
        default:
            return "Performance issue detected - check system resources";
    }
    return "Monitor performance and consider system optimization";
}

PerformanceStats PerformanceMonitor::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);

    PerformanceStats stats = m_statistics;

    // Update session time
    auto now = std::chrono::steady_clock::now();
    stats.totalSessionTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_sessionStart);

    return stats;
}

void PerformanceMonitor::UpdateStatistics() {
    std::lock_guard<std::mutex> statsLock(m_statsMutex);
    std::lock_guard<std::mutex> metricsLock(m_metricsMutex);

    m_statistics.totalSamples++;

    // Update averages
    auto fpsIt = m_metrics.find(MetricType::FPS);
    if (fpsIt != m_metrics.end()) {
        m_statistics.averageFPS = fpsIt->second.averageValue;
    }

    auto frameTimeIt = m_metrics.find(MetricType::FrameTime);
    if (frameTimeIt != m_metrics.end()) {
        m_statistics.averageFrameTime = frameTimeIt->second.averageValue;
    }

    auto cpuIt = m_metrics.find(MetricType::CPUUsage);
    if (cpuIt != m_metrics.end()) {
        m_statistics.averageCPUUsage = cpuIt->second.averageValue;
    }

    auto memoryIt = m_metrics.find(MetricType::MemoryUsage);
    if (memoryIt != m_metrics.end()) {
        m_statistics.averageMemoryUsage = memoryIt->second.averageValue;
    }

    auto latencyIt = m_metrics.find(MetricType::NetworkLatency);
    if (latencyIt != m_metrics.end()) {
        m_statistics.averageNetworkLatency = latencyIt->second.averageValue;
    }

    // Update alert counts
    {
        std::lock_guard<std::mutex> alertLock(m_alertsMutex);
        m_statistics.totalAlerts = m_alertHistory.size();
        m_statistics.activeAlerts = static_cast<uint32_t>(std::count_if(m_activeAlerts.begin(), m_activeAlerts.end(),
            [](const PerformanceAlert& alert) { return alert.isActive; }));
    }
}

std::string PerformanceMonitor::GetPerformanceReport() const {
    std::stringstream report;

    auto stats = GetStatistics();

    report << "=== Performance Report ===\n";
    report << "Session Duration: " << stats.totalSessionTime.count() / 1000.0f << " seconds\n";
    report << "Total Samples: " << stats.totalSamples << "\n";
    report << "Average FPS: " << std::fixed << std::setprecision(1) << stats.averageFPS << "\n";
    report << "Average Frame Time: " << std::fixed << std::setprecision(2) << stats.averageFrameTime << " ms\n";
    report << "Average CPU Usage: " << std::fixed << std::setprecision(1) << stats.averageCPUUsage << "%\n";
    report << "Average Memory Usage: " << std::fixed << std::setprecision(1) << stats.averageMemoryUsage << "%\n";
    report << "Average Network Latency: " << std::fixed << std::setprecision(1) << stats.averageNetworkLatency << " ms\n";
    report << "Total Alerts: " << stats.totalAlerts << "\n";
    report << "Active Alerts: " << stats.activeAlerts << "\n";

    // Add detailed metrics
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    report << "\n=== Detailed Metrics ===\n";

    for (const auto& [type, metric] : m_metrics) {
        report << metric.name << ": "
               << "Current=" << std::fixed << std::setprecision(2) << metric.currentValue
               << ", Avg=" << metric.averageValue
               << ", Min=" << metric.minValue
               << ", Max=" << metric.maxValue
               << " " << metric.unit << "\n";
    }

    return report.str();
}

void PerformanceMonitor::RefreshSystemInfo() {
    m_systemInfo.cpuName = PerformanceUtils::GetCPUName();
    m_systemInfo.totalMemory = PerformanceUtils::GetTotalSystemMemory();
    m_systemInfo.gpuName = PerformanceUtils::GetGPUName();
    m_systemInfo.osVersion = PerformanceUtils::GetOSVersion();
    m_systemInfo.gameVersion = "Cyberpunk 2077 v2.0+";
    m_systemInfo.modVersion = "CoopNet v1.0";
    m_systemInfo.isDebugBuild =
#ifdef _DEBUG
        true;
#else
        false;
#endif
    m_systemInfo.bootTime = std::chrono::steady_clock::now();

    spdlog::info("[PerformanceMonitor] System Info - CPU: {}, Memory: {} GB, GPU: {}",
                 m_systemInfo.cpuName, m_systemInfo.totalMemory / (1024 * 1024 * 1024), m_systemInfo.gpuName);
}

void PerformanceMonitor::StartRealTimeMonitoring() {
    m_realTimeMonitoring.store(true);
    spdlog::info("[PerformanceMonitor] Real-time monitoring started");
}

void PerformanceMonitor::StopRealTimeMonitoring() {
    m_realTimeMonitoring.store(false);
    spdlog::info("[PerformanceMonitor] Real-time monitoring stopped");
}

uint64_t PerformanceMonitor::GenerateAlertId() {
    return m_nextAlertId.fetch_add(1);
}

// Performance profiler implementation
PerformanceProfiler::PerformanceProfiler(const std::string& scopeName)
    : m_scopeName(scopeName), m_startTime(std::chrono::steady_clock::now()) {
    PerformanceMonitor::Instance().BeginProfileScope(m_scopeName);
}

PerformanceProfiler::~PerformanceProfiler() {
    PerformanceMonitor::Instance().EndProfileScope(m_scopeName);
}

void PerformanceMonitor::BeginProfileScope(const std::string& scopeName) {
    std::lock_guard<std::mutex> lock(m_profileMutex);
    m_profileScopes[scopeName] = std::chrono::steady_clock::now();
}

void PerformanceMonitor::EndProfileScope(const std::string& scopeName) {
    auto endTime = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(m_profileMutex);

    auto it = m_profileScopes.find(scopeName);
    if (it != m_profileScopes.end()) {
        auto duration = std::chrono::duration<float, std::milli>(endTime - it->second);
        m_profileStats[scopeName] = duration.count();
        m_profileScopes.erase(it);
    }
}

// Performance presets implementation
namespace PerformancePresets {
    PerformanceProfile GetMidRangeProfile() {
        PerformanceProfile profile;
        profile.profileName = "Mid-Range";
        profile.description = "Balanced performance monitoring for mid-range systems";
        profile.samplingInterval = 1000; // 1 second
        profile.historySize = 300; // 5 minutes at 1 second intervals
        profile.enablePredictiveAnalysis = true;
        profile.enableAutomaticOptimization = false;

        // Set thresholds
        profile.thresholds[MetricType::FPS] = 30.0f;
        profile.thresholds[MetricType::CPUUsage] = 80.0f;
        profile.thresholds[MetricType::MemoryUsage] = 85.0f;
        profile.thresholds[MetricType::NetworkLatency] = 100.0f;

        // Enable all metrics
        profile.enabledMetrics[MetricType::FPS] = true;
        profile.enabledMetrics[MetricType::FrameTime] = true;
        profile.enabledMetrics[MetricType::CPUUsage] = true;
        profile.enabledMetrics[MetricType::MemoryUsage] = true;
        profile.enabledMetrics[MetricType::NetworkLatency] = true;
        profile.enabledMetrics[MetricType::GPUUsage] = true;

        return profile;
    }
}

// Utility functions implementation
namespace PerformanceUtils {
    std::string GetMetricTypeName(MetricType type) {
        switch (type) {
            case MetricType::FPS: return "FPS";
            case MetricType::FrameTime: return "Frame Time";
            case MetricType::CPUUsage: return "CPU Usage";
            case MetricType::MemoryUsage: return "Memory Usage";
            case MetricType::NetworkLatency: return "Network Latency";
            case MetricType::NetworkBandwidth: return "Network Bandwidth";
            case MetricType::GPUUsage: return "GPU Usage";
            case MetricType::AudioLatency: return "Audio Latency";
            case MetricType::VoiceLatency: return "Voice Latency";
            default: return "Unknown";
        }
    }

    float GetCurrentCPUUsage() {
        // Platform-specific CPU usage implementation
#ifdef _WIN32
        static PDH_HQUERY cpuQuery;
        static PDH_HCOUNTER cpuTotal;
        static bool initialized = false;

        if (!initialized) {
            PdhOpenQuery(NULL, NULL, &cpuQuery);
            PdhAddEnglishCounterA(cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
            PdhCollectQueryData(cpuQuery);
            initialized = true;
        }

        PDH_FMT_COUNTERVALUE counterVal;
        PdhCollectQueryData(cpuQuery);
        PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
        return static_cast<float>(counterVal.doubleValue);
#else
        return 0.0f; // Placeholder for other platforms
#endif
    }

    uint64_t GetCurrentMemoryUsage() {
#ifdef _WIN32
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        return memInfo.ullTotalPhys - memInfo.ullAvailPhys;
#else
        return 0; // Placeholder for other platforms
#endif
    }

    std::string GetCPUName() {
#ifdef _WIN32
        char cpu_string[0x40];
        int cpu_info[4] = {-1};
        __cpuid(cpu_info, 0x80000000);
        unsigned int nExIds = cpu_info[0];

        memset(cpu_string, 0, sizeof(cpu_string));

        for (unsigned int i = 0x80000000; i <= nExIds; ++i) {
            __cpuid(cpu_info, i);
            if (i == 0x80000002)
                memcpy(cpu_string, cpu_info, sizeof(cpu_info));
            else if (i == 0x80000003)
                memcpy(cpu_string + 16, cpu_info, sizeof(cpu_info));
            else if (i == 0x80000004)
                memcpy(cpu_string + 32, cpu_info, sizeof(cpu_info));
        }

        return std::string(cpu_string);
#else
        return "Unknown CPU";
#endif
    }

    uint64_t GetTotalSystemMemory() {
#ifdef _WIN32
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        return memInfo.ullTotalPhys;
#else
        return 0;
#endif
    }

    std::string GetOSVersion() {
#ifdef _WIN32
        return "Windows";
#else
        return "Unknown OS";
#endif
    }

    float GetCurrentGPUUsage() {
        // GPU usage monitoring requires vendor-specific APIs
        // This would be implemented using NVIDIA ML API, AMD ADL, etc.
        return 0.0f; // Placeholder
    }

    std::string GetGPUName() {
        // GPU information would be retrieved using graphics APIs
        return "Unknown GPU"; // Placeholder
    }
}

} // namespace CoopNet
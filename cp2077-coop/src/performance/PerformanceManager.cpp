#include "PerformanceManager.hpp"
#include "../core/Logger.hpp"
#include <algorithm>
#include <numeric>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#pragma comment(lib, "pdh.lib")
#endif

namespace CoopNet {

PerformanceManager& PerformanceManager::Instance() {
    static PerformanceManager instance;
    return instance;
}

PerformanceManager::PerformanceManager() {
    m_frameTimeHistory.resize(60); // Keep 1 second of frame history at 60fps
    std::fill(m_frameTimeHistory.begin(), m_frameTimeHistory.end(), 16.67f);
}

PerformanceManager::~PerformanceManager() {
    Shutdown();
}

bool PerformanceManager::Initialize() {
    Logger::Log(LogLevel::INFO, "Initializing Performance Manager");

    // Start monitoring thread
    m_monitoringActive = true;
    m_monitoringThread = std::thread(&PerformanceManager::MonitoringThread, this);

    // Initialize subsystems
    CPUMonitor::Instance();
    MemoryMonitor::Instance();
    GPUMonitor::Instance();
    NetworkMonitor::Instance();
    AutoOptimizer::Instance();

    Logger::Log(LogLevel::INFO, "Performance Manager initialized");
    return true;
}

void PerformanceManager::Shutdown() {
    Logger::Log(LogLevel::INFO, "Shutting down Performance Manager");

    // Stop monitoring thread
    m_monitoringActive = false;
    if (m_monitoringThread.joinable()) {
        m_monitoringThread.join();
    }

    // Clear resource pools
    std::lock_guard<std::mutex> lock(m_poolsMutex);
    m_resourcePools.clear();

    Logger::Log(LogLevel::INFO, "Performance Manager shut down");
}

void PerformanceManager::Update(float deltaTime) {
    UpdateFrameMetrics(deltaTime);
    CheckAndApplyOptimizations();

    // Update quality adjustment cooldown
    if (m_qualityAdjustmentCooldown > 0.0f) {
        m_qualityAdjustmentCooldown -= deltaTime;
    }
}

void PerformanceManager::UpdateFrameMetrics(float deltaTime) {
    m_frameCount++;
    float frameTimeMs = deltaTime * 1000.0f;
    m_frameTime = frameTimeMs;

    // Update frame history
    {
        std::lock_guard<std::mutex> lock(m_frameMetricsMutex);
        m_frameTimeHistory.push_back(frameTimeMs);
        if (m_frameTimeHistory.size() > 60) {
            m_frameTimeHistory.pop_front();
        }

        // Calculate average frame time
        float sum = std::accumulate(m_frameTimeHistory.begin(), m_frameTimeHistory.end(), 0.0f);
        m_averageFrameTime = sum / m_frameTimeHistory.size();
        m_currentFPS = 1000.0f / m_averageFrameTime;
    }
}

void PerformanceManager::UpdateSystemMetrics() {
    // Update CPU usage
    CPUMonitor::Instance().Update();
    m_cpuUsage = CPUMonitor::Instance().GetCPUUsage();

    // Update memory usage
    MemoryMonitor::Instance().Update();
    m_memoryUsageMB = MemoryMonitor::Instance().GetUsedMemoryMB();

    // Update GPU metrics
    GPUMonitor::Instance().Update();

    // Update network metrics
    NetworkMonitor::Instance().Update();
    m_networkLatency = NetworkMonitor::Instance().GetLatency();
    m_networkBandwidth = NetworkMonitor::Instance().GetBandwidthUsage();
    m_packetLoss = NetworkMonitor::Instance().GetPacketLoss();
}

void PerformanceManager::CheckAndApplyOptimizations() {
    if (!m_adaptiveQualityEnabled || m_qualityAdjustmentCooldown > 0.0f) {
        return;
    }

    float currentFPS = GetCurrentFPS();
    float targetFPS = m_targetFrameRate;

    // Adjust quality based on performance
    if (currentFPS < targetFPS * 0.85f) { // Below 85% of target
        if (m_currentQualityLevel > 0) {
            SetQualityLevel(m_currentQualityLevel - 1);
            m_qualityAdjustmentCooldown = 5.0f; // 5 second cooldown
            Logger::Log(LogLevel::INFO, "Lowered quality level to " + std::to_string(m_currentQualityLevel) +
                       " (FPS: " + std::to_string(currentFPS) + ")");
        }
    } else if (currentFPS > targetFPS * 1.15f) { // Above 115% of target
        if (m_currentQualityLevel < 3) {
            SetQualityLevel(m_currentQualityLevel + 1);
            m_qualityAdjustmentCooldown = 10.0f; // 10 second cooldown for upgrades
            Logger::Log(LogLevel::INFO, "Raised quality level to " + std::to_string(m_currentQualityLevel) +
                       " (FPS: " + std::to_string(currentFPS) + ")");
        }
    }

    // Check memory usage
    if (m_memoryUsageMB > m_memoryThresholdMB) {
        OptimizeMemoryUsage();
    }
}

void PerformanceManager::OptimizeMemoryUsage() {
    Logger::Log(LogLevel::INFO, "Optimizing memory usage (current: " + std::to_string(m_memoryUsageMB) + "MB)");

    // Request garbage collection
    RequestGarbageCollection();

    // Trim working set
    MemoryMonitor::Instance().TrimWorkingSet();

    // Compact heaps
    MemoryMonitor::Instance().CompactHeaps();
}

void PerformanceManager::OptimizeCPUUsage() {
    if (m_cpuUsage > 80.0f && !m_cpuThrottlingEnabled) {
        Logger::Log(LogLevel::INFO, "High CPU usage detected, enabling throttling");
        SetCPUThrottling(true);
    } else if (m_cpuUsage < 60.0f && m_cpuThrottlingEnabled) {
        Logger::Log(LogLevel::INFO, "CPU usage normalized, disabling throttling");
        SetCPUThrottling(false);
    }
}

void PerformanceManager::OptimizeGPUUsage() {
    GPUMonitor& gpu = GPUMonitor::Instance();

    if (m_dynamicResolutionEnabled) {
        float gpuUsage = gpu.GetGPUUsage();
        float currentScale = gpu.GetCurrentRenderScale();

        if (gpuUsage > 90.0f && currentScale > 0.5f) {
            gpu.AdjustRenderScale(-0.05f); // Reduce by 5%
            Logger::Log(LogLevel::DEBUG, "Reduced render scale due to high GPU usage");
        } else if (gpuUsage < 70.0f && currentScale < 1.0f) {
            gpu.AdjustRenderScale(0.05f); // Increase by 5%
            Logger::Log(LogLevel::DEBUG, "Increased render scale due to low GPU usage");
        }
    }
}

void PerformanceManager::OptimizeNetworkUsage() {
    NetworkMonitor& network = NetworkMonitor::Instance();

    uint32_t latency = network.GetLatency();
    uint32_t packetLoss = network.GetPacketLoss();

    if (latency > 150 || packetLoss > 5) { // High latency or packet loss
        network.SetCompressionEnabled(true);
        network.SetPacketBatching(true);
        network.SetAdaptiveBitrate(true);
        Logger::Log(LogLevel::INFO, "Enabled network optimizations due to poor network conditions");
    }
}

size_t PerformanceManager::GetUsedMemoryMB() const {
    return MemoryMonitor::Instance().GetUsedMemoryMB();
}

size_t PerformanceManager::GetAvailableMemoryMB() const {
    return MemoryMonitor::Instance().GetAvailableMemoryMB();
}

void PerformanceManager::RequestGarbageCollection() {
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    if (currentTime - m_lastGCRequest > m_gcInterval) {
        m_lastGCRequest = currentTime;

        // Force garbage collection in game engine
        // This would be implemented using RED4ext APIs
        Logger::Log(LogLevel::DEBUG, "Requested garbage collection");
    }
}

void PerformanceManager::SetMemoryThreshold(size_t thresholdMB) {
    m_memoryThresholdMB = thresholdMB;
    Logger::Log(LogLevel::INFO, "Set memory threshold to " + std::to_string(thresholdMB) + "MB");
}

uint32_t PerformanceManager::GetActiveThreadCount() const {
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwNumberOfProcessors;
#else
    return std::thread::hardware_concurrency();
#endif
}

void PerformanceManager::SetCPUThrottling(bool enabled) {
    m_cpuThrottlingEnabled = enabled;

    if (enabled) {
        // Implement CPU throttling logic
        Logger::Log(LogLevel::INFO, "CPU throttling enabled");
    } else {
        Logger::Log(LogLevel::INFO, "CPU throttling disabled");
    }
}

float PerformanceManager::GetGPUUsage() const {
    return GPUMonitor::Instance().GetGPUUsage();
}

float PerformanceManager::GetGPUMemoryUsage() const {
    return GPUMonitor::Instance().GetGPUMemoryUsage();
}

void PerformanceManager::SetDynamicResolution(bool enabled) {
    m_dynamicResolutionEnabled = enabled;
    GPUMonitor::Instance().SetDynamicResolution(enabled);
    Logger::Log(LogLevel::INFO, "Dynamic resolution " + std::string(enabled ? "enabled" : "disabled"));
}

void PerformanceManager::SetTargetFrameRate(float fps) {
    m_targetFrameRate = fps;
    Logger::Log(LogLevel::INFO, "Set target frame rate to " + std::to_string(fps) + " FPS");
}

void PerformanceManager::EnableAdaptiveQuality(bool enabled) {
    m_adaptiveQualityEnabled = enabled;
    Logger::Log(LogLevel::INFO, "Adaptive quality " + std::string(enabled ? "enabled" : "disabled"));
}

void PerformanceManager::SetQualityLevel(uint32_t level) {
    m_currentQualityLevel = std::clamp(level, 0u, 3u);

    // Apply quality settings to game systems
    // This would interface with the game's graphics settings
    Logger::Log(LogLevel::INFO, "Set quality level to " + std::to_string(m_currentQualityLevel));
}

void PerformanceManager::StartProfiling(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_profilingMutex);

    ProfilingEntry& entry = m_profilingData[name];
    entry.name = name;
    entry.startTime = std::chrono::high_resolution_clock::now();
    entry.active = true;
}

void PerformanceManager::EndProfiling(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_profilingMutex);

    auto it = m_profilingData.find(name);
    if (it != m_profilingData.end() && it->second.active) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - it->second.startTime);
        it->second.duration = duration.count() / 1000.0f; // Convert to milliseconds
        it->second.active = false;
    }
}

std::vector<std::pair<std::string, float>> PerformanceManager::GetProfilingResults() const {
    std::lock_guard<std::mutex> lock(m_profilingMutex);

    std::vector<std::pair<std::string, float>> results;
    for (const auto& pair : m_profilingData) {
        if (!pair.second.active) {
            results.emplace_back(pair.first, pair.second.duration);
        }
    }

    return results;
}

void PerformanceManager::RegisterResourcePool(const std::string& name, size_t maxSize) {
    std::lock_guard<std::mutex> lock(m_poolsMutex);

    auto pool = std::make_unique<ResourcePool>();
    pool->name = name;
    pool->maxSize = maxSize;
    pool->available.reserve(maxSize);

    m_resourcePools[name] = std::move(pool);
    Logger::Log(LogLevel::INFO, "Registered resource pool '" + name + "' with max size " + std::to_string(maxSize));
}

void* PerformanceManager::GetPooledResource(const std::string& poolName) {
    std::lock_guard<std::mutex> lock(m_poolsMutex);

    auto it = m_resourcePools.find(poolName);
    if (it != m_resourcePools.end()) {
        auto& pool = it->second;
        std::lock_guard<std::mutex> poolLock(pool->poolMutex);

        if (!pool->available.empty()) {
            void* resource = pool->available.back();
            pool->available.pop_back();
            pool->used.push_back(resource);
            return resource;
        }

        // Create new resource if pool not full
        if (pool->used.size() < pool->maxSize) {
            void* newResource = malloc(1024); // Placeholder allocation
            pool->used.push_back(newResource);
            return newResource;
        }
    }

    return nullptr;
}

void PerformanceManager::ReturnPooledResource(const std::string& poolName, void* resource) {
    std::lock_guard<std::mutex> lock(m_poolsMutex);

    auto it = m_resourcePools.find(poolName);
    if (it != m_resourcePools.end()) {
        auto& pool = it->second;
        std::lock_guard<std::mutex> poolLock(pool->poolMutex);

        auto usedIt = std::find(pool->used.begin(), pool->used.end(), resource);
        if (usedIt != pool->used.end()) {
            pool->used.erase(usedIt);
            pool->available.push_back(resource);
        }
    }
}

void PerformanceManager::MonitoringThread() {
    Logger::Log(LogLevel::INFO, "Performance monitoring thread started");

    while (m_monitoringActive) {
        UpdateSystemMetrics();

        // Run auto-optimizer
        AutoOptimizer::Instance().Update();

        // Sleep for 100ms
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Logger::Log(LogLevel::INFO, "Performance monitoring thread stopped");
}

// CPU Monitor implementation

CPUMonitor& CPUMonitor::Instance() {
    static CPUMonitor instance;
    return instance;
}

void CPUMonitor::Update() {
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastUpdate).count() < 1000) {
        return; // Update only once per second
    }
    m_lastUpdate = now;

#ifdef _WIN32
    // Windows-specific CPU usage calculation
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER idle, kernel, user;
        idle.LowPart = idleTime.dwLowDateTime;
        idle.HighPart = idleTime.dwHighDateTime;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;

        uint64_t totalTime = kernel.QuadPart + user.QuadPart;
        uint64_t idleTimeValue = idle.QuadPart;

        static uint64_t lastTotalTime = 0;
        static uint64_t lastIdleTime = 0;

        if (lastTotalTime != 0) {
            uint64_t totalDiff = totalTime - lastTotalTime;
            uint64_t idleDiff = idleTimeValue - lastIdleTime;

            if (totalDiff > 0) {
                m_overallUsage = ((float)(totalDiff - idleDiff) / totalDiff) * 100.0f;
            }
        }

        lastTotalTime = totalTime;
        lastIdleTime = idleTimeValue;
    }
#else
    // Linux/Unix implementation would go here
    m_overallUsage = 0.0f; // Placeholder
#endif

    std::lock_guard<std::mutex> lock(m_cpuMutex);
    // Update per-core information (simplified for now)
    m_coreInfo.clear();
    uint32_t coreCount = GetCoreCount();
    for (uint32_t i = 0; i < coreCount; ++i) {
        CPUInfo info;
        info.usage = m_overallUsage; // Simplified - would calculate per-core usage
        m_coreInfo.push_back(info);
    }
}

float CPUMonitor::GetCPUUsage() const {
    return m_overallUsage;
}

float CPUMonitor::GetPerCoreUsage(uint32_t core) const {
    std::lock_guard<std::mutex> lock(m_cpuMutex);
    if (core < m_coreInfo.size()) {
        return m_coreInfo[core].usage;
    }
    return 0.0f;
}

uint32_t CPUMonitor::GetCoreCount() const {
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwNumberOfProcessors;
#else
    return std::thread::hardware_concurrency();
#endif
}

void CPUMonitor::SetAffinityMask(uint64_t mask) {
#ifdef _WIN32
    HANDLE process = GetCurrentProcess();
    SetProcessAffinityMask(process, mask);
    Logger::Log(LogLevel::INFO, "Set CPU affinity mask to " + std::to_string(mask));
#endif
}

// Memory Monitor implementation

MemoryMonitor& MemoryMonitor::Instance() {
    static MemoryMonitor instance;
    return instance;
}

void MemoryMonitor::Update() {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        m_totalMemoryMB = memInfo.ullTotalPhys / (1024 * 1024);
        m_usedMemoryMB = (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / (1024 * 1024);
        m_usagePercentage = (float)memInfo.dwMemoryLoad;
    }

    // Get process memory usage
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        // Process-specific memory usage available in pmc.WorkingSetSize
    }
#else
    // Linux/Unix implementation would go here
    m_totalMemoryMB = 8192; // Placeholder
    m_usedMemoryMB = 4096;  // Placeholder
    m_usagePercentage = 50.0f; // Placeholder
#endif
}

size_t MemoryMonitor::GetTotalMemoryMB() const {
    return m_totalMemoryMB;
}

size_t MemoryMonitor::GetUsedMemoryMB() const {
    return m_usedMemoryMB;
}

size_t MemoryMonitor::GetAvailableMemoryMB() const {
    return m_totalMemoryMB - m_usedMemoryMB;
}

float MemoryMonitor::GetMemoryUsagePercentage() const {
    return m_usagePercentage;
}

void MemoryMonitor::TrimWorkingSet() {
#ifdef _WIN32
    HANDLE process = GetCurrentProcess();
    SetProcessWorkingSetSize(process, SIZE_T(-1), SIZE_T(-1));
    Logger::Log(LogLevel::DEBUG, "Trimmed working set");
#endif
}

void MemoryMonitor::CompactHeaps() {
#ifdef _WIN32
    HANDLE heaps[256];
    DWORD numHeaps = GetProcessHeaps(256, heaps);
    for (DWORD i = 0; i < numHeaps; ++i) {
        HeapCompact(heaps[i], 0);
    }
    Logger::Log(LogLevel::DEBUG, "Compacted " + std::to_string(numHeaps) + " heaps");
#endif
}

void MemoryMonitor::FlushUnusedPages() {
    // Implementation would depend on the platform and available APIs
    Logger::Log(LogLevel::DEBUG, "Flushed unused memory pages");
}

// GPU Monitor implementation

GPUMonitor& GPUMonitor::Instance() {
    static GPUMonitor instance;
    return instance;
}

void GPUMonitor::Update() {
    // GPU monitoring would require platform-specific APIs
    // For now, use placeholder values
    m_gpuUsage = 50.0f;      // Placeholder
    m_gpuMemoryUsage = 60.0f; // Placeholder
    m_gpuTemperature = 70.0f; // Placeholder
    m_gpuClockSpeed = 1500;   // Placeholder
}

float GPUMonitor::GetGPUUsage() const {
    return m_gpuUsage;
}

float GPUMonitor::GetGPUMemoryUsage() const {
    return m_gpuMemoryUsage;
}

float GPUMonitor::GetGPUTemperature() const {
    return m_gpuTemperature;
}

uint32_t GPUMonitor::GetGPUClockSpeed() const {
    return m_gpuClockSpeed;
}

void GPUMonitor::SetDynamicResolution(bool enabled) {
    std::lock_guard<std::mutex> lock(m_gpuMutex);
    m_dynamicResolutionEnabled = enabled;
}

float GPUMonitor::GetCurrentRenderScale() const {
    std::lock_guard<std::mutex> lock(m_gpuMutex);
    return m_currentRenderScale;
}

void GPUMonitor::AdjustRenderScale(float delta) {
    std::lock_guard<std::mutex> lock(m_gpuMutex);
    m_currentRenderScale = std::clamp(m_currentRenderScale + delta, 0.5f, 1.0f);
}

// Network Monitor implementation

NetworkMonitor& NetworkMonitor::Instance() {
    static NetworkMonitor instance;
    return instance;
}

void NetworkMonitor::Update() {
    std::lock_guard<std::mutex> lock(m_networkMutex);

    // Calculate packet loss percentage
    if (m_totalPackets > 0) {
        m_packetLoss = (m_packetsLost * 100) / m_totalPackets;
    }

    // Calculate average latency
    if (!m_latencyHistory.empty()) {
        uint32_t sum = std::accumulate(m_latencyHistory.begin(), m_latencyHistory.end(), 0u);
        m_latency = sum / m_latencyHistory.size();
    }

    // Calculate bandwidth usage (placeholder)
    m_bandwidthUsage = 1.5f; // MB/s placeholder
}

uint32_t NetworkMonitor::GetLatency() const {
    return m_latency;
}

float NetworkMonitor::GetBandwidthUsage() const {
    return m_bandwidthUsage;
}

uint32_t NetworkMonitor::GetPacketLoss() const {
    return m_packetLoss;
}

uint32_t NetworkMonitor::GetPacketsPerSecond() const {
    return m_packetsPerSecond;
}

void NetworkMonitor::SetCompressionEnabled(bool enabled) {
    m_compressionEnabled = enabled;
}

void NetworkMonitor::SetPacketBatching(bool enabled) {
    m_packetBatchingEnabled = enabled;
}

void NetworkMonitor::SetAdaptiveBitrate(bool enabled) {
    m_adaptiveBitrateEnabled = enabled;
}

void NetworkMonitor::RecordPacketSent(size_t size) {
    m_bytesSent += size;
    m_totalPackets++;
}

void NetworkMonitor::RecordPacketReceived(size_t size) {
    m_bytesReceived += size;
}

void NetworkMonitor::RecordPacketLoss() {
    m_packetsLost++;
}

void NetworkMonitor::RecordLatencyMeasurement(uint32_t latencyMs) {
    std::lock_guard<std::mutex> lock(m_networkMutex);
    m_latencyHistory.push_back(latencyMs);
    if (m_latencyHistory.size() > 10) {
        m_latencyHistory.pop_front();
    }
}

// Auto Optimizer implementation

AutoOptimizer& AutoOptimizer::Instance() {
    static AutoOptimizer instance;
    return instance;
}

void AutoOptimizer::SetEnabled(bool enabled) {
    m_enabled = enabled;
    Logger::Log(LogLevel::INFO, "Auto optimizer " + std::string(enabled ? "enabled" : "disabled"));
}

void AutoOptimizer::Update() {
    if (!m_enabled || !ShouldOptimize()) {
        return;
    }

    PerformanceManager& perf = PerformanceManager::Instance();
    float currentFPS = perf.GetCurrentFPS();

    switch (m_optimizationMode) {
    case 0: // Performance mode
        OptimizeForFramerate();
        break;
    case 1: // Balanced mode
        if (currentFPS < m_targetFPS * 0.9f) {
            OptimizeForFramerate();
        } else if (perf.GetUsedMemoryMB() > 2048) {
            OptimizeForMemory();
        }
        break;
    case 2: // Quality mode
        OptimizeForQuality();
        break;
    }

    m_lastOptimizationTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void AutoOptimizer::OptimizeForFramerate() {
    PerformanceManager& perf = PerformanceManager::Instance();

    // Lower quality settings
    if (perf.GetCurrentQualityLevel() > 0) {
        perf.SetQualityLevel(perf.GetCurrentQualityLevel() - 1);
    }

    // Enable dynamic resolution
    perf.SetDynamicResolution(true);

    Logger::Log(LogLevel::INFO, "Applied framerate optimizations");
}

void AutoOptimizer::OptimizeForQuality() {
    PerformanceManager& perf = PerformanceManager::Instance();

    // Increase quality if performance allows
    if (perf.GetCurrentFPS() > m_targetFPS * 1.2f && perf.GetCurrentQualityLevel() < 3) {
        perf.SetQualityLevel(perf.GetCurrentQualityLevel() + 1);
    }

    Logger::Log(LogLevel::INFO, "Applied quality optimizations");
}

void AutoOptimizer::OptimizeForNetwork() {
    NetworkMonitor::Instance().SetCompressionEnabled(true);
    NetworkMonitor::Instance().SetPacketBatching(true);
    NetworkMonitor::Instance().SetAdaptiveBitrate(true);

    Logger::Log(LogLevel::INFO, "Applied network optimizations");
}

void AutoOptimizer::OptimizeForMemory() {
    PerformanceManager::Instance().RequestGarbageCollection();
    MemoryMonitor::Instance().TrimWorkingSet();
    MemoryMonitor::Instance().CompactHeaps();

    Logger::Log(LogLevel::INFO, "Applied memory optimizations");
}

void AutoOptimizer::SetOptimizationTarget(float targetFPS) {
    m_targetFPS = targetFPS;
    Logger::Log(LogLevel::INFO, "Set optimization target to " + std::to_string(targetFPS) + " FPS");
}

void AutoOptimizer::SetOptimizationMode(uint32_t mode) {
    m_optimizationMode = std::clamp(mode, 0u, 2u);
    const char* modeNames[] = {"Performance", "Balanced", "Quality"};
    Logger::Log(LogLevel::INFO, "Set optimization mode to " + std::string(modeNames[m_optimizationMode]));
}

bool AutoOptimizer::ShouldOptimize() const {
    float currentTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    return (currentTime - m_lastOptimizationTime) > m_optimizationCooldown;
}

} // namespace CoopNet
#include "SystemManager.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include "Logger.hpp"
#include <algorithm>
#include <fstream>
#include <chrono>
#include <unordered_set>
#include <functional>
#include <thread>

namespace CoopNet {

// ErrorManagerSystem implementation
bool ErrorManagerSystem::Initialize() {
    m_state = SystemState::Initializing;
    m_startTime = std::chrono::steady_clock::now();

    try {
        bool success = ErrorManager::Instance().Initialize();
        m_state = success ? SystemState::Running : SystemState::Error;
        if (!success) {
            m_lastError = "Failed to initialize ErrorManager";
        }
        return success;
    } catch (const std::exception& ex) {
        m_lastError = "Exception during ErrorManager initialization: " + std::string(ex.what());
        m_state = SystemState::Error;
        return false;
    }
}

void ErrorManagerSystem::Shutdown() {
    if (m_state == SystemState::Running || m_state == SystemState::Paused) {
        ErrorManager::Instance().Shutdown();
        m_state = SystemState::Stopped;
    }
}

void ErrorManagerSystem::Update() {
    if (m_state == SystemState::Running) {
        // ErrorManager handles its own internal updates
        // No explicit update needed
    }
}

SystemState ErrorManagerSystem::GetState() const {
    return m_state;
}

SystemMetrics ErrorManagerSystem::GetMetrics() const {
    SystemMetrics metrics = {};

    if (m_state == SystemState::Running) {
        auto stats = ErrorManager::Instance().GetStatistics();
        metrics.requestCount = stats.totalErrors;
        metrics.errorCount = stats.totalCriticalErrors + stats.totalFatalErrors;
        metrics.memoryUsage = 1024 * 1024; // Estimate 1MB for error manager
        metrics.cpuUsage = 0.1f; // Low CPU usage
        metrics.avgResponseTime = std::chrono::microseconds(10);
        metrics.maxResponseTime = std::chrono::microseconds(100);
        metrics.lastUpdate = std::chrono::steady_clock::now();

        metrics.customCounters["total_errors"] = stats.totalErrors;
        metrics.customCounters["warnings"] = stats.totalWarnings;
        metrics.customCounters["critical_errors"] = stats.totalCriticalErrors;
        metrics.customGauges["errors_per_minute"] = stats.errorsPerMinute;
    }

    return metrics;
}

bool ErrorManagerSystem::IsHealthy() const {
    return m_state == SystemState::Running;
}

std::string ErrorManagerSystem::GetLastError() const {
    return m_lastError;
}

void ErrorManagerSystem::Reset() {
    if (m_state != SystemState::Uninitialized) {
        Shutdown();
        m_state = SystemState::Uninitialized;
        m_lastError.clear();
    }
}

// ConfigurationManagerSystem implementation
bool ConfigurationManagerSystem::Initialize() {
    m_state = SystemState::Initializing;
    m_startTime = std::chrono::steady_clock::now();

    try {
        bool success = ConfigurationManager::Instance().Initialize();
        m_state = success ? SystemState::Running : SystemState::Error;
        if (!success) {
            m_lastError = "Failed to initialize ConfigurationManager";
        }
        return success;
    } catch (const std::exception& ex) {
        m_lastError = "Exception during ConfigurationManager initialization: " + std::string(ex.what());
        m_state = SystemState::Error;
        return false;
    }
}

void ConfigurationManagerSystem::Shutdown() {
    if (m_state == SystemState::Running || m_state == SystemState::Paused) {
        ConfigurationManager::Instance().Shutdown();
        m_state = SystemState::Stopped;
    }
}

void ConfigurationManagerSystem::Update() {
    if (m_state == SystemState::Running) {
        // Configuration manager handles file watching internally
    }
}

SystemState ConfigurationManagerSystem::GetState() const {
    return m_state;
}

SystemMetrics ConfigurationManagerSystem::GetMetrics() const {
    SystemMetrics metrics = {};

    if (m_state == SystemState::Running) {
        metrics.memoryUsage = 2 * 1024 * 1024; // Estimate 2MB for config data
        metrics.cpuUsage = 0.05f; // Very low CPU usage
        metrics.avgResponseTime = std::chrono::microseconds(5);
        metrics.maxResponseTime = std::chrono::microseconds(50);
        metrics.lastUpdate = std::chrono::steady_clock::now();

        // Get configuration-specific metrics if available
        auto& configMgr = ConfigurationManager::Instance();
        metrics.customCounters["active_profiles"] = 1; // Default profile
        metrics.customCounters["loaded_configs"] = 1;
    }

    return metrics;
}

bool ConfigurationManagerSystem::IsHealthy() const {
    return m_state == SystemState::Running;
}

std::string ConfigurationManagerSystem::GetLastError() const {
    return m_lastError;
}

void ConfigurationManagerSystem::Reset() {
    if (m_state != SystemState::Uninitialized) {
        Shutdown();
        m_state = SystemState::Uninitialized;
        m_lastError.clear();
    }
}

// DatabaseManagerSystem implementation
bool DatabaseManagerSystem::Initialize() {
    m_state = SystemState::Initializing;
    m_startTime = std::chrono::steady_clock::now();

    try {
        DatabaseConfig config;
        config.type = DatabaseType::SQLite;
        config.database = "coopnet.db";
        config.enableConnectionPooling = true;
        config.maxConnections = 10;

        bool success = DatabaseManager::Instance().Initialize(config);
        m_state = success ? SystemState::Running : SystemState::Error;
        if (!success) {
            m_lastError = "Failed to initialize DatabaseManager";
        }
        return success;
    } catch (const std::exception& ex) {
        m_lastError = "Exception during DatabaseManager initialization: " + std::string(ex.what());
        m_state = SystemState::Error;
        return false;
    }
}

void DatabaseManagerSystem::Shutdown() {
    if (m_state == SystemState::Running || m_state == SystemState::Paused) {
        DatabaseManager::Instance().Shutdown();
        m_state = SystemState::Stopped;
    }
}

void DatabaseManagerSystem::Update() {
    if (m_state == SystemState::Running) {
        // Database manager handles connection maintenance internally
    }
}

SystemState DatabaseManagerSystem::GetState() const {
    return m_state;
}

SystemMetrics DatabaseManagerSystem::GetMetrics() const {
    SystemMetrics metrics = {};

    if (m_state == SystemState::Running) {
        auto& dbMgr = DatabaseManager::Instance();
        auto stats = dbMgr.GetQueryStatistics();

        metrics.memoryUsage = 5 * 1024 * 1024; // Estimate 5MB for database operations
        metrics.cpuUsage = 0.2f;
        metrics.avgResponseTime = std::chrono::microseconds(1000); // 1ms average
        metrics.maxResponseTime = std::chrono::microseconds(10000); // 10ms max
        metrics.lastUpdate = std::chrono::steady_clock::now();

        uint64_t totalQueries = 0;
        for (const auto& [type, count] : stats) {
            totalQueries += count;
            metrics.customCounters[type + "_queries"] = count;
        }
        metrics.requestCount = totalQueries;

        metrics.customCounters["active_connections"] = dbMgr.IsConnected() ? 1 : 0;
    }

    return metrics;
}

bool DatabaseManagerSystem::IsHealthy() const {
    if (m_state != SystemState::Running) return false;
    return DatabaseManager::Instance().IsConnected();
}

std::string DatabaseManagerSystem::GetLastError() const {
    return m_lastError;
}

void DatabaseManagerSystem::Reset() {
    if (m_state != SystemState::Uninitialized) {
        Shutdown();
        m_state = SystemState::Uninitialized;
        m_lastError.clear();
    }
}

// ContentManagerSystem implementation
bool ContentManagerSystem::Initialize() {
    m_state = SystemState::Initializing;
    m_startTime = std::chrono::steady_clock::now();

    try {
        bool success = ContentManager::Instance().Initialize("content/");
        m_state = success ? SystemState::Running : SystemState::Error;
        if (!success) {
            m_lastError = "Failed to initialize ContentManager";
        }
        return success;
    } catch (const std::exception& ex) {
        m_lastError = "Exception during ContentManager initialization: " + std::string(ex.what());
        m_state = SystemState::Error;
        return false;
    }
}

void ContentManagerSystem::Shutdown() {
    if (m_state == SystemState::Running || m_state == SystemState::Paused) {
        ContentManager::Instance().Shutdown();
        m_state = SystemState::Stopped;
    }
}

void ContentManagerSystem::Update() {
    if (m_state == SystemState::Running) {
        // Content manager handles background operations internally
    }
}

SystemState ContentManagerSystem::GetState() const {
    return m_state;
}

SystemMetrics ContentManagerSystem::GetMetrics() const {
    SystemMetrics metrics = {};

    if (m_state == SystemState::Running) {
        auto& contentMgr = ContentManager::Instance();
        auto loadedContent = contentMgr.GetLoadedContent();
        auto allContent = contentMgr.GetAllContent();

        metrics.memoryUsage = contentMgr.GetCacheUsage() + (10 * 1024 * 1024); // Cache + overhead
        metrics.cpuUsage = 0.3f;
        metrics.avgResponseTime = std::chrono::microseconds(500);
        metrics.maxResponseTime = std::chrono::microseconds(5000);
        metrics.lastUpdate = std::chrono::steady_clock::now();

        metrics.customCounters["loaded_content"] = loadedContent.size();
        metrics.customCounters["total_content"] = allContent.size();
        metrics.customCounters["cache_usage"] = contentMgr.GetCacheUsage();
        metrics.customCounters["cache_size"] = contentMgr.GetCacheSize();

        metrics.customGauges["cache_usage_percent"] =
            static_cast<float>(contentMgr.GetCacheUsage()) / contentMgr.GetCacheSize() * 100.0f;
    }

    return metrics;
}

bool ContentManagerSystem::IsHealthy() const {
    return m_state == SystemState::Running;
}

std::string ContentManagerSystem::GetLastError() const {
    return m_lastError;
}

void ContentManagerSystem::Reset() {
    if (m_state != SystemState::Uninitialized) {
        Shutdown();
        m_state = SystemState::Uninitialized;
        m_lastError.clear();
    }
}

// PerformanceMonitorSystem implementation
bool PerformanceMonitorSystem::Initialize() {
    m_state = SystemState::Initializing;
    m_startTime = std::chrono::steady_clock::now();

    try {
        bool success = PerformanceMonitor::Instance().Initialize();
        m_state = success ? SystemState::Running : SystemState::Error;
        if (!success) {
            m_lastError = "Failed to initialize PerformanceMonitor";
        }
        return success;
    } catch (const std::exception& ex) {
        m_lastError = "Exception during PerformanceMonitor initialization: " + std::string(ex.what());
        m_state = SystemState::Error;
        return false;
    }
}

void PerformanceMonitorSystem::Shutdown() {
    if (m_state == SystemState::Running || m_state == SystemState::Paused) {
        PerformanceMonitor::Instance().Shutdown();
        m_state = SystemState::Stopped;
    }
}

void PerformanceMonitorSystem::Update() {
    if (m_state == SystemState::Running) {
        // Calculate deltaTime since last update
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime);
        float deltaTime = elapsed.count() / 1000.0f;
        PerformanceMonitor::Instance().Update(deltaTime);
    }
}

SystemState PerformanceMonitorSystem::GetState() const {
    return m_state;
}

SystemMetrics PerformanceMonitorSystem::GetMetrics() const {
    SystemMetrics metrics = {};

    if (m_state == SystemState::Running) {
        auto& perfMon = PerformanceMonitor::Instance();
        auto perfStats = perfMon.GetStatistics();

        metrics.memoryUsage = 3 * 1024 * 1024; // Estimate 3MB for performance monitoring
        metrics.cpuUsage = 0.5f; // Moderate CPU usage for monitoring
        metrics.avgResponseTime = std::chrono::microseconds(100);
        metrics.maxResponseTime = std::chrono::microseconds(1000);
        metrics.lastUpdate = std::chrono::steady_clock::now();

        // Convert performance stats to system metrics
        metrics.customGauges["system_cpu_usage"] = perfStats.averageCPUUsage;
        metrics.customGauges["system_memory_usage"] = perfStats.averageMemoryUsage;
        metrics.customGauges["fps"] = perfStats.averageFPS;
        metrics.customGauges["frame_time"] = perfStats.averageFrameTime;

        metrics.customCounters["alerts_triggered"] = perfStats.totalAlerts;
    }

    return metrics;
}

bool PerformanceMonitorSystem::IsHealthy() const {
    if (m_state != SystemState::Running) return false;
    return m_state == SystemState::Running;
}

std::string PerformanceMonitorSystem::GetLastError() const {
    return m_lastError;
}

void PerformanceMonitorSystem::Reset() {
    if (m_state != SystemState::Uninitialized) {
        Shutdown();
        m_state = SystemState::Uninitialized;
        m_lastError.clear();
    }
}

// NetworkOptimizerSystem implementation
bool NetworkOptimizerSystem::Initialize() {
    m_state = SystemState::Initializing;
    m_startTime = std::chrono::steady_clock::now();

    try {
        bool success = NetworkOptimizer::Instance().Initialize();
        m_state = success ? SystemState::Running : SystemState::Error;
        if (!success) {
            m_lastError = "Failed to initialize NetworkOptimizer";
        }
        return success;
    } catch (const std::exception& ex) {
        m_lastError = "Exception during NetworkOptimizer initialization: " + std::string(ex.what());
        m_state = SystemState::Error;
        return false;
    }
}

void NetworkOptimizerSystem::Shutdown() {
    if (m_state == SystemState::Running || m_state == SystemState::Paused) {
        NetworkOptimizer::Instance().Shutdown();
        m_state = SystemState::Stopped;
    }
}

void NetworkOptimizerSystem::Update() {
    if (m_state == SystemState::Running) {
        // Calculate deltaTime since last update
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime);
        float deltaTime = elapsed.count() / 1000.0f;
        NetworkOptimizer::Instance().Update(deltaTime);
    }
}

SystemState NetworkOptimizerSystem::GetState() const {
    return m_state;
}

SystemMetrics NetworkOptimizerSystem::GetMetrics() const {
    SystemMetrics metrics = {};

    if (m_state == SystemState::Running) {
        auto& netOpt = NetworkOptimizer::Instance();
        auto netMetrics = netOpt.GetMetrics();

        metrics.memoryUsage = 4 * 1024 * 1024; // Estimate 4MB for network buffers
        metrics.cpuUsage = 0.4f; // Moderate CPU for compression/optimization
        metrics.avgResponseTime = std::chrono::microseconds(static_cast<long long>(netMetrics.averageLatency * 1000));
        metrics.maxResponseTime = std::chrono::microseconds(static_cast<long long>(netMetrics.maxLatency * 1000));
        metrics.lastUpdate = std::chrono::steady_clock::now();

        metrics.requestCount = netMetrics.packetsSent + netMetrics.packetsReceived;
        metrics.errorCount = netMetrics.packetsLost;

        metrics.customCounters["bytes_sent"] = netMetrics.bytesSent;
        metrics.customCounters["bytes_received"] = netMetrics.bytesReceived;
        metrics.customCounters["packets_sent"] = netMetrics.packetsSent;
        metrics.customCounters["packets_received"] = netMetrics.packetsReceived;
        metrics.customCounters["packets_lost"] = netMetrics.packetsLost;

        metrics.customGauges["bandwidth_utilization"] = netOpt.GetBandwidthUtilization();
        metrics.customGauges["compression_ratio"] = netOpt.GetCompressionRatio();
        metrics.customGauges["latency"] = netMetrics.averageLatency;
    }

    return metrics;
}

bool NetworkOptimizerSystem::IsHealthy() const {
    if (m_state != SystemState::Running) return false;
    return NetworkOptimizer::Instance().IsAdaptationEnabled();
}

std::string NetworkOptimizerSystem::GetLastError() const {
    return m_lastError;
}

void NetworkOptimizerSystem::Reset() {
    if (m_state != SystemState::Uninitialized) {
        Shutdown();
        m_state = SystemState::Uninitialized;
        m_lastError.clear();
    }
}

// VoiceCommunicationSystem implementation
bool VoiceCommunicationSystem::Initialize() {
    m_state = SystemState::Initializing;
    m_startTime = std::chrono::steady_clock::now();

    try {
        VoiceConfig config;
        config.quality = VoiceQuality::High;
        config.spatialMode = SpatialAudioMode::Advanced;
        config.vadThreshold = 0.1f;
        config.enableEchoCancellation = true;
        config.enableNoiseSupression = true;

        bool success = VoiceCommunicationCore::Instance().Initialize(config);
        m_state = success ? SystemState::Running : SystemState::Error;
        if (!success) {
            m_lastError = "Failed to initialize VoiceCommunicationCore";
        }
        return success;
    } catch (const std::exception& ex) {
        m_lastError = "Exception during VoiceCommunicationCore initialization: " + std::string(ex.what());
        m_state = SystemState::Error;
        return false;
    }
}

void VoiceCommunicationSystem::Shutdown() {
    if (m_state == SystemState::Running || m_state == SystemState::Paused) {
        VoiceCommunicationCore::Instance().Shutdown();
        m_state = SystemState::Stopped;
    }
}

void VoiceCommunicationSystem::Update() {
    if (m_state == SystemState::Running) {
        // Calculate deltaTime since last update
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime);
        float deltaTime = elapsed.count() / 1000.0f;
        VoiceCommunicationCore::Instance().Update(deltaTime);
    }
}

SystemState VoiceCommunicationSystem::GetState() const {
    return m_state;
}

SystemMetrics VoiceCommunicationSystem::GetMetrics() const {
    SystemMetrics metrics = {};

    if (m_state == SystemState::Running) {
        auto& voiceCore = VoiceCommunicationCore::Instance();
        auto voiceStats = voiceCore.GetStatistics();

        metrics.memoryUsage = 8 * 1024 * 1024; // Estimate 8MB for audio buffers
        metrics.cpuUsage = 0.6f; // Higher CPU for audio processing
        metrics.avgResponseTime = std::chrono::microseconds(5000); // 5ms for audio processing
        metrics.maxResponseTime = std::chrono::microseconds(20000); // 20ms max
        metrics.lastUpdate = std::chrono::steady_clock::now();

        metrics.customCounters["active_channels"] = voiceStats.activeChannels;
        metrics.customCounters["active_speakers"] = voiceStats.activeSpeakers;
        metrics.customCounters["packets_transmitted"] = voiceStats.packetsTransmitted;
        metrics.customCounters["packets_received"] = voiceStats.packetsReceived;

        metrics.customGauges["compression_ratio"] = voiceStats.compressionRatio;
        metrics.customGauges["latency"] = voiceStats.averageLatency;
        metrics.customGauges["packet_loss"] = voiceStats.packetLossRate;
    }

    return metrics;
}

bool VoiceCommunicationSystem::IsHealthy() const {
    if (m_state != SystemState::Running) return false;
    return m_state == SystemState::Running;
}

std::string VoiceCommunicationSystem::GetLastError() const {
    return m_lastError;
}

void VoiceCommunicationSystem::Reset() {
    if (m_state != SystemState::Uninitialized) {
        Shutdown();
        m_state = SystemState::Uninitialized;
        m_lastError.clear();
    }
}

// SystemManager implementation
SystemManager& SystemManager::Instance() {
    static SystemManager instance;
    return instance;
}

bool SystemManager::Initialize(const InitializationConfig& config) {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    if (m_initialized) {
        // spdlog::warn("[SystemManager] Already initialized");
        return true;
    }

    m_config = config;
    m_managerState = SystemState::Initializing;
    m_startTime = std::chrono::steady_clock::now();

    spdlog::info("[SystemManager] Starting system initialization...");

    // Register all systems with their dependencies
    RegisterAllSystemsInternal();

    // Validate dependencies before initialization
    if (!ValidateDependencies()) {
        spdlog::error("[SystemManager] System dependency validation failed");
        m_managerState = SystemState::Error;
        return false;
    }

    // Initialize systems in dependency order
    if (!InitializeSystemsInOrder()) {
        spdlog::error("[SystemManager] Failed to initialize systems");
        m_managerState = SystemState::Error;
        return false;
    }

    // Start monitoring threads
    if (m_config.enableWatchdog) {
        EnableWatchdog(true);
    }

    if (m_healthMonitoringEnabled) {
        m_shouldStop = false;
        m_healthMonitoringThread = std::thread(&SystemManager::HealthMonitoringLoop, this);
    }

    m_initialized = true;
    m_managerState = SystemState::Running;

    spdlog::info("[SystemManager] All systems initialized successfully");
    NotifyAllSystemsReady();

    return true;
}

void SystemManager::Shutdown() {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    if (!m_initialized) return;

    spdlog::info("[SystemManager] Starting system shutdown...");
    m_managerState = SystemState::Stopping;

    // Stop monitoring threads
    m_shouldStop = true;

    if (m_watchdogEnabled && m_watchdogThread.joinable()) {
        m_watchdogThread.join();
    }

    if (m_healthMonitoringThread.joinable()) {
        m_healthMonitoringThread.join();
    }

    // Shutdown systems in reverse dependency order
    ShutdownSystemsInOrder();

    // Clear all data
    m_systems.clear();
    m_systemInfo.clear();
    m_systemMetrics.clear();
    m_systemErrors.clear();
    m_systemTimeouts.clear();

    m_initialized = false;
    m_managerState = SystemState::Stopped;

    spdlog::info("[SystemManager] System shutdown completed");
}

void SystemManager::RegisterAllSystemsInternal() {
    // Register ErrorManager (highest priority - no dependencies)
    {
        SystemInfo info;
        info.type = SystemType::ErrorManager;
        info.name = "ErrorManager";
        info.version = "1.0.0";
        info.state = SystemState::Uninitialized;
        info.priority = SystemPriority::Critical;
        info.isEssential = true;
        info.supportsHotReload = false;
        info.requiresCleanShutdown = true;

        RegisterSystem(SystemType::ErrorManager, std::make_shared<ErrorManagerSystem>(), info);
    }

    // Register ConfigurationManager (depends on ErrorManager)
    {
        SystemInfo info;
        info.type = SystemType::ConfigurationManager;
        info.name = "ConfigurationManager";
        info.version = "1.0.0";
        info.state = SystemState::Uninitialized;
        info.priority = SystemPriority::Critical;
        info.isEssential = true;
        info.supportsHotReload = true;
        info.requiresCleanShutdown = true;

        SystemDependency errorDep;
        errorDep.dependsOn = SystemType::ErrorManager;
        errorDep.name = "ErrorManager";
        errorDep.isRequired = true;
        errorDep.isCircular = false;
        info.dependencies.push_back(errorDep);

        RegisterSystem(SystemType::ConfigurationManager, std::make_shared<ConfigurationManagerSystem>(), info);
    }

    // Register DatabaseManager (depends on ErrorManager and ConfigurationManager)
    if (m_config.enableDatabaseIntegration) {
        SystemInfo info;
        info.type = SystemType::DatabaseManager;
        info.name = "DatabaseManager";
        info.version = "1.0.0";
        info.state = SystemState::Uninitialized;
        info.priority = SystemPriority::High;
        info.isEssential = false;
        info.supportsHotReload = true;
        info.requiresCleanShutdown = true;

        SystemDependency errorDep;
        errorDep.dependsOn = SystemType::ErrorManager;
        errorDep.name = "ErrorManager";
        errorDep.isRequired = true;
        info.dependencies.push_back(errorDep);

        SystemDependency configDep;
        configDep.dependsOn = SystemType::ConfigurationManager;
        configDep.name = "ConfigurationManager";
        configDep.isRequired = true;
        info.dependencies.push_back(configDep);

        RegisterSystem(SystemType::DatabaseManager, std::make_shared<DatabaseManagerSystem>(), info);
    }

    // Register ContentManager (depends on ErrorManager and ConfigurationManager)
    if (m_config.enableContentStreaming) {
        SystemInfo info;
        info.type = SystemType::ContentManager;
        info.name = "ContentManager";
        info.version = "1.0.0";
        info.state = SystemState::Uninitialized;
        info.priority = SystemPriority::Medium;
        info.isEssential = false;
        info.supportsHotReload = true;
        info.requiresCleanShutdown = true;

        SystemDependency errorDep;
        errorDep.dependsOn = SystemType::ErrorManager;
        errorDep.name = "ErrorManager";
        errorDep.isRequired = true;
        info.dependencies.push_back(errorDep);

        SystemDependency configDep;
        configDep.dependsOn = SystemType::ConfigurationManager;
        configDep.name = "ConfigurationManager";
        configDep.isRequired = true;
        info.dependencies.push_back(configDep);

        RegisterSystem(SystemType::ContentManager, std::make_shared<ContentManagerSystem>(), info);
    }

    // Register PerformanceMonitor (depends on ErrorManager and ConfigurationManager)
    if (m_config.enablePerformanceMonitoring) {
        SystemInfo info;
        info.type = SystemType::PerformanceMonitor;
        info.name = "PerformanceMonitor";
        info.version = "1.0.0";
        info.state = SystemState::Uninitialized;
        info.priority = SystemPriority::High;
        info.isEssential = false;
        info.supportsHotReload = true;
        info.requiresCleanShutdown = false;

        SystemDependency errorDep;
        errorDep.dependsOn = SystemType::ErrorManager;
        errorDep.name = "ErrorManager";
        errorDep.isRequired = true;
        info.dependencies.push_back(errorDep);

        SystemDependency configDep;
        configDep.dependsOn = SystemType::ConfigurationManager;
        configDep.name = "ConfigurationManager";
        configDep.isRequired = true;
        info.dependencies.push_back(configDep);

        RegisterSystem(SystemType::PerformanceMonitor, std::make_shared<PerformanceMonitorSystem>(), info);
    }

    // Register NetworkOptimizer (depends on ErrorManager, ConfigurationManager, and PerformanceMonitor)
    if (m_config.enableNetworkOptimization) {
        SystemInfo info;
        info.type = SystemType::NetworkOptimizer;
        info.name = "NetworkOptimizer";
        info.version = "1.0.0";
        info.state = SystemState::Uninitialized;
        info.priority = SystemPriority::Medium;
        info.isEssential = false;
        info.supportsHotReload = true;
        info.requiresCleanShutdown = true;

        SystemDependency errorDep;
        errorDep.dependsOn = SystemType::ErrorManager;
        errorDep.name = "ErrorManager";
        errorDep.isRequired = true;
        info.dependencies.push_back(errorDep);

        SystemDependency configDep;
        configDep.dependsOn = SystemType::ConfigurationManager;
        configDep.name = "ConfigurationManager";
        configDep.isRequired = true;
        info.dependencies.push_back(configDep);

        if (m_config.enablePerformanceMonitoring) {
            SystemDependency perfDep;
            perfDep.dependsOn = SystemType::PerformanceMonitor;
            perfDep.name = "PerformanceMonitor";
            perfDep.isRequired = false; // Soft dependency
            info.dependencies.push_back(perfDep);
        }

        RegisterSystem(SystemType::NetworkOptimizer, std::make_shared<NetworkOptimizerSystem>(), info);
    }

    // Register VoiceCommunicationCore (depends on ErrorManager, ConfigurationManager, and NetworkOptimizer)
    if (m_config.enableVoiceChat) {
        SystemInfo info;
        info.type = SystemType::VoiceCommunicationCore;
        info.name = "VoiceCommunicationCore";
        info.version = "1.0.0";
        info.state = SystemState::Uninitialized;
        info.priority = SystemPriority::Low;
        info.isEssential = false;
        info.supportsHotReload = true;
        info.requiresCleanShutdown = true;

        SystemDependency errorDep;
        errorDep.dependsOn = SystemType::ErrorManager;
        errorDep.name = "ErrorManager";
        errorDep.isRequired = true;
        info.dependencies.push_back(errorDep);

        SystemDependency configDep;
        configDep.dependsOn = SystemType::ConfigurationManager;
        configDep.name = "ConfigurationManager";
        configDep.isRequired = true;
        info.dependencies.push_back(configDep);

        if (m_config.enableNetworkOptimization) {
            SystemDependency netDep;
            netDep.dependsOn = SystemType::NetworkOptimizer;
            netDep.name = "NetworkOptimizer";
            netDep.isRequired = false; // Soft dependency
            info.dependencies.push_back(netDep);
        }

        RegisterSystem(SystemType::VoiceCommunicationCore, std::make_shared<VoiceCommunicationSystem>(), info);
    }
}

bool SystemManager::RegisterSystem(SystemType type, std::shared_ptr<ISystem> system, const SystemInfo& info) {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    if (m_systems.count(type)) {
        // spdlog::warn("[SystemManager] System already registered: {}", SystemUtils::GetSystemTypeName(type));
        return false;
    }

    m_systems[type] = system;
    m_systemInfo[type] = info;
    m_systemInfo[type].type = type; // Ensure type consistency
    m_restartCounts[type] = 0;

    spdlog::info("[SystemManager] Registered system: {} ({})", info.name, SystemUtils::GetSystemTypeName(type));
    return true;
}

bool SystemManager::InitializeSystemsInOrder() {
    auto initOrder = GetInitializationOrder();

    for (SystemType type : initOrder) {
        if (!InitializeSystem(type)) {
            spdlog::error("[SystemManager] Failed to initialize system: {}", SystemUtils::GetSystemTypeName(type));

            // Check if this is an essential system
            if (IsSystemEssential(type)) {
                spdlog::critical("[SystemManager] Essential system failed, aborting initialization");
                return false;
            } else {
                spdlog::warn("[SystemManager] Non-essential system failed, continuing");
                continue;
            }
        }
    }

    return true;
}

bool SystemManager::InitializeSystem(SystemType type) {
    auto systemIt = m_systems.find(type);
    if (systemIt == m_systems.end()) {
        // spdlog::error("[SystemManager] System not found: {}", SystemUtils::GetSystemTypeName(type));
        return false;
    }

    auto& system = systemIt->second;
    auto& info = m_systemInfo[type];

    spdlog::info("[SystemManager] Initializing system: {}", info.name);

    auto startTime = std::chrono::steady_clock::now();

    try {
        bool success = system->Initialize();
        auto endTime = std::chrono::steady_clock::now();

        info.initializationTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        info.startTime = startTime;
        info.lastHealthCheck = startTime;

        if (success) {
            info.state = SystemState::Running;
            spdlog::info("[SystemManager] System initialized successfully: {} ({}ms)", info.name, info.initializationTime.count());
            NotifyStateChanged(type, SystemState::Running);
        } else {
            info.state = SystemState::Error;
            info.errorCount++;
            std::string error = system->GetLastError();
            spdlog::error("[SystemManager] System initialization failed: {} - {}", info.name, error);
            NotifyError(type, error);
        }

        return success;

    } catch (const std::exception& ex) {
        auto endTime = std::chrono::steady_clock::now();
        info.initializationTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        info.state = SystemState::Error;
        info.errorCount++;

        std::string error = "Exception during initialization: " + std::string(ex.what());
        // spdlog::error("[SystemManager] {}: {}", info.name, error);
        NotifyError(type, error);
        return false;
    }
}

std::vector<SystemType> SystemManager::GetInitializationOrder() const {
    return TopologicalSort();
}

std::vector<SystemType> SystemManager::TopologicalSort() const {
    std::vector<SystemType> result;
    std::unordered_set<SystemType> visited;
    std::unordered_set<SystemType> tempMarks;

    std::function<bool(SystemType)> visit = [&](SystemType systemType) -> bool {
        if (tempMarks.count(systemType)) {
            // Circular dependency detected
            spdlog::error("[SystemManager] Circular dependency detected involving: {}", SystemUtils::GetSystemTypeName(systemType));
            return false;
        }

        if (visited.count(systemType)) {
            return true;
        }

        tempMarks.insert(systemType);

        auto infoIt = m_systemInfo.find(systemType);
        if (infoIt != m_systemInfo.end()) {
            for (const auto& dep : infoIt->second.dependencies) {
                if (m_systems.count(dep.dependsOn) && !visit(dep.dependsOn)) {
                    return false;
                }
            }
        }

        tempMarks.erase(systemType);
        visited.insert(systemType);
        result.push_back(systemType);
        return true;
    };

    for (const auto& [systemType, system] : m_systems) {
        if (!visit(systemType)) {
            spdlog::error("[SystemManager] Failed to resolve dependencies");
            return {};
        }
    }

    return result;
}

void SystemManager::HealthMonitoringLoop() {
    while (!m_shouldStop) {
        try {
            PerformHealthCheck();
            std::this_thread::sleep_for(std::chrono::milliseconds(m_config.healthCheckInterval));
        } catch (const std::exception& ex) {
            spdlog::error("[SystemManager] Health monitoring error: {}", ex.what());
        }
    }
}

void SystemManager::PerformHealthCheck() {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    for (const auto& [type, system] : m_systems) {
        CheckSystemHealth(type);
    }
}

void SystemManager::CheckSystemHealth(SystemType type) {
    auto systemIt = m_systems.find(type);
    auto infoIt = m_systemInfo.find(type);

    if (systemIt == m_systems.end() || infoIt == m_systemInfo.end()) {
        return;
    }

    auto& system = systemIt->second;
    auto& info = infoIt->second;

    try {
        bool isHealthy = system->IsHealthy();
        SystemState currentState = system->GetState();

        if (!isHealthy || currentState == SystemState::Error) {
            std::string error = system->GetLastError();
            HandleSystemFailure(type, error);
        } else {
            // Update metrics
            {
                std::lock_guard<std::mutex> metricsLock(m_metricsMutex);
                m_systemMetrics[type] = system->GetMetrics();
            }

            info.lastHealthCheck = std::chrono::steady_clock::now();

            // Update watchdog timer
            if (m_watchdogEnabled) {
                UpdateWatchdogTimer(type);
            }
        }

    } catch (const std::exception& ex) {
        std::string error = "Health check exception: " + std::string(ex.what());
        HandleSystemFailure(type, error);
    }
}

void SystemManager::HandleSystemFailure(SystemType type, const std::string& error) {
    auto infoIt = m_systemInfo.find(type);
    if (infoIt == m_systemInfo.end()) {
        return;
    }

    auto& info = infoIt->second;
    info.errorCount++;
    info.state = SystemState::Error;

    spdlog::error("[SystemManager] System failure detected: {} - {}", info.name, error);
    NotifyError(type, error);

    // Store error for later retrieval
    {
        std::lock_guard<std::recursive_mutex> lock(m_systemMutex);
        m_systemErrors[type].push_back(error);

        // Keep only last 10 errors
        if (m_systemErrors[type].size() > 10) {
            m_systemErrors[type].erase(m_systemErrors[type].begin());
        }
    }

    // Attempt auto-restart if enabled
    if (m_autoRestartEnabled && m_restartCounts[type] < m_maxRestartAttempts) {
        spdlog::info("[SystemManager] Attempting to restart system: {}", info.name);

        if (RestartSystem(type)) {
            m_restartCounts[type]++;
            info.restartCount++;
            NotifySystemRestarted(type);
        } else {
            spdlog::error("[SystemManager] Failed to restart system: {}", info.name);

            if (info.isEssential) {
                TriggerEmergencyShutdown("Essential system failure: " + info.name);
            }
        }
    } else {
        if (info.isEssential) {
            TriggerEmergencyShutdown("Essential system failure with restart limit exceeded: " + info.name);
        }
    }
}

bool SystemManager::RestartSystem(SystemType type) {
    auto systemIt = m_systems.find(type);
    if (systemIt == m_systems.end()) {
        return false;
    }

    auto& system = systemIt->second;
    auto& info = m_systemInfo[type];

    try {
        // Shutdown the system
        system->Shutdown();

        // Reset the system
        system->Reset();

        // Reinitialize
        return InitializeSystem(type);

    } catch (const std::exception& ex) {
        spdlog::error("[SystemManager] Exception during system restart: {} - {}", info.name, ex.what());
        return false;
    }
}

void SystemManager::TriggerEmergencyShutdown(const std::string& reason) {
    spdlog::critical("[SystemManager] EMERGENCY SHUTDOWN TRIGGERED: {}", reason);

    m_emergencyShutdown = true;
    m_shutdownReason = reason;

    NotifyCriticalFailure(reason);

    // Perform immediate shutdown
    Shutdown();
}

// Event notification methods
void SystemManager::NotifyStateChanged(SystemType type, SystemState newState) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    if (m_events.onStateChanged) {
        try {
            m_events.onStateChanged(type, newState);
        } catch (const std::exception& ex) {
            spdlog::error("[SystemManager] State change callback error: {}", ex.what());
        }
    }
}

void SystemManager::NotifyError(SystemType type, const std::string& error) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    if (m_events.onError) {
        try {
            m_events.onError(type, error);
        } catch (const std::exception& ex) {
            spdlog::error("[SystemManager] Error callback error: {}", ex.what());
        }
    }
}

void SystemManager::NotifyAllSystemsReady() {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    if (m_events.onAllSystemsReady) {
        try {
            m_events.onAllSystemsReady();
        } catch (const std::exception& ex) {
            spdlog::error("[SystemManager] All systems ready callback error: {}", ex.what());
        }
    }
}

void SystemManager::NotifyCriticalFailure(const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    if (m_events.onCriticalFailure) {
        try {
            m_events.onCriticalFailure(reason);
        } catch (const std::exception& ex) {
            spdlog::error("[SystemManager] Critical failure callback error: {}", ex.what());
        }
    }
}

// Utility methods
bool SystemManager::IsSystemEssential(SystemType type) const {
    auto it = m_systemInfo.find(type);
    return it != m_systemInfo.end() ? it->second.isEssential : false;
}

// Missing SystemManager public methods implementation
void SystemManager::Update() {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    if (!m_initialized) return;

    auto now = std::chrono::steady_clock::now();
    m_updateCount++;
    m_lastUpdate = now;

    // Update all systems
    for (const auto& [type, system] : m_systems) {
        auto& info = m_systemInfo[type];
        if (info.state == SystemState::Running) {
            try {
                system->Update();
            } catch (const std::exception& ex) {
                std::string error = "Update exception: " + std::string(ex.what());
                HandleSystemFailure(type, error);
            }
        }
    }
}

bool SystemManager::IsInitialized() const {
    return m_initialized;
}

bool SystemManager::UnregisterSystem(SystemType type) {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    auto it = m_systems.find(type);
    if (it == m_systems.end()) {
        return false;
    }

    // Shutdown the system first
    if (it->second->GetState() == SystemState::Running) {
        it->second->Shutdown();
    }

    // Remove from all tracking structures
    m_systems.erase(type);
    m_systemInfo.erase(type);
    m_systemMetrics.erase(type);
    m_systemErrors.erase(type);
    m_systemTimeouts.erase(type);
    m_restartCounts.erase(type);

    spdlog::info("[SystemManager] Unregistered system: {}", SystemUtils::GetSystemTypeName(type));
    return true;
}

std::shared_ptr<ISystem> SystemManager::GetSystem(SystemType type) const {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    auto it = m_systems.find(type);
    return (it != m_systems.end()) ? it->second : nullptr;
}

bool SystemManager::IsSystemRunning(SystemType type) const {
    return GetSystemState(type) == SystemState::Running;
}

SystemState SystemManager::GetSystemState(SystemType type) const {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    auto it = m_systemInfo.find(type);
    return (it != m_systemInfo.end()) ? it->second.state : SystemState::Uninitialized;
}

bool SystemManager::StartSystem(SystemType type) {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    auto systemIt = m_systems.find(type);
    if (systemIt == m_systems.end()) {
        return false;
    }

    return InitializeSystem(type);
}

bool SystemManager::StopSystem(SystemType type) {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    auto systemIt = m_systems.find(type);
    if (systemIt == m_systems.end()) {
        return false;
    }

    ShutdownSystem(type);
    return true;
}

bool SystemManager::PauseSystem(SystemType type) {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    auto infoIt = m_systemInfo.find(type);
    if (infoIt == m_systemInfo.end()) {
        return false;
    }

    if (infoIt->second.state == SystemState::Running) {
        infoIt->second.state = SystemState::Paused;
        NotifyStateChanged(type, SystemState::Paused);
        return true;
    }

    return false;
}

bool SystemManager::ResumeSystem(SystemType type) {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    auto infoIt = m_systemInfo.find(type);
    if (infoIt == m_systemInfo.end()) {
        return false;
    }

    if (infoIt->second.state == SystemState::Paused) {
        infoIt->second.state = SystemState::Running;
        NotifyStateChanged(type, SystemState::Running);
        return true;
    }

    return false;
}

bool SystemManager::StartAllSystems() {
    return InitializeSystemsInOrder();
}

bool SystemManager::StopAllSystems() {
    ShutdownSystemsInOrder();
    return true;
}

bool SystemManager::RestartAllSystems() {
    ShutdownSystemsInOrder();
    return InitializeSystemsInOrder();
}

std::vector<SystemType> SystemManager::GetFailedSystems() const {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    std::vector<SystemType> failed;
    for (const auto& [type, info] : m_systemInfo) {
        if (info.state == SystemState::Error) {
            failed.push_back(type);
        }
    }
    return failed;
}

std::vector<SystemType> SystemManager::GetRunningSystems() const {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    std::vector<SystemType> running;
    for (const auto& [type, info] : m_systemInfo) {
        if (info.state == SystemState::Running) {
            running.push_back(type);
        }
    }
    return running;
}

void SystemManager::EnableHealthMonitoring(bool enabled) {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);
    m_healthMonitoringEnabled = enabled;
}

bool SystemManager::IsHealthMonitoringEnabled() const {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);
    return m_healthMonitoringEnabled;
}

bool SystemManager::AreAllSystemsHealthy() const {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    for (const auto& [type, system] : m_systems) {
        if (!system->IsHealthy()) {
            return false;
        }
    }
    return true;
}

SystemMetrics SystemManager::GetSystemMetrics(SystemType type) const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);

    auto it = m_systemMetrics.find(type);
    return (it != m_systemMetrics.end()) ? it->second : SystemMetrics{};
}

std::unordered_map<SystemType, SystemMetrics> SystemManager::GetAllMetrics() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    return m_systemMetrics;
}

std::vector<SystemType> SystemManager::GetShutdownOrder() const {
    auto order = GetInitializationOrder();
    std::reverse(order.begin(), order.end());
    return order;
}

bool SystemManager::HasCircularDependencies() const {
    std::unordered_set<SystemType> visited;
    std::unordered_set<SystemType> recursionStack;

    for (const auto& [type, system] : m_systems) {
        if (HasCircularDependency(type, visited, recursionStack)) {
            return true;
        }
    }
    return false;
}

void SystemManager::SetInitializationTimeout(std::chrono::milliseconds timeout) {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);
    m_config.maxInitializationTime = static_cast<uint32_t>(timeout.count());
}

void SystemManager::SetHealthCheckInterval(std::chrono::milliseconds interval) {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);
    m_config.healthCheckInterval = static_cast<uint32_t>(interval.count());
}

void SystemManager::EnableAutoRestart(bool enabled, uint32_t maxAttempts) {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);
    m_autoRestartEnabled = enabled;
    m_maxRestartAttempts = maxAttempts;
}

void SystemManager::SetSystemTimeout(SystemType type, std::chrono::milliseconds timeout) {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);
    m_systemTimeouts[type] = timeout;
}

void SystemManager::SetEventCallbacks(const SystemEvents& events) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    m_events = events;
}

void SystemManager::ClearEventCallbacks() {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    m_events = {};
}

std::vector<SystemInfo> SystemManager::GetSystemInformation() const {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    std::vector<SystemInfo> infoList;
    infoList.reserve(m_systemInfo.size());

    for (const auto& [type, info] : m_systemInfo) {
        infoList.push_back(info);
    }
    return infoList;
}

SystemInfo SystemManager::GetSystemInfo(SystemType type) const {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    auto it = m_systemInfo.find(type);
    return (it != m_systemInfo.end()) ? it->second : SystemInfo{};
}

std::string SystemManager::GenerateSystemReport() const {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    std::stringstream report;
    report << "System Manager Report\n";
    report << "===================\n\n";

    report << "Manager State: " << SystemUtils::GetSystemStateName(m_managerState) << "\n";
    report << "Initialized: " << (m_initialized ? "Yes" : "No") << "\n";
    report << "Health Monitoring: " << (m_healthMonitoringEnabled ? "Enabled" : "Disabled") << "\n";
    report << "Auto Restart: " << (m_autoRestartEnabled ? "Enabled" : "Disabled") << "\n";
    report << "Watchdog: " << (m_watchdogEnabled ? "Enabled" : "Disabled") << "\n\n";

    report << "Registered Systems (" << m_systems.size() << "):\n";
    for (const auto& [type, info] : m_systemInfo) {
        report << "- " << info.name << " (" << SystemUtils::GetSystemTypeName(type) << ")\n";
        report << "  State: " << SystemUtils::GetSystemStateName(info.state) << "\n";
        report << "  Priority: " << SystemUtils::GetSystemPriorityName(info.priority) << "\n";
        report << "  Essential: " << (info.isEssential ? "Yes" : "No") << "\n";
        report << "  Errors: " << info.errorCount << "\n";
        report << "  Restarts: " << info.restartCount << "\n\n";
    }

    return report.str();
}

std::string SystemManager::GetSystemStatusSummary() const {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    uint32_t running = 0, stopped = 0, errors = 0;
    for (const auto& [type, info] : m_systemInfo) {
        switch (info.state) {
            case SystemState::Running: running++; break;
            case SystemState::Stopped: stopped++; break;
            case SystemState::Error: errors++; break;
            default: break;
        }
    }

    std::stringstream summary;
    summary << "Running: " << running << ", Stopped: " << stopped << ", Errors: " << errors;
    return summary.str();
}

bool SystemManager::IsWatchdogEnabled() const {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);
    return m_watchdogEnabled;
}

bool SystemManager::SaveSystemSnapshot(const std::string& filename) const {
    // Simplified snapshot implementation
    try {
        std::ofstream file(filename);
        file << GenerateSystemReport();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool SystemManager::LoadSystemSnapshot(const std::string& filename) {
    // Simplified snapshot loading (placeholder)
    return std::filesystem::exists(filename);
}

void SystemManager::CollectPerformanceMetrics() {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    for (const auto& [type, system] : m_systems) {
        if (system->GetState() == SystemState::Running) {
            std::lock_guard<std::mutex> metricsLock(m_metricsMutex);
            m_systemMetrics[type] = system->GetMetrics();
        }
    }
}

void SystemManager::ResetPerformanceCounters() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_systemMetrics.clear();
}

float SystemManager::GetOverallSystemHealth() const {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    if (m_systems.empty()) return 1.0f;

    uint32_t healthyCount = 0;
    for (const auto& [type, system] : m_systems) {
        if (system->IsHealthy()) {
            healthyCount++;
        }
    }

    return static_cast<float>(healthyCount) / static_cast<float>(m_systems.size());
}

void SystemManager::ReportSystemError(SystemType type, const std::string& error, bool isCritical) {
    HandleSystemFailure(type, error);

    if (isCritical) {
        TriggerEmergencyShutdown("Critical system error: " + error);
    }
}

std::vector<std::string> SystemManager::GetSystemErrors(SystemType type) const {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    auto it = m_systemErrors.find(type);
    return (it != m_systemErrors.end()) ? it->second : std::vector<std::string>();
}

void SystemManager::ClearSystemErrors(SystemType type) {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    auto it = m_systemErrors.find(type);
    if (it != m_systemErrors.end()) {
        it->second.clear();
    }
}

// Private helper methods implementation
void SystemManager::ShutdownSystemsInOrder() {
    auto shutdownOrder = GetShutdownOrder();

    for (SystemType type : shutdownOrder) {
        ShutdownSystem(type);
    }
}

void SystemManager::ShutdownSystem(SystemType type) {
    auto systemIt = m_systems.find(type);
    auto infoIt = m_systemInfo.find(type);

    if (systemIt == m_systems.end() || infoIt == m_systemInfo.end()) {
        return;
    }

    auto& system = systemIt->second;
    auto& info = infoIt->second;

    spdlog::info("[SystemManager] Shutting down system: {}", info.name);

    try {
        system->Shutdown();
        info.state = SystemState::Stopped;
        NotifyStateChanged(type, SystemState::Stopped);
    } catch (const std::exception& ex) {
        spdlog::error("[SystemManager] Exception during shutdown of {}: {}", info.name, ex.what());
        info.state = SystemState::Error;
        NotifyError(type, ex.what());
    }
}

bool SystemManager::HasCircularDependency(SystemType type, std::unordered_set<SystemType>& visited,
                                        std::unordered_set<SystemType>& recursionStack) const {
    if (recursionStack.count(type)) {
        return true; // Found circular dependency
    }

    if (visited.count(type)) {
        return false; // Already processed
    }

    visited.insert(type);
    recursionStack.insert(type);

    auto infoIt = m_systemInfo.find(type);
    if (infoIt != m_systemInfo.end()) {
        for (const auto& dep : infoIt->second.dependencies) {
            if (m_systems.count(dep.dependsOn) &&
                HasCircularDependency(dep.dependsOn, visited, recursionStack)) {
                return true;
            }
        }
    }

    recursionStack.erase(type);
    return false;
}

void SystemManager::WatchdogLoop() {
    while (!m_shouldStop && m_watchdogEnabled) {
        try {
            for (const auto& [type, system] : m_systems) {
                if (!IsSystemResponsive(type)) {
                    std::string error = "System watchdog timeout";
                    HandleSystemFailure(type, error);
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
        } catch (const std::exception& ex) {
            spdlog::error("[SystemManager] Watchdog error: {}", ex.what());
        }
    }
}

void SystemManager::UpdateWatchdogTimer(SystemType type) {
    if (m_watchdogEnabled) {
        m_watchdogTimers[type] = std::chrono::steady_clock::now();
    }
}

bool SystemManager::IsSystemResponsive(SystemType type) const {
    auto it = m_watchdogTimers.find(type);
    if (it == m_watchdogTimers.end()) {
        return true; // No timer set, assume responsive
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second);
    return elapsed.count() < 30; // 30 second timeout
}

void SystemManager::NotifyMetricsUpdated(SystemType type, const SystemMetrics& metrics) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    if (m_events.onMetricsUpdated) {
        try {
            m_events.onMetricsUpdated(type, metrics);
        } catch (const std::exception& ex) {
            spdlog::error("[SystemManager] Metrics callback error: {}", ex.what());
        }
    }
}

void SystemManager::NotifySystemRestarted(SystemType type) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    if (m_events.onSystemRestarted) {
        try {
            m_events.onSystemRestarted(type);
        } catch (const std::exception& ex) {
            spdlog::error("[SystemManager] Restart callback error: {}", ex.what());
        }
    }
}

std::string SystemManager::GetSystemTypeName(SystemType type) const {
    return SystemUtils::GetSystemTypeName(type);
}

std::string SystemManager::GetSystemStateName(SystemState state) const {
    return SystemUtils::GetSystemStateName(state);
}

SystemPriority SystemManager::GetSystemPriority(SystemType type) const {
    auto it = m_systemInfo.find(type);
    return (it != m_systemInfo.end()) ? it->second.priority : SystemPriority::Optional;
}

void SystemManager::EnableWatchdog(bool enabled) {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    if (m_watchdogEnabled == enabled) return;

    m_watchdogEnabled = enabled;

    if (enabled && !m_watchdogThread.joinable()) {
        m_watchdogThread = std::thread(&SystemManager::WatchdogLoop, this);
    } else if (!enabled && m_watchdogThread.joinable()) {
        m_watchdogThread.join();
    }
}

// SystemUtils implementation
namespace SystemUtils {
    std::string GetSystemTypeName(SystemType type) {
        static const std::unordered_map<SystemType, std::string> typeNames = {
            {SystemType::ErrorManager, "ErrorManager"},
            {SystemType::ConfigurationManager, "ConfigurationManager"},
            {SystemType::DatabaseManager, "DatabaseManager"},
            {SystemType::ContentManager, "ContentManager"},
            {SystemType::PerformanceMonitor, "PerformanceMonitor"},
            {SystemType::NetworkOptimizer, "NetworkOptimizer"},
            {SystemType::VoiceCommunicationCore, "VoiceCommunicationCore"},
            {SystemType::SystemManager, "SystemManager"}
        };

        auto it = typeNames.find(type);
        return it != typeNames.end() ? it->second : "Unknown";
    }

    std::string GetSystemStateName(SystemState state) {
        static const std::unordered_map<SystemState, std::string> stateNames = {
            {SystemState::Uninitialized, "Uninitialized"},
            {SystemState::Initializing, "Initializing"},
            {SystemState::Running, "Running"},
            {SystemState::Paused, "Paused"},
            {SystemState::Stopping, "Stopping"},
            {SystemState::Stopped, "Stopped"},
            {SystemState::Error, "Error"},
            {SystemState::Recovering, "Recovering"}
        };

        auto it = stateNames.find(state);
        return it != stateNames.end() ? it->second : "Unknown";
    }

    std::string GetSystemPriorityName(SystemPriority priority) {
        static const std::unordered_map<SystemPriority, std::string> priorityNames = {
            {SystemPriority::Critical, "Critical"},
            {SystemPriority::High, "High"},
            {SystemPriority::Medium, "Medium"},
            {SystemPriority::Low, "Low"},
            {SystemPriority::Optional, "Optional"}
        };

        auto it = priorityNames.find(priority);
        return it != priorityNames.end() ? it->second : "Unknown";
    }

    SystemType GetSystemTypeFromName(const std::string& name) {
        static const std::unordered_map<std::string, SystemType> nameTypes = {
            {"ErrorManager", SystemType::ErrorManager},
            {"ConfigurationManager", SystemType::ConfigurationManager},
            {"DatabaseManager", SystemType::DatabaseManager},
            {"ContentManager", SystemType::ContentManager},
            {"PerformanceMonitor", SystemType::PerformanceMonitor},
            {"NetworkOptimizer", SystemType::NetworkOptimizer},
            {"VoiceCommunicationCore", SystemType::VoiceCommunicationCore},
            {"SystemManager", SystemType::SystemManager}
        };

        auto it = nameTypes.find(name);
        return (it != nameTypes.end()) ? it->second : SystemType::ErrorManager;
    }

    bool IsSystemTypeValid(SystemType type) {
        return static_cast<uint8_t>(type) <= static_cast<uint8_t>(SystemType::SystemManager);
    }

    std::string FormatSystemMetrics(const SystemMetrics& metrics) {
        std::stringstream ss;
        ss << "Memory: " << (metrics.memoryUsage / 1024 / 1024) << " MB, ";
        ss << "CPU: " << std::fixed << std::setprecision(1) << (metrics.cpuUsage * 100.0f) << "%, ";
        ss << "Requests: " << metrics.requestCount << ", ";
        ss << "Errors: " << metrics.errorCount << ", ";
        ss << "Avg Response: " << metrics.avgResponseTime.count() << " μs";
        return ss.str();
    }

    std::string FormatUptime(std::chrono::steady_clock::time_point startTime) {
        auto now = std::chrono::steady_clock::now();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);

        auto hours = std::chrono::duration_cast<std::chrono::hours>(uptime);
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(uptime % std::chrono::hours(1));
        auto seconds = uptime % std::chrono::minutes(1);

        std::stringstream ss;
        ss << hours.count() << "h " << minutes.count() << "m " << seconds.count() << "s";
        return ss.str();
    }

    bool ValidateSystemConfiguration(const InitializationConfig& config) {
        if (config.healthCheckInterval == 0) return false;
        if (config.maxInitializationTime == 0) return false;
        if (config.systemTimeoutMs == 0) return false;
        return true;
    }

    InitializationConfig LoadConfigurationFromFile(const std::string& filename) {
        InitializationConfig config;
        try {
            if (std::filesystem::exists(filename)) {
                std::ifstream file(filename);
                nlohmann::json j;
                file >> j;

                if (j.contains("configDirectory")) config.configDirectory = j["configDirectory"];
                if (j.contains("dataDirectory")) config.dataDirectory = j["dataDirectory"];
                if (j.contains("logDirectory")) config.logDirectory = j["logDirectory"];
                if (j.contains("contentDirectory")) config.contentDirectory = j["contentDirectory"];

                if (j.contains("enablePerformanceMonitoring")) config.enablePerformanceMonitoring = j["enablePerformanceMonitoring"];
                if (j.contains("enableVoiceChat")) config.enableVoiceChat = j["enableVoiceChat"];
                if (j.contains("enableNetworkOptimization")) config.enableNetworkOptimization = j["enableNetworkOptimization"];
                if (j.contains("enableContentStreaming")) config.enableContentStreaming = j["enableContentStreaming"];
                if (j.contains("enableDatabaseIntegration")) config.enableDatabaseIntegration = j["enableDatabaseIntegration"];

                if (j.contains("maxInitializationTime")) config.maxInitializationTime = j["maxInitializationTime"];
                if (j.contains("healthCheckInterval")) config.healthCheckInterval = j["healthCheckInterval"];
                if (j.contains("systemTimeoutMs")) config.systemTimeoutMs = j["systemTimeoutMs"];

                if (j.contains("autoRestartOnFailure")) config.autoRestartOnFailure = j["autoRestartOnFailure"];
                if (j.contains("maxRestartAttempts")) config.maxRestartAttempts = j["maxRestartAttempts"];
                if (j.contains("enableWatchdog")) config.enableWatchdog = j["enableWatchdog"];
            }
        } catch (const std::exception&) {
            // Return default config on error
        }
        return config;
    }

    bool SaveConfigurationToFile(const InitializationConfig& config, const std::string& filename) {
        try {
            nlohmann::json j;
            j["configDirectory"] = config.configDirectory;
            j["dataDirectory"] = config.dataDirectory;
            j["logDirectory"] = config.logDirectory;
            j["contentDirectory"] = config.contentDirectory;

            j["enablePerformanceMonitoring"] = config.enablePerformanceMonitoring;
            j["enableVoiceChat"] = config.enableVoiceChat;
            j["enableNetworkOptimization"] = config.enableNetworkOptimization;
            j["enableContentStreaming"] = config.enableContentStreaming;
            j["enableDatabaseIntegration"] = config.enableDatabaseIntegration;

            j["maxInitializationTime"] = config.maxInitializationTime;
            j["healthCheckInterval"] = config.healthCheckInterval;
            j["systemTimeoutMs"] = config.systemTimeoutMs;

            j["autoRestartOnFailure"] = config.autoRestartOnFailure;
            j["maxRestartAttempts"] = config.maxRestartAttempts;
            j["enableWatchdog"] = config.enableWatchdog;

            std::ofstream file(filename);
            file << j.dump(2);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
}

// Missing SystemManager::ValidateDependencies implementation
bool SystemManager::ValidateDependencies() const {
    std::lock_guard<std::recursive_mutex> lock(m_systemMutex);

    // Check for circular dependencies first
    if (HasCircularDependencies()) {
        spdlog::error("[SystemManager] Circular dependencies detected");
        return false;
    }

    // Validate each system's dependencies
    for (const auto& [systemType, info] : m_systemInfo) {
        for (const auto& dep : info.dependencies) {
            // Check if required dependency exists
            if (dep.isRequired && m_systems.find(dep.dependsOn) == m_systems.end()) {
                spdlog::error("[SystemManager] Missing required dependency: {} requires {}",
                             info.name, SystemUtils::GetSystemTypeName(dep.dependsOn));
                return false;
            }

            // Validate dependency version compatibility if specified
            if (!dep.name.empty() && m_systems.count(dep.dependsOn)) {
                auto depInfoIt = m_systemInfo.find(dep.dependsOn);
                if (depInfoIt != m_systemInfo.end()) {
                    // For now, simple version string comparison
                    // In a full implementation, you'd do proper semantic version comparison
                    if (!dep.minimumVersion.empty() &&
                        depInfoIt->second.version < dep.minimumVersion) {
                        spdlog::error("[SystemManager] Version compatibility issue: {} requires {} >= {}, but found {}",
                                     info.name, dep.name, dep.minimumVersion, depInfoIt->second.version);
                        return false;
                    }
                }
            }
        }
    }

    spdlog::debug("[SystemManager] Dependency validation passed for {} systems", m_systems.size());
    return true;
}

} // namespace CoopNet
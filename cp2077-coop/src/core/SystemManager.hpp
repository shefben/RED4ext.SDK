#pragma once

#include <RED4ext/RED4ext.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>

// Include all our systems
#include "../voice/VoiceCommunicationCore.hpp"
#include "../performance/PerformanceMonitor.hpp"
#include "../net/NetworkOptimizer.hpp"
#include "ErrorManager.hpp"
#include "../config/ConfigurationManager.hpp"
#include "../database/DatabaseManager.hpp"
#include "../content/ContentManager.hpp"

namespace CoopNet {

// System states
enum class SystemState : uint8_t {
    Uninitialized = 0,
    Initializing = 1,
    Running = 2,
    Paused = 3,
    Stopping = 4,
    Stopped = 5,
    Error = 6,
    Recovering = 7
};

// System types for identification
enum class SystemType : uint8_t {
    ErrorManager = 0,
    ConfigurationManager = 1,
    DatabaseManager = 2,
    ContentManager = 3,
    PerformanceMonitor = 4,
    NetworkOptimizer = 5,
    VoiceCommunicationCore = 6,
    SystemManager = 7
};

// System priorities for initialization order
enum class SystemPriority : uint8_t {
    Critical = 0,       // Error, Config systems
    High = 1,           // Database, Performance
    Medium = 2,         // Content, Network
    Low = 3,            // Voice, UI systems
    Optional = 4        // Analytics, telemetry
};

// Forward declarations
struct SystemInfo;
struct SystemMetrics;
struct SystemDependency;
struct InitializationConfig;

// System information
struct SystemInfo {
    SystemType type;
    std::string name;
    std::string version;
    SystemState state;
    SystemPriority priority;

    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastHealthCheck;
    std::chrono::milliseconds initializationTime;

    uint64_t errorCount;
    uint64_t restartCount;
    bool isEssential;           // System failure causes shutdown
    bool supportsHotReload;     // Can be restarted without full shutdown
    bool requiresCleanShutdown; // Needs graceful shutdown

    std::vector<SystemDependency> dependencies;
    std::unordered_map<std::string, std::string> metadata;
};

// System performance metrics
struct SystemMetrics {
    uint64_t memoryUsage;       // bytes
    float cpuUsage;             // percentage
    uint64_t requestCount;      // operations processed
    uint64_t errorCount;        // errors encountered
    std::chrono::microseconds avgResponseTime;
    std::chrono::microseconds maxResponseTime;
    std::chrono::steady_clock::time_point lastUpdate;

    std::unordered_map<std::string, uint64_t> customCounters;
    std::unordered_map<std::string, float> customGauges;
};

// System dependency definition
struct SystemDependency {
    SystemType dependsOn;
    std::string name;
    bool isRequired;            // Hard dependency vs soft dependency
    bool isCircular;            // Detected circular dependency
    std::string minimumVersion;
    std::string description;
};

// Initialization configuration
struct InitializationConfig {
    std::string configDirectory = "config/";
    std::string dataDirectory = "data/";
    std::string logDirectory = "logs/";
    std::string contentDirectory = "content/";

    bool enablePerformanceMonitoring = true;
    bool enableVoiceChat = true;
    bool enableNetworkOptimization = true;
    bool enableContentStreaming = true;
    bool enableDatabaseIntegration = true;

    uint32_t maxInitializationTime = 30000; // milliseconds
    uint32_t healthCheckInterval = 5000;    // milliseconds
    uint32_t systemTimeoutMs = 10000;       // milliseconds

    bool autoRestartOnFailure = true;
    uint32_t maxRestartAttempts = 3;
    bool enableWatchdog = true;

    std::unordered_map<std::string, std::string> systemSpecificConfig;
};

// System interface for unified management
class ISystem {
public:
    virtual ~ISystem() = default;
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Update() = 0;
    virtual SystemState GetState() const = 0;
    virtual SystemMetrics GetMetrics() const = 0;
    virtual bool IsHealthy() const = 0;
    virtual std::string GetLastError() const = 0;
    virtual void Reset() = 0;
};

// System event callbacks
struct SystemEvents {
    std::function<void(SystemType, SystemState)> onStateChanged;
    std::function<void(SystemType, const std::string&)> onError;
    std::function<void(SystemType, const SystemMetrics&)> onMetricsUpdated;
    std::function<void(SystemType)> onSystemRestarted;
    std::function<void()> onAllSystemsReady;
    std::function<void(const std::string&)> onCriticalFailure;
};

// Main system coordinator and manager
class SystemManager {
public:
    static SystemManager& Instance();

    // Core lifecycle
    bool Initialize(const InitializationConfig& config = InitializationConfig{});
    void Shutdown();
    void Update();
    bool IsInitialized() const;

    // System management
    bool RegisterSystem(SystemType type, std::shared_ptr<ISystem> system, const SystemInfo& info);
    bool UnregisterSystem(SystemType type);
    std::shared_ptr<ISystem> GetSystem(SystemType type) const;
    bool IsSystemRunning(SystemType type) const;
    SystemState GetSystemState(SystemType type) const;

    // System control
    bool StartSystem(SystemType type);
    bool StopSystem(SystemType type);
    bool RestartSystem(SystemType type);
    bool PauseSystem(SystemType type);
    bool ResumeSystem(SystemType type);

    // Bulk operations
    bool StartAllSystems();
    bool StopAllSystems();
    bool RestartAllSystems();
    std::vector<SystemType> GetFailedSystems() const;
    std::vector<SystemType> GetRunningSystems() const;

    // Health monitoring
    void EnableHealthMonitoring(bool enabled);
    bool IsHealthMonitoringEnabled() const;
    void PerformHealthCheck();
    bool AreAllSystemsHealthy() const;
    SystemMetrics GetSystemMetrics(SystemType type) const;
    std::unordered_map<SystemType, SystemMetrics> GetAllMetrics() const;

    // Dependency management
    bool ValidateDependencies() const;
    std::vector<SystemType> GetInitializationOrder() const;
    std::vector<SystemType> GetShutdownOrder() const;
    bool HasCircularDependencies() const;

    // Configuration
    void SetInitializationTimeout(std::chrono::milliseconds timeout);
    void SetHealthCheckInterval(std::chrono::milliseconds interval);
    void EnableAutoRestart(bool enabled, uint32_t maxAttempts = 3);
    void SetSystemTimeout(SystemType type, std::chrono::milliseconds timeout);

    // Event handling
    void SetEventCallbacks(const SystemEvents& events);
    void ClearEventCallbacks();

    // System information
    std::vector<SystemInfo> GetSystemInformation() const;
    SystemInfo GetSystemInfo(SystemType type) const;
    std::string GenerateSystemReport() const;
    std::string GetSystemStatusSummary() const;

    // Advanced features
    void EnableWatchdog(bool enabled);
    bool IsWatchdogEnabled() const;
    void TriggerEmergencyShutdown(const std::string& reason);
    bool SaveSystemSnapshot(const std::string& filename) const;
    bool LoadSystemSnapshot(const std::string& filename);

    // Integration helpers
    template<typename T>
    std::shared_ptr<T> GetTypedSystem(SystemType type) const {
        auto system = GetSystem(type);
        return std::dynamic_pointer_cast<T>(system);
    }

    // Performance monitoring integration
    void CollectPerformanceMetrics();
    void ResetPerformanceCounters();
    float GetOverallSystemHealth() const;

    // Error handling integration
    void ReportSystemError(SystemType type, const std::string& error, bool isCritical = false);
    std::vector<std::string> GetSystemErrors(SystemType type) const;
    void ClearSystemErrors(SystemType type);

private:
    SystemManager() = default;
    ~SystemManager() = default;
    SystemManager(const SystemManager&) = delete;
    SystemManager& operator=(const SystemManager&) = delete;

    // Core operations
    bool InitializeSystemsInOrder();
    void ShutdownSystemsInOrder();
    bool InitializeSystem(SystemType type);
    void ShutdownSystem(SystemType type);
    void RegisterAllSystemsInternal();

    // Dependency resolution
    std::vector<SystemType> TopologicalSort() const;
    bool HasCircularDependency(SystemType type, std::unordered_set<SystemType>& visited,
                              std::unordered_set<SystemType>& recursionStack) const;

    // Health monitoring
    void HealthMonitoringLoop();
    void CheckSystemHealth(SystemType type);
    void HandleSystemFailure(SystemType type, const std::string& error);

    // Watchdog functionality
    void WatchdogLoop();
    void UpdateWatchdogTimer(SystemType type);
    bool IsSystemResponsive(SystemType type) const;

    // Event processing
    void NotifyStateChanged(SystemType type, SystemState newState);
    void NotifyError(SystemType type, const std::string& error);
    void NotifyMetricsUpdated(SystemType type, const SystemMetrics& metrics);
    void NotifySystemRestarted(SystemType type);
    void NotifyAllSystemsReady();
    void NotifyCriticalFailure(const std::string& reason);

    // Utility methods
    std::string GetSystemTypeName(SystemType type) const;
    std::string GetSystemStateName(SystemState state) const;
    SystemPriority GetSystemPriority(SystemType type) const;
    bool IsSystemEssential(SystemType type) const;

    // Data storage
    std::unordered_map<SystemType, std::shared_ptr<ISystem>> m_systems;
    std::unordered_map<SystemType, SystemInfo> m_systemInfo;
    std::unordered_map<SystemType, SystemMetrics> m_systemMetrics;
    std::unordered_map<SystemType, std::vector<std::string>> m_systemErrors;

    // Configuration
    InitializationConfig m_config;
    std::unordered_map<SystemType, std::chrono::milliseconds> m_systemTimeouts;

    // Event callbacks
    SystemEvents m_events;

    // Health monitoring
    bool m_healthMonitoringEnabled = true;
    std::thread m_healthMonitoringThread;
    std::unordered_map<SystemType, std::chrono::steady_clock::time_point> m_lastHealthCheck;

    // Watchdog
    bool m_watchdogEnabled = false;
    std::thread m_watchdogThread;
    std::unordered_map<SystemType, std::chrono::steady_clock::time_point> m_watchdogTimers;

    // System state
    std::atomic<SystemState> m_managerState{SystemState::Uninitialized};
    bool m_initialized = false;
    bool m_emergencyShutdown = false;
    std::string m_shutdownReason;

    // Threading
    std::atomic<bool> m_shouldStop{false};
    mutable std::recursive_mutex m_systemMutex;
    mutable std::mutex m_metricsMutex;
    mutable std::mutex m_eventMutex;

    // Performance tracking
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_lastUpdate;
    uint64_t m_updateCount = 0;

    // Auto-restart functionality
    bool m_autoRestartEnabled = true;
    uint32_t m_maxRestartAttempts = 3;
    std::unordered_map<SystemType, uint32_t> m_restartCounts;
};

// System wrapper implementations for our existing managers
class ErrorManagerSystem : public ISystem {
public:
    bool Initialize() override;
    void Shutdown() override;
    void Update() override;
    SystemState GetState() const override;
    SystemMetrics GetMetrics() const override;
    bool IsHealthy() const override;
    std::string GetLastError() const override;
    void Reset() override;

private:
    SystemState m_state = SystemState::Uninitialized;
    std::string m_lastError;
    std::chrono::steady_clock::time_point m_startTime;
};

class ConfigurationManagerSystem : public ISystem {
public:
    bool Initialize() override;
    void Shutdown() override;
    void Update() override;
    SystemState GetState() const override;
    SystemMetrics GetMetrics() const override;
    bool IsHealthy() const override;
    std::string GetLastError() const override;
    void Reset() override;

private:
    SystemState m_state = SystemState::Uninitialized;
    std::string m_lastError;
    std::chrono::steady_clock::time_point m_startTime;
};

class DatabaseManagerSystem : public ISystem {
public:
    bool Initialize() override;
    void Shutdown() override;
    void Update() override;
    SystemState GetState() const override;
    SystemMetrics GetMetrics() const override;
    bool IsHealthy() const override;
    std::string GetLastError() const override;
    void Reset() override;

private:
    SystemState m_state = SystemState::Uninitialized;
    std::string m_lastError;
    std::chrono::steady_clock::time_point m_startTime;
};

class ContentManagerSystem : public ISystem {
public:
    bool Initialize() override;
    void Shutdown() override;
    void Update() override;
    SystemState GetState() const override;
    SystemMetrics GetMetrics() const override;
    bool IsHealthy() const override;
    std::string GetLastError() const override;
    void Reset() override;

private:
    SystemState m_state = SystemState::Uninitialized;
    std::string m_lastError;
    std::chrono::steady_clock::time_point m_startTime;
};

class PerformanceMonitorSystem : public ISystem {
public:
    bool Initialize() override;
    void Shutdown() override;
    void Update() override;
    SystemState GetState() const override;
    SystemMetrics GetMetrics() const override;
    bool IsHealthy() const override;
    std::string GetLastError() const override;
    void Reset() override;

private:
    SystemState m_state = SystemState::Uninitialized;
    std::string m_lastError;
    std::chrono::steady_clock::time_point m_startTime;
};

class NetworkOptimizerSystem : public ISystem {
public:
    bool Initialize() override;
    void Shutdown() override;
    void Update() override;
    SystemState GetState() const override;
    SystemMetrics GetMetrics() const override;
    bool IsHealthy() const override;
    std::string GetLastError() const override;
    void Reset() override;

private:
    SystemState m_state = SystemState::Uninitialized;
    std::string m_lastError;
    std::chrono::steady_clock::time_point m_startTime;
};

class VoiceCommunicationSystem : public ISystem {
public:
    bool Initialize() override;
    void Shutdown() override;
    void Update() override;
    SystemState GetState() const override;
    SystemMetrics GetMetrics() const override;
    bool IsHealthy() const override;
    std::string GetLastError() const override;
    void Reset() override;

private:
    SystemState m_state = SystemState::Uninitialized;
    std::string m_lastError;
    std::chrono::steady_clock::time_point m_startTime;
};

// Utility functions
namespace SystemUtils {
    std::string GetSystemTypeName(SystemType type);
    std::string GetSystemStateName(SystemState state);
    std::string GetSystemPriorityName(SystemPriority priority);

    SystemType GetSystemTypeFromName(const std::string& name);
    bool IsSystemTypeValid(SystemType type);

    std::string FormatSystemMetrics(const SystemMetrics& metrics);
    std::string FormatUptime(std::chrono::steady_clock::time_point startTime);

    bool ValidateSystemConfiguration(const InitializationConfig& config);
    InitializationConfig LoadConfigurationFromFile(const std::string& filename);
    bool SaveConfigurationToFile(const InitializationConfig& config, const std::string& filename);
}

} // namespace CoopNet
#pragma once

#include <RED4ext/RED4ext.hpp>
#include "SystemManager.hpp"
#include "../net/Connection.hpp"
#include <memory>
#include <string>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>

namespace CoopNet {

// Main integration point for the cooperative multiplayer system
class CoopNetCore {
public:
    static CoopNetCore& Instance();

    // Core lifecycle - integrates with RED4ext plugin lifecycle
    bool Initialize();
    void Shutdown();
    void Update();

    // System management
    SystemManager& GetSystemManager() { return SystemManager::Instance(); }
    bool AreAllSystemsReady() const;
    std::string GetSystemStatus() const;

    // Integration with existing coop functionality
    bool InitializeNetworking();
    void ShutdownNetworking();
    bool IsNetworkingActive() const;

    // Game integration hooks
    void OnGameStart();
    void OnGameStop();
    void OnPlayerConnect(uint32_t playerId);
    void OnPlayerDisconnect(uint32_t playerId);

    // Configuration management
    bool LoadConfiguration(const std::string& configPath = "config/coopnet.json");
    bool SaveConfiguration(const std::string& configPath = "config/coopnet.json");

    // Status and diagnostics
    bool PerformSystemDiagnostics();
    std::string GenerateSystemReport();
    void EnableDebugMode(bool enabled);

    // Event system for inter-system communication
    void RegisterEventHandler(const std::string& eventType, std::function<void(const nlohmann::json&)> handler);
    void UnregisterEventHandler(const std::string& eventType);
    void BroadcastEvent(const std::string& eventType, const nlohmann::json& data);

    // Error handling integration
    void ReportError(const std::string& component, const std::string& error, bool isCritical = false);

private:
    CoopNetCore() = default;
    ~CoopNetCore() = default;
    CoopNetCore(const CoopNetCore&) = delete;
    CoopNetCore& operator=(const CoopNetCore&) = delete;

    // System event callbacks
    void OnSystemStateChanged(SystemType type, SystemState state);
    void OnSystemError(SystemType type, const std::string& error);
    void OnAllSystemsReady();
    void OnCriticalFailure(const std::string& reason);

    // Configuration helpers
    InitializationConfig CreateDefaultConfig();
    bool ValidateConfiguration(const nlohmann::json& config);

    // Integration with existing networking
    void IntegrateWithExistingNetworking();
    void SetupSystemInterconnections();

    // Event system
    std::unordered_map<std::string, std::function<void(const nlohmann::json&)>> m_eventHandlers;
    std::mutex m_eventMutex;

    // State tracking
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_systemsReady{false};
    std::atomic<bool> m_networkingActive{false};
    std::atomic<bool> m_debugMode{false};

    mutable std::mutex m_stateMutex;
    std::string m_lastError;
};

// Global convenience functions for easy access from existing codebase
namespace CoopNetAPI {
    // Quick access functions that can be called from existing C++ code
    bool InitializeCoopNet();
    void ShutdownCoopNet();
    bool IsCoopNetReady();
    std::string GetCoopNetStatus();

    // System-specific accessors
    ErrorManager& GetErrorManager();
    ConfigurationManager& GetConfigManager();
    DatabaseManager& GetDatabaseManager();
    ContentManager& GetContentManager();
    PerformanceMonitor& GetPerformanceMonitor();
    NetworkOptimizer& GetNetworkOptimizer();
    VoiceCommunicationCore& GetVoiceCore();

    // Event system convenience functions
    void RegisterForEvents(const std::string& eventType, std::function<void(const nlohmann::json&)> handler);
    void SendEvent(const std::string& eventType, const nlohmann::json& data);

    // Error reporting convenience
    void ReportError(const std::string& component, const std::string& error);
    void ReportCriticalError(const std::string& component, const std::string& error);

    // Configuration convenience
    template<typename T>
    T GetConfigValue(const std::string& key, const T& defaultValue = T{}) {
        try {
            return ConfigurationManager::Instance().GetValueOrDefault<T>("", key, defaultValue);
        } catch (...) {
            return defaultValue;
        }
    }

    template<typename T>
    bool SetConfigValue(const std::string& key, const T& value) {
        try {
            return ConfigurationManager::Instance().SetValue<T>("", key, value);
        } catch (...) {
            return false;
        }
    }
}

} // namespace CoopNet

// C-style exports for integration with existing C code or REDscript bindings
extern "C" {
    // Core functions
    bool CoopNet_Initialize();
    void CoopNet_Shutdown();
    void CoopNet_Update();
    bool CoopNet_IsReady();

    // Status functions
    const char* CoopNet_GetStatus();
    const char* CoopNet_GetLastError();
    bool CoopNet_PerformDiagnostics();

    // Event functions
    void CoopNet_RegisterEventHandler(const char* eventType, void(*handler)(const char* jsonData));
    void CoopNet_SendEvent(const char* eventType, const char* jsonData);

    // Configuration functions
    bool CoopNet_LoadConfig(const char* configPath);
    bool CoopNet_SaveConfig(const char* configPath);
    const char* CoopNet_GetConfigString(const char* key, const char* defaultValue);
    bool CoopNet_SetConfigString(const char* key, const char* value);
    int CoopNet_GetConfigInt(const char* key, int defaultValue);
    bool CoopNet_SetConfigInt(const char* key, int value);
    float CoopNet_GetConfigFloat(const char* key, float defaultValue);
    bool CoopNet_SetConfigFloat(const char* key, float value);
    bool CoopNet_GetConfigBool(const char* key, bool defaultValue);
    bool CoopNet_SetConfigBool(const char* key, bool value);

    // Error reporting
    void CoopNet_ReportError(const char* component, const char* error);
    void CoopNet_ReportCriticalError(const char* component, const char* error);

    // System-specific functions
    bool CoopNet_EnableVoiceChat(bool enabled);
    bool CoopNet_IsVoiceChatEnabled();
    bool CoopNet_EnablePerformanceMonitoring(bool enabled);
    bool CoopNet_IsPerformanceMonitoringEnabled();
    bool CoopNet_EnableNetworkOptimization(bool enabled);
    bool CoopNet_IsNetworkOptimizationEnabled();

    // Memory management for returned strings
    void CoopNet_FreeString(const char* str);
}

// Integration macros for easy use in existing codebase
#define COOPNET_INIT() CoopNet::CoopNetAPI::InitializeCoopNet()
#define COOPNET_SHUTDOWN() CoopNet::CoopNetAPI::ShutdownCoopNet()
#define COOPNET_READY() CoopNet::CoopNetAPI::IsCoopNetReady()
#define COOPNET_ERROR(component, msg) CoopNet::CoopNetAPI::ReportError(component, msg)
#define COOPNET_CRITICAL(component, msg) CoopNet::CoopNetAPI::ReportCriticalError(component, msg)
#define COOPNET_CONFIG_GET(key, type, def) CoopNet::CoopNetAPI::GetConfigValue<type>(key, def)
#define COOPNET_CONFIG_SET(key, value) CoopNet::CoopNetAPI::SetConfigValue(key, value)
#define COOPNET_EVENT_SEND(type, data) CoopNet::CoopNetAPI::SendEvent(type, data)
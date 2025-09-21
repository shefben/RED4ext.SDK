#include "CoopNetCore.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include "Logger.hpp"
#include <fstream>
#include <filesystem>

namespace CoopNet {

CoopNetCore& CoopNetCore::Instance() {
    static CoopNetCore instance;
    return instance;
}

bool CoopNetCore::Initialize() {
    std::lock_guard<std::mutex> lock(m_stateMutex);

    if (m_initialized) {
        spdlog::warn("[CoopNetCore] Already initialized");
        return true;
    }

    spdlog::info("[CoopNetCore] Initializing CoopNet core systems...");

    try {
        // Load configuration first
        if (!LoadConfiguration()) {
            // spdlog::warn("[CoopNetCore] Failed to load configuration, using defaults");
        }

        // Create initialization config
        InitializationConfig config = CreateDefaultConfig();

        // Set up system event callbacks
        SystemEvents events;
        events.onStateChanged = [this](SystemType type, SystemState state) {
            OnSystemStateChanged(type, state);
        };
        events.onError = [this](SystemType type, const std::string& error) {
            OnSystemError(type, error);
        };
        events.onAllSystemsReady = [this]() {
            OnAllSystemsReady();
        };
        events.onCriticalFailure = [this](const std::string& reason) {
            OnCriticalFailure(reason);
        };

        auto& systemManager = SystemManager::Instance();
        systemManager.SetEventCallbacks(events);

        // Initialize all systems
        if (!systemManager.Initialize(config)) {
            m_lastError = "Failed to initialize system manager";
            // spdlog::error("[CoopNetCore] {}", m_lastError);
            return false;
        }

        // Set up system interconnections
        SetupSystemInterconnections();

        // Initialize networking integration
        if (!InitializeNetworking()) {
            // spdlog::warn("[CoopNetCore] Failed to initialize networking, continuing without it");
        }

        m_initialized = true;
        spdlog::info("[CoopNetCore] CoopNet core initialized successfully");

        return true;

    } catch (const std::exception& ex) {
        m_lastError = "Exception during initialization: " + std::string(ex.what());
        // spdlog::error("[CoopNetCore] {}", m_lastError);
        return false;
    }
}

void CoopNetCore::Shutdown() {
    std::lock_guard<std::mutex> lock(m_stateMutex);

    if (!m_initialized) return;

    spdlog::info("[CoopNetCore] Shutting down CoopNet core...");

    try {
        // Shutdown networking first
        ShutdownNetworking();

        // Clear event handlers
        {
            std::lock_guard<std::mutex> eventLock(m_eventMutex);
            m_eventHandlers.clear();
        }

        // Shutdown system manager
        SystemManager::Instance().Shutdown();

        m_initialized = false;
        m_systemsReady = false;
        m_networkingActive = false;

        spdlog::info("[CoopNetCore] CoopNet core shutdown completed");

    } catch (const std::exception& ex) {
        spdlog::error("[CoopNetCore] Exception during shutdown: {}", ex.what());
    }
}

void CoopNetCore::Update() {
    if (!m_initialized) return;

    try {
        // Update system manager
        SystemManager::Instance().Update();

        // Process any pending events or maintenance tasks
        // This could include inter-system communication, etc.

    } catch (const std::exception& ex) {
        // spdlog::error("[CoopNetCore] Exception during update: {}", ex.what());
        ReportError("CoopNetCore", ex.what(), false);
    }
}

bool CoopNetCore::AreAllSystemsReady() const {
    return m_systemsReady && m_initialized;
}

std::string CoopNetCore::GetSystemStatus() const {
    if (!m_initialized) {
        return "Not initialized";
    }

    auto& systemManager = SystemManager::Instance();
    return systemManager.GetSystemStatusSummary();
}

bool CoopNetCore::InitializeNetworking() {
    try {
        // Integration with existing networking would go here
        // This would coordinate with Connection.hpp and Net.cpp

        // spdlog::info("[CoopNetCore] Networking integration initialized");
        m_networkingActive = true;
        return true;

    } catch (const std::exception& ex) {
        // spdlog::error("[CoopNetCore] Failed to initialize networking: {}", ex.what());
        return false;
    }
}

void CoopNetCore::ShutdownNetworking() {
    if (m_networkingActive) {
        try {
            // Shutdown networking integration
            m_networkingActive = false;
            // spdlog::info("[CoopNetCore] Networking integration shutdown");
        } catch (const std::exception& ex) {
            // spdlog::error("[CoopNetCore] Error during networking shutdown: {}", ex.what());
        }
    }
}

bool CoopNetCore::IsNetworkingActive() const {
    return m_networkingActive;
}

void CoopNetCore::OnGameStart() {
    // spdlog::info("[CoopNetCore] Game start detected");

    if (m_initialized && m_systemsReady) {
        // Notify systems that the game has started
        nlohmann::json eventData;
        eventData["timestamp"] = std::time(nullptr);
        BroadcastEvent("game_start", eventData);

        // Enable performance monitoring
        if (auto perfMonitor = SystemManager::Instance().GetTypedSystem<PerformanceMonitorSystem>(SystemType::PerformanceMonitor)) {
            // Start intensive monitoring during gameplay
        }
    }
}

void CoopNetCore::OnGameStop() {
    // spdlog::info("[CoopNetCore] Game stop detected");

    if (m_initialized) {
        // Notify systems that the game has stopped
        nlohmann::json eventData;
        eventData["timestamp"] = std::time(nullptr);
        BroadcastEvent("game_stop", eventData);

        // Reduce performance monitoring
        if (auto perfMonitor = SystemManager::Instance().GetTypedSystem<PerformanceMonitorSystem>(SystemType::PerformanceMonitor)) {
            // Reduce monitoring intensity when not in game
        }
    }
}

void CoopNetCore::OnPlayerConnect(uint32_t playerId) {
    // spdlog::info("[CoopNetCore] Player connected: {}", playerId);

    if (m_initialized && m_systemsReady) {
        nlohmann::json eventData;
        eventData["player_id"] = playerId;
        eventData["timestamp"] = std::time(nullptr);
        BroadcastEvent("player_connect", eventData);

        // Initialize voice channel for new player
        if (auto voiceCore = SystemManager::Instance().GetTypedSystem<VoiceCommunicationSystem>(SystemType::VoiceCommunicationCore)) {
            // Set up voice communication for the new player
        }
    }
}

void CoopNetCore::OnPlayerDisconnect(uint32_t playerId) {
    // spdlog::info("[CoopNetCore] Player disconnected: {}", playerId);

    if (m_initialized) {
        nlohmann::json eventData;
        eventData["player_id"] = playerId;
        eventData["timestamp"] = std::time(nullptr);
        BroadcastEvent("player_disconnect", eventData);

        // Clean up voice channel for disconnected player
        if (auto voiceCore = SystemManager::Instance().GetTypedSystem<VoiceCommunicationSystem>(SystemType::VoiceCommunicationCore)) {
            // Clean up voice communication for the disconnected player
        }
    }
}

bool CoopNetCore::LoadConfiguration(const std::string& configPath) {
    try {
        if (!std::filesystem::exists(configPath)) {
            // spdlog::info("[CoopNetCore] Configuration file not found, creating default: {}", configPath);
            return SaveConfiguration(configPath);
        }

        std::ifstream file(configPath);
        if (!file.is_open()) {
            // spdlog::error("[CoopNetCore] Failed to open configuration file: {}", configPath);
            return false;
        }

        nlohmann::json config;
        file >> config;

        if (!ValidateConfiguration(config)) {
            // spdlog::error("[CoopNetCore] Invalid configuration file: {}", configPath);
            return false;
        }

        // spdlog::info("[CoopNetCore] Configuration loaded successfully from: {}", configPath);
        return true;

    } catch (const std::exception& ex) {
        // spdlog::error("[CoopNetCore] Exception loading configuration: {}", ex.what());
        return false;
    }
}

bool CoopNetCore::SaveConfiguration(const std::string& configPath) {
    try {
        std::filesystem::create_directories(std::filesystem::path(configPath).parent_path());

        nlohmann::json config;
        config["version"] = "1.0.0";
        config["coopnet"] = {
            {"enable_voice_chat", true},
            {"enable_performance_monitoring", true},
            {"enable_network_optimization", true},
            {"enable_content_streaming", true},
            {"enable_database_integration", true},
            {"debug_mode", false},
            {"log_level", "info"}
        };

        config["systems"] = {
            {"error_manager", {{"enabled", true}}},
            {"configuration_manager", {{"enabled", true}}},
            {"database_manager", {
                {"enabled", true},
                {"database_path", "coopnet.db"},
                {"max_connections", 10}
            }},
            {"content_manager", {
                {"enabled", true},
                {"content_directory", "content/"},
                {"cache_size_mb", 512}
            }},
            {"performance_monitor", {
                {"enabled", true},
                {"monitoring_interval_ms", 1000},
                {"enable_gpu_monitoring", true}
            }},
            {"network_optimizer", {
                {"enabled", true},
                {"enable_compression", true},
                {"max_bandwidth_mbps", 10}
            }},
            {"voice_communication", {
                {"enabled", true},
                {"quality", "high"},
                {"spatial_audio", true},
                {"noise_suppression", true}
            }}
        };

        std::ofstream file(configPath);
        if (!file.is_open()) {
            // spdlog::error("[CoopNetCore] Failed to create configuration file: {}", configPath);
            return false;
        }

        file << config.dump(4);
        file.close();

        // spdlog::info("[CoopNetCore] Configuration saved to: {}", configPath);
        return true;

    } catch (const std::exception& ex) {
        // spdlog::error("[CoopNetCore] Exception saving configuration: {}", ex.what());
        return false;
    }
}

bool CoopNetCore::PerformSystemDiagnostics() {
    if (!m_initialized) {
        // spdlog::error("[CoopNetCore] Cannot perform diagnostics - not initialized");
        return false;
    }

    // spdlog::info("[CoopNetCore] Performing system diagnostics...");

    try {
        auto& systemManager = SystemManager::Instance();

        // Check if all systems are healthy
        bool allHealthy = systemManager.AreAllSystemsHealthy();

        // Get failed systems
        auto failedSystems = systemManager.GetFailedSystems();

        // Generate detailed report
        std::string report = systemManager.GenerateSystemReport();

        // spdlog::info("[CoopNetCore] Diagnostics completed - All systems healthy: {}", allHealthy);

        if (!failedSystems.empty()) {
            for (SystemType type : failedSystems) {
                // spdlog::error("[CoopNetCore] Failed system: {}", SystemUtils::GetSystemTypeName(type));
            }
        }

        if (m_debugMode) {
            // spdlog::debug("[CoopNetCore] System Report:\n{}", report);
        }

        return allHealthy;

    } catch (const std::exception& ex) {
        // spdlog::error("[CoopNetCore] Exception during diagnostics: {}", ex.what());
        return false;
    }
}

std::string CoopNetCore::GenerateSystemReport() {
    if (!m_initialized) {
        return "CoopNet not initialized";
    }

    try {
        auto& systemManager = SystemManager::Instance();
        return systemManager.GenerateSystemReport();
    } catch (const std::exception& ex) {
        return "Error generating report: " + std::string(ex.what());
    }
}

void CoopNetCore::EnableDebugMode(bool enabled) {
    m_debugMode = enabled;
    // spdlog::info("[CoopNetCore] Debug mode: {}", enabled ? "enabled" : "disabled");
}

void CoopNetCore::RegisterEventHandler(const std::string& eventType, std::function<void(const nlohmann::json&)> handler) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    m_eventHandlers[eventType] = handler;
    // spdlog::debug("[CoopNetCore] Registered event handler for: {}", eventType);
}

void CoopNetCore::UnregisterEventHandler(const std::string& eventType) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    m_eventHandlers.erase(eventType);
    // spdlog::debug("[CoopNetCore] Unregistered event handler for: {}", eventType);
}

void CoopNetCore::BroadcastEvent(const std::string& eventType, const nlohmann::json& data) {
    std::lock_guard<std::mutex> lock(m_eventMutex);

    auto it = m_eventHandlers.find(eventType);
    if (it != m_eventHandlers.end()) {
        try {
            it->second(data);
            // spdlog::debug("[CoopNetCore] Event broadcasted: {}", eventType);
        } catch (const std::exception& ex) {
            // spdlog::error("[CoopNetCore] Exception in event handler for {}: {}", eventType, ex.what());
        }
    }
}

void CoopNetCore::ReportError(const std::string& component, const std::string& error, bool isCritical) {
    if (m_initialized) {
        auto& errorManager = ErrorManager::Instance();

        ErrorCategory category = ErrorCategory::System;
        ErrorSeverity severity = isCritical ? ErrorSeverity::Critical : ErrorSeverity::Error;

        errorManager.ReportError(category, severity, error, "", "", "", 0);

        // spdlog::error("[CoopNetCore] {} error in {}: {}", isCritical ? "Critical" : "Regular", component, error);
    } else {
        // spdlog::error("[CoopNetCore] Error in {} (system not initialized): {}", component, error);
    }
}

// System event callbacks
void CoopNetCore::OnSystemStateChanged(SystemType type, SystemState state) {
    // spdlog::info("[CoopNetCore] System state changed: {} -> {}", SystemUtils::GetSystemTypeName(type), SystemUtils::GetSystemStateName(state));

    nlohmann::json data;
    data["system_type"] = SystemUtils::GetSystemTypeName(type);
    data["new_state"] = SystemUtils::GetSystemStateName(state);
    data["timestamp"] = std::time(nullptr);

    BroadcastEvent("system_state_changed", data);
}

void CoopNetCore::OnSystemError(SystemType type, const std::string& error) {
    // spdlog::error("[CoopNetCore] System error in {}: {}", SystemUtils::GetSystemTypeName(type), error);

    nlohmann::json data;
    data["system_type"] = SystemUtils::GetSystemTypeName(type);
    data["error"] = error;
    data["timestamp"] = std::time(nullptr);

    BroadcastEvent("system_error", data);
}

void CoopNetCore::OnAllSystemsReady() {
    // spdlog::info("[CoopNetCore] All systems are ready!");
    m_systemsReady = true;

    BroadcastEvent("all_systems_ready", nlohmann::json{{"timestamp", std::time(nullptr)}});
}

void CoopNetCore::OnCriticalFailure(const std::string& reason) {
    // spdlog::critical("[CoopNetCore] Critical system failure: {}", reason);

    nlohmann::json data;
    data["reason"] = reason;
    data["timestamp"] = std::time(nullptr);

    BroadcastEvent("critical_failure", data);
}

InitializationConfig CoopNetCore::CreateDefaultConfig() {
    InitializationConfig config;
    config.configDirectory = "config/";
    config.dataDirectory = "data/";
    config.logDirectory = "logs/";
    config.contentDirectory = "content/";

    config.enablePerformanceMonitoring = true;
    config.enableVoiceChat = true;
    config.enableNetworkOptimization = true;
    config.enableContentStreaming = true;
    config.enableDatabaseIntegration = true;

    config.maxInitializationTime = 30000; // 30 seconds
    config.healthCheckInterval = 5000;    // 5 seconds
    config.systemTimeoutMs = 10000;       // 10 seconds

    config.autoRestartOnFailure = true;
    config.maxRestartAttempts = 3;
    config.enableWatchdog = true;

    return config;
}

bool CoopNetCore::ValidateConfiguration(const nlohmann::json& config) {
    try {
        if (!config.contains("version")) {
            // spdlog::error("[CoopNetCore] Configuration missing version");
            return false;
        }

        if (!config.contains("coopnet")) {
            // spdlog::error("[CoopNetCore] Configuration missing coopnet section");
            return false;
        }

        return true;
    } catch (const std::exception& ex) {
        // spdlog::error("[CoopNetCore] Configuration validation error: {}", ex.what());
        return false;
    }
}

void CoopNetCore::SetupSystemInterconnections() {
    // Set up inter-system communication channels
    // This would configure how systems talk to each other

    // spdlog::debug("[CoopNetCore] Setting up system interconnections");

    // Example: Configure error manager to handle system errors
    // Configure performance monitor to track all systems
    // Configure network optimizer to work with voice communication
    // etc.
}

// CoopNetAPI namespace implementation
namespace CoopNetAPI {
    bool InitializeCoopNet() {
        return CoopNetCore::Instance().Initialize();
    }

    void ShutdownCoopNet() {
        CoopNetCore::Instance().Shutdown();
    }

    bool IsCoopNetReady() {
        return CoopNetCore::Instance().AreAllSystemsReady();
    }

    std::string GetCoopNetStatus() {
        return CoopNetCore::Instance().GetSystemStatus();
    }

    ErrorManager& GetErrorManager() {
        return ErrorManager::Instance();
    }

    ConfigurationManager& GetConfigManager() {
        return ConfigurationManager::Instance();
    }

    DatabaseManager& GetDatabaseManager() {
        return DatabaseManager::Instance();
    }

    ContentManager& GetContentManager() {
        return ContentManager::Instance();
    }

    PerformanceMonitor& GetPerformanceMonitor() {
        return PerformanceMonitor::Instance();
    }

    NetworkOptimizer& GetNetworkOptimizer() {
        return NetworkOptimizer::Instance();
    }

    VoiceCommunicationCore& GetVoiceCore() {
        return VoiceCommunicationCore::Instance();
    }

    void RegisterForEvents(const std::string& eventType, std::function<void(const nlohmann::json&)> handler) {
        CoopNetCore::Instance().RegisterEventHandler(eventType, handler);
    }

    void SendEvent(const std::string& eventType, const nlohmann::json& data) {
        CoopNetCore::Instance().BroadcastEvent(eventType, data);
    }

    void ReportError(const std::string& component, const std::string& error) {
        CoopNetCore::Instance().ReportError(component, error, false);
    }

    void ReportCriticalError(const std::string& component, const std::string& error) {
        CoopNetCore::Instance().ReportError(component, error, true);
    }
}

} // namespace CoopNet

// C-style exports implementation
extern "C" {
    bool CoopNet_Initialize() {
        return CoopNet::CoopNetAPI::InitializeCoopNet();
    }

    void CoopNet_Shutdown() {
        CoopNet::CoopNetAPI::ShutdownCoopNet();
    }

    void CoopNet_Update() {
        CoopNet::CoopNetCore::Instance().Update();
    }

    bool CoopNet_IsReady() {
        return CoopNet::CoopNetAPI::IsCoopNetReady();
    }

    const char* CoopNet_GetStatus() {
        static std::string status;
        status = CoopNet::CoopNetAPI::GetCoopNetStatus();
        return status.c_str();
    }

    const char* CoopNet_GetLastError() {
        static std::string error;
        error = CoopNet::CoopNetCore::Instance().GetSystemStatus();
        return error.c_str();
    }

    bool CoopNet_PerformDiagnostics() {
        return CoopNet::CoopNetCore::Instance().PerformSystemDiagnostics();
    }

    bool CoopNet_LoadConfig(const char* configPath) {
        return CoopNet::CoopNetCore::Instance().LoadConfiguration(configPath ? configPath : "config/coopnet.json");
    }

    bool CoopNet_SaveConfig(const char* configPath) {
        return CoopNet::CoopNetCore::Instance().SaveConfiguration(configPath ? configPath : "config/coopnet.json");
    }

    void CoopNet_ReportError(const char* component, const char* error) {
        if (component && error) {
            CoopNet::CoopNetAPI::ReportError(component, error);
        }
    }

    void CoopNet_ReportCriticalError(const char* component, const char* error) {
        if (component && error) {
            CoopNet::CoopNetAPI::ReportCriticalError(component, error);
        }
    }

    // Configuration functions
    const char* CoopNet_GetConfigString(const char* key, const char* defaultValue) {
        if (!key) return defaultValue;

        static std::string result;
        result = CoopNet::CoopNetAPI::GetConfigValue<std::string>(key, defaultValue ? defaultValue : "");
        return result.c_str();
    }

    bool CoopNet_SetConfigString(const char* key, const char* value) {
        if (!key || !value) return false;
        return CoopNet::CoopNetAPI::SetConfigValue<std::string>(key, value);
    }

    int CoopNet_GetConfigInt(const char* key, int defaultValue) {
        if (!key) return defaultValue;
        return CoopNet::CoopNetAPI::GetConfigValue<int>(key, defaultValue);
    }

    bool CoopNet_SetConfigInt(const char* key, int value) {
        if (!key) return false;
        return CoopNet::CoopNetAPI::SetConfigValue<int>(key, value);
    }

    float CoopNet_GetConfigFloat(const char* key, float defaultValue) {
        if (!key) return defaultValue;
        return CoopNet::CoopNetAPI::GetConfigValue<float>(key, defaultValue);
    }

    bool CoopNet_SetConfigFloat(const char* key, float value) {
        if (!key) return false;
        return CoopNet::CoopNetAPI::SetConfigValue<float>(key, value);
    }

    bool CoopNet_GetConfigBool(const char* key, bool defaultValue) {
        if (!key) return defaultValue;
        return CoopNet::CoopNetAPI::GetConfigValue<bool>(key, defaultValue);
    }

    bool CoopNet_SetConfigBool(const char* key, bool value) {
        if (!key) return false;
        return CoopNet::CoopNetAPI::SetConfigValue<bool>(key, value);
    }

    void CoopNet_FreeString(const char* str) {
        // In this implementation, strings are static, so no need to free
        // In a more complex implementation, this would free dynamically allocated strings
    }
}
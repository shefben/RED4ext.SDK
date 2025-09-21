#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
#include "../core/CoopNetCore.hpp"

namespace CoopNet {
namespace Scripting {

// REDscript class definitions for CoopNet integration

// Base CoopNet manager class
struct CoopNetManager : RED4ext::IScriptable {
    RED4EXT_RTTI_IMPL(CoopNetManager, RED4ext::IScriptable);

public:
    // Core system management
    RED4EXT_METHOD(Initialize);
    RED4EXT_METHOD(Shutdown);
    RED4EXT_METHOD(IsReady);
    RED4EXT_METHOD(GetSystemStatus);
    RED4EXT_METHOD(PerformDiagnostics);

    // Configuration management
    RED4EXT_METHOD(LoadConfiguration);
    RED4EXT_METHOD(SaveConfiguration);
    RED4EXT_METHOD(GetConfigString);
    RED4EXT_METHOD(SetConfigString);
    RED4EXT_METHOD(GetConfigInt);
    RED4EXT_METHOD(SetConfigInt);
    RED4EXT_METHOD(GetConfigFloat);
    RED4EXT_METHOD(SetConfigFloat);
    RED4EXT_METHOD(GetConfigBool);
    RED4EXT_METHOD(SetConfigBool);

    // Error reporting
    RED4EXT_METHOD(ReportError);
    RED4EXT_METHOD(ReportCriticalError);

    // Event system
    RED4EXT_METHOD(SendEvent);
    RED4EXT_METHOD(RegisterEventCallback);
    RED4EXT_METHOD(UnregisterEventCallback);

    // Game integration hooks
    RED4EXT_METHOD(OnGameStart);
    RED4EXT_METHOD(OnGameStop);
    RED4EXT_METHOD(OnPlayerConnect);
    RED4EXT_METHOD(OnPlayerDisconnect);

    // Static instance accessor
    static CoopNetManager* GetInstance();

private:
    static CoopNetManager* s_instance;
    std::unordered_map<std::string, RED4ext::Handle<RED4ext::IScriptable>> m_eventCallbacks;
};

// Voice communication system bindings
struct CoopNetVoiceSystem : RED4ext::IScriptable {
    RED4EXT_RTTI_IMPL(CoopNetVoiceSystem, RED4ext::IScriptable);

public:
    // Voice system control
    RED4EXT_METHOD(IsEnabled);
    RED4EXT_METHOD(SetEnabled);
    RED4EXT_METHOD(IsInitialized);

    // Voice channels
    RED4EXT_METHOD(CreateChannel);
    RED4EXT_METHOD(JoinChannel);
    RED4EXT_METHOD(LeaveChannel);
    RED4EXT_METHOD(GetActiveChannels);
    RED4EXT_METHOD(SetChannelVolume);

    // Voice settings
    RED4EXT_METHOD(SetVoiceQuality);
    RED4EXT_METHOD(GetVoiceQuality);
    RED4EXT_METHOD(SetSpatialAudio);
    RED4EXT_METHOD(IsSpatialAudioEnabled);
    RED4EXT_METHOD(SetNoiseSuppression);
    RED4EXT_METHOD(IsNoiseSuppressionEnabled);

    // Push-to-talk
    RED4EXT_METHOD(SetPushToTalkKey);
    RED4EXT_METHOD(SetPushToTalkEnabled);
    RED4EXT_METHOD(IsPushToTalkEnabled);
    RED4EXT_METHOD(SetVoiceActivation);
    RED4EXT_METHOD(IsVoiceActivationEnabled);

    // Player voice management
    RED4EXT_METHOD(MutePlayer);
    RED4EXT_METHOD(UnmutePlayer);
    RED4EXT_METHOD(IsPlayerMuted);
    RED4EXT_METHOD(SetPlayerVolume);
    RED4EXT_METHOD(GetPlayerVolume);

    // Statistics
    RED4EXT_METHOD(GetVoiceStatistics);
    RED4EXT_METHOD(GetConnectionQuality);

    static CoopNetVoiceSystem* GetInstance();

private:
    static CoopNetVoiceSystem* s_instance;
};

// Performance monitoring system bindings
struct CoopNetPerformanceMonitor : RED4ext::IScriptable {
    RED4EXT_RTTI_IMPL(CoopNetPerformanceMonitor, RED4ext::IScriptable);

public:
    // Monitoring control
    RED4EXT_METHOD(IsEnabled);
    RED4EXT_METHOD(SetEnabled);
    RED4EXT_METHOD(StartMonitoring);
    RED4EXT_METHOD(StopMonitoring);

    // Performance metrics
    RED4EXT_METHOD(GetCurrentFPS);
    RED4EXT_METHOD(GetAverageFPS);
    RED4EXT_METHOD(GetFrameTime);
    RED4EXT_METHOD(GetCPUUsage);
    RED4EXT_METHOD(GetMemoryUsage);
    RED4EXT_METHOD(GetGPUUsage);

    // Performance optimization
    RED4EXT_METHOD(EnableAdaptiveQuality);
    RED4EXT_METHOD(IsAdaptiveQualityEnabled);
    RED4EXT_METHOD(SetPerformanceTarget);
    RED4EXT_METHOD(GetPerformanceTarget);

    // Alerts and thresholds
    RED4EXT_METHOD(SetFPSThreshold);
    RED4EXT_METHOD(SetMemoryThreshold);
    RED4EXT_METHOD(SetCPUThreshold);
    RED4EXT_METHOD(GetActiveAlerts);

    // Statistics
    RED4EXT_METHOD(GetPerformanceReport);
    RED4EXT_METHOD(ResetStatistics);

    static CoopNetPerformanceMonitor* GetInstance();

private:
    static CoopNetPerformanceMonitor* s_instance;
};

// Network optimization system bindings
struct CoopNetNetworkOptimizer : RED4ext::IScriptable {
    RED4EXT_RTTI_IMPL(CoopNetNetworkOptimizer, RED4ext::IScriptable);

public:
    // Network optimization control
    RED4EXT_METHOD(IsEnabled);
    RED4EXT_METHOD(SetEnabled);
    RED4EXT_METHOD(IsOptimizationActive);

    // Compression settings
    RED4EXT_METHOD(SetCompressionEnabled);
    RED4EXT_METHOD(IsCompressionEnabled);
    RED4EXT_METHOD(SetCompressionLevel);
    RED4EXT_METHOD(GetCompressionLevel);

    // Bandwidth management
    RED4EXT_METHOD(SetMaxBandwidth);
    RED4EXT_METHOD(GetMaxBandwidth);
    RED4EXT_METHOD(GetCurrentBandwidthUsage);
    RED4EXT_METHOD(SetAdaptiveBitrate);
    RED4EXT_METHOD(IsAdaptiveBitrateEnabled);

    // Packet prioritization
    RED4EXT_METHOD(SetPacketPrioritization);
    RED4EXT_METHOD(IsPacketPrioritizationEnabled);

    // Network statistics
    RED4EXT_METHOD(GetNetworkStatistics);
    RED4EXT_METHOD(GetLatency);
    RED4EXT_METHOD(GetPacketLoss);
    RED4EXT_METHOD(GetBandwidthUtilization);

    static CoopNetNetworkOptimizer* GetInstance();

private:
    static CoopNetNetworkOptimizer* s_instance;
};

// Content management system bindings
struct CoopNetContentManager : RED4ext::IScriptable {
    RED4EXT_RTTI_IMPL(CoopNetContentManager, RED4ext::IScriptable);

public:
    // Content loading
    RED4EXT_METHOD(LoadContent);
    RED4EXT_METHOD(LoadContentAsync);
    RED4EXT_METHOD(UnloadContent);
    RED4EXT_METHOD(ReloadContent);
    RED4EXT_METHOD(IsContentLoaded);
    RED4EXT_METHOD(GetContentState);

    // Content discovery
    RED4EXT_METHOD(SearchContent);
    RED4EXT_METHOD(GetContentByType);
    RED4EXT_METHOD(GetLoadedContent);

    // Package management
    RED4EXT_METHOD(InstallPackage);
    RED4EXT_METHOD(UninstallPackage);
    RED4EXT_METHOD(UpdatePackage);
    RED4EXT_METHOD(GetInstalledPackages);

    // Cache management
    RED4EXT_METHOD(GetCacheUsage);
    RED4EXT_METHOD(GetCacheSize);
    RED4EXT_METHOD(ClearCache);
    RED4EXT_METHOD(SetCacheSize);

    // Content streaming
    RED4EXT_METHOD(StartContentStream);
    RED4EXT_METHOD(GetStreamProgress);
    RED4EXT_METHOD(CancelContentStream);

    static CoopNetContentManager* GetInstance();

private:
    static CoopNetContentManager* s_instance;
};

// Database integration system bindings
struct CoopNetDatabase : RED4ext::IScriptable {
    RED4EXT_RTTI_IMPL(CoopNetDatabase, RED4ext::IScriptable);

public:
    // Connection management
    RED4EXT_METHOD(IsConnected);
    RED4EXT_METHOD(Connect);
    RED4EXT_METHOD(Disconnect);

    // Query execution
    RED4EXT_METHOD(ExecuteQuery);
    RED4EXT_METHOD(ExecuteQueryAsync);

    // Transaction management
    RED4EXT_METHOD(BeginTransaction);
    RED4EXT_METHOD(CommitTransaction);
    RED4EXT_METHOD(RollbackTransaction);
    RED4EXT_METHOD(IsInTransaction);

    // Data operations
    RED4EXT_METHOD(Insert);
    RED4EXT_METHOD(Update);
    RED4EXT_METHOD(Delete);
    RED4EXT_METHOD(Select);

    // Backup and restore
    RED4EXT_METHOD(CreateBackup);
    RED4EXT_METHOD(RestoreBackup);
    RED4EXT_METHOD(GetBackupList);

    static CoopNetDatabase* GetInstance();

private:
    static CoopNetDatabase* s_instance;
};

// Event callback interface for REDscript
struct ICoopNetEventCallback : RED4ext::IScriptable {
    RED4EXT_RTTI_IMPL(ICoopNetEventCallback, RED4ext::IScriptable);

public:
    // Event callback method that REDscript classes can override
    RED4EXT_METHOD(OnEvent);
};

// Data structures for REDscript integration
struct CoopNetSystemStatus : RED4ext::IScriptable {
    RED4EXT_RTTI_IMPL(CoopNetSystemStatus, RED4ext::IScriptable);

public:
    RED4ext::CString systemName;
    RED4ext::CString status;
    bool isHealthy;
    bool isEnabled;
    float cpuUsage;
    uint64_t memoryUsage;
    uint32_t errorCount;
};

struct CoopNetVoiceStatistics : RED4ext::IScriptable {
    RED4EXT_RTTI_IMPL(CoopNetVoiceStatistics, RED4ext::IScriptable);

public:
    uint32_t activeChannels;
    uint32_t connectedPlayers;
    float audioQuality;
    float latency;
    float packetLoss;
    uint64_t packetsProcessed;
    uint64_t droppedPackets;
};

struct CoopNetPerformanceMetrics : RED4ext::IScriptable {
    RED4EXT_RTTI_IMPL(CoopNetPerformanceMetrics, RED4ext::IScriptable);

public:
    float currentFPS;
    float averageFPS;
    float frameTime;
    float cpuUsage;
    float memoryUsage;
    float gpuUsage;
    uint32_t drawCalls;
    uint32_t triangles;
};

struct CoopNetNetworkStatistics : RED4ext::IScriptable {
    RED4EXT_RTTI_IMPL(CoopNetNetworkStatistics, RED4ext::IScriptable);

public:
    uint64_t bytesSent;
    uint64_t bytesReceived;
    uint64_t packetsSent;
    uint64_t packetsReceived;
    uint64_t droppedPackets;
    float latency;
    float packetLoss;
    float bandwidthUtilization;
    float compressionRatio;
};

// Function registration helper
void RegisterCoopNetScriptBindings();

// Method implementation helpers
template<typename T>
void RegisterScriptClass();

template<typename T, typename RetType, typename... Args>
void RegisterScriptMethod(const char* className, const char* methodName, RetType(T::*method)(Args...));

// Conversion utilities between C++ and REDscript types
namespace TypeConversion {
    RED4ext::CString StdStringToCString(const std::string& str);
    std::string CStringToStdString(const RED4ext::CString& cstr);

    RED4ext::Handle<CoopNetSystemStatus> CreateSystemStatus(const std::string& name, const std::string& status, bool healthy);
    RED4ext::Handle<CoopNetVoiceStatistics> CreateVoiceStatistics(const VoiceStatistics& stats);
    RED4ext::Handle<CoopNetPerformanceMetrics> CreatePerformanceMetrics(const PerformanceMetrics& metrics);
    RED4ext::Handle<CoopNetNetworkStatistics> CreateNetworkStatistics(const NetworkStatistics& stats);
}

} // namespace Scripting
} // namespace CoopNet
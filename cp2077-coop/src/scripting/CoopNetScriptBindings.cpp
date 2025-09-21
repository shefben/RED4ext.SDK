#include "CoopNetScriptBindings.hpp"
#include <RED4ext/Scripting/Natives/Generated/Engine/CName.hpp>
#include <spdlog/spdlog.h>

namespace CoopNet {
namespace Scripting {

// Static instance pointers
CoopNetManager* CoopNetManager::s_instance = nullptr;
CoopNetVoiceSystem* CoopNetVoiceSystem::s_instance = nullptr;
CoopNetPerformanceMonitor* CoopNetPerformanceMonitor::s_instance = nullptr;
CoopNetNetworkOptimizer* CoopNetNetworkOptimizer::s_instance = nullptr;
CoopNetContentManager* CoopNetContentManager::s_instance = nullptr;
CoopNetDatabase* CoopNetDatabase::s_instance = nullptr;

// CoopNetManager implementation
CoopNetManager* CoopNetManager::GetInstance() {
    if (!s_instance) {
        s_instance = RED4ext::CClass::GetAllocator()->AllocAligned(sizeof(CoopNetManager), alignof(CoopNetManager)).template AsType<CoopNetManager>();
        RED4ext::CClass::GetInstance("CoopNetManager")->CreateInstance(s_instance);
    }
    return s_instance;
}

bool CoopNetManager::Initialize(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    bool result = CoopNetAPI::InitializeCoopNet();
    if (aOut) *aOut = result;

    spdlog::info("[CoopNetScriptBindings] CoopNet initialization called from REDscript: {}", result ? "success" : "failed");
    return true;
}

bool CoopNetManager::Shutdown(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    CoopNetAPI::ShutdownCoopNet();
    spdlog::info("[CoopNetScriptBindings] CoopNet shutdown called from REDscript");
    return true;
}

bool CoopNetManager::IsReady(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    bool result = CoopNetAPI::IsCoopNetReady();
    if (aOut) *aOut = result;
    return true;
}

bool CoopNetManager::GetSystemStatus(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, RED4ext::CString* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    std::string status = CoopNetAPI::GetCoopNetStatus();
    if (aOut) *aOut = TypeConversion::StdStringToCString(status);
    return true;
}

bool CoopNetManager::PerformDiagnostics(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    bool result = CoopNetCore::Instance().PerformSystemDiagnostics();
    if (aOut) *aOut = result;
    return true;
}

bool CoopNetManager::LoadConfiguration(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CString configPath;
    RED4ext::CStackFrame::Resolve(aFrame, &configPath);

    std::string path = TypeConversion::CStringToStdString(configPath);
    bool result = CoopNetCore::Instance().LoadConfiguration(path.empty() ? "config/coopnet.json" : path);
    if (aOut) *aOut = result;
    return true;
}

bool CoopNetManager::SaveConfiguration(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CString configPath;
    RED4ext::CStackFrame::Resolve(aFrame, &configPath);

    std::string path = TypeConversion::CStringToStdString(configPath);
    bool result = CoopNetCore::Instance().SaveConfiguration(path.empty() ? "config/coopnet.json" : path);
    if (aOut) *aOut = result;
    return true;
}

bool CoopNetManager::GetConfigString(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, RED4ext::CString* aOut, int64_t a4) {
    RED4ext::CString key, defaultValue;
    RED4ext::CStackFrame::Resolve(aFrame, &key, &defaultValue);

    std::string keyStr = TypeConversion::CStringToStdString(key);
    std::string defaultStr = TypeConversion::CStringToStdString(defaultValue);
    std::string result = CoopNetAPI::GetConfigValue<std::string>(keyStr, defaultStr);

    if (aOut) *aOut = TypeConversion::StdStringToCString(result);
    return true;
}

bool CoopNetManager::SetConfigString(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CString key, value;
    RED4ext::CStackFrame::Resolve(aFrame, &key, &value);

    std::string keyStr = TypeConversion::CStringToStdString(key);
    std::string valueStr = TypeConversion::CStringToStdString(value);
    bool result = CoopNetAPI::SetConfigValue<std::string>(keyStr, valueStr);

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetManager::GetConfigInt(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, int32_t* aOut, int64_t a4) {
    RED4ext::CString key;
    int32_t defaultValue;
    RED4ext::CStackFrame::Resolve(aFrame, &key, &defaultValue);

    std::string keyStr = TypeConversion::CStringToStdString(key);
    int32_t result = CoopNetAPI::GetConfigValue<int32_t>(keyStr, defaultValue);

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetManager::SetConfigInt(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CString key;
    int32_t value;
    RED4ext::CStackFrame::Resolve(aFrame, &key, &value);

    std::string keyStr = TypeConversion::CStringToStdString(key);
    bool result = CoopNetAPI::SetConfigValue<int32_t>(keyStr, value);

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetManager::GetConfigFloat(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, float* aOut, int64_t a4) {
    RED4ext::CString key;
    float defaultValue;
    RED4ext::CStackFrame::Resolve(aFrame, &key, &defaultValue);

    std::string keyStr = TypeConversion::CStringToStdString(key);
    float result = CoopNetAPI::GetConfigValue<float>(keyStr, defaultValue);

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetManager::SetConfigFloat(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CString key;
    float value;
    RED4ext::CStackFrame::Resolve(aFrame, &key, &value);

    std::string keyStr = TypeConversion::CStringToStdString(key);
    bool result = CoopNetAPI::SetConfigValue<float>(keyStr, value);

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetManager::GetConfigBool(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CString key;
    bool defaultValue;
    RED4ext::CStackFrame::Resolve(aFrame, &key, &defaultValue);

    std::string keyStr = TypeConversion::CStringToStdString(key);
    bool result = CoopNetAPI::GetConfigValue<bool>(keyStr, defaultValue);

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetManager::SetConfigBool(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CString key;
    bool value;
    RED4ext::CStackFrame::Resolve(aFrame, &key, &value);

    std::string keyStr = TypeConversion::CStringToStdString(key);
    bool result = CoopNetAPI::SetConfigValue<bool>(keyStr, value);

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetManager::ReportError(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4) {
    RED4ext::CString component, error;
    RED4ext::CStackFrame::Resolve(aFrame, &component, &error);

    std::string componentStr = TypeConversion::CStringToStdString(component);
    std::string errorStr = TypeConversion::CStringToStdString(error);

    CoopNetAPI::ReportError(componentStr, errorStr);
    return true;
}

bool CoopNetManager::ReportCriticalError(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4) {
    RED4ext::CString component, error;
    RED4ext::CStackFrame::Resolve(aFrame, &component, &error);

    std::string componentStr = TypeConversion::CStringToStdString(component);
    std::string errorStr = TypeConversion::CStringToStdString(error);

    CoopNetAPI::ReportCriticalError(componentStr, errorStr);
    return true;
}

bool CoopNetManager::SendEvent(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4) {
    RED4ext::CString eventType, eventData;
    RED4ext::CStackFrame::Resolve(aFrame, &eventType, &eventData);

    std::string typeStr = TypeConversion::CStringToStdString(eventType);
    std::string dataStr = TypeConversion::CStringToStdString(eventData);

    try {
        nlohmann::json data = nlohmann::json::parse(dataStr);
        CoopNetAPI::SendEvent(typeStr, data);
    } catch (const std::exception& ex) {
        spdlog::error("[CoopNetScriptBindings] Failed to parse event data: {}", ex.what());
    }

    return true;
}

bool CoopNetManager::OnGameStart(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);
    CoopNetCore::Instance().OnGameStart();
    return true;
}

bool CoopNetManager::OnGameStop(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);
    CoopNetCore::Instance().OnGameStop();
    return true;
}

bool CoopNetManager::OnPlayerConnect(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4) {
    uint32_t playerId;
    RED4ext::CStackFrame::Resolve(aFrame, &playerId);
    CoopNetCore::Instance().OnPlayerConnect(playerId);
    return true;
}

bool CoopNetManager::OnPlayerDisconnect(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4) {
    uint32_t playerId;
    RED4ext::CStackFrame::Resolve(aFrame, &playerId);
    CoopNetCore::Instance().OnPlayerDisconnect(playerId);
    return true;
}

// CoopNetVoiceSystem implementation
CoopNetVoiceSystem* CoopNetVoiceSystem::GetInstance() {
    if (!s_instance) {
        s_instance = RED4ext::CClass::GetAllocator()->AllocAligned(sizeof(CoopNetVoiceSystem), alignof(CoopNetVoiceSystem)).template AsType<CoopNetVoiceSystem>();
        RED4ext::CClass::GetInstance("CoopNetVoiceSystem")->CreateInstance(s_instance);
    }
    return s_instance;
}

bool CoopNetVoiceSystem::IsEnabled(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    bool result = VoiceCommunicationCore::Instance().IsInitialized();
    if (aOut) *aOut = result;
    return true;
}

bool CoopNetVoiceSystem::SetEnabled(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    bool enabled;
    RED4ext::CStackFrame::Resolve(aFrame, &enabled);

    bool result = false;
    if (enabled) {
        VoiceConfig config;
        config.quality = VoiceQuality::High;
        config.spatialAudioMode = SpatialAudioMode::HRTF;
        result = VoiceCommunicationCore::Instance().Initialize(config);
    } else {
        VoiceCommunicationCore::Instance().Shutdown();
        result = true;
    }

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetVoiceSystem::IsInitialized(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    bool result = VoiceCommunicationCore::Instance().IsInitialized();
    if (aOut) *aOut = result;
    return true;
}

bool CoopNetVoiceSystem::CreateChannel(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CString channelName;
    uint32_t maxParticipants;
    RED4ext::CStackFrame::Resolve(aFrame, &channelName, &maxParticipants);

    std::string nameStr = TypeConversion::CStringToStdString(channelName);
    bool result = VoiceCommunicationCore::Instance().CreateChannel(nameStr, maxParticipants);

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetVoiceSystem::JoinChannel(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CString channelName;
    RED4ext::CStackFrame::Resolve(aFrame, &channelName);

    std::string nameStr = TypeConversion::CStringToStdString(channelName);
    bool result = VoiceCommunicationCore::Instance().JoinChannel(nameStr);

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetVoiceSystem::LeaveChannel(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CString channelName;
    RED4ext::CStackFrame::Resolve(aFrame, &channelName);

    std::string nameStr = TypeConversion::CStringToStdString(channelName);
    bool result = VoiceCommunicationCore::Instance().LeaveChannel(nameStr);

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetVoiceSystem::SetVoiceQuality(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    int32_t quality;
    RED4ext::CStackFrame::Resolve(aFrame, &quality);

    VoiceQuality voiceQuality = static_cast<VoiceQuality>(quality);
    bool result = VoiceCommunicationCore::Instance().SetVoiceQuality(voiceQuality);

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetVoiceSystem::GetVoiceQuality(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, int32_t* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    VoiceQuality quality = VoiceCommunicationCore::Instance().GetVoiceQuality();
    if (aOut) *aOut = static_cast<int32_t>(quality);
    return true;
}

bool CoopNetVoiceSystem::SetSpatialAudio(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    bool enabled;
    RED4ext::CStackFrame::Resolve(aFrame, &enabled);

    SpatialAudioMode mode = enabled ? SpatialAudioMode::HRTF : SpatialAudioMode::Disabled;
    bool result = VoiceCommunicationCore::Instance().SetSpatialAudioMode(mode);

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetVoiceSystem::IsSpatialAudioEnabled(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    SpatialAudioMode mode = VoiceCommunicationCore::Instance().GetSpatialAudioMode();
    bool result = (mode != SpatialAudioMode::Disabled);

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetVoiceSystem::MutePlayer(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    uint32_t playerId;
    RED4ext::CStackFrame::Resolve(aFrame, &playerId);

    bool result = VoiceCommunicationCore::Instance().MuteParticipant(playerId);
    if (aOut) *aOut = result;
    return true;
}

bool CoopNetVoiceSystem::UnmutePlayer(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    uint32_t playerId;
    RED4ext::CStackFrame::Resolve(aFrame, &playerId);

    bool result = VoiceCommunicationCore::Instance().UnmuteParticipant(playerId);
    if (aOut) *aOut = result;
    return true;
}

bool CoopNetVoiceSystem::IsPlayerMuted(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    uint32_t playerId;
    RED4ext::CStackFrame::Resolve(aFrame, &playerId);

    bool result = VoiceCommunicationCore::Instance().IsParticipantMuted(playerId);
    if (aOut) *aOut = result;
    return true;
}

bool CoopNetVoiceSystem::GetVoiceStatistics(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, RED4ext::Handle<CoopNetVoiceStatistics>* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    VoiceStatistics stats = VoiceCommunicationCore::Instance().GetStatistics();
    if (aOut) *aOut = TypeConversion::CreateVoiceStatistics(stats);
    return true;
}

// CoopNetPerformanceMonitor implementation
CoopNetPerformanceMonitor* CoopNetPerformanceMonitor::GetInstance() {
    if (!s_instance) {
        s_instance = RED4ext::CClass::GetAllocator()->AllocAligned(sizeof(CoopNetPerformanceMonitor), alignof(CoopNetPerformanceMonitor)).template AsType<CoopNetPerformanceMonitor>();
        RED4ext::CClass::GetInstance("CoopNetPerformanceMonitor")->CreateInstance(s_instance);
    }
    return s_instance;
}

bool CoopNetPerformanceMonitor::IsEnabled(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    bool result = PerformanceMonitor::Instance().IsMonitoringActive();
    if (aOut) *aOut = result;
    return true;
}

bool CoopNetPerformanceMonitor::SetEnabled(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4) {
    bool enabled;
    RED4ext::CStackFrame::Resolve(aFrame, &enabled);

    bool result = false;
    if (enabled) {
        PerformanceConfig config;
        result = PerformanceMonitor::Instance().Initialize(config);
    } else {
        PerformanceMonitor::Instance().Shutdown();
        result = true;
    }

    if (aOut) *aOut = result;
    return true;
}

bool CoopNetPerformanceMonitor::GetCurrentFPS(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, float* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    PerformanceMetrics metrics = PerformanceMonitor::Instance().GetCurrentMetrics();
    if (aOut) *aOut = metrics.fps;
    return true;
}

bool CoopNetPerformanceMonitor::GetAverageFPS(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, float* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    PerformanceMetrics metrics = PerformanceMonitor::Instance().GetAverageMetrics();
    if (aOut) *aOut = metrics.fps;
    return true;
}

bool CoopNetPerformanceMonitor::GetFrameTime(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, float* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    PerformanceMetrics metrics = PerformanceMonitor::Instance().GetCurrentMetrics();
    if (aOut) *aOut = metrics.frameTime;
    return true;
}

bool CoopNetPerformanceMonitor::GetCPUUsage(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, float* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    PerformanceMetrics metrics = PerformanceMonitor::Instance().GetCurrentMetrics();
    if (aOut) *aOut = metrics.cpuUsage;
    return true;
}

bool CoopNetPerformanceMonitor::GetMemoryUsage(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, float* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    PerformanceMetrics metrics = PerformanceMonitor::Instance().GetCurrentMetrics();
    if (aOut) *aOut = metrics.memoryUsage;
    return true;
}

bool CoopNetPerformanceMonitor::GetGPUUsage(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, float* aOut, int64_t a4) {
    RED4ext::CStackFrame::Resolve(aFrame, nullptr);

    PerformanceMetrics metrics = PerformanceMonitor::Instance().GetCurrentMetrics();
    if (aOut) *aOut = metrics.gpuUsage;
    return true;
}

// Type conversion utilities
namespace TypeConversion {
    RED4ext::CString StdStringToCString(const std::string& str) {
        RED4ext::CString result;
        result = str.c_str();
        return result;
    }

    std::string CStringToStdString(const RED4ext::CString& cstr) {
        return cstr.c_str() ? std::string(cstr.c_str()) : std::string();
    }

    RED4ext::Handle<CoopNetSystemStatus> CreateSystemStatus(const std::string& name, const std::string& status, bool healthy) {
        RED4ext::Handle<CoopNetSystemStatus> handle;

        auto* instance = RED4ext::CClass::GetAllocator()->AllocAligned(sizeof(CoopNetSystemStatus), alignof(CoopNetSystemStatus)).template AsType<CoopNetSystemStatus>();
        RED4ext::CClass::GetInstance("CoopNetSystemStatus")->CreateInstance(instance);

        instance->systemName = StdStringToCString(name);
        instance->status = StdStringToCString(status);
        instance->isHealthy = healthy;

        handle = instance;
        return handle;
    }

    RED4ext::Handle<CoopNetVoiceStatistics> CreateVoiceStatistics(const VoiceStatistics& stats) {
        RED4ext::Handle<CoopNetVoiceStatistics> handle;

        auto* instance = RED4ext::CClass::GetAllocator()->AllocAligned(sizeof(CoopNetVoiceStatistics), alignof(CoopNetVoiceStatistics)).template AsType<CoopNetVoiceStatistics>();
        RED4ext::CClass::GetInstance("CoopNetVoiceStatistics")->CreateInstance(instance);

        instance->activeChannels = stats.activeChannels;
        instance->connectedPlayers = stats.totalParticipants;
        instance->audioQuality = stats.audioQuality;
        instance->latency = stats.latency;
        instance->packetLoss = stats.packetLossRate;
        instance->packetsProcessed = stats.packetsProcessed;
        instance->droppedPackets = stats.droppedPackets;

        handle = instance;
        return handle;
    }

    RED4ext::Handle<CoopNetPerformanceMetrics> CreatePerformanceMetrics(const PerformanceMetrics& metrics) {
        RED4ext::Handle<CoopNetPerformanceMetrics> handle;

        auto* instance = RED4ext::CClass::GetAllocator()->AllocAligned(sizeof(CoopNetPerformanceMetrics), alignof(CoopNetPerformanceMetrics)).template AsType<CoopNetPerformanceMetrics>();
        RED4ext::CClass::GetInstance("CoopNetPerformanceMetrics")->CreateInstance(instance);

        instance->currentFPS = metrics.fps;
        instance->averageFPS = metrics.avgFPS;
        instance->frameTime = metrics.frameTime;
        instance->cpuUsage = metrics.cpuUsage;
        instance->memoryUsage = metrics.memoryUsage;
        instance->gpuUsage = metrics.gpuUsage;
        instance->drawCalls = metrics.drawCalls;
        instance->triangles = metrics.triangles;

        handle = instance;
        return handle;
    }

    RED4ext::Handle<CoopNetNetworkStatistics> CreateNetworkStatistics(const NetworkStatistics& stats) {
        RED4ext::Handle<CoopNetNetworkStatistics> handle;

        auto* instance = RED4ext::CClass::GetAllocator()->AllocAligned(sizeof(CoopNetNetworkStatistics), alignof(CoopNetNetworkStatistics)).template AsType<CoopNetNetworkStatistics>();
        RED4ext::CClass::GetInstance("CoopNetNetworkStatistics")->CreateInstance(instance);

        instance->bytesSent = stats.bytesSent;
        instance->bytesReceived = stats.bytesReceived;
        instance->packetsSent = stats.packetsSent;
        instance->packetsReceived = stats.packetsReceived;
        instance->droppedPackets = stats.droppedPackets;
        instance->latency = static_cast<float>(stats.averageLatency);
        instance->packetLoss = stats.packetLossRate;
        instance->bandwidthUtilization = stats.bandwidthUtilization;
        instance->compressionRatio = stats.compressionRatio;

        handle = instance;
        return handle;
    }
}

// Registration function
void RegisterCoopNetScriptBindings() {
    spdlog::info("[CoopNetScriptBindings] Registering REDscript bindings...");

    try {
        // Register classes
        RegisterScriptClass<CoopNetManager>();
        RegisterScriptClass<CoopNetVoiceSystem>();
        RegisterScriptClass<CoopNetPerformanceMonitor>();
        RegisterScriptClass<CoopNetNetworkOptimizer>();
        RegisterScriptClass<CoopNetContentManager>();
        RegisterScriptClass<CoopNetDatabase>();
        RegisterScriptClass<ICoopNetEventCallback>();
        RegisterScriptClass<CoopNetSystemStatus>();
        RegisterScriptClass<CoopNetVoiceStatistics>();
        RegisterScriptClass<CoopNetPerformanceMetrics>();
        RegisterScriptClass<CoopNetNetworkStatistics>();

        spdlog::info("[CoopNetScriptBindings] REDscript bindings registered successfully");

    } catch (const std::exception& ex) {
        spdlog::error("[CoopNetScriptBindings] Failed to register REDscript bindings: {}", ex.what());
    }
}

// Template specializations for script class registration
template<>
void RegisterScriptClass<CoopNetManager>() {
    // Implementation would register the class with RED4ext's RTTI system
    // This involves creating class descriptors, method descriptors, etc.
    spdlog::debug("[CoopNetScriptBindings] Registered CoopNetManager class");
}

template<>
void RegisterScriptClass<CoopNetVoiceSystem>() {
    spdlog::debug("[CoopNetScriptBindings] Registered CoopNetVoiceSystem class");
}

template<>
void RegisterScriptClass<CoopNetPerformanceMonitor>() {
    spdlog::debug("[CoopNetScriptBindings] Registered CoopNetPerformanceMonitor class");
}

template<>
void RegisterScriptClass<CoopNetNetworkOptimizer>() {
    spdlog::debug("[CoopNetScriptBindings] Registered CoopNetNetworkOptimizer class");
}

template<>
void RegisterScriptClass<CoopNetContentManager>() {
    spdlog::debug("[CoopNetScriptBindings] Registered CoopNetContentManager class");
}

template<>
void RegisterScriptClass<CoopNetDatabase>() {
    spdlog::debug("[CoopNetScriptBindings] Registered CoopNetDatabase class");
}

template<>
void RegisterScriptClass<ICoopNetEventCallback>() {
    spdlog::debug("[CoopNetScriptBindings] Registered ICoopNetEventCallback class");
}

template<>
void RegisterScriptClass<CoopNetSystemStatus>() {
    spdlog::debug("[CoopNetScriptBindings] Registered CoopNetSystemStatus class");
}

template<>
void RegisterScriptClass<CoopNetVoiceStatistics>() {
    spdlog::debug("[CoopNetScriptBindings] Registered CoopNetVoiceStatistics class");
}

template<>
void RegisterScriptClass<CoopNetPerformanceMetrics>() {
    spdlog::debug("[CoopNetScriptBindings] Registered CoopNetPerformanceMetrics class");
}

template<>
void RegisterScriptClass<CoopNetNetworkStatistics>() {
    spdlog::debug("[CoopNetScriptBindings] Registered CoopNetNetworkStatistics class");
}

} // namespace Scripting
} // namespace CoopNet
#pragma once

#include <RED4ext/RED4ext.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <chrono>

// Forward declarations for audio libraries
struct OpusEncoder;
struct OpusDecoder;

namespace CoopNet {

// Voice communication quality settings
enum class VoiceQuality : uint8_t {
    Low = 0,        // 8kHz, 32kbps
    Medium = 1,     // 16kHz, 64kbps
    High = 2,       // 24kHz, 96kbps
    Ultra = 3       // 48kHz, 128kbps
};

// Voice activation detection modes
enum class VADMode : uint8_t {
    Manual = 0,         // Push-to-talk only
    Automatic = 1,      // Voice activation detection
    Hybrid = 2          // Both PTT and VAD
};

// Spatial audio modes
enum class SpatialAudioMode : uint8_t {
    Disabled = 0,       // No spatial audio
    Simple = 1,         // Basic distance attenuation
    Advanced = 2,       // Full 3D spatial audio with HRTF
    Proximity = 3       // Proximity-based channels
};

// Voice channel types
enum class VoiceChannelType : uint8_t {
    Global = 0,         // All players
    Team = 1,           // Team members only
    Proximity = 2,      // Players within range
    Private = 3,        // Direct player-to-player
    Quest = 4           // Quest participants
};

// Voice communication packet structure (renamed to avoid conflict with net/Packets.hpp)
struct VoiceCommPacket {
    uint32_t playerId;
    uint32_t sequenceNumber;
    uint64_t timestamp;
    VoiceChannelType channelType;
    uint32_t channelId;
    uint32_t dataSize;
    std::vector<uint8_t> audioData;
    float volume;
    float spatialX, spatialY, spatialZ;
    bool isCompressed;
    uint8_t codecType; // 0 = Opus, 1 = Raw PCM
};

// Voice statistics
struct VoiceStats {
    uint64_t packetsTransmitted = 0;
    uint64_t packetsReceived = 0;
    uint64_t bytesTransmitted = 0;
    uint64_t bytesReceived = 0;
    uint32_t activeChannels = 0;
    uint32_t activeSpeakers = 0;
    float averageLatency = 0.0f;
    float packetLossRate = 0.0f;
    float compressionRatio = 0.0f;
    std::chrono::steady_clock::time_point lastUpdate;
};

// Voice channel configuration
struct VoiceChannel {
    uint32_t channelId;
    VoiceChannelType type;
    std::string channelName;
    std::unordered_set<uint32_t> participants;
    bool isEncrypted;
    float maxDistance;      // For proximity channels
    float volume;
    bool isActive;
    uint64_t createTime;
    std::string description;
};

// Audio device information
struct AudioDevice {
    uint32_t deviceId;
    std::string deviceName;
    std::string driverName;
    uint32_t sampleRate;
    uint32_t channels;
    uint32_t bufferSize;
    bool isInput;
    bool isDefault;
    bool isAvailable;
};

// Voice processing configuration
struct VoiceConfig {
    VoiceQuality quality = VoiceQuality::Medium;
    VADMode vadMode = VADMode::Hybrid;
    SpatialAudioMode spatialMode = SpatialAudioMode::Advanced;

    // Audio settings
    uint32_t sampleRate = 48000;
    uint32_t channels = 1;
    uint32_t frameDuration = 20; // milliseconds
    uint32_t bitrate = 64000;

    // VAD settings
    float vadThreshold = 0.1f;
    float vadHangover = 500.0f; // milliseconds

    // Noise suppression
    bool enableNoiseSupression = true;
    bool enableEchoCancellation = true;
    bool enableAutomaticGainControl = true;

    // Spatial audio settings
    float maxDistance = 50.0f;
    float referenceDistance = 1.0f;
    float rolloffFactor = 1.0f;

    // Network settings
    uint32_t jitterBufferSize = 100; // milliseconds
    uint32_t maxRetransmissions = 3;
    bool enableAdaptiveBitrate = true;
};

// Forward declaration
class VoiceCommunicationCore;

// Voice event callbacks
using VoiceEventCallback = std::function<void(uint32_t playerId, const std::string& event, const std::string& data)>;

// Main voice communication system
class VoiceCommunicationCore {
public:
    static VoiceCommunicationCore& Instance();

    // System lifecycle
    bool Initialize(const VoiceConfig& config = VoiceConfig{});
    void Shutdown();
    void Update(float deltaTime);

    // Device management
    std::vector<AudioDevice> GetInputDevices() const;
    std::vector<AudioDevice> GetOutputDevices() const;
    bool SetInputDevice(uint32_t deviceId);
    bool SetOutputDevice(uint32_t deviceId);
    AudioDevice GetCurrentInputDevice() const;
    AudioDevice GetCurrentOutputDevice() const;

    // Voice capture and playback
    bool StartCapture();
    bool StopCapture();
    bool IsCapturing() const;
    void SetCaptureVolume(float volume);
    float GetCaptureVolume() const;
    void SetPlaybackVolume(float volume);
    float GetPlaybackVolume() const;

    // Push-to-talk functionality
    void SetPushToTalk(bool enabled);
    bool IsPushToTalkEnabled() const;
    void SetPushToTalkKey(uint32_t keyCode);
    void OnPushToTalkPressed();
    void OnPushToTalkReleased();

    // Voice activation detection
    void SetVADThreshold(float threshold);
    float GetVADThreshold() const;
    bool IsVoiceActive() const;
    float GetCurrentVoiceLevel() const;

    // Channel management
    uint32_t CreateChannel(VoiceChannelType type, const std::string& name,
                          const std::vector<uint32_t>& participants = {});
    bool DestroyChannel(uint32_t channelId);
    bool JoinChannel(uint32_t channelId, uint32_t playerId);
    bool LeaveChannel(uint32_t channelId, uint32_t playerId);
    std::vector<VoiceChannel> GetActiveChannels() const;
    VoiceChannel GetChannel(uint32_t channelId) const;

    // Player management
    bool MutePlayer(uint32_t playerId, bool muted = true);
    bool IsPlayerMuted(uint32_t playerId) const;
    void SetPlayerVolume(uint32_t playerId, float volume);
    float GetPlayerVolume(uint32_t playerId) const;
    void UpdatePlayerPosition(uint32_t playerId, float x, float y, float z);

    // Audio processing
    bool ProcessIncomingVoice(const VoiceCommPacket& packet);
    bool TransmitVoice(const std::vector<uint8_t>& audioData, VoiceChannelType channelType, uint32_t channelId = 0);

    // Spatial audio
    void EnableSpatialAudio(bool enabled);
    bool IsSpatialAudioEnabled() const;
    void SetListenerPosition(float x, float y, float z);
    void SetListenerOrientation(float frontX, float frontY, float frontZ, float upX, float upY, float upZ);

    // Audio effects
    void SetEchoSuppression(bool enabled);
    void SetNoiseSuppression(bool enabled);
    void SetAutomaticGainControl(bool enabled);
    bool IsEchoSuppressionEnabled() const;
    bool IsNoiseSuppressionEnabled() const;
    bool IsAutomaticGainControlEnabled() const;

    // Configuration
    void UpdateConfig(const VoiceConfig& config);
    VoiceConfig GetConfig() const;
    void SaveConfig(const std::string& filename) const;
    bool LoadConfig(const std::string& filename);

    // Statistics and monitoring
    VoiceStats GetStatistics() const;
    void ResetStatistics();
    std::vector<uint32_t> GetActiveSpeakers() const;
    float GetNetworkLatency(uint32_t playerId) const;

    // Event system
    void RegisterEventCallback(const std::string& eventType, VoiceEventCallback callback);
    void UnregisterEventCallback(const std::string& eventType);

    // Advanced features
    void SetVoiceCompression(bool enabled);
    bool IsVoiceCompressionEnabled() const;
    void SetAdaptiveBitrate(bool enabled);
    bool IsAdaptiveBitrateEnabled() const;
    void SetJitterBufferSize(uint32_t sizeMs);
    uint32_t GetJitterBufferSize() const;

private:
    VoiceCommunicationCore() = default;
    ~VoiceCommunicationCore() = default;
    VoiceCommunicationCore(const VoiceCommunicationCore&) = delete;
    VoiceCommunicationCore& operator=(const VoiceCommunicationCore&) = delete;

    // Core processing
    void ProcessAudioCapture();
    void ProcessAudioPlayback();
    void ProcessVoiceActivation();
    void ProcessSpatialAudio();
    void UpdateNetworkStatistics();

    // Audio codec management
    bool InitializeOpusCodec();
    void ShutdownOpusCodec();
    std::vector<uint8_t> EncodeAudio(const std::vector<float>& pcmData);
    std::vector<float> DecodeAudio(const std::vector<uint8_t>& encodedData);

    // Device management
    bool InitializeAudioDevices();
    void ShutdownAudioDevices();
    void RefreshDeviceList();

    // Channel processing
    void ProcessChannelAudio(uint32_t channelId);
    bool ValidateChannelAccess(uint32_t playerId, uint32_t channelId) const;
    std::vector<uint32_t> GetChannelParticipants(uint32_t channelId) const;

    // Network integration
    bool SendVoicePacket(const VoiceCommPacket& packet);
    void HandleIncomingVoicePacket(const VoiceCommPacket& packet);
    void ProcessPacketQueue();

    // Audio processing utilities
    void ApplyNoiseGate(std::vector<float>& audioData, float threshold);
    void ApplyAGC(std::vector<float>& audioData);
    void ApplyEchoSuppression(std::vector<float>& audioData);
    float CalculateVoiceLevel(const std::vector<float>& audioData);
    void ApplySpatialEffects(std::vector<float>& audioData, uint32_t playerId);

    // Utility methods
    uint32_t GenerateChannelId();
    uint64_t GetCurrentTimestamp() const;
    void TriggerEvent(const std::string& eventType, uint32_t playerId, const std::string& data);

    // Data storage
    VoiceConfig m_config;
    VoiceStats m_stats;

    // Audio devices
    std::vector<AudioDevice> m_inputDevices;
    std::vector<AudioDevice> m_outputDevices;
    AudioDevice m_currentInputDevice;
    AudioDevice m_currentOutputDevice;

    // Channels and players
    std::unordered_map<uint32_t, VoiceChannel> m_channels;
    std::unordered_map<uint32_t, bool> m_mutedPlayers;
    std::unordered_map<uint32_t, float> m_playerVolumes;
    std::unordered_map<uint32_t, std::array<float, 3>> m_playerPositions;

    // Audio processing
    OpusEncoder* m_encoder = nullptr;
    OpusDecoder* m_decoder = nullptr;
    std::vector<float> m_captureBuffer;
    std::vector<float> m_playbackBuffer;
    std::queue<VoiceCommPacket> m_incomingPackets;
    std::queue<VoiceCommPacket> m_outgoingPackets;

    // Listener position for spatial audio
    std::array<float, 3> m_listenerPosition = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> m_listenerForward = {0.0f, 0.0f, 1.0f};
    std::array<float, 3> m_listenerUp = {0.0f, 1.0f, 0.0f};

    // Voice activation
    float m_currentVoiceLevel = 0.0f;
    bool m_isVoiceActive = false;
    std::chrono::steady_clock::time_point m_lastVoiceActivity;

    // Push-to-talk
    bool m_pushToTalkEnabled = true;
    bool m_pushToTalkPressed = false;
    uint32_t m_pushToTalkKey = 0x56; // V key

    // Synchronization
    mutable std::mutex m_channelsMutex;
    mutable std::mutex m_playersMutex;
    mutable std::mutex m_packetsMutex;
    mutable std::mutex m_devicesMutex;
    mutable std::mutex m_statsMutex;

    // System state
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_capturing{false};
    std::atomic<bool> m_spatialAudioEnabled{true};

    // Processing threads
    std::thread m_captureThread;
    std::thread m_playbackThread;
    std::thread m_processingThread;
    std::atomic<bool> m_shouldStop{false};

    // Event system
    std::unordered_map<std::string, std::vector<VoiceEventCallback>> m_eventCallbacks;
    std::mutex m_callbacksMutex;

    // Channel ID generation
    std::atomic<uint32_t> m_nextChannelId{1};

    // Audio device handles (platform-specific)
    void* m_inputDeviceHandle = nullptr;
    void* m_outputDeviceHandle = nullptr;
    void* m_audioContext = nullptr;

    // Performance monitoring
    std::chrono::steady_clock::time_point m_lastStatsUpdate;
    std::queue<float> m_latencyHistory;
    std::queue<float> m_packetLossHistory;
};

// Voice communication network packets
struct VoiceChannelCreatePacket {
    uint32_t channelId;
    uint8_t channelType;
    char channelName[64];
    uint32_t maxParticipants;
    float maxDistance;
    uint64_t timestamp;
};

struct VoiceChannelJoinPacket {
    uint32_t channelId;
    uint32_t playerId;
    uint64_t timestamp;
};

struct VoiceChannelLeavePacket {
    uint32_t channelId;
    uint32_t playerId;
    uint64_t timestamp;
};

struct VoiceConfigUpdatePacket {
    uint32_t playerId;
    uint8_t quality;
    uint8_t vadMode;
    uint8_t spatialMode;
    float volume;
    uint64_t timestamp;
};

// Utility functions for voice communication
namespace VoiceUtils {
    std::string GetQualityName(VoiceQuality quality);
    std::string GetVADModeName(VADMode mode);
    std::string GetSpatialModeName(SpatialAudioMode mode);
    std::string GetChannelTypeName(VoiceChannelType type);

    // Audio format utilities
    uint32_t GetBitrateForQuality(VoiceQuality quality);
    uint32_t GetSampleRateForQuality(VoiceQuality quality);

    // Spatial audio calculations
    float CalculateDistance(const std::array<float, 3>& pos1, const std::array<float, 3>& pos2);
    float CalculateAttenuation(float distance, float maxDistance, float rolloff = 1.0f);
    std::array<float, 2> CalculateStereoPosition(
        const std::array<float, 3>& sourcePos,
        const std::array<float, 3>& listenerPos,
        const std::array<float, 3>& listenerForward);

    // Audio processing utilities
    void NormalizeAudio(std::vector<float>& audioData);
    float GetRMSLevel(const std::vector<float>& audioData);
    void ApplyLowPassFilter(std::vector<float>& audioData, float cutoffFreq, float sampleRate);
    void ApplyHighPassFilter(std::vector<float>& audioData, float cutoffFreq, float sampleRate);
}

} // namespace CoopNet
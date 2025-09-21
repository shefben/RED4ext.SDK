#pragma once

#include <RED4ext/RED4ext.hpp>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <queue>
#include <chrono>
#include "VoiceEncoder.hpp"
#include "../net/Packets.hpp"

namespace CoopNet {

// Voice quality settings
enum class VoiceQuality : uint8_t {
    Low = 0,      // 8kHz, low bitrate, high compression
    Medium = 1,   // 16kHz, medium bitrate, balanced
    High = 2,     // 24kHz, high bitrate, low compression
    Ultra = 3     // 48kHz, maximum bitrate, minimal compression
};

// Voice channel types
enum class VoiceChannel : uint8_t {
    Global = 0,       // All players
    Team = 1,         // Team members only
    Proximity = 2,    // Players within range
    Direct = 3,       // Private channel
    Radio = 4,        // Radio communication
    Whisper = 5       // Close proximity only
};

// Voice transmission modes
enum class TransmissionMode : uint8_t {
    PTT = 0,          // Push to talk
    VAD = 1,          // Voice activity detection
    Open = 2,         // Open microphone
    Disabled = 3      // Voice disabled
};

// VoicePacket is now included from Packets.hpp

// Player voice state
struct PlayerVoiceState {
    uint32_t playerId;
    std::string playerName;
    bool isTransmitting;
    bool isMuted;
    bool isDeafened;
    float inputVolume;
    float outputVolume;
    VoiceChannel activeChannel;
    float lastActivityTime;
    uint32_t lastSequenceNumber;

    // Voice metrics
    uint32_t packetsReceived;
    uint32_t packetsLost;
    float averageLatency;
    float signalStrength;
};

// Voice channel configuration
struct VoiceChannelConfig {
    VoiceChannel channelType;
    std::string channelName;
    std::vector<uint32_t> participants;
    float maxDistance; // For proximity channels
    bool requiresPermission;
    bool allowWhisper;
    float volumeMultiplier;
};

class VoiceManager {
public:
    static VoiceManager& Instance();

    // System lifecycle
    bool Initialize();
    void Cleanup();

    // Voice capture and playback
    bool StartVoiceCapture(const std::string& deviceName = "");
    void StopVoiceCapture();
    bool StartVoicePlayback(const std::string& deviceName = "");
    void StopVoicePlayback();

    // Player management
    bool AddPlayer(uint32_t playerId, const std::string& playerName);
    bool RemovePlayer(uint32_t playerId);
    PlayerVoiceState GetPlayerVoiceState(uint32_t playerId) const;
    std::vector<uint32_t> GetTalkingPlayers() const;

    // Channel management
    bool CreateChannel(VoiceChannel channelType, const std::string& channelName);
    bool JoinChannel(uint32_t playerId, VoiceChannel channel);
    bool LeaveChannel(uint32_t playerId, VoiceChannel channel);
    std::vector<VoiceChannelConfig> GetAvailableChannels() const;

    // Voice control
    void SetPlayerMuted(uint32_t playerId, bool muted);
    void SetPlayerDeafened(uint32_t playerId, bool deafened);
    void SetPlayerVolume(uint32_t playerId, float volume);
    void SetMasterVolume(float volume);

    // Transmission control
    void SetTransmissionMode(TransmissionMode mode);
    void SetPushToTalkKey(uint32_t keyCode);
    void SetVoiceActivationThreshold(float threshold);
    void StartTransmission();
    void StopTransmission();
    void SetMicrophoneGain(float gain);
    void SetOutputVolume(float volume);
    bool IsTransmitting() const;
    void SetVoiceChannels(const std::vector<VoiceChannel>& channels);
    void SetActiveChannel(VoiceChannel channel);

    // Quality settings
    void SetVoiceQuality(VoiceQuality quality);
    void SetNoiseReduction(bool enabled);
    void SetEchoCancellation(bool enabled);
    void SetAutomaticGainControl(bool enabled);

    // Voice processing
    bool ProcessVoiceData(const VoicePacket& packet);
    void ProcessVoiceData(uint32_t playerId, const uint8_t* data, size_t length);
    void SendVoiceData(const std::vector<uint8_t>& data, VoiceChannel channel);

    // Spatial audio
    void SetPlayerPosition(uint32_t playerId, float x, float y, float z);
    void SetListenerPosition(float x, float y, float z, float yaw, float pitch, float roll);
    void SetProximityDistance(float distance);

    // Audio effects
    void ApplyRadioEffect(bool enabled);
    void ApplyUnderwaterEffect(bool enabled);
    void SetReverbProfile(const std::string& profile);

    // Diagnostics
    float GetInputLevel() const;
    float GetOutputLevel() const;
    std::vector<std::string> GetAvailableInputDevices() const;
    std::vector<std::string> GetAvailableOutputDevices() const;
    void RunVoiceTest();

    // Network statistics
    uint32_t GetVoiceBandwidth() const;
    float GetVoiceLatency() const;
    uint32_t GetPacketLoss() const;

private:
    VoiceManager();
    ~VoiceManager();
    VoiceManager(const VoiceManager&) = delete;
    VoiceManager& operator=(const VoiceManager&) = delete;

    // Internal methods
    void VoiceCaptureThread();
    void VoicePlaybackThread();
    void VoiceNetworkThread();

    bool InitializeAudioDevices();
    void ShutdownAudioDevices();

    void ProcessIncomingVoice();
    void ProcessOutgoingVoice();

    bool ShouldTransmit() const;
    float CalculateVoiceVolume(uint32_t playerId) const;
    float CalculateProximityVolume(uint32_t playerId) const;

    void ApplyVoiceEffects(std::vector<uint8_t>& audioData, uint32_t playerId);
    void ApplyNoiseReduction(std::vector<uint8_t>& audioData);
    void ApplyEchoCancellation(std::vector<uint8_t>& audioData);

    void UpdateVoiceMetrics(uint32_t playerId, const VoicePacket& packet);

    // Thread management
    std::atomic<bool> m_running{false};
    std::thread m_captureThread;
    std::thread m_playbackThread;
    std::thread m_networkThread;

    // Audio devices
    void* m_inputDevice = nullptr;
    void* m_outputDevice = nullptr;
    std::string m_currentInputDevice;
    std::string m_currentOutputDevice;

    // Voice state
    std::unordered_map<uint32_t, PlayerVoiceState> m_playerStates;
    std::unordered_map<VoiceChannel, VoiceChannelConfig> m_channels;
    mutable std::mutex m_playerStatesMutex;
    mutable std::mutex m_channelsMutex;

    // Voice processing
    std::queue<VoicePacket> m_incomingVoice;
    std::queue<VoicePacket> m_outgoingVoice;
    mutable std::mutex m_incomingVoiceMutex;
    mutable std::mutex m_outgoingVoiceMutex;

    // Settings
    VoiceQuality m_voiceQuality = VoiceQuality::Medium;
    TransmissionMode m_transmissionMode = TransmissionMode::PTT;
    uint32_t m_pushToTalkKey = 0x54; // 'T' key by default
    float m_voiceActivationThreshold = 0.1f;
    float m_masterVolume = 1.0f;
    float m_proximityDistance = 50.0f;

    // Audio processing settings
    bool m_noiseReductionEnabled = true;
    bool m_echoCancellationEnabled = true;
    bool m_automaticGainControlEnabled = true;
    bool m_radioEffectEnabled = false;
    bool m_underwaterEffectEnabled = false;
    std::string m_reverbProfile = "none";

    // Current state
    std::atomic<bool> m_isTransmitting{false};
    std::atomic<bool> m_pushToTalkActive{false};
    std::atomic<float> m_currentInputLevel{0.0f};
    std::atomic<float> m_currentOutputLevel{0.0f};

    // Listener position for 3D audio
    float m_listenerX = 0.0f, m_listenerY = 0.0f, m_listenerZ = 0.0f;
    float m_listenerYaw = 0.0f, m_listenerPitch = 0.0f, m_listenerRoll = 0.0f;

    // Missing method declarations
    void UpdateListenerPosition(float x, float y, float z);

    // Network statistics
    std::atomic<uint32_t> m_voiceBandwidth{0};
    std::atomic<float> m_voiceLatency{0.0f};
    std::atomic<uint32_t> m_packetLoss{0};

    // Sequence numbers
    std::atomic<uint32_t> m_outgoingSequenceNumber{0};

    // Voice buffer management
    static constexpr size_t MAX_VOICE_BUFFER_SIZE = 8192;
    static constexpr float VOICE_PACKET_INTERVAL = 20.0f; // 20ms packets
    static constexpr float VOICE_TIMEOUT_SECONDS = 3.0f;

    // Implementation-specific members
    bool m_initialized = false;
    bool m_isCapturing = false;
    bool m_isPlayback = false;
    float m_microphoneGain = 1.0f;
    float m_outputVolume = 1.0f;
    VoiceChannel m_currentChannel = VoiceChannel::Global;
    std::vector<VoiceChannel> m_activeChannels;


    // Audio settings
    uint32_t m_sampleRate = 48000;
    uint32_t m_audioChannels = 1;
    uint32_t m_bitsPerSample = 16;
    float m_vadThreshold = 0.01f;
    bool m_compressionEnabled = true;
    bool m_spatialAudioEnabled = true;
    float m_listenerPosition[3] = {0.0f, 0.0f, 0.0f};

    // Player tracking
    std::unordered_map<uint32_t, std::array<float, 3>> m_playerPositions;
    std::unordered_map<uint32_t, std::vector<uint8_t>> m_voiceBuffers;
    std::mutex m_playerMutex;

    // Processing thread
    bool m_processingActive = false;
    std::thread m_processingThread;

    // Audio system functions
    bool InitializeAudioSystem();
    void CleanupAudioSystem();
    bool InitializeCodecs();
    void CleanupCodecs();
    bool InitializeCaptureDevice(const std::string& deviceName);
    void CleanupCaptureDevice();
    bool InitializePlaybackDevice(const std::string& deviceName);
    void CleanupPlaybackDevice();

    // Processing functions
    void ProcessingThreadMain();
    void ProcessCapturedAudio();
    void ProcessPlaybackAudio();
    void UpdateTransmissionStatus();
    void MixPlayerAudio(std::vector<uint8_t>& output);
    float CalculateSpatialVolume(uint32_t playerId);
    void MixAudioBuffer(std::vector<uint8_t>& output, const std::vector<uint8_t>& input, float volume);
    void ApplyVolumeEffect(std::vector<uint8_t>& audioData, float volume);
    void ApplySpatialAudioEffect(std::vector<uint8_t>& audioData, uint32_t playerId);
    bool EncodeVoiceData(const std::vector<uint8_t>& pcmData, std::vector<uint8_t>& encodedData);
    bool DecodeVoiceData(const uint8_t* encodedData, size_t length, std::vector<uint8_t>& pcmData);

    // Additional API functions
    void SetCompressionEnabled(bool enabled);
    void SetSpatialAudioEnabled(bool enabled);
    void SetVADThreshold(float threshold);
};

// Voice effect processor
class VoiceEffectProcessor {
public:
    static VoiceEffectProcessor& Instance();

    // Effect application
    void ApplyRadioDistortion(std::vector<uint8_t>& audioData);
    void ApplyUnderwaterMuffling(std::vector<uint8_t>& audioData);
    void ApplyReverb(std::vector<uint8_t>& audioData, const std::string& profile);
    void ApplyEcho(std::vector<uint8_t>& audioData, float delay, float decay);
    void ApplyBandpassFilter(std::vector<uint8_t>& audioData, float lowFreq, float highFreq);

    // Noise processing
    void ReduceNoise(std::vector<uint8_t>& audioData);
    void CancelEcho(std::vector<uint8_t>& audioData, const std::vector<uint8_t>& referenceData);
    void AutomaticGainControl(std::vector<uint8_t>& audioData);

    // Volume processing
    void NormalizeVolume(std::vector<uint8_t>& audioData);
    void ApplyVolumeMultiplier(std::vector<uint8_t>& audioData, float multiplier);
    void ApplyCompression(std::vector<uint8_t>& audioData, float threshold, float ratio);

private:
    VoiceEffectProcessor() = default;
    ~VoiceEffectProcessor() = default;
    VoiceEffectProcessor(const VoiceEffectProcessor&) = delete;
    VoiceEffectProcessor& operator=(const VoiceEffectProcessor&) = delete;
};

// Voice metrics collector
class VoiceMetrics {
public:
    static VoiceMetrics& Instance();

    void RecordVoicePacket(uint32_t playerId, size_t packetSize);
    void RecordVoiceLatency(uint32_t playerId, float latency);
    void RecordPacketLoss(uint32_t playerId);
    void RecordVoiceQuality(uint32_t playerId, float quality);

    float GetAverageLatency(uint32_t playerId) const;
    float GetPacketLossRate(uint32_t playerId) const;
    uint32_t GetBandwidthUsage(uint32_t playerId) const;
    float GetVoiceQuality(uint32_t playerId) const;

private:
    VoiceMetrics() = default;
    ~VoiceMetrics() = default;
    VoiceMetrics(const VoiceMetrics&) = delete;
    VoiceMetrics& operator=(const VoiceMetrics&) = delete;

    struct PlayerMetrics {
        std::vector<float> latencyHistory;
        std::vector<size_t> packetSizes;
        uint32_t packetsReceived = 0;
        uint32_t packetsLost = 0;
        float averageQuality = 0.0f;
        std::chrono::steady_clock::time_point lastUpdate;
    };

    std::unordered_map<uint32_t, PlayerMetrics> m_playerMetrics;
    mutable std::mutex m_metricsMutex;
};

} // namespace CoopNet
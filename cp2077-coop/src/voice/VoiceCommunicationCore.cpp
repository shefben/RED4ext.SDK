#include "VoiceCommunicationCore.hpp"
#include "../core/Logger.hpp"
#include "../net/Net.hpp"
#include "../../third_party/opus/include/opus.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <cstdint>
// #include <spdlog/spdlog.h> // Replaced with CoopNet::Logger

namespace CoopNet {

VoiceCommunicationCore& VoiceCommunicationCore::Instance() {
    static VoiceCommunicationCore instance;
    return instance;
}

bool VoiceCommunicationCore::Initialize(const VoiceConfig& config) {
    if (m_initialized.load()) {
        return true;
    }

    Logger::Log(LogLevel::INFO, "[VoiceCore] Initializing voice communication system");

    m_config = config;

    // Initialize audio devices
    if (!InitializeAudioDevices()) {
        Logger::Log(LogLevel::ERROR, "[VoiceCore] Failed to initialize audio devices");
        return false;
    }

    // Initialize Opus codec
    if (!InitializeOpusCodec()) {
        Logger::Log(LogLevel::ERROR, "[VoiceCore] Failed to initialize Opus codec");
        return false;
    }

    // Create default channels
    CreateChannel(VoiceChannelType::Global, "Global Chat");
    CreateChannel(VoiceChannelType::Proximity, "Proximity Chat");

    // Start processing threads
    m_shouldStop.store(false);
    m_captureThread = std::thread(&VoiceCommunicationCore::ProcessAudioCapture, this);
    m_playbackThread = std::thread(&VoiceCommunicationCore::ProcessAudioPlayback, this);
    m_processingThread = std::thread(&VoiceCommunicationCore::ProcessPacketQueue, this);

    // Reset statistics
    ResetStatistics();

    m_initialized.store(true);
    Logger::Log(LogLevel::INFO, "[VoiceCore] Voice communication system initialized successfully");

    TriggerEvent("voice_system_initialized", 0, "");
    return true;
}

void VoiceCommunicationCore::Shutdown() {
    if (!m_initialized.load()) {
        return;
    }

    Logger::Log(LogLevel::INFO, "[VoiceCore] Shutting down voice communication system");

    // Stop processing threads
    m_shouldStop.store(true);

    if (m_captureThread.joinable()) {
        m_captureThread.join();
    }
    if (m_playbackThread.joinable()) {
        m_playbackThread.join();
    }
    if (m_processingThread.joinable()) {
        m_processingThread.join();
    }

    // Stop audio capture
    StopCapture();

    // Clear all channels
    {
        std::lock_guard<std::mutex> lock(m_channelsMutex);
        m_channels.clear();
    }

    // Shutdown codecs
    ShutdownOpusCodec();

    // Shutdown audio devices
    ShutdownAudioDevices();

    m_initialized.store(false);
    TriggerEvent("voice_system_shutdown", 0, "");
}

bool VoiceCommunicationCore::InitializeOpusCodec() {
    int error;

    // Create Opus encoder
    m_encoder = opus_encoder_create(m_config.sampleRate, m_config.channels, OPUS_APPLICATION_VOIP, &error);
    if (error != OPUS_OK || !m_encoder) {
        Logger::Log(LogLevel::ERROR, "[VoiceCore] Failed to create Opus encoder: " + std::string(opus_strerror(error)));
        return false;
    }

    // Configure encoder
    opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(m_config.bitrate));
    opus_encoder_ctl(m_encoder, OPUS_SET_VBR(1));
    opus_encoder_ctl(m_encoder, OPUS_SET_COMPLEXITY(8));

    if (m_config.enableNoiseSupression) {
        opus_encoder_ctl(m_encoder, OPUS_SET_DTX(1));
    }

    // Create Opus decoder
    m_decoder = opus_decoder_create(m_config.sampleRate, m_config.channels, &error);
    if (error != OPUS_OK || !m_decoder) {
        Logger::Log(LogLevel::ERROR, "[VoiceCore] Failed to create Opus decoder: " + std::string(opus_strerror(error)));
        opus_encoder_destroy(m_encoder);
        m_encoder = nullptr;
        return false;
    }

    Logger::Log(LogLevel::INFO, "[VoiceCore] Opus codec initialized - Sample rate: " + std::to_string(m_config.sampleRate) + "Hz, Bitrate: " + std::to_string(m_config.bitrate) + "bps");

    return true;
}

void VoiceCommunicationCore::ShutdownOpusCodec() {
    if (m_encoder) {
        opus_encoder_destroy(m_encoder);
        m_encoder = nullptr;
    }

    if (m_decoder) {
        opus_decoder_destroy(m_decoder);
        m_decoder = nullptr;
    }

    Logger::Log(LogLevel::DEBUG, "[VoiceCore] Opus codec shutdown complete");
}

bool VoiceCommunicationCore::InitializeAudioDevices() {
    // Initialize audio system (platform-specific implementation would go here)
    // For now, create some default devices

    // Default input device
    AudioDevice defaultInput;
    defaultInput.deviceId = 1;
    defaultInput.deviceName = "Default Microphone";
    defaultInput.driverName = "DirectSound";
    defaultInput.sampleRate = m_config.sampleRate;
    defaultInput.channels = 1;
    defaultInput.bufferSize = 1024;
    defaultInput.isInput = true;
    defaultInput.isDefault = true;
    defaultInput.isAvailable = true;

    // Default output device
    AudioDevice defaultOutput;
    defaultOutput.deviceId = 2;
    defaultOutput.deviceName = "Default Speakers";
    defaultOutput.driverName = "DirectSound";
    defaultOutput.sampleRate = m_config.sampleRate;
    defaultOutput.channels = 2;
    defaultOutput.bufferSize = 1024;
    defaultOutput.isInput = false;
    defaultOutput.isDefault = true;
    defaultOutput.isAvailable = true;

    {
        std::lock_guard<std::mutex> lock(m_devicesMutex);
        m_inputDevices.push_back(defaultInput);
        m_outputDevices.push_back(defaultOutput);
        m_currentInputDevice = defaultInput;
        m_currentOutputDevice = defaultOutput;
    }

    // spdlog::info("[VoiceCore] Audio devices initialized - Input: '{}', Output: '{}'",
    //              defaultInput.deviceName, defaultOutput.deviceName);

    return true;
}

void VoiceCommunicationCore::ShutdownAudioDevices() {
    std::lock_guard<std::mutex> lock(m_devicesMutex);

    m_inputDevices.clear();
    m_outputDevices.clear();

    // Close device handles if open
    if (m_inputDeviceHandle) {
        // Platform-specific cleanup
        m_inputDeviceHandle = nullptr;
    }

    if (m_outputDeviceHandle) {
        // Platform-specific cleanup
        m_outputDeviceHandle = nullptr;
    }

    // spdlog::debug("[VoiceCore] Audio devices shutdown complete");
}

bool VoiceCommunicationCore::StartCapture() {
    if (m_capturing.load()) {
        return true;
    }

    // Initialize audio capture (platform-specific implementation)
    // spdlog::info("[VoiceCore] Starting audio capture");

    m_capturing.store(true);
    TriggerEvent("capture_started", 0, "");
    return true;
}

bool VoiceCommunicationCore::StopCapture() {
    if (!m_capturing.load()) {
        return true;
    }

    // spdlog::info("[VoiceCore] Stopping audio capture");

    m_capturing.store(false);
    TriggerEvent("capture_stopped", 0, "");
    return true;
}

uint32_t VoiceCommunicationCore::CreateChannel(VoiceChannelType type, const std::string& name,
                                              const std::vector<uint32_t>& participants) {
    VoiceChannel channel;
    channel.channelId = GenerateChannelId();
    channel.type = type;
    channel.channelName = name;
    channel.participants.insert(participants.begin(), participants.end());
    channel.isEncrypted = false;
    channel.maxDistance = (type == VoiceChannelType::Proximity) ? m_config.maxDistance : 0.0f;
    channel.volume = 1.0f;
    channel.isActive = true;
    channel.createTime = GetCurrentTimestamp();
    channel.description = "Auto-created channel: " + name;

    {
        std::lock_guard<std::mutex> lock(m_channelsMutex);
        m_channels[channel.channelId] = channel;
    }

    // spdlog::info("[VoiceCore] Created voice channel '{}' (ID: {}, Type: {})",
    //              name, channel.channelId, static_cast<int>(type));

    TriggerEvent("channel_created", 0, std::to_string(channel.channelId));
    return channel.channelId;
}

bool VoiceCommunicationCore::JoinChannel(uint32_t channelId, uint32_t playerId) {
    std::lock_guard<std::mutex> lock(m_channelsMutex);

    auto it = m_channels.find(channelId);
    if (it == m_channels.end()) {
        return false;
    }

    it->second.participants.insert(playerId);
    // spdlog::debug("[VoiceCore] Player {} joined channel {}", playerId, channelId);

    TriggerEvent("channel_joined", playerId, std::to_string(channelId));
    return true;
}

bool VoiceCommunicationCore::LeaveChannel(uint32_t channelId, uint32_t playerId) {
    std::lock_guard<std::mutex> lock(m_channelsMutex);

    auto it = m_channels.find(channelId);
    if (it == m_channels.end()) {
        return false;
    }

    it->second.participants.erase(playerId);
    // spdlog::debug("[VoiceCore] Player {} left channel {}", playerId, channelId);

    TriggerEvent("channel_left", playerId, std::to_string(channelId));
    return true;
}

void VoiceCommunicationCore::ProcessAudioCapture() {
    // spdlog::debug("[VoiceCore] Audio capture thread started");

    const uint32_t frameSamples = (m_config.sampleRate * m_config.frameDuration) / 1000;
    std::vector<float> audioFrame(frameSamples);

    while (!m_shouldStop.load()) {
        if (!m_capturing.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Simulate audio capture (in real implementation, this would read from audio device)
        // For now, generate silence or test tone
        std::fill(audioFrame.begin(), audioFrame.end(), 0.0f);

        // Apply voice activation detection
        ProcessVoiceActivation();

        bool shouldTransmit = false;
        if (m_config.vadMode == VADMode::Manual) {
            shouldTransmit = m_pushToTalkPressed;
        } else if (m_config.vadMode == VADMode::Automatic) {
            shouldTransmit = m_isVoiceActive;
        } else { // Hybrid
            shouldTransmit = m_pushToTalkPressed || m_isVoiceActive;
        }

        if (shouldTransmit) {
            // Apply audio processing
            if (m_config.enableNoiseSupression) {
                ApplyNoiseGate(audioFrame, m_config.vadThreshold);
            }

            if (m_config.enableAutomaticGainControl) {
                ApplyAGC(audioFrame);
            }

            // Encode and transmit
            auto encodedData = EncodeAudio(audioFrame);
            if (!encodedData.empty()) {
                TransmitVoice(encodedData, VoiceChannelType::Global, 1);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(m_config.frameDuration));
    }

    // spdlog::debug("[VoiceCore] Audio capture thread stopped");
}

void VoiceCommunicationCore::ProcessAudioPlayback() {
    // spdlog::debug("[VoiceCore] Audio playback thread started");

    while (!m_shouldStop.load()) {
        // Process incoming voice packets
        std::queue<VoiceCommPacket> packetsToProcess;

        {
            std::lock_guard<std::mutex> lock(m_packetsMutex);
            packetsToProcess.swap(m_incomingPackets);
        }

        while (!packetsToProcess.empty()) {
            const auto& packet = packetsToProcess.front();
            HandleIncomingVoicePacket(packet);
            packetsToProcess.pop();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // spdlog::debug("[VoiceCore] Audio playback thread stopped");
}

void VoiceCommunicationCore::ProcessVoiceActivation() {
    // Calculate current voice level (simplified implementation)
    m_currentVoiceLevel = 0.1f; // Placeholder

    // Update voice activation state
    if (m_currentVoiceLevel > m_config.vadThreshold) {
        if (!m_isVoiceActive) {
            m_isVoiceActive = true;
            TriggerEvent("voice_activity_started", 0, "");
        }
        m_lastVoiceActivity = std::chrono::steady_clock::now();
    } else {
        // Check hangover period
        auto now = std::chrono::steady_clock::now();
        auto hangoverTime = std::chrono::milliseconds(static_cast<int>(m_config.vadHangover));

        if (m_isVoiceActive && (now - m_lastVoiceActivity) > hangoverTime) {
            m_isVoiceActive = false;
            TriggerEvent("voice_activity_stopped", 0, "");
        }
    }
}

std::vector<uint8_t> VoiceCommunicationCore::EncodeAudio(const std::vector<float>& pcmData) {
    if (!m_encoder) {
        return {};
    }

    // Convert float to int16
    std::vector<opus_int16> int16Data(pcmData.size());
    for (size_t i = 0; i < pcmData.size(); ++i) {
        int16Data[i] = static_cast<opus_int16>(pcmData[i] * 32767.0f);
    }

    // Encode with Opus
    std::vector<uint8_t> encodedData(4000); // Max Opus packet size
    int encodedSize = opus_encode(m_encoder, int16Data.data(), static_cast<int>(int16Data.size()),
                                 encodedData.data(), static_cast<int>(encodedData.size()));

    if (encodedSize < 0) {
        // spdlog::warn("[VoiceCore] Opus encoding failed: {}", opus_strerror(encodedSize));
        return {};
    }

    encodedData.resize(encodedSize);

    // Update statistics
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_stats.bytesTransmitted += encodedSize;
        m_stats.packetsTransmitted++;
        m_stats.compressionRatio = static_cast<float>(pcmData.size() * sizeof(float)) / encodedSize;
    }

    return encodedData;
}

std::vector<float> VoiceCommunicationCore::DecodeAudio(const std::vector<uint8_t>& encodedData) {
    if (!m_decoder || encodedData.empty()) {
        return {};
    }

    const int frameSamples = (m_config.sampleRate * m_config.frameDuration) / 1000;
    std::vector<int16_t> int16Data(frameSamples);

    int decodedSamples = opus_decode(m_decoder, encodedData.data(), static_cast<int>(encodedData.size()),
                                    int16Data.data(), frameSamples, 0);

    if (decodedSamples < 0) {
        // spdlog::warn("[VoiceCore] Opus decoding failed: {}", opus_strerror(decodedSamples));
        return {};
    }

    // Convert int16 to float
    std::vector<float> floatData(decodedSamples);
    for (int i = 0; i < decodedSamples; ++i) {
        floatData[i] = static_cast<float>(int16Data[i]) / 32767.0f;
    }

    // Update statistics
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_stats.bytesReceived += encodedData.size();
        m_stats.packetsReceived++;
    }

    return floatData;
}

bool VoiceCommunicationCore::TransmitVoice(const std::vector<uint8_t>& audioData,
                                          VoiceChannelType channelType, uint32_t channelId) {
    VoiceCommPacket packet;
    packet.playerId = 1; // Current player ID (would be retrieved from session)
    packet.sequenceNumber = m_stats.packetsTransmitted;
    packet.timestamp = GetCurrentTimestamp();
    packet.channelType = channelType;
    packet.channelId = channelId;
    packet.dataSize = static_cast<uint32_t>(audioData.size());
    packet.audioData = audioData;
    packet.volume = 1.0f;
    packet.spatialX = m_listenerPosition[0];
    packet.spatialY = m_listenerPosition[1];
    packet.spatialZ = m_listenerPosition[2];
    packet.isCompressed = true;
    packet.codecType = 0; // Opus

    return SendVoicePacket(packet);
}

bool VoiceCommunicationCore::SendVoicePacket(const VoiceCommPacket& packet) {
    // Check if we're connected to a network
    if (!Net_IsConnected()) {
        return false;
    }

    // Serialize packet data
    std::vector<uint8_t> packetData;
    packetData.reserve(sizeof(VoiceCommPacket) + packet.audioData.size());

    // Add packet header
    packetData.insert(packetData.end(), reinterpret_cast<const uint8_t*>(&packet.playerId),
                     reinterpret_cast<const uint8_t*>(&packet.playerId) + sizeof(packet.playerId));
    packetData.insert(packetData.end(), reinterpret_cast<const uint8_t*>(&packet.sequenceNumber),
                     reinterpret_cast<const uint8_t*>(&packet.sequenceNumber) + sizeof(packet.sequenceNumber));
    packetData.insert(packetData.end(), reinterpret_cast<const uint8_t*>(&packet.timestamp),
                     reinterpret_cast<const uint8_t*>(&packet.timestamp) + sizeof(packet.timestamp));

    // Add audio data
    packetData.insert(packetData.end(), packet.audioData.begin(), packet.audioData.end());

    // Send voice data using the Net_SendVoice function
    Net_SendVoice(packetData.data(), static_cast<uint16_t>(packetData.size()), packet.sequenceNumber);

    return true;
}

void VoiceCommunicationCore::HandleIncomingVoicePacket(const VoiceCommPacket& packet) {
    // Check if player is muted
    if (IsPlayerMuted(packet.playerId)) {
        return;
    }

    // Decode audio data
    auto audioData = DecodeAudio(packet.audioData);
    if (audioData.empty()) {
        return;
    }

    // Apply player-specific volume
    float playerVolume = GetPlayerVolume(packet.playerId);
    for (float& sample : audioData) {
        sample *= playerVolume;
    }

    // Apply spatial effects if enabled
    if (m_spatialAudioEnabled.load() && packet.channelType == VoiceChannelType::Proximity) {
        ApplySpatialEffects(audioData, packet.playerId);
    }

    // Mix into playback buffer (simplified - real implementation would use proper audio mixing)
    // This would typically involve adding to an audio mixer or queuing for playback

    TriggerEvent("voice_received", packet.playerId, std::to_string(audioData.size()));
}

std::vector<uint32_t> VoiceCommunicationCore::GetChannelParticipants(uint32_t channelId) const {
    std::lock_guard<std::mutex> lock(m_channelsMutex);

    auto it = m_channels.find(channelId);
    if (it == m_channels.end()) {
        return {};
    }

    return std::vector<uint32_t>(it->second.participants.begin(), it->second.participants.end());
}

void VoiceCommunicationCore::ApplySpatialEffects(std::vector<float>& audioData, uint32_t playerId) {
    auto posIt = m_playerPositions.find(playerId);
    if (posIt == m_playerPositions.end()) {
        return;
    }

    const auto& playerPos = posIt->second;

    // Calculate distance and attenuation
    float distance = VoiceUtils::CalculateDistance(playerPos, m_listenerPosition);
    float attenuation = VoiceUtils::CalculateAttenuation(distance, m_config.maxDistance, m_config.rolloffFactor);

    // Apply distance attenuation
    for (float& sample : audioData) {
        sample *= attenuation;
    }

    // Calculate stereo positioning
    auto stereoPos = VoiceUtils::CalculateStereoPosition(playerPos, m_listenerPosition, m_listenerForward);

    // Apply stereo effects (would modify audioData for stereo positioning)
    // Implementation would depend on the specific spatial audio algorithm used
}

void VoiceCommunicationCore::Update(float deltaTime) {
    if (!m_initialized.load()) {
        return;
    }

    // Update statistics periodically
    static float statsTimer = 0.0f;
    statsTimer += deltaTime;

    if (statsTimer >= 1.0f) {
        UpdateNetworkStatistics();
        statsTimer = 0.0f;
    }
}

void VoiceCommunicationCore::UpdateNetworkStatistics() {
    std::lock_guard<std::mutex> lock(m_statsMutex);

    m_stats.lastUpdate = std::chrono::steady_clock::now();

    // Update active channels count
    {
        std::lock_guard<std::mutex> channelLock(m_channelsMutex);
        m_stats.activeChannels = 0;
        for (const auto& [id, channel] : m_channels) {
            if (channel.isActive && !channel.participants.empty()) {
                m_stats.activeChannels++;
            }
        }
    }

    // Calculate average latency (simplified)
    if (!m_latencyHistory.empty()) {
        float totalLatency = 0.0f;
        auto tempQueue = m_latencyHistory;
        while (!tempQueue.empty()) {
            totalLatency += tempQueue.front();
            tempQueue.pop();
        }
        m_stats.averageLatency = totalLatency / m_latencyHistory.size();
    }
}

VoiceStats VoiceCommunicationCore::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_stats;
}

void VoiceCommunicationCore::ResetStatistics() {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    m_stats = VoiceStats{};
    m_stats.lastUpdate = std::chrono::steady_clock::now();
}

void VoiceCommunicationCore::TriggerEvent(const std::string& eventType, uint32_t playerId, const std::string& data) {
    std::lock_guard<std::mutex> lock(m_callbacksMutex);

    auto it = m_eventCallbacks.find(eventType);
    if (it != m_eventCallbacks.end()) {
        for (const auto& callback : it->second) {
            try {
                callback(playerId, eventType, data);
            } catch (const std::exception& e) {
                // spdlog::error("[VoiceCore] Exception in event callback for '{}': {}", eventType, e.what());
            }
        }
    }
}

uint32_t VoiceCommunicationCore::GenerateChannelId() {
    return m_nextChannelId.fetch_add(1);
}

uint64_t VoiceCommunicationCore::GetCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Utility function implementations
namespace VoiceUtils {
    float CalculateDistance(const std::array<float, 3>& pos1, const std::array<float, 3>& pos2) {
        float dx = pos1[0] - pos2[0];
        float dy = pos1[1] - pos2[1];
        float dz = pos1[2] - pos2[2];
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    float CalculateAttenuation(float distance, float maxDistance, float rolloff) {
        if (distance <= 0.0f) return 1.0f;
        if (distance >= maxDistance) return 0.0f;

        return 1.0f - std::pow(distance / maxDistance, rolloff);
    }

    std::array<float, 2> CalculateStereoPosition(
        const std::array<float, 3>& sourcePos,
        const std::array<float, 3>& listenerPos,
        const std::array<float, 3>& listenerForward) {

        // Calculate relative position
        float dx = sourcePos[0] - listenerPos[0];
        float dz = sourcePos[2] - listenerPos[2];

        // Calculate angle relative to listener forward direction
        float angle = std::atan2(dx, dz);

        // Convert to stereo pan (-1.0 = left, 1.0 = right)
        float pan = std::sin(angle);

        return {std::max(0.0f, -pan), std::max(0.0f, pan)};
    }

    std::string GetQualityName(VoiceQuality quality) {
        switch (quality) {
            case VoiceQuality::Low: return "Low";
            case VoiceQuality::Medium: return "Medium";
            case VoiceQuality::High: return "High";
            case VoiceQuality::Ultra: return "Ultra";
            default: return "Unknown";
        }
    }

    uint32_t GetBitrateForQuality(VoiceQuality quality) {
        switch (quality) {
            case VoiceQuality::Low: return 32000;
            case VoiceQuality::Medium: return 64000;
            case VoiceQuality::High: return 96000;
            case VoiceQuality::Ultra: return 128000;
            default: return 64000;
        }
    }
}

void VoiceCommunicationCore::ProcessPacketQueue() {
    while (!m_shouldStop.load()) {
        // Process incoming voice packets
        std::queue<VoiceCommPacket> packetsToProcess;

        {
            std::lock_guard<std::mutex> lock(m_packetsMutex);
            packetsToProcess.swap(m_incomingPackets);
        }

        while (!packetsToProcess.empty()) {
            const auto& packet = packetsToProcess.front();
            HandleIncomingVoicePacket(packet);
            packetsToProcess.pop();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

bool VoiceCommunicationCore::IsPlayerMuted(uint32_t playerId) const {
    std::lock_guard<std::mutex> lock(m_playersMutex);
    auto it = m_mutedPlayers.find(playerId);
    return it != m_mutedPlayers.end() ? it->second : false;
}

float VoiceCommunicationCore::GetPlayerVolume(uint32_t playerId) const {
    std::lock_guard<std::mutex> lock(m_playersMutex);
    auto it = m_playerVolumes.find(playerId);
    return it != m_playerVolumes.end() ? it->second : 1.0f;
}

void VoiceCommunicationCore::ApplyNoiseGate(std::vector<float>& audioData, float threshold) {
    for (float& sample : audioData) {
        if (std::abs(sample) < threshold) {
            sample = 0.0f;
        }
    }
}

void VoiceCommunicationCore::ApplyAGC(std::vector<float>& audioData) {
    if (audioData.empty()) return;

    // Calculate RMS level
    float rms = 0.0f;
    for (const float sample : audioData) {
        rms += sample * sample;
    }
    rms = std::sqrt(rms / audioData.size());

    // Apply gain adjustment
    if (rms > 0.001f) {
        float targetLevel = 0.3f;  // Target RMS level
        float gain = targetLevel / rms;
        gain = std::min(gain, 4.0f);  // Limit maximum gain

        for (float& sample : audioData) {
            sample *= gain;
        }
    }
}

void VoiceCommunicationCore::ApplyEchoSuppression(std::vector<float>& audioData) {
    // Simple echo suppression - reduce amplitude of delayed signals
    if (audioData.size() < 1024) return;

    for (size_t i = 1024; i < audioData.size(); ++i) {
        audioData[i] = audioData[i] - 0.3f * audioData[i - 1024];
    }
}

float VoiceCommunicationCore::CalculateVoiceLevel(const std::vector<float>& audioData) {
    if (audioData.empty()) return 0.0f;

    float sum = 0.0f;
    for (const float sample : audioData) {
        sum += sample * sample;
    }
    return std::sqrt(sum / audioData.size());
}

// Additional missing member functions
std::vector<AudioDevice> VoiceCommunicationCore::GetInputDevices() const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    return m_inputDevices;
}

std::vector<AudioDevice> VoiceCommunicationCore::GetOutputDevices() const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    return m_outputDevices;
}

bool VoiceCommunicationCore::SetInputDevice(uint32_t deviceId) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    for (const auto& device : m_inputDevices) {
        if (device.deviceId == deviceId && device.isAvailable) {
            m_currentInputDevice = device;
            return true;
        }
    }
    return false;
}

bool VoiceCommunicationCore::SetOutputDevice(uint32_t deviceId) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    for (const auto& device : m_outputDevices) {
        if (device.deviceId == deviceId && device.isAvailable) {
            m_currentOutputDevice = device;
            return true;
        }
    }
    return false;
}

AudioDevice VoiceCommunicationCore::GetCurrentInputDevice() const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    return m_currentInputDevice;
}

AudioDevice VoiceCommunicationCore::GetCurrentOutputDevice() const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    return m_currentOutputDevice;
}

bool VoiceCommunicationCore::IsCapturing() const {
    return m_capturing.load();
}

void VoiceCommunicationCore::SetCaptureVolume(float volume) {
    m_config.vadThreshold = std::clamp(volume, 0.0f, 1.0f);
}

float VoiceCommunicationCore::GetCaptureVolume() const {
    return m_config.vadThreshold;
}

void VoiceCommunicationCore::SetPlaybackVolume(float volume) {
    // Store playback volume in config
    m_config.rolloffFactor = std::clamp(volume, 0.0f, 1.0f);
}

float VoiceCommunicationCore::GetPlaybackVolume() const {
    return m_config.rolloffFactor;
}

} // namespace CoopNet
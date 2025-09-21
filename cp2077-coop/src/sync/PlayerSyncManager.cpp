#include "PlayerSyncManager.hpp"
#include "../core/ErrorManager.hpp"
#include "../net/NetworkOptimizer.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

namespace CoopNet {

PlayerSyncManager& PlayerSyncManager::Instance() {
    static PlayerSyncManager instance;
    return instance;
}

bool PlayerSyncManager::Initialize(const SyncConfig& config) {
    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    if (m_initialized) {
        spdlog::warn("[PlayerSyncManager] Already initialized");
        return true;
    }

    m_config = config;
    m_updateInterval = std::chrono::microseconds(1000000 / m_config.updateFrequencyHz);

    // Initialize systems
    m_players.clear();
    m_previousStates.clear();
    m_lastUpdate = std::chrono::steady_clock::now();
    m_lastStatUpdate = m_lastUpdate;

    // Set default configuration
    m_interpolationEnabled = true;
    m_predictionEnabled = m_config.enableClientPrediction;
    m_deltaCompressionEnabled = true;
    m_serverValidationEnabled = m_config.enableServerValidation;

    m_initialized = true;
    spdlog::info("[PlayerSyncManager] Initialized with {} max players at {}Hz",
                 m_config.maxPlayersSupported, m_config.updateFrequencyHz);

    return true;
}

void PlayerSyncManager::Shutdown() {
    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    if (!m_initialized) return;

    // Notify all players leaving
    for (const auto& [playerId, playerInfo] : m_players) {
        NotifyPlayerLeft(playerId);
    }

    // Clear all data
    m_players.clear();
    m_updateQueue = std::queue<PlayerSyncData>();
    m_previousStates.clear();
    m_syncStats.clear();
    m_validator.reset();

    m_initialized = false;
    spdlog::info("[PlayerSyncManager] Shutdown completed");
}

void PlayerSyncManager::Update() {
    if (!m_initialized) return;

    auto currentTime = std::chrono::steady_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_lastUpdate);

    if (deltaTime >= m_updateInterval) {
        ProcessUpdateQueue();
        ProcessInterpolation();

        if (m_predictionEnabled) {
            ProcessPrediction();
        }

        if (m_serverValidationEnabled) {
            ProcessValidation();
        }

        if (m_statisticsEnabled) {
            UpdateStatistics();
        }

        m_lastUpdate = currentTime;
    }
}

bool PlayerSyncManager::RegisterPlayer(uint32_t playerId, const std::string& playerName, bool isLocal) {
    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    if (m_players.count(playerId)) {
        spdlog::warn("[PlayerSyncManager] Player {} already registered", playerId);
        return false;
    }

    if (m_players.size() >= m_config.maxPlayersSupported) {
        spdlog::error("[PlayerSyncManager] Maximum player count reached: {}", m_config.maxPlayersSupported);
        return false;
    }

    PlayerInfo info;
    info.playerId = playerId;
    info.playerName = playerName;
    info.state = PlayerState::Spawning;
    info.lastUpdateTime = std::chrono::steady_clock::now();
    info.connectedTime = info.lastUpdateTime;
    info.lastSequenceNumber = 0;
    info.averageLatency = 0.0f;
    info.packetLossRate = 0.0f;
    info.isLocal = isLocal;
    info.isHost = false; // TODO: Determine host status
    info.packetsReceived = 0;
    info.packetsDropped = 0;
    info.interpolationsPerformed = 0;
    info.predictionsPerformed = 0;

    // Initialize player state
    info.currentState.playerId = playerId;
    info.currentState.timestamp = GetCurrentTimestamp();
    info.currentState.sequenceNumber = 0;
    info.currentState.position = {0.0f, 0.0f, 0.0f, 1.0f};
    info.currentState.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    info.currentState.health = 100.0f;
    info.currentState.maxHealth = 100.0f;
    info.currentState.isAlive = true;

    m_players[playerId] = info;

    if (isLocal) {
        m_localPlayerId = playerId;
        m_localPlayerState = info.currentState;
    }

    spdlog::info("[PlayerSyncManager] Registered player: {} ({})", playerName, playerId);
    NotifyPlayerJoined(playerId);

    return true;
}

bool PlayerSyncManager::UnregisterPlayer(uint32_t playerId) {
    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) {
        spdlog::warn("[PlayerSyncManager] Player {} not found for unregistration", playerId);
        return false;
    }

    std::string playerName = it->second.playerName;
    m_players.erase(it);
    m_previousStates.erase(playerId);
    m_syncStats.erase(playerId);

    if (playerId == m_localPlayerId) {
        m_localPlayerId = 0;
    }

    spdlog::info("[PlayerSyncManager] Unregistered player: {} ({})", playerName, playerId);
    NotifyPlayerLeft(playerId);

    return true;
}

bool PlayerSyncManager::IsPlayerRegistered(uint32_t playerId) const {
    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);
    return m_players.count(playerId) > 0;
}

std::vector<uint32_t> PlayerSyncManager::GetRegisteredPlayers() const {
    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    std::vector<uint32_t> players;
    players.reserve(m_players.size());

    for (const auto& [playerId, playerInfo] : m_players) {
        players.push_back(playerId);
    }

    return players;
}

std::optional<PlayerInfo> PlayerSyncManager::GetPlayerInfo(uint32_t playerId) const {
    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it != m_players.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool PlayerSyncManager::UpdatePlayerState(uint32_t playerId, const PlayerSyncData& data) {
    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) {
        spdlog::warn("[PlayerSyncManager] Cannot update state for unregistered player: {}", playerId);
        return false;
    }

    // Validate the update
    if (m_serverValidationEnabled && !ValidatePlayerUpdate(playerId, data)) {
        return false;
    }

    // Store previous state for delta calculations
    m_previousStates[playerId] = it->second.currentState;

    // Update player state
    it->second.currentState = data;
    it->second.lastUpdateTime = std::chrono::steady_clock::now();
    it->second.lastSequenceNumber = data.sequenceNumber;

    // Add to snapshot history for interpolation
    PlayerSnapshot snapshot;
    snapshot.timestamp = data.timestamp;
    snapshot.data = data;
    snapshot.isReliable = true;
    snapshot.interpolationWeight = 1.0f;

    it->second.snapshotHistory.push(snapshot);

    // Limit snapshot history size
    while (it->second.snapshotHistory.size() > m_config.interpolationBufferSize) {
        it->second.snapshotHistory.pop();
    }

    // Update statistics
    it->second.packetsReceived++;

    // Update local player state if this is the local player
    if (playerId == m_localPlayerId) {
        m_localPlayerState = data;
    }

    // Notify callbacks
    NotifyPlayerStateUpdated(playerId, data);

    return true;
}

bool PlayerSyncManager::SendPlayerUpdate(uint32_t playerId, SyncDataType dataType, SyncPriority priority) {
    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) {
        return false;
    }

    PlayerSyncData data = it->second.currentState;
    data.dataType = dataType;
    data.priority = priority;
    data.timestamp = GetCurrentTimestamp();
    data.sequenceNumber = GenerateSequenceNumber();

    // Add to update queue for processing
    std::lock_guard<std::mutex> updateLock(m_updateMutex);
    m_updateQueue.push(data);

    return true;
}

PlayerSyncData PlayerSyncManager::GetPlayerState(uint32_t playerId) const {
    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it != m_players.end()) {
        return it->second.currentState;
    }

    // Return empty state for unknown players
    PlayerSyncData emptyState;
    emptyState.playerId = playerId;
    emptyState.timestamp = GetCurrentTimestamp();
    return emptyState;
}

PlayerSyncData PlayerSyncManager::GetLocalPlayerState() const {
    return m_localPlayerState;
}

bool PlayerSyncManager::SendSyncPacket(uint32_t targetPlayerId, const PlayerSyncData& data) {
    try {
        std::vector<uint8_t> packet = SerializeSyncData(data);

        if (m_deltaCompressionEnabled && m_previousStates.count(data.playerId)) {
            packet = CreateDelta(m_previousStates[data.playerId], data);
        }

        if (m_config.compressionThreshold > 0.0f && packet.size() > 100) {
            CompressPacket(packet);
        }

        // Use NetworkOptimizer for actual packet sending
        auto& netOptimizer = NetworkOptimizer::Instance();
        if (netOptimizer.IsOptimizationActive()) {
            // Optimize packet based on priority
            switch (data.priority) {
                case SyncPriority::Critical:
                    netOptimizer.SetPacketPriority("player_sync", PacketPriority::Critical);
                    break;
                case SyncPriority::High:
                    netOptimizer.SetPacketPriority("player_sync", PacketPriority::High);
                    break;
                default:
                    netOptimizer.SetPacketPriority("player_sync", PacketPriority::Medium);
                    break;
            }
        }

        // TODO: Send packet through network layer
        // This would integrate with the existing networking system

        return true;

    } catch (const std::exception& ex) {
        spdlog::error("[PlayerSyncManager] Failed to send sync packet: {}", ex.what());
        return false;
    }
}

bool PlayerSyncManager::BroadcastSyncPacket(const PlayerSyncData& data, const std::vector<uint32_t>& excludePlayers) {
    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    bool allSuccess = true;

    for (const auto& [playerId, playerInfo] : m_players) {
        // Skip excluded players and the sender
        if (playerId == data.playerId ||
            std::find(excludePlayers.begin(), excludePlayers.end(), playerId) != excludePlayers.end()) {
            continue;
        }

        if (!SendSyncPacket(playerId, data)) {
            allSuccess = false;
        }
    }

    return allSuccess;
}

void PlayerSyncManager::ReceiveSyncPacket(const std::vector<uint8_t>& packetData, uint32_t fromPlayerId) {
    try {
        std::vector<uint8_t> data = packetData;

        // Decompress if needed
        if (m_config.compressionThreshold > 0.0f) {
            DecompressPacket(data);
        }

        PlayerSyncData syncData;

        // Check if this is a delta packet
        if (m_deltaCompressionEnabled && m_previousStates.count(fromPlayerId)) {
            syncData = ApplyDelta(m_previousStates[fromPlayerId], data);
        } else {
            syncData = DeserializeSyncData(data);
        }

        // Update player state
        UpdatePlayerState(fromPlayerId, syncData);

    } catch (const std::exception& ex) {
        spdlog::error("[PlayerSyncManager] Failed to process received sync packet: {}", ex.what());

        // Update error statistics
        std::lock_guard<std::recursive_mutex> lock(m_playerMutex);
        auto it = m_players.find(fromPlayerId);
        if (it != m_players.end()) {
            it->second.packetsDropped++;
        }
    }
}

PlayerSyncData PlayerSyncManager::InterpolatePlayerState(uint32_t playerId, uint64_t targetTime) {
    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end() || it->second.snapshotHistory.empty()) {
        return GetPlayerState(playerId);
    }

    auto& history = it->second.snapshotHistory;
    std::vector<PlayerSnapshot> snapshots;

    // Convert queue to vector for easier processing
    std::queue<PlayerSnapshot> tempQueue = history;
    while (!tempQueue.empty()) {
        snapshots.push_back(tempQueue.front());
        tempQueue.pop();
    }

    if (snapshots.size() < 2) {
        return snapshots.empty() ? GetPlayerState(playerId) : snapshots[0].data;
    }

    // Find the two snapshots to interpolate between
    for (size_t i = 0; i < snapshots.size() - 1; ++i) {
        if (snapshots[i].timestamp <= targetTime && snapshots[i + 1].timestamp >= targetTime) {
            float t = static_cast<float>(targetTime - snapshots[i].timestamp) /
                     static_cast<float>(snapshots[i + 1].timestamp - snapshots[i].timestamp);

            PlayerSyncData result = InterpolateState(snapshots[i], snapshots[i + 1], t);

            // Update statistics
            it->second.interpolationsPerformed++;

            return result;
        }
    }

    // If target time is outside our history, return the closest snapshot
    if (targetTime < snapshots[0].timestamp) {
        return snapshots[0].data;
    } else {
        return snapshots.back().data;
    }
}

PlayerSyncData PlayerSyncManager::PredictPlayerState(uint32_t playerId, float deltaTimeMs) {
    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) {
        return GetPlayerState(playerId);
    }

    PlayerSyncData current = it->second.currentState;
    PlayerSyncData predicted = current;

    // Predict position based on velocity
    predicted.position = PredictPosition(current, deltaTimeMs);
    predicted.rotation = PredictRotation(current, deltaTimeMs);
    predicted.timestamp = GetCurrentTimestamp();

    // Update statistics
    it->second.predictionsPerformed++;

    return predicted;
}

bool PlayerSyncManager::UpdateLocalPlayerPosition(const RED4ext::Vector4& position, const RED4ext::Quaternion& rotation) {
    if (m_localPlayerId == 0) {
        return false;
    }

    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    m_localPlayerState.position = position;
    m_localPlayerState.rotation = rotation;
    m_localPlayerState.timestamp = GetCurrentTimestamp();
    m_localPlayerState.sequenceNumber = GenerateSequenceNumber();

    // Check if we should send an update
    bool shouldUpdate = false;
    if (m_previousStates.count(m_localPlayerId)) {
        const auto& previous = m_previousStates[m_localPlayerId];
        float positionDelta = PlayerSyncUtils::CalculateDistance3D(position, previous.position);
        float rotationDelta = PlayerSyncUtils::CalculateAngleDifference(rotation, previous.rotation);

        shouldUpdate = positionDelta > m_config.positionSyncThreshold ||
                      rotationDelta > m_config.rotationSyncThreshold;
    } else {
        shouldUpdate = true; // First update
    }

    if (shouldUpdate) {
        UpdatePlayerState(m_localPlayerId, m_localPlayerState);
        BroadcastSyncPacket(m_localPlayerState);
    }

    return true;
}

bool PlayerSyncManager::UpdateLocalPlayerHealth(float health, float maxHealth) {
    if (m_localPlayerId == 0) {
        return false;
    }

    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    float previousHealth = m_localPlayerState.health;
    m_localPlayerState.health = health;
    m_localPlayerState.maxHealth = maxHealth;
    m_localPlayerState.isAlive = health > 0.0f;
    m_localPlayerState.timestamp = GetCurrentTimestamp();

    // Send update if health changed significantly
    if (std::abs(health - previousHealth) > m_config.healthSyncThreshold) {
        m_localPlayerState.sequenceNumber = GenerateSequenceNumber();
        UpdatePlayerState(m_localPlayerId, m_localPlayerState);
        BroadcastSyncPacket(m_localPlayerState);
    }

    return true;
}

bool PlayerSyncManager::UpdateLocalPlayerAnimation(const std::string& animationState, float animationTime) {
    if (m_localPlayerId == 0) {
        return false;
    }

    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    if (m_localPlayerState.animationState != animationState) {
        m_localPlayerState.animationState = animationState;
        m_localPlayerState.animationTime = animationTime;
        m_localPlayerState.timestamp = GetCurrentTimestamp();
        m_localPlayerState.sequenceNumber = GenerateSequenceNumber();

        UpdatePlayerState(m_localPlayerId, m_localPlayerState);
        BroadcastSyncPacket(m_localPlayerState);
    }

    return true;
}

// Private methods implementation

void PlayerSyncManager::ProcessUpdateQueue() {
    std::lock_guard<std::mutex> lock(m_updateMutex);

    while (!m_updateQueue.empty()) {
        PlayerSyncData data = m_updateQueue.front();
        m_updateQueue.pop();

        // Process the update
        BroadcastSyncPacket(data);
    }
}

void PlayerSyncManager::ProcessInterpolation() {
    if (!m_interpolationEnabled) return;

    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);
    uint64_t currentTime = GetCurrentTimestamp();

    for (auto& [playerId, playerInfo] : m_players) {
        if (playerId == m_localPlayerId) continue; // Don't interpolate local player

        // Interpolate to a slightly past time for smoothness
        uint64_t targetTime = currentTime - static_cast<uint64_t>(m_config.predictionTimeMs);
        playerInfo.targetState = InterpolatePlayerState(playerId, targetTime);
    }
}

void PlayerSyncManager::ProcessPrediction() {
    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    for (auto& [playerId, playerInfo] : m_players) {
        if (playerId == m_localPlayerId) continue; // Don't predict local player

        // Predict forward by the prediction time
        playerInfo.targetState = PredictPlayerState(playerId, m_config.predictionTimeMs);
    }
}

void PlayerSyncManager::ProcessValidation() {
    if (!m_validator || !m_serverValidationEnabled) return;

    std::lock_guard<std::recursive_mutex> lock(m_playerMutex);

    for (const auto& [playerId, playerInfo] : m_players) {
        if (!m_validator->ValidatePlayerState(playerInfo.currentState)) {
            NotifyValidationFailed(playerId, m_validator->GetValidationError());
        }
    }
}

void PlayerSyncManager::UpdateStatistics() {
    std::lock_guard<std::mutex> lock(m_statsMutex);

    auto currentTime = std::chrono::steady_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - m_lastStatUpdate);

    if (deltaTime.count() >= 1) {
        // Update average latency
        float totalLatency = 0.0f;
        uint32_t playerCount = 0;
        float totalPacketLoss = 0.0f;

        std::lock_guard<std::recursive_mutex> playerLock(m_playerMutex);
        for (const auto& [playerId, playerInfo] : m_players) {
            totalLatency += playerInfo.averageLatency;
            totalPacketLoss += playerInfo.packetLossRate;
            playerCount++;
        }

        if (playerCount > 0) {
            m_averageLatency = totalLatency / playerCount;
            m_packetLossRate = totalPacketLoss / playerCount;
        }

        m_lastStatUpdate = currentTime;
    }
}

PlayerSyncData PlayerSyncManager::InterpolateState(const PlayerSnapshot& from, const PlayerSnapshot& to, float t) {
    PlayerSyncData result = from.data;

    // Interpolate position
    result.position = PlayerSyncUtils::LerpVector(from.data.position, to.data.position, t);

    // Interpolate rotation
    result.rotation = PlayerSyncUtils::SlerpQuaternion(from.data.rotation, to.data.rotation, t);

    // Interpolate other values
    result.health = from.data.health + (to.data.health - from.data.health) * t;
    result.stamina = from.data.stamina + (to.data.stamina - from.data.stamina) * t;
    result.moveSpeed = from.data.moveSpeed + (to.data.moveSpeed - from.data.moveSpeed) * t;

    // Use target timestamp
    result.timestamp = from.timestamp + static_cast<uint64_t>((to.timestamp - from.timestamp) * t);

    return result;
}

RED4ext::Vector4 PlayerSyncManager::PredictPosition(const PlayerSyncData& current, float deltaTime) {
    // Simple linear prediction based on velocity
    RED4ext::Vector4 predicted = current.position;

    float deltaSeconds = deltaTime / 1000.0f;
    predicted.X += current.velocity.X * deltaSeconds;
    predicted.Y += current.velocity.Y * deltaSeconds;
    predicted.Z += current.velocity.Z * deltaSeconds;

    return predicted;
}

RED4ext::Quaternion PlayerSyncManager::PredictRotation(const PlayerSyncData& current, float deltaTime) {
    // Simple angular prediction
    RED4ext::Quaternion predicted = current.rotation;

    // Apply angular velocity (simplified)
    float deltaSeconds = deltaTime / 1000.0f;
    // TODO: Proper quaternion integration with angular velocity

    return predicted;
}

std::vector<uint8_t> PlayerSyncManager::SerializeSyncData(const PlayerSyncData& data) {
    std::vector<uint8_t> buffer;
    buffer.reserve(256); // Estimate

    // Serialize the sync data structure
    // TODO: Implement proper binary serialization
    // For now, this is a placeholder implementation

    return buffer;
}

PlayerSyncData PlayerSyncManager::DeserializeSyncData(const std::vector<uint8_t>& data) {
    PlayerSyncData result;

    // TODO: Implement proper binary deserialization
    // For now, this is a placeholder implementation

    return result;
}

uint64_t PlayerSyncManager::GetCurrentTimestamp() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

uint64_t PlayerSyncManager::GenerateSequenceNumber() {
    return m_nextSequenceNumber++;
}

bool PlayerSyncManager::ValidatePlayerUpdate(uint32_t playerId, const PlayerSyncData& data) {
    if (!m_validator) return true;

    // Basic validation
    if (!PlayerSyncUtils::IsValidPosition(data.position) ||
        !PlayerSyncUtils::IsValidRotation(data.rotation) ||
        !PlayerSyncUtils::IsValidHealth(data.health, data.maxHealth)) {
        return false;
    }

    // Use custom validator if available
    return m_validator->ValidatePlayerState(data);
}

// Event notification methods
void PlayerSyncManager::NotifyPlayerStateUpdated(uint32_t playerId, const PlayerSyncData& data) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_callbacks.onPlayerStateUpdated) {
        try {
            m_callbacks.onPlayerStateUpdated(playerId, data);
        } catch (const std::exception& ex) {
            spdlog::error("[PlayerSyncManager] State update callback error: {}", ex.what());
        }
    }
}

void PlayerSyncManager::NotifyPlayerJoined(uint32_t playerId) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_callbacks.onPlayerJoined) {
        try {
            m_callbacks.onPlayerJoined(playerId);
        } catch (const std::exception& ex) {
            spdlog::error("[PlayerSyncManager] Player joined callback error: {}", ex.what());
        }
    }
}

void PlayerSyncManager::NotifyPlayerLeft(uint32_t playerId) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_callbacks.onPlayerLeft) {
        try {
            m_callbacks.onPlayerLeft(playerId);
        } catch (const std::exception& ex) {
            spdlog::error("[PlayerSyncManager] Player left callback error: {}", ex.what());
        }
    }
}

void PlayerSyncManager::NotifyValidationFailed(uint32_t playerId, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_callbacks.onValidationFailed) {
        try {
            m_callbacks.onValidationFailed(playerId, reason);
        } catch (const std::exception& ex) {
            spdlog::error("[PlayerSyncManager] Validation failed callback error: {}", ex.what());
        }
    }
}

// Utility functions implementation
namespace PlayerSyncUtils {
    float CalculateDistance3D(const RED4ext::Vector4& a, const RED4ext::Vector4& b) {
        float dx = a.X - b.X;
        float dy = a.Y - b.Y;
        float dz = a.Z - b.Z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    float CalculateAngleDifference(const RED4ext::Quaternion& a, const RED4ext::Quaternion& b) {
        // Calculate the angular difference between two quaternions
        float dot = a.i * b.i + a.j * b.j + a.k * b.k + a.r * b.r;
        dot = std::clamp(dot, -1.0f, 1.0f);
        return std::acos(std::abs(dot)) * 2.0f * 180.0f / 3.14159f; // Convert to degrees
    }

    RED4ext::Vector4 LerpVector(const RED4ext::Vector4& a, const RED4ext::Vector4& b, float t) {
        RED4ext::Vector4 result;
        result.X = a.X + (b.X - a.X) * t;
        result.Y = a.Y + (b.Y - a.Y) * t;
        result.Z = a.Z + (b.Z - a.Z) * t;
        result.W = a.W + (b.W - a.W) * t;
        return result;
    }

    RED4ext::Quaternion SlerpQuaternion(const RED4ext::Quaternion& a, const RED4ext::Quaternion& b, float t) {
        RED4ext::Quaternion result;

        // Simplified slerp implementation
        float dot = a.i * b.i + a.j * b.j + a.k * b.k + a.r * b.r;

        if (dot < 0.0f) {
            // Take the shorter path
            result.i = a.i + (-b.i - a.i) * t;
            result.j = a.j + (-b.j - a.j) * t;
            result.k = a.k + (-b.k - a.k) * t;
            result.r = a.r + (-b.r - a.r) * t;
        } else {
            result.i = a.i + (b.i - a.i) * t;
            result.j = a.j + (b.j - a.j) * t;
            result.k = a.k + (b.k - a.k) * t;
            result.r = a.r + (b.r - a.r) * t;
        }

        // Normalize the result
        float length = std::sqrt(result.i * result.i + result.j * result.j + result.k * result.k + result.r * result.r);
        if (length > 0.0f) {
            result.i /= length;
            result.j /= length;
            result.k /= length;
            result.r /= length;
        }

        return result;
    }

    bool IsValidPosition(const RED4ext::Vector4& position) {
        // Check for NaN or infinite values
        return std::isfinite(position.X) && std::isfinite(position.Y) && std::isfinite(position.Z);
    }

    bool IsValidRotation(const RED4ext::Quaternion& rotation) {
        // Check for NaN or infinite values and valid quaternion length
        bool isFinite = std::isfinite(rotation.i) && std::isfinite(rotation.j) &&
                       std::isfinite(rotation.k) && std::isfinite(rotation.r);

        if (!isFinite) return false;

        float length = std::sqrt(rotation.i * rotation.i + rotation.j * rotation.j +
                                rotation.k * rotation.k + rotation.r * rotation.r);
        return std::abs(length - 1.0f) < 0.1f; // Allow some tolerance
    }

    bool IsValidHealth(float health, float maxHealth) {
        return std::isfinite(health) && std::isfinite(maxHealth) &&
               health >= 0.0f && maxHealth > 0.0f && health <= maxHealth;
    }

    uint64_t GetNetworkTime() {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    }
}

} // namespace CoopNet
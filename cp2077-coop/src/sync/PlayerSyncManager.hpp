#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/Generated/Quaternion.hpp>
#include <RED4ext/Scripting/Natives/Generated/EulerAngles.hpp>
#include "../core/CoopNetCore.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>

namespace CoopNet {

// Player synchronization data types
enum class SyncDataType : uint8_t {
    Position = 0,
    Rotation = 1,
    Animation = 2,
    Health = 3,
    Status = 4,
    Equipment = 5,
    Vehicle = 6,
    Combat = 7,
    Interaction = 8,
    Custom = 255
};

// Synchronization priorities
enum class SyncPriority : uint8_t {
    Critical = 0,       // Position, health
    High = 1,           // Combat actions, animations
    Medium = 2,         // Equipment, status effects
    Low = 3,            // UI state, cosmetics
    Background = 4      // Statistics, achievements
};

// Interpolation methods
enum class InterpolationMethod : uint8_t {
    None = 0,
    Linear = 1,
    Smooth = 2,
    Cubic = 3,
    Prediction = 4
};

// Player states
enum class PlayerState : uint8_t {
    Inactive = 0,
    Spawning = 1,
    Active = 2,
    Dead = 3,
    Disconnected = 4,
    Loading = 5,
    Spectating = 6
};

// Forward declarations
struct PlayerSyncData;
struct PlayerSnapshot;
struct SyncConfig;
struct AnimationSyncData;
struct HealthSyncData;
struct EquipmentSyncData;
struct VehicleSyncData;
struct CombatSyncData;

// Core player synchronization data
struct PlayerSyncData {
    uint32_t playerId;
    uint64_t timestamp;
    uint32_t sequenceNumber;
    SyncDataType dataType;
    SyncPriority priority;

    // Position and movement
    RED4ext::Vector4 position;
    RED4ext::Quaternion rotation;
    RED4ext::Vector4 velocity;
    RED4ext::Vector4 angularVelocity;
    bool isGrounded;
    bool isMoving;
    float moveSpeed;

    // Animation state
    std::string animationState;
    float animationTime;
    std::string weaponState;
    bool isCrouching;
    bool isAiming;
    bool isSprinting;

    // Health and status
    float health;
    float maxHealth;
    float stamina;
    float maxStamina;
    bool isAlive;
    std::vector<std::string> statusEffects;

    // Equipment
    std::string primaryWeapon;
    std::string secondaryWeapon;
    std::string currentWeapon;
    std::unordered_map<std::string, std::string> equipment;

    // Vehicle information
    uint32_t vehicleId;
    bool isInVehicle;
    bool isVehicleDriver;
    RED4ext::Vector4 vehiclePosition;
    RED4ext::Quaternion vehicleRotation;

    // Combat state
    bool isInCombat;
    uint32_t targetPlayerId;
    std::string combatStance;
    float aimDirection;

    // Custom data
    std::unordered_map<std::string, std::string> customData;
};

// Player snapshot for interpolation
struct PlayerSnapshot {
    uint64_t timestamp;
    PlayerSyncData data;
    bool isReliable;
    float interpolationWeight;
};

// Synchronization configuration
struct SyncConfig {
    uint32_t maxPlayersSupported = 32;
    uint32_t updateFrequencyHz = 60;
    uint32_t interpolationBufferSize = 3;
    float predictionTimeMs = 100.0f;
    float maxInterpolationTimeMs = 500.0f;
    bool enableClientPrediction = true;
    bool enableServerValidation = true;
    bool enablePositionSmoothing = true;
    bool enableAnimationBlending = true;
    float positionSyncThreshold = 0.1f; // meters
    float rotationSyncThreshold = 5.0f; // degrees
    float healthSyncThreshold = 1.0f;   // health points
    InterpolationMethod positionInterpolation = InterpolationMethod::Smooth;
    InterpolationMethod rotationInterpolation = InterpolationMethod::Linear;
    uint32_t maxPacketsPerSecond = 120;
    float compressionThreshold = 0.8f;
};

// Player information
struct PlayerInfo {
    uint32_t playerId;
    std::string playerName;
    std::string characterClass;
    PlayerState state;
    std::chrono::steady_clock::time_point lastUpdateTime;
    std::chrono::steady_clock::time_point connectedTime;
    uint64_t lastSequenceNumber;
    float averageLatency;
    float packetLossRate;
    bool isLocal;
    bool isHost;

    // Sync statistics
    uint64_t packetsReceived;
    uint64_t packetsDropped;
    uint64_t interpolationsPerformed;
    uint64_t predictionsPerformed;

    // Player history for interpolation
    std::queue<PlayerSnapshot> snapshotHistory;
    PlayerSyncData currentState;
    PlayerSyncData targetState;
};

// Synchronization events
struct SyncEvent {
    enum Type {
        PlayerJoined,
        PlayerLeft,
        StateUpdated,
        ValidationFailed,
        InterpolationCompleted,
        PredictionCorrected,
        DesyncDetected,
        PacketLost
    };

    Type type;
    uint32_t playerId;
    std::chrono::steady_clock::time_point timestamp;
    std::string details;
    std::unordered_map<std::string, std::string> metadata;
};

// Player validation interface
class IPlayerValidator {
public:
    virtual ~IPlayerValidator() = default;
    virtual bool ValidatePlayerState(const PlayerSyncData& data) = 0;
    virtual bool ValidatePlayerMovement(const PlayerSyncData& previous, const PlayerSyncData& current) = 0;
    virtual bool ValidatePlayerAction(const PlayerSyncData& data, const std::string& action) = 0;
    virtual std::string GetValidationError() const = 0;
};

// Player synchronization callbacks
struct PlayerSyncCallbacks {
    std::function<void(uint32_t playerId, const PlayerSyncData&)> onPlayerStateUpdated;
    std::function<void(uint32_t playerId)> onPlayerJoined;
    std::function<void(uint32_t playerId)> onPlayerLeft;
    std::function<void(uint32_t playerId, const std::string&)> onValidationFailed;
    std::function<void(uint32_t playerId, float)> onLatencyUpdated;
    std::function<void(const SyncEvent&)> onSyncEvent;
};

// Main player synchronization manager
class PlayerSyncManager {
public:
    static PlayerSyncManager& Instance();

    // Core lifecycle
    bool Initialize(const SyncConfig& config = SyncConfig{});
    void Shutdown();
    void Update();

    // Player management
    bool RegisterPlayer(uint32_t playerId, const std::string& playerName, bool isLocal = false);
    bool UnregisterPlayer(uint32_t playerId);
    bool IsPlayerRegistered(uint32_t playerId) const;
    std::vector<uint32_t> GetRegisteredPlayers() const;
    std::optional<PlayerInfo> GetPlayerInfo(uint32_t playerId) const;

    // State synchronization
    bool UpdatePlayerState(uint32_t playerId, const PlayerSyncData& data);
    bool SendPlayerUpdate(uint32_t playerId, SyncDataType dataType = SyncDataType::Position, SyncPriority priority = SyncPriority::High);
    PlayerSyncData GetPlayerState(uint32_t playerId) const;
    PlayerSyncData GetLocalPlayerState() const;

    // Network messaging
    bool SendSyncPacket(uint32_t targetPlayerId, const PlayerSyncData& data);
    bool BroadcastSyncPacket(const PlayerSyncData& data, const std::vector<uint32_t>& excludePlayers = {});
    void ReceiveSyncPacket(const std::vector<uint8_t>& packetData, uint32_t fromPlayerId);

    // Interpolation and prediction
    bool EnableInterpolation(uint32_t playerId, bool enabled);
    bool EnablePrediction(uint32_t playerId, bool enabled);
    void SetInterpolationMethod(InterpolationMethod method);
    PlayerSyncData InterpolatePlayerState(uint32_t playerId, uint64_t targetTime);
    PlayerSyncData PredictPlayerState(uint32_t playerId, float deltaTimeMs);

    // Validation
    void RegisterValidator(std::shared_ptr<IPlayerValidator> validator);
    void UnregisterValidator();
    bool ValidatePlayerUpdate(uint32_t playerId, const PlayerSyncData& data);
    void EnableServerValidation(bool enabled);

    // Configuration
    void UpdateConfig(const SyncConfig& config);
    SyncConfig GetConfig() const;
    void SetUpdateFrequency(uint32_t frequencyHz);
    void SetMaxPlayers(uint32_t maxPlayers);

    // Statistics and monitoring
    void EnableStatistics(bool enabled);
    std::unordered_map<uint32_t, uint64_t> GetSyncStatistics() const;
    float GetAverageLatency() const;
    float GetPacketLossRate() const;
    uint32_t GetUpdateRate() const;
    std::string GenerateSyncReport() const;

    // Event handling
    void SetCallbacks(const PlayerSyncCallbacks& callbacks);
    void ClearCallbacks();

    // Advanced features
    void EnableDeltaCompression(bool enabled);
    void EnableBandwidthOptimization(bool enabled);
    void SetCompressionLevel(float level);
    void EnableAdaptiveQuality(bool enabled);

    // Local player helpers
    void SetLocalPlayerId(uint32_t playerId);
    uint32_t GetLocalPlayerId() const;
    bool UpdateLocalPlayerPosition(const RED4ext::Vector4& position, const RED4ext::Quaternion& rotation);
    bool UpdateLocalPlayerHealth(float health, float maxHealth);
    bool UpdateLocalPlayerAnimation(const std::string& animationState, float animationTime);

    // Utility functions
    uint64_t GetCurrentTimestamp() const;
    float CalculateLatency(uint32_t playerId) const;
    bool IsPlayerNearby(uint32_t playerId, float maxDistance) const;
    std::vector<uint32_t> GetPlayersInRange(const RED4ext::Vector4& center, float radius) const;

private:
    PlayerSyncManager() = default;
    ~PlayerSyncManager() = default;
    PlayerSyncManager(const PlayerSyncManager&) = delete;
    PlayerSyncManager& operator=(const PlayerSyncManager&) = delete;

    // Core processing
    void ProcessUpdateQueue();
    void ProcessInterpolation();
    void ProcessPrediction();
    void ProcessValidation();
    void UpdateStatistics();

    // Interpolation methods
    RED4ext::Vector4 InterpolatePosition(const std::vector<PlayerSnapshot>& snapshots, uint64_t targetTime);
    RED4ext::Quaternion InterpolateRotation(const std::vector<PlayerSnapshot>& snapshots, uint64_t targetTime);
    PlayerSyncData InterpolateState(const PlayerSnapshot& from, const PlayerSnapshot& to, float t);

    // Prediction methods
    RED4ext::Vector4 PredictPosition(const PlayerSyncData& current, float deltaTime);
    RED4ext::Quaternion PredictRotation(const PlayerSyncData& current, float deltaTime);

    // Packet processing
    std::vector<uint8_t> SerializeSyncData(const PlayerSyncData& data);
    PlayerSyncData DeserializeSyncData(const std::vector<uint8_t>& data);
    bool CompressPacket(std::vector<uint8_t>& packet);
    bool DecompressPacket(std::vector<uint8_t>& packet);

    // Delta compression
    std::vector<uint8_t> CreateDelta(const PlayerSyncData& previous, const PlayerSyncData& current);
    PlayerSyncData ApplyDelta(const PlayerSyncData& base, const std::vector<uint8_t>& delta);

    // Event processing
    void NotifyPlayerStateUpdated(uint32_t playerId, const PlayerSyncData& data);
    void NotifyPlayerJoined(uint32_t playerId);
    void NotifyPlayerLeft(uint32_t playerId);
    void NotifyValidationFailed(uint32_t playerId, const std::string& reason);
    void NotifySyncEvent(const SyncEvent& event);

    // Utility methods
    bool IsValidPlayerData(const PlayerSyncData& data);
    float CalculateDistance(const RED4ext::Vector4& pos1, const RED4ext::Vector4& pos2);
    uint64_t GenerateSequenceNumber();
    void CleanupOldSnapshots(uint32_t playerId);

    // Data storage
    std::unordered_map<uint32_t, PlayerInfo> m_players;
    std::queue<PlayerSyncData> m_updateQueue;
    std::unordered_map<uint32_t, PlayerSyncData> m_previousStates;

    // Configuration
    SyncConfig m_config;
    std::shared_ptr<IPlayerValidator> m_validator;

    // Local player
    uint32_t m_localPlayerId = 0;
    PlayerSyncData m_localPlayerState;

    // Statistics
    bool m_statisticsEnabled = true;
    std::unordered_map<uint32_t, uint64_t> m_syncStats;
    std::chrono::steady_clock::time_point m_lastStatUpdate;
    float m_averageLatency = 0.0f;
    float m_packetLossRate = 0.0f;
    uint32_t m_currentUpdateRate = 0;

    // Event callbacks
    PlayerSyncCallbacks m_callbacks;

    // Threading and synchronization
    mutable std::recursive_mutex m_playerMutex;
    mutable std::mutex m_updateMutex;
    mutable std::mutex m_statsMutex;
    mutable std::mutex m_callbackMutex;

    // System state
    bool m_initialized = false;
    bool m_interpolationEnabled = true;
    bool m_predictionEnabled = true;
    bool m_deltaCompressionEnabled = true;
    bool m_serverValidationEnabled = true;
    bool m_adaptiveQualityEnabled = false;

    // ID generation
    std::atomic<uint64_t> m_nextSequenceNumber{1};

    // Update timing
    std::chrono::steady_clock::time_point m_lastUpdate;
    std::chrono::microseconds m_updateInterval;
};

// Utility classes and functions
namespace PlayerSyncUtils {
    std::string GetSyncDataTypeName(SyncDataType type);
    std::string GetSyncPriorityName(SyncPriority priority);
    std::string GetInterpolationMethodName(InterpolationMethod method);
    std::string GetPlayerStateName(PlayerState state);

    // Math utilities for synchronization
    float CalculateDistance3D(const RED4ext::Vector4& a, const RED4ext::Vector4& b);
    float CalculateAngleDifference(const RED4ext::Quaternion& a, const RED4ext::Quaternion& b);
    RED4ext::Vector4 LerpVector(const RED4ext::Vector4& a, const RED4ext::Vector4& b, float t);
    RED4ext::Quaternion SlerpQuaternion(const RED4ext::Quaternion& a, const RED4ext::Quaternion& b, float t);

    // Data validation utilities
    bool IsValidPosition(const RED4ext::Vector4& position);
    bool IsValidRotation(const RED4ext::Quaternion& rotation);
    bool IsValidHealth(float health, float maxHealth);
    bool IsValidMovementSpeed(float speed);

    // Compression utilities
    std::vector<uint8_t> CompressFloat(float value, float precision = 0.01f);
    float DecompressFloat(const std::vector<uint8_t>& data);
    std::vector<uint8_t> CompressVector(const RED4ext::Vector4& vector, float precision = 0.01f);
    RED4ext::Vector4 DecompressVector(const std::vector<uint8_t>& data);

    // Time utilities
    uint64_t GetNetworkTime();
    float TimeDifferenceMs(uint64_t from, uint64_t to);
    bool IsTimestampValid(uint64_t timestamp, uint64_t maxAgeMs = 5000);
}

} // namespace CoopNet
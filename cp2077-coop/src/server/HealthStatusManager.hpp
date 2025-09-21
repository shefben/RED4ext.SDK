#pragma once

#include "RED4ext/Api/Sdk.hpp"
#include "RED4ext/Common.hpp"
#include "RED4ext/NativeTypes.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <chrono>
#include <functional>
#include <string>
#include <memory>

namespace RED4ext
{
    // Health and status enums
    enum class PlayerStance : uint8_t
    {
        Standing = 0,
        Crouching = 1,
        Prone = 2,
        Combat = 3,
        Vehicle = 4
    };

    enum class StatusEffectType : uint8_t
    {
        None = 0,
        Bleeding = 1,
        Poisoned = 2,
        Burning = 3,
        Electrified = 4,
        Stunned = 5,
        Blinded = 6,
        Slowed = 7,
        Weakened = 8,
        Berserker = 9,
        Berserk = 10,
        Healing = 11,
        Armor_Boost = 12,
        Speed_Boost = 13,
        Damage_Boost = 14
    };

    enum class HealthEventType : uint8_t
    {
        Damage = 0,
        Healing = 1,
        StatusApplied = 2,
        StatusRemoved = 3,
        Downed = 4,
        Revived = 5,
        CriticalCondition = 6
    };

    // Data structures
    struct HealthSyncData
    {
        uint32_t playerId;
        float currentHealth;
        float maxHealth;
        float currentArmor;
        float maxArmor;
        float currentStamina;
        float maxStamina;
        float healthPercentage;
        float armorPercentage;
        float staminaPercentage;
        std::chrono::steady_clock::time_point timestamp;

        // Status conditions
        bool isInCombat;
        bool isUnconscious;
        bool isBleeding;
        bool isPoisoned;
        bool isBurning;
        bool isElectrified;
        bool isStunned;
        bool isBlinded;

        // Position for downed state
        float posX, posY, posZ;

        HealthSyncData()
            : playerId(0), currentHealth(100.0f), maxHealth(100.0f),
              currentArmor(0.0f), maxArmor(100.0f), currentStamina(100.0f),
              maxStamina(100.0f), healthPercentage(1.0f), armorPercentage(0.0f),
              staminaPercentage(1.0f), timestamp(std::chrono::steady_clock::now()),
              isInCombat(false), isUnconscious(false), isBleeding(false),
              isPoisoned(false), isBurning(false), isElectrified(false),
              isStunned(false), isBlinded(false), posX(0.0f), posY(0.0f), posZ(0.0f) {}
    };

    struct StatusEffectData
    {
        uint32_t playerId;
        StatusEffectType effectType;
        bool isActive;
        float duration;
        float intensity;
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point timestamp;

        StatusEffectData()
            : playerId(0), effectType(StatusEffectType::None), isActive(false),
              duration(0.0f), intensity(1.0f), startTime(std::chrono::steady_clock::now()),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct PlayerConditions
    {
        uint32_t playerId;
        bool isMoving;
        bool isSprinting;
        bool isCrouching;
        bool isAiming;
        bool isInVehicle;
        bool isSwimming;
        bool isClimbing;
        bool isJumping;
        bool isSliding;
        PlayerStance currentStance;
        float movementSpeed;
        std::chrono::steady_clock::time_point timestamp;

        PlayerConditions()
            : playerId(0), isMoving(false), isSprinting(false), isCrouching(false),
              isAiming(false), isInVehicle(false), isSwimming(false),
              isClimbing(false), isJumping(false), isSliding(false),
              currentStance(PlayerStance::Standing), movementSpeed(0.0f),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct CriticalHealthEvent
    {
        uint32_t playerId;
        HealthEventType eventType;
        float healthBefore;
        float healthAfter;
        float damageAmount;
        uint32_t attackerId;
        std::string weaponType;
        bool canBeRevived;
        float reviveTime;
        std::chrono::steady_clock::time_point timestamp;

        CriticalHealthEvent()
            : playerId(0), eventType(HealthEventType::Damage), healthBefore(0.0f),
              healthAfter(0.0f), damageAmount(0.0f), attackerId(0),
              canBeRevived(false), reviveTime(0.0f),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct PlayerHealthState
    {
        uint32_t playerId;
        std::string playerName;
        HealthSyncData healthData;
        std::unordered_map<StatusEffectType, StatusEffectData> activeEffects;
        PlayerConditions conditions;
        std::vector<CriticalHealthEvent> recentEvents;

        std::chrono::steady_clock::time_point lastHealthUpdate;
        std::chrono::steady_clock::time_point lastStatusUpdate;
        std::chrono::steady_clock::time_point lastConditionUpdate;
        std::chrono::steady_clock::time_point lastActivity;

        bool isConnected;
        float syncPriority; // Higher priority = more frequent updates

        PlayerHealthState()
            : playerId(0), lastHealthUpdate(std::chrono::steady_clock::now()),
              lastStatusUpdate(std::chrono::steady_clock::now()),
              lastConditionUpdate(std::chrono::steady_clock::now()),
              lastActivity(std::chrono::steady_clock::now()),
              isConnected(false), syncPriority(1.0f) {}
    };

    // Main health status management class
    class HealthStatusManager
    {
    public:
        static HealthStatusManager& GetInstance();

        // Lifecycle
        void Initialize();
        void Shutdown();
        void Update();

        // Player management
        void AddPlayer(uint32_t playerId, const std::string& playerName);
        void RemovePlayer(uint32_t playerId);
        void UpdatePlayerActivity(uint32_t playerId);

        // Health synchronization
        void UpdatePlayerHealth(uint32_t playerId, const HealthSyncData& healthData);
        void ProcessHealthEvent(uint32_t playerId, const CriticalHealthEvent& event);
        void SyncCriticalHealth(uint32_t playerId, HealthEventType eventType);

        // Status effect management
        void ApplyStatusEffect(uint32_t playerId, const StatusEffectData& effect);
        void RemoveStatusEffect(uint32_t playerId, StatusEffectType effectType);
        void UpdateStatusEffect(uint32_t playerId, const StatusEffectData& effect);
        void ClearAllStatusEffects(uint32_t playerId);

        // Condition monitoring
        void UpdatePlayerConditions(uint32_t playerId, const PlayerConditions& conditions);
        void UpdatePlayerPosition(uint32_t playerId, float x, float y, float z);
        void UpdatePlayerStance(uint32_t playerId, PlayerStance stance);

        // Critical events
        void OnPlayerDowned(uint32_t playerId, uint32_t attackerId = 0, const std::string& weaponType = "");
        void OnPlayerRevived(uint32_t playerId, uint32_t reviverId = 0);
        void OnPlayerDamaged(uint32_t playerId, float damage, uint32_t attackerId = 0);
        void OnPlayerHealed(uint32_t playerId, float healing, uint32_t healerId = 0);

        // Query methods
        PlayerHealthState* GetPlayerHealthState(uint32_t playerId);
        const PlayerHealthState* GetPlayerHealthState(uint32_t playerId) const;
        std::vector<uint32_t> GetPlayersInCriticalCondition() const;
        std::vector<uint32_t> GetPlayersWithStatusEffect(StatusEffectType effectType) const;
        std::vector<uint32_t> GetPlayersInCombat() const;

        // Health validation and anti-cheat
        bool ValidateHealthData(uint32_t playerId, const HealthSyncData& healthData) const;
        bool IsHealthChangeValid(uint32_t playerId, float oldHealth, float newHealth, float deltaTime) const;
        void DetectHealthAnomalies(uint32_t playerId);

        // Synchronization control
        void SetSyncPriority(uint32_t playerId, float priority);
        void ForceSyncPlayer(uint32_t playerId);
        void BroadcastHealthUpdate(uint32_t playerId);
        void BroadcastStatusEffectUpdate(uint32_t playerId, StatusEffectType effectType);
        void BroadcastCriticalEvent(uint32_t playerId, const CriticalHealthEvent& event);

        // Statistics and monitoring
        uint32_t GetActivePlayerCount() const;
        float GetAveragePlayerHealth() const;
        uint32_t GetTotalStatusEffectsActive() const;
        std::unordered_map<StatusEffectType, uint32_t> GetStatusEffectDistribution() const;

        // Event callbacks
        using HealthUpdatedCallback = std::function<void(uint32_t playerId, const HealthSyncData& healthData)>;
        using StatusEffectChangedCallback = std::function<void(uint32_t playerId, const StatusEffectData& effect)>;
        using CriticalEventCallback = std::function<void(uint32_t playerId, const CriticalHealthEvent& event)>;
        using PlayerDownedCallback = std::function<void(uint32_t playerId, uint32_t attackerId)>;
        using PlayerRevivedCallback = std::function<void(uint32_t playerId, uint32_t reviverId)>;
        using ConditionsUpdatedCallback = std::function<void(uint32_t playerId, const PlayerConditions& conditions)>;

        void SetHealthUpdatedCallback(HealthUpdatedCallback callback);
        void SetStatusEffectChangedCallback(StatusEffectChangedCallback callback);
        void SetCriticalEventCallback(CriticalEventCallback callback);
        void SetPlayerDownedCallback(PlayerDownedCallback callback);
        void SetPlayerRevivedCallback(PlayerRevivedCallback callback);
        void SetConditionsUpdatedCallback(ConditionsUpdatedCallback callback);

    private:
        HealthStatusManager() = default;
        ~HealthStatusManager() = default;
        HealthStatusManager(const HealthStatusManager&) = delete;
        HealthStatusManager& operator=(const HealthStatusManager&) = delete;

        // Internal data
        std::unordered_map<uint32_t, std::unique_ptr<PlayerHealthState>> m_playerStates;
        std::unordered_map<StatusEffectType, std::vector<uint32_t>> m_effectToPlayers;
        std::vector<CriticalHealthEvent> m_recentCriticalEvents;

        // Thread safety
        mutable std::shared_mutex m_statesMutex;
        mutable std::mutex m_callbacksMutex;
        mutable std::mutex m_eventsMutex;

        // Update timing
        std::chrono::steady_clock::time_point m_lastUpdate;
        std::chrono::steady_clock::time_point m_lastCleanup;
        float m_updateInterval;

        // Statistics
        uint32_t m_totalHealthUpdates;
        uint32_t m_totalStatusEffectChanges;
        uint32_t m_totalCriticalEvents;

        // Event callbacks
        HealthUpdatedCallback m_healthUpdatedCallback;
        StatusEffectChangedCallback m_statusEffectChangedCallback;
        CriticalEventCallback m_criticalEventCallback;
        PlayerDownedCallback m_playerDownedCallback;
        PlayerRevivedCallback m_playerRevivedCallback;
        ConditionsUpdatedCallback m_conditionsUpdatedCallback;

        // Internal methods
        void UpdatePlayerHealthStates(float deltaTime);
        void ProcessStatusEffectUpdates(float deltaTime);
        void CleanupExpiredEffects();
        void CleanupOldEvents();
        void ValidatePlayerStates();

        bool IsValidHealthValue(float health, float maxHealth) const;
        bool IsValidArmorValue(float armor, float maxArmor) const;
        bool IsValidStaminaValue(float stamina, float maxStamina) const;

        float CalculateHealthChangeRate(uint32_t playerId, float newHealth, float deltaTime) const;
        bool IsHealthChangeRateValid(float changeRate, bool isHealing) const;

        void UpdateEffectToPlayersMapping(uint32_t playerId, StatusEffectType effectType, bool isActive);
        void RemovePlayerFromAllEffectMappings(uint32_t playerId);
        void UpdateHealthDataFromEffects(PlayerHealthState* playerState);

        void NotifyHealthUpdated(uint32_t playerId, const HealthSyncData& healthData);
        void NotifyStatusEffectChanged(uint32_t playerId, const StatusEffectData& effect);
        void NotifyCriticalEvent(uint32_t playerId, const CriticalHealthEvent& event);
        void NotifyPlayerDowned(uint32_t playerId, uint32_t attackerId);
        void NotifyPlayerRevived(uint32_t playerId, uint32_t reviverId);
        void NotifyConditionsUpdated(uint32_t playerId, const PlayerConditions& conditions);

        void SendHealthUpdateToClients(uint32_t playerId, const HealthSyncData& healthData);
        void SendStatusEffectUpdateToClients(uint32_t playerId, const StatusEffectData& effect);
        void SendCriticalEventToClients(uint32_t playerId, const CriticalHealthEvent& event);
        void SendConditionsUpdateToClients(uint32_t playerId, const PlayerConditions& conditions);
    };

    // Utility functions for health status management
    namespace HealthStatusUtils
    {
        std::string PlayerStanceToString(PlayerStance stance);
        PlayerStance StringToPlayerStance(const std::string& stanceStr);

        std::string StatusEffectTypeToString(StatusEffectType effectType);
        StatusEffectType StringToStatusEffectType(const std::string& effectStr);

        std::string HealthEventTypeToString(HealthEventType eventType);
        HealthEventType StringToHealthEventType(const std::string& eventStr);

        bool IsDebuffEffect(StatusEffectType effectType);
        bool IsBuffEffect(StatusEffectType effectType);
        bool IsCriticalStatusEffect(StatusEffectType effectType);

        float CalculateHealthPercentage(float current, float maximum);
        float CalculateEffectIntensity(const StatusEffectData& effect, float deltaTime);

        bool ShouldSyncImmediately(const HealthSyncData& oldData, const HealthSyncData& newData);
        float CalculateSyncPriority(const PlayerHealthState& state);

        uint32_t HashHealthState(const HealthSyncData& healthData);
        bool IsHealthDataEquivalent(const HealthSyncData& data1, const HealthSyncData& data2, float tolerance = 1.0f);
    }

    // Network message structures for client-server communication
    struct HealthStateUpdate
    {
        uint32_t playerId;
        HealthSyncData healthData;
        std::vector<StatusEffectData> activeEffects;
        PlayerConditions conditions;
        std::chrono::steady_clock::time_point updateTime;
        uint32_t syncVersion;
    };

    struct StatusEffectUpdate
    {
        uint32_t playerId;
        StatusEffectData effectData;
        bool isApplication; // true for apply, false for remove
        std::chrono::steady_clock::time_point updateTime;
    };

    struct CriticalHealthUpdate
    {
        uint32_t playerId;
        CriticalHealthEvent event;
        HealthSyncData currentState;
        std::chrono::steady_clock::time_point updateTime;
    };

    struct ConditionsUpdate
    {
        uint32_t playerId;
        PlayerConditions conditions;
        std::chrono::steady_clock::time_point updateTime;
    };
}
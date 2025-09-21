#pragma once

#include "RED4ext/Api/Sdk.hpp"
#include "RED4ext/Common.hpp"
#include "RED4ext/NativeTypes.hpp"
#include <RED4ext/Scripting/Natives/Generated/Vector3.hpp>
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
    // Combat system Vector3 to avoid conflicts with RED4ext::Vector3
    struct CombatVector3
    {
        float x, y, z;

        CombatVector3() : x(0.0f), y(0.0f), z(0.0f) {}
        CombatVector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

        // Conversion from RED4ext::Vector3
        CombatVector3(const RED4ext::Vector3& v) : x(v.X), y(v.Y), z(v.Z) {}

        // Conversion to RED4ext::Vector3
        operator RED4ext::Vector3() const { return RED4ext::Vector3(x, y, z); }
    };

    // Combat state enums matching REDscript
    enum class CombatState : uint8_t
    {
        OutOfCombat = 0,
        CombatReady = 1,
        InCombat = 2,
        ActiveCombat = 3,
        PostCombat = 4
    };

    enum class CombatStance : uint8_t
    {
        Standing = 0,
        Crouching = 1,
        InCover = 2,
        Prone = 3,
        Moving = 4
    };

    enum class CoverState : uint8_t
    {
        NoCover = 0,
        LightCover = 1,
        HeavyCover = 2,
        FullCover = 3
    };

    enum class AimingState : uint8_t
    {
        NotAiming = 0,
        HipAiming = 1,
        AimingDownSights = 2,
        Scoped = 3
    };

    enum class MovementMode : uint8_t
    {
        Walking = 0,
        Running = 1,
        Sprinting = 2,
        Sneaking = 3,
        Crawling = 4
    };

    enum class AlertLevel : uint8_t
    {
        Relaxed = 0,
        Cautious = 1,
        Alert = 2,
        Combat = 3,
        Panicked = 4
    };

    enum class FireMode : uint8_t
    {
        Single = 0,
        Burst = 1,
        FullAuto = 2,
        Charged = 3
    };

    enum class CombatEventType : uint8_t
    {
        CombatStarted = 0,
        CombatEnded = 1,
        WeaponDrawn = 2,
        WeaponHolstered = 3,
        TakingCover = 4,
        LeavingCover = 5
    };

    enum class KillMethod : uint8_t
    {
        Weapon = 0,
        Explosion = 1,
        Environmental = 2,
        Cyberware = 3,
        Melee = 4,
        Unknown = 5
    };

    enum class DamageType : uint8_t
    {
        Physical = 0,
        Thermal = 1,
        Chemical = 2,
        Electrical = 3,
        Explosive = 4,
        Emp = 5
    };

    // Data structures

    struct CombatSyncData
    {
        uint32_t playerId;
        CombatState combatState;
        CombatStance stance;
        CoverState coverState;
        AimingState aimingState;
        MovementMode movementMode;
        AlertLevel alertLevel;
        uint64_t currentWeapon;
        bool weaponDrawn;
        bool isReloading;
        bool isFiring;
        uint64_t currentTarget;
        CombatVector3 position;
        CombatVector3 aimDirection;
        std::chrono::steady_clock::time_point timestamp;

        CombatSyncData()
            : playerId(0), combatState(CombatState::OutOfCombat), stance(CombatStance::Standing),
              coverState(CoverState::NoCover), aimingState(AimingState::NotAiming),
              movementMode(MovementMode::Walking), alertLevel(AlertLevel::Relaxed),
              currentWeapon(0), weaponDrawn(false), isReloading(false), isFiring(false),
              currentTarget(0), position(), aimDirection(0.0f, 1.0f, 0.0f),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct WeaponSyncData
    {
        uint32_t playerId;
        uint64_t weaponId;
        uint32_t weaponType;
        bool isDrawn;
        bool isReloading;
        uint32_t ammoCount;
        uint32_t maxAmmo;
        float reloadProgress;
        std::chrono::steady_clock::time_point timestamp;

        WeaponSyncData()
            : playerId(0), weaponId(0), weaponType(0), isDrawn(false), isReloading(false),
              ammoCount(0), maxAmmo(0), reloadProgress(0.0f),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct TargetingSyncData
    {
        uint32_t playerId;
        uint64_t targetId;
        CombatVector3 aimDirection;
        bool isAiming;
        float aimAccuracy;
        std::chrono::steady_clock::time_point timestamp;

        TargetingSyncData()
            : playerId(0), targetId(0), aimDirection(0.0f, 1.0f, 0.0f), isAiming(false),
              aimAccuracy(1.0f), timestamp(std::chrono::steady_clock::now()) {}
    };

    struct WeaponFireData
    {
        uint32_t playerId;
        uint64_t weaponId;
        uint64_t targetId;
        FireMode fireMode;
        CombatVector3 firePosition;
        CombatVector3 aimDirection;
        uint32_t shotsFired;
        float damage;
        std::chrono::steady_clock::time_point timestamp;

        WeaponFireData()
            : playerId(0), weaponId(0), targetId(0), fireMode(FireMode::Single),
              firePosition(), aimDirection(0.0f, 1.0f, 0.0f), shotsFired(1), damage(0.0f),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct DamageDealtData
    {
        uint32_t attackerId;
        uint64_t targetId;
        float damage;
        DamageType damageType;
        CombatVector3 position;
        CombatVector3 hitDirection;
        bool isCritical;
        bool isHeadshot;
        std::chrono::steady_clock::time_point timestamp;

        DamageDealtData()
            : attackerId(0), targetId(0), damage(0.0f), damageType(DamageType::Physical),
              position(), hitDirection(0.0f, 0.0f, 0.0f), isCritical(false), isHeadshot(false),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct CombatEventData
    {
        uint32_t playerId;
        CombatEventType eventType;
        CombatVector3 position;
        uint64_t relatedEntityId;
        std::chrono::steady_clock::time_point timestamp;

        CombatEventData()
            : playerId(0), eventType(CombatEventType::CombatStarted), position(),
              relatedEntityId(0), timestamp(std::chrono::steady_clock::now()) {}
    };

    struct PlayerKillData
    {
        uint32_t killerId;
        uint64_t victimId;
        KillMethod killMethod;
        uint64_t weaponId;
        CombatVector3 position;
        bool isHeadshot;
        float distance;
        std::chrono::steady_clock::time_point timestamp;

        PlayerKillData()
            : killerId(0), victimId(0), killMethod(KillMethod::Unknown), weaponId(0),
              position(), isHeadshot(false), distance(0.0f),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct LocalCombatState
    {
        uint32_t playerId;
        CombatState combatState;
        CombatStance combatStance;
        CoverState coverState;
        AimingState aimingState;
        MovementMode movementMode;
        AlertLevel alertLevel;
        uint64_t currentWeapon;
        bool weaponDrawn;
        bool isReloading;
        bool isFiring;
        uint64_t currentTarget;
        CombatVector3 position;
        CombatVector3 aimDirection;
        std::chrono::steady_clock::time_point lastUpdate;
        bool hasStateChanged;

        LocalCombatState()
            : playerId(0), combatState(CombatState::OutOfCombat), combatStance(CombatStance::Standing),
              coverState(CoverState::NoCover), aimingState(AimingState::NotAiming),
              movementMode(MovementMode::Walking), alertLevel(AlertLevel::Relaxed),
              currentWeapon(0), weaponDrawn(false), isReloading(false), isFiring(false),
              currentTarget(0), position(), aimDirection(0.0f, 1.0f, 0.0f),
              lastUpdate(std::chrono::steady_clock::now()), hasStateChanged(true) {}
    };

    struct RemoteCombatState
    {
        uint32_t playerId;
        CombatState combatState;
        CombatStance combatStance;
        uint64_t currentWeapon;
        bool weaponDrawn;
        bool isAiming;
        bool isFiring;
        uint64_t currentTarget;
        CombatVector3 position;
        CombatVector3 aimDirection;
        std::chrono::steady_clock::time_point lastUpdate;

        RemoteCombatState()
            : playerId(0), combatState(CombatState::OutOfCombat), combatStance(CombatStance::Standing),
              currentWeapon(0), weaponDrawn(false), isAiming(false), isFiring(false),
              currentTarget(0), position(), aimDirection(0.0f, 1.0f, 0.0f),
              lastUpdate(std::chrono::steady_clock::now()) {}
    };

    struct WeaponState
    {
        uint64_t weaponId;
        uint32_t weaponType;
        bool isDrawn;
        bool isReloading;
        uint32_t ammoCount;
        uint32_t maxAmmo;
        float reloadProgress;
        uint32_t totalShots;
        float totalDamageDealt;
        std::chrono::steady_clock::time_point lastFired;
        std::chrono::steady_clock::time_point lastReload;

        WeaponState()
            : weaponId(0), weaponType(0), isDrawn(false), isReloading(false),
              ammoCount(0), maxAmmo(0), reloadProgress(0.0f), totalShots(0),
              totalDamageDealt(0.0f), lastFired(std::chrono::steady_clock::now()),
              lastReload(std::chrono::steady_clock::now()) {}
    };

    struct CombatEngagement
    {
        uint32_t engagementId;
        std::vector<uint32_t> participants;
        std::vector<uint64_t> enemyEntities;
        CombatVector3 centerPosition;
        float engagementRadius;
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point lastActivity;
        bool isActive;

        // Statistics
        uint32_t totalDamageDealt;
        uint32_t totalKills;
        uint32_t totalDeaths;

        CombatEngagement()
            : engagementId(0), centerPosition(), engagementRadius(50.0f),
              startTime(std::chrono::steady_clock::now()),
              lastActivity(std::chrono::steady_clock::now()), isActive(true),
              totalDamageDealt(0), totalKills(0), totalDeaths(0) {}
    };

    struct PlayerCombatState
    {
        uint32_t playerId;
        std::string playerName;
        LocalCombatState localState;
        std::unordered_map<uint64_t, std::unique_ptr<WeaponState>> weapons;
        std::vector<CombatEventData> recentEvents;
        std::vector<DamageDealtData> recentDamage;
        std::vector<WeaponFireData> recentShots;

        std::chrono::steady_clock::time_point lastCombatUpdate;
        std::chrono::steady_clock::time_point lastWeaponUpdate;
        std::chrono::steady_clock::time_point lastActivity;

        bool isConnected;
        float syncPriority;
        uint32_t currentEngagementId;

        // Combat statistics
        uint32_t totalShotsFired;
        float totalDamageDealt;
        uint32_t totalKills;
        uint32_t totalDeaths;
        float accuracyPercentage;

        PlayerCombatState()
            : playerId(0), currentEngagementId(0),
              lastCombatUpdate(std::chrono::steady_clock::now()),
              lastWeaponUpdate(std::chrono::steady_clock::now()),
              lastActivity(std::chrono::steady_clock::now()),
              isConnected(false), syncPriority(1.0f), totalShotsFired(0),
              totalDamageDealt(0.0f), totalKills(0), totalDeaths(0),
              accuracyPercentage(0.0f) {}
    };

    // Main combat state management class
    class CombatStateManager
    {
    public:
        static CombatStateManager& GetInstance();

        // Lifecycle
        void Initialize();
        void Shutdown();
        void Update();

        // Player management
        void AddPlayer(uint32_t playerId, const std::string& playerName);
        void RemovePlayer(uint32_t playerId);
        void UpdatePlayerActivity(uint32_t playerId);

        // Combat state management
        bool UpdateCombatState(uint32_t playerId, const CombatSyncData& combatData);
        void SynchronizeCombatState(uint32_t playerId);
        void ForceCombatSync();

        // Weapon management
        bool UpdateWeaponState(uint32_t playerId, const WeaponSyncData& weaponData);
        bool ProcessWeaponFire(uint32_t playerId, const WeaponFireData& fireData);
        void SynchronizeWeaponState(uint32_t playerId);

        // Targeting management
        bool UpdateTargeting(uint32_t playerId, const TargetingSyncData& targetingData);
        void SynchronizeTargeting(uint32_t playerId);

        // Damage management
        bool ProcessDamageDealt(uint32_t attackerId, const DamageDealtData& damageData);
        bool ProcessPlayerKill(uint32_t killerId, const PlayerKillData& killData);
        void ValidateDamageData(const DamageDealtData& damageData);

        // Combat events
        void ProcessCombatEvent(uint32_t playerId, const CombatEventData& eventData);
        void OnCombatStarted(uint32_t playerId);
        void OnCombatEnded(uint32_t playerId);

        // Combat engagements
        uint32_t StartCombatEngagement(uint32_t initiatorId, const std::vector<uint64_t>& enemyIds);
        void EndCombatEngagement(uint32_t engagementId);
        void UpdateCombatEngagements(float deltaTime);
        bool JoinCombatEngagement(uint32_t playerId, uint32_t engagementId);
        void LeaveCombatEngagement(uint32_t playerId);

        // Query methods
        PlayerCombatState* GetPlayerCombatState(uint32_t playerId);
        const PlayerCombatState* GetPlayerCombatState(uint32_t playerId) const;
        CombatEngagement* GetCombatEngagement(uint32_t engagementId);
        std::vector<uint32_t> GetPlayersInCombat() const;
        std::vector<uint32_t> GetPlayersInEngagement(uint32_t engagementId) const;
        std::vector<uint32_t> GetNearbyPlayers(uint32_t playerId, float radius) const;

        // Validation and anti-cheat
        bool ValidateCombatData(uint32_t playerId, const CombatSyncData& data) const;
        bool ValidateWeaponData(uint32_t playerId, const WeaponSyncData& data) const;
        bool ValidateFireData(uint32_t playerId, const WeaponFireData& data) const;
        void DetectCombatAnomalies(uint32_t playerId);
        bool IsFireRateLimited(uint32_t playerId, uint64_t weaponId) const;

        // Synchronization
        void BroadcastCombatUpdate(uint32_t playerId, const CombatSyncData& data);
        void BroadcastWeaponUpdate(uint32_t playerId, const WeaponSyncData& data);
        void BroadcastWeaponFire(uint32_t playerId, const WeaponFireData& fireData);
        void BroadcastDamageDealt(uint32_t attackerId, const DamageDealtData& damageData);
        void BroadcastCombatEvent(uint32_t playerId, const CombatEventData& eventData);
        void ForceSyncPlayer(uint32_t playerId);
        void SetSyncPriority(uint32_t playerId, float priority);

        // Statistics and monitoring
        uint32_t GetActivePlayerCount() const;
        uint32_t GetActiveCombatEngagements() const;
        uint32_t GetTotalShotsFired() const;
        float GetTotalDamageDealt() const;
        std::unordered_map<uint32_t, float> GetPlayerDamageStats() const;
        std::unordered_map<uint32_t, uint32_t> GetPlayerKillStats() const;

        // Event callbacks
        using CombatStateChangedCallback = std::function<void(uint32_t playerId, CombatState oldState, CombatState newState)>;
        using WeaponFiredCallback = std::function<void(uint32_t playerId, const WeaponFireData& fireData)>;
        using DamageDealtCallback = std::function<void(uint32_t attackerId, const DamageDealtData& damageData)>;
        using PlayerKilledCallback = std::function<void(uint32_t killerId, const PlayerKillData& killData)>;
        using CombatEngagementCallback = std::function<void(uint32_t engagementId, bool started)>;

        void SetCombatStateChangedCallback(CombatStateChangedCallback callback);
        void SetWeaponFiredCallback(WeaponFiredCallback callback);
        void SetDamageDealtCallback(DamageDealtCallback callback);
        void SetPlayerKilledCallback(PlayerKilledCallback callback);
        void SetCombatEngagementCallback(CombatEngagementCallback callback);

    private:
        CombatStateManager() = default;
        ~CombatStateManager() = default;
        CombatStateManager(const CombatStateManager&) = delete;
        CombatStateManager& operator=(const CombatStateManager&) = delete;

        // Internal data
        std::unordered_map<uint32_t, std::unique_ptr<PlayerCombatState>> m_playerStates;
        std::unordered_map<uint32_t, std::unique_ptr<RemoteCombatState>> m_remoteCombatStates;
        std::unordered_map<uint32_t, std::unique_ptr<CombatEngagement>> m_combatEngagements;
        std::unordered_map<CombatState, std::vector<uint32_t>> m_stateToPlayers;

        // Thread safety
        mutable std::shared_mutex m_statesMutex;
        mutable std::mutex m_callbacksMutex;
        mutable std::shared_mutex m_engagementsMutex;

        // Update timing
        std::chrono::steady_clock::time_point m_lastUpdate;
        std::chrono::steady_clock::time_point m_lastCleanup;
        float m_updateInterval;

        // Statistics
        uint32_t m_totalShotsFired;
        float m_totalDamageDealt;
        uint32_t m_totalCombatEngagements;
        uint32_t m_totalPlayerKills;

        // Event callbacks
        CombatStateChangedCallback m_combatStateChangedCallback;
        WeaponFiredCallback m_weaponFiredCallback;
        DamageDealtCallback m_damageDealtCallback;
        PlayerKilledCallback m_playerKilledCallback;
        CombatEngagementCallback m_combatEngagementCallback;

        // Internal methods
        uint32_t GenerateEngagementId();
        void UpdatePlayerCombatStates(float deltaTime);
        void ProcessRecentEvents(float deltaTime);
        void CleanupExpiredData();
        void ValidateCombatStates();

        CombatState DetermineCombatState(const PlayerCombatState& playerState) const;
        AlertLevel CalculateAlertLevel(const PlayerCombatState& playerState) const;
        bool ShouldStartCombatEngagement(uint32_t playerId) const;
        float CalculateEngagementDistance(const CombatVector3& pos1, const CombatVector3& pos2) const;

        void UpdateStateToPlayersMapping(uint32_t playerId, CombatState combatState, bool isActive);
        void RemovePlayerFromAllMappings(uint32_t playerId);

        bool IsWeaponFireValid(uint32_t playerId, const WeaponFireData& fireData) const;
        bool IsDamageValid(const DamageDealtData& damageData) const;
        float CalculateFireRate(uint32_t playerId, uint64_t weaponId) const;
        float CalculateMaxDamageRate(uint32_t playerId) const;

        void NotifyCombatStateChanged(uint32_t playerId, CombatState oldState, CombatState newState);
        void NotifyWeaponFired(uint32_t playerId, const WeaponFireData& fireData);
        void NotifyDamageDealt(uint32_t attackerId, const DamageDealtData& damageData);
        void NotifyPlayerKilled(uint32_t killerId, const PlayerKillData& killData);
        void NotifyCombatEngagement(uint32_t engagementId, bool started);

        void SendCombatUpdateToClients(uint32_t playerId, const CombatSyncData& data);
        void SendWeaponUpdateToClients(uint32_t playerId, const WeaponSyncData& data);
        void SendWeaponFireToClients(uint32_t playerId, const WeaponFireData& fireData);
        void SendDamageUpdateToClients(uint32_t attackerId, const DamageDealtData& damageData);
        void SendCombatEventToClients(uint32_t playerId, const CombatEventData& eventData);
    };

    // Utility functions for combat management
    namespace CombatUtils
    {
        std::string CombatStateToString(CombatState state);
        CombatState StringToCombatState(const std::string& stateStr);

        std::string CombatStanceToString(CombatStance stance);
        CombatStance StringToCombatStance(const std::string& stanceStr);

        std::string AimingStateToString(AimingState state);
        std::string MovementModeToString(MovementMode mode);
        std::string AlertLevelToString(AlertLevel level);
        std::string FireModeToString(FireMode mode);
        std::string KillMethodToString(KillMethod method);
        std::string DamageTypeToString(DamageType type);

        bool IsOffensiveCombatState(CombatState state);
        bool IsDefensiveCombatStance(CombatStance stance);
        bool IsHighAlertLevel(AlertLevel level);

        float CalculateDistance(const CombatVector3& pos1, const CombatVector3& pos2);
        CombatVector3 CalculateDirection(const CombatVector3& from, const CombatVector3& to);
        float CalculateDamageMultiplier(DamageType damageType, bool isCritical, bool isHeadshot);

        bool IsValidPosition(const CombatVector3& position);
        bool IsValidDirection(const CombatVector3& direction);
        bool ShouldSyncCombatState(const CombatSyncData& oldData, const CombatSyncData& newData);

        uint32_t HashCombatState(const PlayerCombatState& state);
        bool AreCombatStatesEquivalent(const CombatSyncData& data1, const CombatSyncData& data2, float tolerance = 0.1f);
    }

    // Network message structures for client-server communication
    struct CombatStateUpdate
    {
        uint32_t playerId;
        CombatSyncData combatData;
        std::vector<WeaponSyncData> weaponStates;
        TargetingSyncData targetingData;
        std::chrono::steady_clock::time_point updateTime;
        uint32_t syncVersion;
    };

    struct WeaponFireUpdate
    {
        uint32_t playerId;
        WeaponFireData fireData;
        std::vector<DamageDealtData> damageData;
        std::chrono::steady_clock::time_point updateTime;
    };

    struct CombatEngagementUpdate
    {
        uint32_t engagementId;
        std::vector<uint32_t> participants;
        CombatVector3 centerPosition;
        bool isStarting; // true for start, false for end
        std::chrono::steady_clock::time_point updateTime;
    };

    struct DamageUpdate
    {
        uint32_t attackerId;
        DamageDealtData damageData;
        bool isKill;
        std::chrono::steady_clock::time_point updateTime;
    };
}
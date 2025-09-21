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
    // Cyberware enums matching REDscript
    enum class CyberwareState : uint8_t
    {
        Operational = 0,
        Active = 1,
        Degraded = 2,
        Damaged = 3,
        Malfunctioning = 4,
        Offline = 5
    };

    enum class CyberwareAbility : uint16_t
    {
        // Arm Cyberware
        Mantis_Blades = 0,
        Monowire = 1,
        Projectile_Launch_System = 2,
        Gorilla_Arms = 3,

        // Leg Cyberware
        Reinforced_Tendons = 10,
        Lynx_Paws = 11,
        Fortified_Ankles = 12,

        // Eye Cyberware
        Kiroshi_Optics = 20,
        Ballistic_Coprocessor = 21,
        Target_Analysis = 22,

        // Nervous System
        Kerenzikov = 30,
        Sandevistan = 31,
        Synaptic_Signal_Optimizer = 32,

        // Circulatory System
        Biomonitor = 40,
        Blood_Pump = 41,
        Biomodulator = 42,

        // Other Systems
        Subdermal_Armor = 50,
        Optical_Camo = 51,
        Thermal_Damage_Protection = 52
    };

    enum class MalfunctionType : uint8_t
    {
        None = 0,
        ComponentFailure = 1,
        PowerFailure = 2,
        SoftwareGlitch = 3,
        OverHeating = 4,
        SignalInterference = 5,
        MemoryCorruption = 6
    };

    enum class MalfunctionSeverity : uint8_t
    {
        None = 0,
        Minor = 1,
        Moderate = 2,
        Major = 3,
        Critical = 4
    };

    enum class CyberwareSlot : uint8_t
    {
        SystemReplacementCyberware = 0,
        ArmsCyberware = 1,
        LegsCyberware = 2,
        NervousSystemCyberware = 3,
        IntegumentarySystemCyberware = 4,
        FrontalCortexCyberware = 5,
        OcularCyberware = 6,
        CardiovascularSystemCyberware = 7,
        ImmuneSystemCyberware = 8,
        MusculoskeletalSystemCyberware = 9,
        HandsCyberware = 10,
        EyesCyberware = 11
    };

    // Data structures
    struct CyberwareSyncData
    {
        uint32_t playerId;
        uint32_t cyberwareId;
        CyberwareSlot slotType;
        CyberwareState currentState;
        float healthPercentage;
        bool isActive;
        bool isOnCooldown;
        float cooldownRemaining;
        bool isMalfunctioning;
        float batteryLevel;
        std::chrono::steady_clock::time_point timestamp;

        CyberwareSyncData()
            : playerId(0), cyberwareId(0), slotType(CyberwareSlot::SystemReplacementCyberware),
              currentState(CyberwareState::Operational), healthPercentage(1.0f), isActive(false),
              isOnCooldown(false), cooldownRemaining(0.0f), isMalfunctioning(false),
              batteryLevel(1.0f), timestamp(std::chrono::steady_clock::now()) {}
    };

    struct CyberwareAbilityData
    {
        uint32_t playerId;
        CyberwareAbility abilityType;
        bool isActivated;
        float duration;
        float intensity;
        std::chrono::steady_clock::time_point timestamp;

        CyberwareAbilityData()
            : playerId(0), abilityType(CyberwareAbility::Mantis_Blades), isActivated(false),
              duration(0.0f), intensity(1.0f), timestamp(std::chrono::steady_clock::now()) {}
    };

    struct CyberwareCooldownData
    {
        uint32_t playerId;
        uint32_t cyberwareId;
        bool isOnCooldown;
        float cooldownDuration;
        float remainingTime;
        std::chrono::steady_clock::time_point timestamp;

        CyberwareCooldownData()
            : playerId(0), cyberwareId(0), isOnCooldown(false), cooldownDuration(0.0f),
              remainingTime(0.0f), timestamp(std::chrono::steady_clock::now()) {}
    };

    struct CyberwareMalfunctionData
    {
        uint32_t playerId;
        uint32_t cyberwareId;
        MalfunctionType malfunctionType;
        MalfunctionSeverity severity;
        bool isActive;
        float startTime;
        std::chrono::steady_clock::time_point timestamp;

        CyberwareMalfunctionData()
            : playerId(0), cyberwareId(0), malfunctionType(MalfunctionType::None),
              severity(MalfunctionSeverity::None), isActive(false), startTime(0.0f),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct SlowMotionData
    {
        uint32_t playerId;
        float factor;
        float duration;
        float remainingTime;
        bool isActive;
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point timestamp;

        SlowMotionData()
            : playerId(0), factor(1.0f), duration(0.0f), remainingTime(0.0f),
              isActive(false), startTime(std::chrono::steady_clock::now()),
              timestamp(std::chrono::steady_clock::now()) {}
    };

    struct ActiveCyberware
    {
        uint32_t cyberwareId;
        CyberwareSlot slot;
        CyberwareState state;
        CyberwareAbility primaryAbility;
        float healthPercentage;
        float batteryLevel;
        bool isActive;
        bool isOnCooldown;
        float cooldownRemaining;
        bool isMalfunctioning;
        MalfunctionType malfunctionType;
        MalfunctionSeverity malfunctionSeverity;
        std::chrono::steady_clock::time_point lastUpdate;
        std::chrono::steady_clock::time_point installTime;

        ActiveCyberware()
            : cyberwareId(0), slot(CyberwareSlot::SystemReplacementCyberware),
              state(CyberwareState::Operational), primaryAbility(CyberwareAbility::Mantis_Blades),
              healthPercentage(1.0f), batteryLevel(1.0f), isActive(false), isOnCooldown(false),
              cooldownRemaining(0.0f), isMalfunctioning(false), malfunctionType(MalfunctionType::None),
              malfunctionSeverity(MalfunctionSeverity::None),
              lastUpdate(std::chrono::steady_clock::now()),
              installTime(std::chrono::steady_clock::now()) {}
    };

    struct PlayerCyberwareState
    {
        uint32_t playerId;
        std::string playerName;
        std::unordered_map<uint32_t, std::unique_ptr<ActiveCyberware>> installedCyberware;
        std::vector<CyberwareAbilityData> recentAbilities;
        SlowMotionData activeSlowMotion;

        std::chrono::steady_clock::time_point lastCyberwareUpdate;
        std::chrono::steady_clock::time_point lastAbilityUpdate;
        std::chrono::steady_clock::time_point lastActivity;

        bool isConnected;
        float syncPriority;
        uint32_t totalCyberwareCount;

        // Statistics
        uint32_t abilitiesUsed;
        uint32_t malfunctionsOccurred;
        uint32_t cyberwareInstalled;
        uint32_t cyberwareRemoved;

        PlayerCyberwareState()
            : playerId(0), lastCyberwareUpdate(std::chrono::steady_clock::now()),
              lastAbilityUpdate(std::chrono::steady_clock::now()),
              lastActivity(std::chrono::steady_clock::now()),
              isConnected(false), syncPriority(1.0f), totalCyberwareCount(0),
              abilitiesUsed(0), malfunctionsOccurred(0), cyberwareInstalled(0),
              cyberwareRemoved(0) {}
    };

    // Main cyberware management class
    class CyberwareManager
    {
    public:
        static CyberwareManager& GetInstance();

        // Lifecycle
        void Initialize();
        void Shutdown();
        void Update();

        // Player management
        void AddPlayer(uint32_t playerId, const std::string& playerName);
        void RemovePlayer(uint32_t playerId);
        void UpdatePlayerActivity(uint32_t playerId);

        // Cyberware management
        bool InstallCyberware(uint32_t playerId, uint32_t cyberwareId, CyberwareSlot slot, CyberwareAbility primaryAbility);
        bool RemoveCyberware(uint32_t playerId, uint32_t cyberwareId);
        bool UpdateCyberwareState(uint32_t playerId, const CyberwareSyncData& cyberwareData);
        void SynchronizeCyberware(uint32_t playerId);

        // Ability management
        bool ActivateCyberwareAbility(uint32_t playerId, const CyberwareAbilityData& abilityData);
        bool DeactivateCyberwareAbility(uint32_t playerId, CyberwareAbility abilityType);
        void UpdateAbilityCooldowns(float deltaTime);
        bool IsAbilityOnCooldown(uint32_t playerId, CyberwareAbility abilityType) const;

        // Slow motion management
        bool ActivateSlowMotion(uint32_t playerId, float factor, float duration);
        void UpdateSlowMotionEffects(float deltaTime);
        bool IsPlayerInSlowMotion(uint32_t playerId) const;

        // Cooldown management
        void StartCyberwareCooldown(uint32_t playerId, uint32_t cyberwareId, float duration);
        void UpdateCooldowns(float deltaTime);
        float GetCyberwareCooldownRemaining(uint32_t playerId, uint32_t cyberwareId) const;

        // Malfunction management
        void TriggerCyberwareMalfunction(uint32_t playerId, uint32_t cyberwareId, MalfunctionType type, MalfunctionSeverity severity);
        void ResolveCyberwareMalfunction(uint32_t playerId, uint32_t cyberwareId);
        void UpdateMalfunctions(float deltaTime);
        bool HasActiveMalfunction(uint32_t playerId, uint32_t cyberwareId) const;

        // Query methods
        PlayerCyberwareState* GetPlayerCyberwareState(uint32_t playerId);
        const PlayerCyberwareState* GetPlayerCyberwareState(uint32_t playerId) const;
        ActiveCyberware* GetPlayerCyberware(uint32_t playerId, uint32_t cyberwareId);
        std::vector<uint32_t> GetPlayersWithCyberware(CyberwareAbility abilityType) const;
        std::vector<uint32_t> GetPlayersWithMalfunctions() const;
        std::vector<uint32_t> GetPlayersInSlowMotion() const;

        // Validation and anti-cheat
        bool ValidateCyberwareData(uint32_t playerId, const CyberwareSyncData& data) const;
        bool ValidateAbilityUsage(uint32_t playerId, const CyberwareAbilityData& abilityData) const;
        bool IsAbilityUsageRateLimited(uint32_t playerId, CyberwareAbility abilityType) const;
        void DetectCyberwareAnomalies(uint32_t playerId);

        // Synchronization
        void BroadcastCyberwareUpdate(uint32_t playerId, const CyberwareSyncData& data);
        void BroadcastAbilityActivation(uint32_t playerId, const CyberwareAbilityData& abilityData);
        void BroadcastSlowMotionEffect(uint32_t playerId, const SlowMotionData& slowMoData);
        void ForceSyncPlayer(uint32_t playerId);
        void SetSyncPriority(uint32_t playerId, float priority);

        // Statistics and monitoring
        uint32_t GetActivePlayerCount() const;
        uint32_t GetTotalInstalledCyberware() const;
        uint32_t GetActiveMalfunctionCount() const;
        std::unordered_map<CyberwareAbility, uint32_t> GetAbilityUsageStats() const;
        std::unordered_map<MalfunctionType, uint32_t> GetMalfunctionStats() const;

        // Event callbacks
        using CyberwareInstalledCallback = std::function<void(uint32_t playerId, uint32_t cyberwareId, CyberwareSlot slot)>;
        using CyberwareRemovedCallback = std::function<void(uint32_t playerId, uint32_t cyberwareId)>;
        using AbilityActivatedCallback = std::function<void(uint32_t playerId, const CyberwareAbilityData& abilityData)>;
        using MalfunctionTriggeredCallback = std::function<void(uint32_t playerId, const CyberwareMalfunctionData& malfunctionData)>;
        using SlowMotionActivatedCallback = std::function<void(uint32_t playerId, const SlowMotionData& slowMoData)>;

        void SetCyberwareInstalledCallback(CyberwareInstalledCallback callback);
        void SetCyberwareRemovedCallback(CyberwareRemovedCallback callback);
        void SetAbilityActivatedCallback(AbilityActivatedCallback callback);
        void SetMalfunctionTriggeredCallback(MalfunctionTriggeredCallback callback);
        void SetSlowMotionActivatedCallback(SlowMotionActivatedCallback callback);

    private:
        CyberwareManager() = default;
        ~CyberwareManager() = default;
        CyberwareManager(const CyberwareManager&) = delete;
        CyberwareManager& operator=(const CyberwareManager&) = delete;

        // Internal data
        std::unordered_map<uint32_t, std::unique_ptr<PlayerCyberwareState>> m_playerStates;
        std::unordered_map<CyberwareAbility, std::vector<uint32_t>> m_abilityToPlayers;
        std::unordered_map<uint32_t, std::vector<uint32_t>> m_playerMalfunctions; // playerId -> cyberwareIds with malfunctions

        // Thread safety
        mutable std::shared_mutex m_statesMutex;
        mutable std::mutex m_callbacksMutex;

        // Update timing
        std::chrono::steady_clock::time_point m_lastUpdate;
        std::chrono::steady_clock::time_point m_lastCleanup;
        float m_updateInterval;

        // Statistics
        uint32_t m_totalCyberwareInstalled;
        uint32_t m_totalAbilitiesUsed;
        uint32_t m_totalMalfunctions;
        uint32_t m_totalSlowMotionActivations;

        // Event callbacks
        CyberwareInstalledCallback m_cyberwareInstalledCallback;
        CyberwareRemovedCallback m_cyberwareRemovedCallback;
        AbilityActivatedCallback m_abilityActivatedCallback;
        MalfunctionTriggeredCallback m_malfunctionTriggeredCallback;
        SlowMotionActivatedCallback m_slowMotionActivatedCallback;

        // Internal methods
        void UpdatePlayerCyberware(float deltaTime);
        void ProcessAbilityExpirations();
        void ProcessSlowMotionExpirations();
        void CleanupExpiredData();
        void ValidateCyberwareStates();

        uint32_t GenerateCyberwareId();
        bool IsCyberwareCompatible(CyberwareSlot slot, CyberwareAbility ability) const;
        float GetAbilityCooldownDuration(CyberwareAbility abilityType) const;
        float GetAbilityBaseDuration(CyberwareAbility abilityType) const;
        CyberwareState DetermineOptimalState(const ActiveCyberware& cyberware) const;

        void UpdateAbilityToPlayersMapping(uint32_t playerId, CyberwareAbility abilityType, bool isActive);
        void UpdateMalfunctionMapping(uint32_t playerId, uint32_t cyberwareId, bool hasMalfunction);
        void RemovePlayerFromAllMappings(uint32_t playerId);

        bool IsAbilityRateLimited(uint32_t playerId, CyberwareAbility abilityType) const;
        bool ValidateCyberwareHealth(float healthPercentage) const;
        bool ValidateBatteryLevel(float batteryLevel) const;
        void TriggerRandomMalfunction(uint32_t playerId, uint32_t cyberwareId);

        void NotifyCyberwareInstalled(uint32_t playerId, uint32_t cyberwareId, CyberwareSlot slot);
        void NotifyCyberwareRemoved(uint32_t playerId, uint32_t cyberwareId);
        void NotifyAbilityActivated(uint32_t playerId, const CyberwareAbilityData& abilityData);
        void NotifyMalfunctionTriggered(uint32_t playerId, const CyberwareMalfunctionData& malfunctionData);
        void NotifySlowMotionActivated(uint32_t playerId, const SlowMotionData& slowMoData);

        void SendCyberwareUpdateToClients(uint32_t playerId, const CyberwareSyncData& data);
        void SendAbilityUpdateToClients(uint32_t playerId, const CyberwareAbilityData& abilityData);
        void SendSlowMotionUpdateToClients(uint32_t playerId, const SlowMotionData& slowMoData);
        void SendMalfunctionUpdateToClients(uint32_t playerId, const CyberwareMalfunctionData& malfunctionData);
    };

    // Utility functions for cyberware management
    namespace CyberwareUtils
    {
        std::string CyberwareStateToString(CyberwareState state);
        CyberwareState StringToCyberwareState(const std::string& stateStr);

        std::string CyberwareAbilityToString(CyberwareAbility ability);
        CyberwareAbility StringToCyberwareAbility(const std::string& abilityStr);

        std::string CyberwareSlotToString(CyberwareSlot slot);
        CyberwareSlot StringToCyberwareSlot(const std::string& slotStr);

        std::string MalfunctionTypeToString(MalfunctionType type);
        std::string MalfunctionSeverityToString(MalfunctionSeverity severity);

        bool IsOffensiveAbility(CyberwareAbility ability);
        bool IsDefensiveAbility(CyberwareAbility ability);
        bool IsUtilityAbility(CyberwareAbility ability);
        bool IsPassiveAbility(CyberwareAbility ability);

        float CalculateAbilityEffectiveness(const ActiveCyberware& cyberware, float baseEffectiveness);
        float CalculateMalfunctionImpact(MalfunctionType type, MalfunctionSeverity severity);
        bool ShouldTriggerMalfunction(const ActiveCyberware& cyberware, float deltaTime);

        uint32_t HashCyberwareState(const PlayerCyberwareState& state);
        bool AreCyberwareStatesEquivalent(const CyberwareSyncData& data1, const CyberwareSyncData& data2, float tolerance = 0.1f);
    }

    // Network message structures for client-server communication
    struct CyberwareStateUpdate
    {
        uint32_t playerId;
        std::vector<CyberwareSyncData> installedCyberware;
        std::vector<CyberwareAbilityData> activeAbilities;
        SlowMotionData slowMotionState;
        std::chrono::steady_clock::time_point updateTime;
        uint32_t syncVersion;
    };

    struct CyberwareAbilityUpdate
    {
        uint32_t playerId;
        CyberwareAbilityData abilityData;
        bool isActivation; // true for activate, false for deactivate
        std::chrono::steady_clock::time_point updateTime;
    };

    struct SlowMotionUpdate
    {
        uint32_t playerId;
        SlowMotionData slowMoData;
        bool isActivation; // true for activate, false for deactivate
        std::chrono::steady_clock::time_point updateTime;
    };

    struct CyberwareInstallUpdate
    {
        uint32_t playerId;
        uint32_t cyberwareId;
        CyberwareSlot slot;
        CyberwareAbility primaryAbility;
        bool isInstallation; // true for install, false for remove
        std::chrono::steady_clock::time_point updateTime;
    };
}
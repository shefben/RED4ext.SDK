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
    // Buff and debuff enums
    enum class BuffType : uint16_t
    {
        // Attribute Buffs
        StrengthBoost = 0,
        ReflexesBoost = 1,
        TechnicalAbilityBoost = 2,
        IntelligenceBoost = 3,
        CoolBoost = 4,

        // Combat Buffs
        DamageBoost = 10,
        ArmorBoost = 11,
        CriticalChanceBoost = 12,
        CriticalDamageBoost = 13,
        AccuracyBoost = 14,
        ReloadSpeedBoost = 15,

        // Movement Buffs
        SpeedBoost = 20,
        JumpBoost = 21,
        StaminaBoost = 22,
        ClimbingBoost = 23,

        // Stealth Buffs
        StealthBoost = 30,
        HackingBoost = 31,
        DetectionReduction = 32,

        // Consumable Buffs
        FoodBuff = 40,
        AlcoholBuff = 41,
        StimulantBuff = 42,
        MedicationBuff = 43,

        // Cyberware Buffs
        CyberwareBoost = 50,
        OpticsEnhancement = 51,
        ProcessingBoost = 52,
        MemoryBoost = 53,

        // Environmental Buffs
        EnvironmentalAdaptation = 60,
        TemperatureResistance = 61,
        RadiationResistance = 62
    };

    enum class DebuffType : uint16_t
    {
        // Status Effect Debuffs
        Bleeding = 0,
        Poisoned = 1,
        Burning = 2,
        Electrified = 3,
        Stunned = 4,
        Blinded = 5,
        Slowed = 6,
        Weakened = 7,

        // Environmental Debuffs
        Radiation = 10,
        ToxicAir = 11,
        ExtremeHeat = 12,
        ExtremeCold = 13,
        LowOxygen = 14,

        // Combat Debuffs
        Suppressed = 20,
        Disoriented = 21,
        Overheated = 22,
        WeaponJammed = 23,
        ArmorDamaged = 24,

        // Substance Debuffs
        AlcoholPenalty = 30,
        DrugCrash = 31,
        Withdrawal = 32,
        Overdose = 33,

        // Cyberware Debuffs
        CyberwareMalfunction = 40,
        SystemError = 41,
        MemoryLeak = 42,
        ProcessingLag = 43,

        // Psychological Debuffs
        Fear = 50,
        Confusion = 51,
        Hallucination = 52,
        Panic = 53
    };

    enum class EffectCategory : uint8_t
    {
        Attribute = 0,
        Combat = 1,
        Movement = 2,
        Stealth = 3,
        Consumable = 4,
        Cyberware = 5,
        Environmental = 6,
        Psychological = 7,
        Status = 8
    };

    enum class EffectPriority : uint8_t
    {
        Low = 0,
        Normal = 1,
        High = 2,
        Critical = 3,
        Emergency = 4
    };

    // Data structures
    struct BuffData
    {
        uint32_t playerId;
        BuffType buffType;
        bool isActive;
        float duration;
        float remainingTime;
        float intensity;
        float stackCount;
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point timestamp;
        uint32_t sourceId; // ID of player/entity that applied the buff
        std::string sourceType; // "consumable", "cyberware", "skill", etc.

        BuffData()
            : playerId(0), buffType(BuffType::StrengthBoost), isActive(false),
              duration(0.0f), remainingTime(0.0f), intensity(1.0f), stackCount(1.0f),
              startTime(std::chrono::steady_clock::now()),
              timestamp(std::chrono::steady_clock::now()),
              sourceId(0) {}
    };

    struct DebuffData
    {
        uint32_t playerId;
        DebuffType debuffType;
        bool isActive;
        float duration;
        float remainingTime;
        float intensity;
        float stackCount;
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point timestamp;
        uint32_t sourceId; // ID of player/entity that applied the debuff
        std::string sourceType; // "damage", "environment", "consumable", etc.

        DebuffData()
            : playerId(0), debuffType(DebuffType::Bleeding), isActive(false),
              duration(0.0f), remainingTime(0.0f), intensity(1.0f), stackCount(1.0f),
              startTime(std::chrono::steady_clock::now()),
              timestamp(std::chrono::steady_clock::now()),
              sourceId(0) {}
    };

    struct ActiveEffect
    {
        uint32_t effectId;
        bool isBuff; // true for buff, false for debuff
        union {
            BuffType buffType;
            DebuffType debuffType;
        };

        float duration;
        float remainingTime;
        float intensity;
        float stackCount;
        EffectCategory category;
        EffectPriority priority;

        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point lastUpdate;
        uint32_t sourceId;
        std::string sourceType;

        bool isPermanent;
        bool canStack;
        float maxStacks;
        bool refreshOnReapply;

        ActiveEffect()
            : effectId(0), isBuff(true), buffType(BuffType::StrengthBoost),
              duration(0.0f), remainingTime(0.0f), intensity(1.0f), stackCount(1.0f),
              category(EffectCategory::Attribute), priority(EffectPriority::Normal),
              startTime(std::chrono::steady_clock::now()),
              lastUpdate(std::chrono::steady_clock::now()),
              sourceId(0), isPermanent(false), canStack(false),
              maxStacks(1.0f), refreshOnReapply(true) {}
    };

    struct PlayerEffectState
    {
        uint32_t playerId;
        std::string playerName;
        std::unordered_map<BuffType, std::unique_ptr<ActiveEffect>> activeBuffs;
        std::unordered_map<DebuffType, std::unique_ptr<ActiveEffect>> activeDebuffs;

        std::chrono::steady_clock::time_point lastBuffUpdate;
        std::chrono::steady_clock::time_point lastDebuffUpdate;
        std::chrono::steady_clock::time_point lastActivity;

        bool isConnected;
        float syncPriority;
        uint32_t totalEffectsCount;

        // Effect statistics
        uint32_t buffsApplied;
        uint32_t debuffsApplied;
        uint32_t effectsExpired;
        uint32_t effectsStacked;

        PlayerEffectState()
            : playerId(0), lastBuffUpdate(std::chrono::steady_clock::now()),
              lastDebuffUpdate(std::chrono::steady_clock::now()),
              lastActivity(std::chrono::steady_clock::now()),
              isConnected(false), syncPriority(1.0f), totalEffectsCount(0),
              buffsApplied(0), debuffsApplied(0), effectsExpired(0), effectsStacked(0) {}
    };

    // Main buff/debuff management class
    class BuffDebuffManager
    {
    public:
        static BuffDebuffManager& GetInstance();

        // Lifecycle
        void Initialize();
        void Shutdown();
        void Update();

        // Player management
        void AddPlayer(uint32_t playerId, const std::string& playerName);
        void RemovePlayer(uint32_t playerId);
        void UpdatePlayerActivity(uint32_t playerId);

        // Buff management
        bool ApplyBuff(uint32_t playerId, const BuffData& buffData);
        bool RemoveBuff(uint32_t playerId, BuffType buffType);
        bool UpdateBuff(uint32_t playerId, const BuffData& buffData);
        void ClearAllBuffs(uint32_t playerId);
        void ClearBuffsByCategory(uint32_t playerId, EffectCategory category);

        // Debuff management
        bool ApplyDebuff(uint32_t playerId, const DebuffData& debuffData);
        bool RemoveDebuff(uint32_t playerId, DebuffType debuffType);
        bool UpdateDebuff(uint32_t playerId, const DebuffData& debuffData);
        void ClearAllDebuffs(uint32_t playerId);
        void ClearDebuffsByCategory(uint32_t playerId, EffectCategory category);

        // Effect stacking
        bool StackEffect(uint32_t playerId, BuffType buffType, float additionalStacks = 1.0f);
        bool StackEffect(uint32_t playerId, DebuffType debuffType, float additionalStacks = 1.0f);
        bool RefreshEffect(uint32_t playerId, BuffType buffType);
        bool RefreshEffect(uint32_t playerId, DebuffType debuffType);

        // Effect queries
        PlayerEffectState* GetPlayerEffectState(uint32_t playerId);
        const PlayerEffectState* GetPlayerEffectState(uint32_t playerId) const;
        bool HasBuff(uint32_t playerId, BuffType buffType) const;
        bool HasDebuff(uint32_t playerId, DebuffType debuffType) const;
        std::vector<BuffType> GetActiveBuffs(uint32_t playerId) const;
        std::vector<DebuffType> GetActiveDebuffs(uint32_t playerId) const;
        std::vector<uint32_t> GetPlayersWithBuff(BuffType buffType) const;
        std::vector<uint32_t> GetPlayersWithDebuff(DebuffType debuffType) const;

        // Effect calculations
        float CalculateEffectIntensity(uint32_t playerId, BuffType buffType) const;
        float CalculateEffectIntensity(uint32_t playerId, DebuffType debuffType) const;
        float GetTotalBuffMultiplier(uint32_t playerId, EffectCategory category) const;
        float GetTotalDebuffMultiplier(uint32_t playerId, EffectCategory category) const;

        // Effect validation and anti-cheat
        bool ValidateBuffData(uint32_t playerId, const BuffData& buffData) const;
        bool ValidateDebuffData(uint32_t playerId, const DebuffData& debuffData) const;
        bool IsEffectApplicationValid(uint32_t playerId, BuffType buffType, uint32_t sourceId) const;
        bool IsEffectApplicationValid(uint32_t playerId, DebuffType debuffType, uint32_t sourceId) const;

        // Effect synchronization
        void SyncPlayerEffects(uint32_t playerId);
        void BroadcastEffectUpdate(uint32_t playerId, const BuffData& buffData);
        void BroadcastEffectUpdate(uint32_t playerId, const DebuffData& debuffData);
        void ForceSyncAllPlayers();
        void SetSyncPriority(uint32_t playerId, float priority);

        // Statistics and monitoring
        uint32_t GetActivePlayerCount() const;
        uint32_t GetTotalActiveEffects() const;
        uint32_t GetTotalBuffsActive() const;
        uint32_t GetTotalDebuffsActive() const;
        std::unordered_map<BuffType, uint32_t> GetBuffDistribution() const;
        std::unordered_map<DebuffType, uint32_t> GetDebuffDistribution() const;
        std::unordered_map<EffectCategory, uint32_t> GetEffectsByCategory() const;

        // Event callbacks
        using BuffAppliedCallback = std::function<void(uint32_t playerId, const BuffData& buffData)>;
        using BuffRemovedCallback = std::function<void(uint32_t playerId, BuffType buffType)>;
        using DebuffAppliedCallback = std::function<void(uint32_t playerId, const DebuffData& debuffData)>;
        using DebuffRemovedCallback = std::function<void(uint32_t playerId, DebuffType debuffType)>;
        using EffectStackedCallback = std::function<void(uint32_t playerId, bool isBuff, uint32_t effectId, float newStacks)>;
        using EffectExpiredCallback = std::function<void(uint32_t playerId, bool isBuff, uint32_t effectId)>;

        void SetBuffAppliedCallback(BuffAppliedCallback callback);
        void SetBuffRemovedCallback(BuffRemovedCallback callback);
        void SetDebuffAppliedCallback(DebuffAppliedCallback callback);
        void SetDebuffRemovedCallback(DebuffRemovedCallback callback);
        void SetEffectStackedCallback(EffectStackedCallback callback);
        void SetEffectExpiredCallback(EffectExpiredCallback callback);

    private:
        BuffDebuffManager() = default;
        ~BuffDebuffManager() = default;
        BuffDebuffManager(const BuffDebuffManager&) = delete;
        BuffDebuffManager& operator=(const BuffDebuffManager&) = delete;

        // Internal data
        std::unordered_map<uint32_t, std::unique_ptr<PlayerEffectState>> m_playerStates;
        std::unordered_map<BuffType, std::vector<uint32_t>> m_buffToPlayers;
        std::unordered_map<DebuffType, std::vector<uint32_t>> m_debuffToPlayers;
        std::unordered_map<EffectCategory, std::vector<uint32_t>> m_categoryToPlayers;

        // Thread safety
        mutable std::shared_mutex m_statesMutex;
        mutable std::mutex m_callbacksMutex;

        // Update timing
        std::chrono::steady_clock::time_point m_lastUpdate;
        std::chrono::steady_clock::time_point m_lastCleanup;
        float m_updateInterval;

        // Statistics
        uint32_t m_totalBuffsApplied;
        uint32_t m_totalDebuffsApplied;
        uint32_t m_totalEffectsExpired;
        uint32_t m_totalEffectsStacked;

        // Event callbacks
        BuffAppliedCallback m_buffAppliedCallback;
        BuffRemovedCallback m_buffRemovedCallback;
        DebuffAppliedCallback m_debuffAppliedCallback;
        DebuffRemovedCallback m_debuffRemovedCallback;
        EffectStackedCallback m_effectStackedCallback;
        EffectExpiredCallback m_effectExpiredCallback;

        // Internal methods
        uint32_t GenerateEffectId();
        void UpdatePlayerEffects(float deltaTime);
        void ProcessEffectExpirations();
        void CleanupExpiredEffects();
        void ValidateEffectStates();

        EffectCategory GetBuffCategory(BuffType buffType) const;
        EffectCategory GetDebuffCategory(DebuffType debuffType) const;
        EffectPriority GetEffectPriority(BuffType buffType) const;
        EffectPriority GetEffectPriority(DebuffType debuffType) const;

        float GetDefaultBuffDuration(BuffType buffType) const;
        float GetDefaultDebuffDuration(DebuffType debuffType) const;
        float GetDefaultBuffIntensity(BuffType buffType) const;
        float GetDefaultDebuffIntensity(DebuffType debuffType) const;
        bool CanEffectStack(BuffType buffType) const;
        bool CanEffectStack(DebuffType debuffType) const;
        float GetMaxStackCount(BuffType buffType) const;
        float GetMaxStackCount(DebuffType debuffType) const;

        void UpdateBuffToPlayersMapping(uint32_t playerId, BuffType buffType, bool isActive);
        void UpdateDebuffToPlayersMapping(uint32_t playerId, DebuffType debuffType, bool isActive);
        void UpdateCategoryToPlayersMapping(uint32_t playerId, EffectCategory category, bool isActive);
        void RemovePlayerFromAllMappings(uint32_t playerId);

        bool IsBuffIncompatible(BuffType buffType1, BuffType buffType2) const;
        bool IsDebuffIncompatible(DebuffType debuffType1, DebuffType debuffType2) const;
        void HandleIncompatibleEffects(uint32_t playerId, BuffType newBuffType);
        void HandleIncompatibleEffects(uint32_t playerId, DebuffType newDebuffType);

        void NotifyBuffApplied(uint32_t playerId, const BuffData& buffData);
        void NotifyBuffRemoved(uint32_t playerId, BuffType buffType);
        void NotifyDebuffApplied(uint32_t playerId, const DebuffData& debuffData);
        void NotifyDebuffRemoved(uint32_t playerId, DebuffType debuffType);
        void NotifyEffectStacked(uint32_t playerId, bool isBuff, uint32_t effectId, float newStacks);
        void NotifyEffectExpired(uint32_t playerId, bool isBuff, uint32_t effectId);

        void SendBuffUpdateToClients(uint32_t playerId, const BuffData& buffData);
        void SendDebuffUpdateToClients(uint32_t playerId, const DebuffData& debuffData);
        void SendEffectSyncToClients(uint32_t playerId);
    };

    // Utility functions for buff/debuff management
    namespace BuffDebuffUtils
    {
        std::string BuffTypeToString(BuffType buffType);
        BuffType StringToBuffType(const std::string& buffStr);

        std::string DebuffTypeToString(DebuffType debuffType);
        DebuffType StringToDebuffType(const std::string& debuffStr);

        std::string EffectCategoryToString(EffectCategory category);
        EffectCategory StringToEffectCategory(const std::string& categoryStr);

        std::string EffectPriorityToString(EffectPriority priority);

        bool IsAttributeBuff(BuffType buffType);
        bool IsCombatBuff(BuffType buffType);
        bool IsMovementBuff(BuffType buffType);
        bool IsConsumableBuff(BuffType buffType);
        bool IsCyberwareBuff(BuffType buffType);

        bool IsStatusDebuff(DebuffType debuffType);
        bool IsEnvironmentalDebuff(DebuffType debuffType);
        bool IsCombatDebuff(DebuffType debuffType);
        bool IsSubstanceDebuff(DebuffType debuffType);
        bool IsPsychologicalDebuff(DebuffType debuffType);

        float CalculateStackedIntensity(float baseIntensity, float stackCount, bool isBuff);
        float CalculateEffectiveIntensity(const ActiveEffect& effect, float deltaTime);
        bool ShouldEffectOverride(EffectPriority newPriority, EffectPriority existingPriority);

        uint32_t HashEffectState(const PlayerEffectState& state);
        bool AreEffectsEquivalent(const BuffData& data1, const BuffData& data2, float tolerance = 0.1f);
        bool AreEffectsEquivalent(const DebuffData& data1, const DebuffData& data2, float tolerance = 0.1f);
    }

    // Network message structures for client-server communication
    struct EffectSyncUpdate
    {
        uint32_t playerId;
        std::vector<BuffData> activeBuffs;
        std::vector<DebuffData> activeDebuffs;
        std::chrono::steady_clock::time_point updateTime;
        uint32_t syncVersion;
    };

    struct BuffUpdate
    {
        uint32_t playerId;
        BuffData buffData;
        bool isApplication; // true for apply, false for remove
        std::chrono::steady_clock::time_point updateTime;
    };

    struct DebuffUpdate
    {
        uint32_t playerId;
        DebuffData debuffData;
        bool isApplication; // true for apply, false for remove
        std::chrono::steady_clock::time_point updateTime;
    };

    struct EffectStackUpdate
    {
        uint32_t playerId;
        bool isBuff;
        uint32_t effectId;
        float oldStacks;
        float newStacks;
        std::chrono::steady_clock::time_point updateTime;
    };
}
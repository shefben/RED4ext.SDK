#include "BuffDebuffManager.hpp"
#include "RED4ext/Api/Sdk.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace RED4ext
{
    // BuffDebuffManager Implementation
    BuffDebuffManager& BuffDebuffManager::GetInstance()
    {
        static BuffDebuffManager instance;
        return instance;
    }

    void BuffDebuffManager::Initialize()
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Clear existing data
        m_playerStates.clear();
        m_buffToPlayers.clear();
        m_debuffToPlayers.clear();
        m_categoryToPlayers.clear();

        // Initialize timing
        m_lastUpdate = std::chrono::steady_clock::now();
        m_lastCleanup = m_lastUpdate;
        m_updateInterval = 0.2f; // 5 FPS effect updates

        // Initialize statistics
        m_totalBuffsApplied = 0;
        m_totalDebuffsApplied = 0;
        m_totalEffectsExpired = 0;
        m_totalEffectsStacked = 0;

        // Initialize mapping structures for all buff types
        for (int i = 0; i <= static_cast<int>(BuffType::RadiationResistance); ++i) {
            BuffType buffType = static_cast<BuffType>(i);
            m_buffToPlayers[buffType] = {};
        }

        // Initialize mapping structures for all debuff types
        for (int i = 0; i <= static_cast<int>(DebuffType::Panic); ++i) {
            DebuffType debuffType = static_cast<DebuffType>(i);
            m_debuffToPlayers[debuffType] = {};
        }

        // Initialize category mappings
        for (int i = 0; i <= static_cast<int>(EffectCategory::Status); ++i) {
            EffectCategory category = static_cast<EffectCategory>(i);
            m_categoryToPlayers[category] = {};
        }
    }

    void BuffDebuffManager::Shutdown()
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        m_playerStates.clear();
        m_buffToPlayers.clear();
        m_debuffToPlayers.clear();
        m_categoryToPlayers.clear();

        // Clear callbacks
        std::lock_guard<std::mutex> callbacksLock(m_callbacksMutex);
        m_buffAppliedCallback = nullptr;
        m_buffRemovedCallback = nullptr;
        m_debuffAppliedCallback = nullptr;
        m_debuffRemovedCallback = nullptr;
        m_effectStackedCallback = nullptr;
        m_effectExpiredCallback = nullptr;
    }

    void BuffDebuffManager::Update()
    {
        auto currentTime = std::chrono::steady_clock::now();
        auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - m_lastUpdate).count() / 1000.0f;

        m_lastUpdate = currentTime;

        // Update player effects
        UpdatePlayerEffects(deltaTime);

        // Process effect expirations
        ProcessEffectExpirations();

        // Validate effect states
        ValidateEffectStates();

        // Periodic cleanup (every 60 seconds)
        auto timeSinceCleanup = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - m_lastCleanup).count();

        if (timeSinceCleanup >= 60) {
            CleanupExpiredEffects();
            m_lastCleanup = currentTime;
        }
    }

    void BuffDebuffManager::AddPlayer(uint32_t playerId, const std::string& playerName)
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        auto playerState = std::make_unique<PlayerEffectState>();
        playerState->playerId = playerId;
        playerState->playerName = playerName;
        playerState->isConnected = true;
        playerState->syncPriority = 1.0f;
        playerState->totalEffectsCount = 0;

        m_playerStates[playerId] = std::move(playerState);
    }

    void BuffDebuffManager::RemovePlayer(uint32_t playerId)
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Remove from all mappings
        RemovePlayerFromAllMappings(playerId);

        // Remove player state
        m_playerStates.erase(playerId);
    }

    bool BuffDebuffManager::ApplyBuff(uint32_t playerId, const BuffData& buffData)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return false;
        }

        auto& playerState = it->second;

        // Validate buff data
        if (!ValidateBuffData(playerId, buffData)) {
            return false;
        }

        // Check for incompatible effects
        HandleIncompatibleEffects(playerId, buffData.buffType);

        // Check if buff already exists
        auto buffIt = playerState->activeBuffs.find(buffData.buffType);
        if (buffIt != playerState->activeBuffs.end()) {
            // Handle stacking or refreshing
            auto& existingEffect = buffIt->second;

            if (CanEffectStack(buffData.buffType)) {
                // Stack the effect
                float newStacks = std::min(existingEffect->stackCount + buffData.stackCount,
                                         GetMaxStackCount(buffData.buffType));
                existingEffect->stackCount = newStacks;
                existingEffect->intensity = BuffDebuffUtils::CalculateStackedIntensity(
                    GetDefaultBuffIntensity(buffData.buffType), newStacks, true);

                m_totalEffectsStacked++;

                lock.unlock();
                NotifyEffectStacked(playerId, true, existingEffect->effectId, newStacks);
                return true;
            } else if (existingEffect->refreshOnReapply) {
                // Refresh the effect
                existingEffect->remainingTime = buffData.duration;
                existingEffect->lastUpdate = std::chrono::steady_clock::now();
                return true;
            } else {
                return false; // Effect already exists and can't be stacked or refreshed
            }
        }

        // Create new buff effect
        auto effect = std::make_unique<ActiveEffect>();
        effect->effectId = GenerateEffectId();
        effect->isBuff = true;
        effect->buffType = buffData.buffType;
        effect->duration = buffData.duration;
        effect->remainingTime = buffData.duration;
        effect->intensity = buffData.intensity;
        effect->stackCount = buffData.stackCount;
        effect->category = GetBuffCategory(buffData.buffType);
        effect->priority = GetEffectPriority(buffData.buffType);
        effect->startTime = std::chrono::steady_clock::now();
        effect->lastUpdate = effect->startTime;
        effect->sourceId = buffData.sourceId;
        effect->sourceType = buffData.sourceType;
        effect->canStack = CanEffectStack(buffData.buffType);
        effect->maxStacks = GetMaxStackCount(buffData.buffType);

        playerState->activeBuffs[buffData.buffType] = std::move(effect);
        playerState->totalEffectsCount++;
        playerState->buffsApplied++;
        playerState->lastBuffUpdate = std::chrono::steady_clock::now();
        playerState->lastActivity = playerState->lastBuffUpdate;

        // Update mappings
        UpdateBuffToPlayersMapping(playerId, buffData.buffType, true);
        UpdateCategoryToPlayersMapping(playerId, GetBuffCategory(buffData.buffType), true);

        m_totalBuffsApplied++;

        lock.unlock();

        // Notify listeners
        NotifyBuffApplied(playerId, buffData);

        // Broadcast update
        BroadcastEffectUpdate(playerId, buffData);

        return true;
    }

    bool BuffDebuffManager::ApplyDebuff(uint32_t playerId, const DebuffData& debuffData)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return false;
        }

        auto& playerState = it->second;

        // Validate debuff data
        if (!ValidateDebuffData(playerId, debuffData)) {
            return false;
        }

        // Check for incompatible effects
        HandleIncompatibleEffects(playerId, debuffData.debuffType);

        // Check if debuff already exists
        auto debuffIt = playerState->activeDebuffs.find(debuffData.debuffType);
        if (debuffIt != playerState->activeDebuffs.end()) {
            // Handle stacking or refreshing
            auto& existingEffect = debuffIt->second;

            if (CanEffectStack(debuffData.debuffType)) {
                // Stack the effect
                float newStacks = std::min(existingEffect->stackCount + debuffData.stackCount,
                                         GetMaxStackCount(debuffData.debuffType));
                existingEffect->stackCount = newStacks;
                existingEffect->intensity = BuffDebuffUtils::CalculateStackedIntensity(
                    GetDefaultDebuffIntensity(debuffData.debuffType), newStacks, false);

                m_totalEffectsStacked++;

                lock.unlock();
                NotifyEffectStacked(playerId, false, existingEffect->effectId, newStacks);
                return true;
            } else if (existingEffect->refreshOnReapply) {
                // Refresh the effect
                existingEffect->remainingTime = debuffData.duration;
                existingEffect->lastUpdate = std::chrono::steady_clock::now();
                return true;
            } else {
                return false; // Effect already exists and can't be stacked or refreshed
            }
        }

        // Create new debuff effect
        auto effect = std::make_unique<ActiveEffect>();
        effect->effectId = GenerateEffectId();
        effect->isBuff = false;
        effect->debuffType = debuffData.debuffType;
        effect->duration = debuffData.duration;
        effect->remainingTime = debuffData.duration;
        effect->intensity = debuffData.intensity;
        effect->stackCount = debuffData.stackCount;
        effect->category = GetDebuffCategory(debuffData.debuffType);
        effect->priority = GetEffectPriority(debuffData.debuffType);
        effect->startTime = std::chrono::steady_clock::now();
        effect->lastUpdate = effect->startTime;
        effect->sourceId = debuffData.sourceId;
        effect->sourceType = debuffData.sourceType;
        effect->canStack = CanEffectStack(debuffData.debuffType);
        effect->maxStacks = GetMaxStackCount(debuffData.debuffType);

        playerState->activeDebuffs[debuffData.debuffType] = std::move(effect);
        playerState->totalEffectsCount++;
        playerState->debuffsApplied++;
        playerState->lastDebuffUpdate = std::chrono::steady_clock::now();
        playerState->lastActivity = playerState->lastDebuffUpdate;

        // Update mappings
        UpdateDebuffToPlayersMapping(playerId, debuffData.debuffType, true);
        UpdateCategoryToPlayersMapping(playerId, GetDebuffCategory(debuffData.debuffType), true);

        m_totalDebuffsApplied++;

        lock.unlock();

        // Notify listeners
        NotifyDebuffApplied(playerId, debuffData);

        // Broadcast update
        BroadcastEffectUpdate(playerId, debuffData);

        return true;
    }

    bool BuffDebuffManager::RemoveBuff(uint32_t playerId, BuffType buffType)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return false;
        }

        auto& playerState = it->second;

        auto buffIt = playerState->activeBuffs.find(buffType);
        if (buffIt == playerState->activeBuffs.end()) {
            return false;
        }

        uint32_t effectId = buffIt->second->effectId;

        // Remove buff
        playerState->activeBuffs.erase(buffIt);
        playerState->totalEffectsCount--;
        playerState->lastBuffUpdate = std::chrono::steady_clock::now();

        // Update mappings
        UpdateBuffToPlayersMapping(playerId, buffType, false);
        UpdateCategoryToPlayersMapping(playerId, GetBuffCategory(buffType), false);

        lock.unlock();

        // Notify listeners
        NotifyBuffRemoved(playerId, buffType);
        NotifyEffectExpired(playerId, true, effectId);

        // Broadcast removal
        BuffData removalData;
        removalData.playerId = playerId;
        removalData.buffType = buffType;
        removalData.isActive = false;
        removalData.timestamp = std::chrono::steady_clock::now();
        BroadcastEffectUpdate(playerId, removalData);

        return true;
    }

    bool BuffDebuffManager::RemoveDebuff(uint32_t playerId, DebuffType debuffType)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return false;
        }

        auto& playerState = it->second;

        auto debuffIt = playerState->activeDebuffs.find(debuffType);
        if (debuffIt == playerState->activeDebuffs.end()) {
            return false;
        }

        uint32_t effectId = debuffIt->second->effectId;

        // Remove debuff
        playerState->activeDebuffs.erase(debuffIt);
        playerState->totalEffectsCount--;
        playerState->lastDebuffUpdate = std::chrono::steady_clock::now();

        // Update mappings
        UpdateDebuffToPlayersMapping(playerId, debuffType, false);
        UpdateCategoryToPlayersMapping(playerId, GetDebuffCategory(debuffType), false);

        lock.unlock();

        // Notify listeners
        NotifyDebuffRemoved(playerId, debuffType);
        NotifyEffectExpired(playerId, false, effectId);

        // Broadcast removal
        DebuffData removalData;
        removalData.playerId = playerId;
        removalData.debuffType = debuffType;
        removalData.isActive = false;
        removalData.timestamp = std::chrono::steady_clock::now();
        BroadcastEffectUpdate(playerId, removalData);

        return true;
    }

    PlayerEffectState* BuffDebuffManager::GetPlayerEffectState(uint32_t playerId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        return (it != m_playerStates.end()) ? it->second.get() : nullptr;
    }

    const PlayerEffectState* BuffDebuffManager::GetPlayerEffectState(uint32_t playerId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        return (it != m_playerStates.end()) ? it->second.get() : nullptr;
    }

    bool BuffDebuffManager::HasBuff(uint32_t playerId, BuffType buffType) const
    {
        auto* playerState = GetPlayerEffectState(playerId);
        if (!playerState) {
            return false;
        }

        return playerState->activeBuffs.find(buffType) != playerState->activeBuffs.end();
    }

    bool BuffDebuffManager::HasDebuff(uint32_t playerId, DebuffType debuffType) const
    {
        auto* playerState = GetPlayerEffectState(playerId);
        if (!playerState) {
            return false;
        }

        return playerState->activeDebuffs.find(debuffType) != playerState->activeDebuffs.end();
    }

    std::vector<uint32_t> BuffDebuffManager::GetPlayersWithBuff(BuffType buffType) const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_buffToPlayers.find(buffType);
        return (it != m_buffToPlayers.end()) ? it->second : std::vector<uint32_t>();
    }

    std::vector<uint32_t> BuffDebuffManager::GetPlayersWithDebuff(DebuffType debuffType) const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_debuffToPlayers.find(debuffType);
        return (it != m_debuffToPlayers.end()) ? it->second : std::vector<uint32_t>();
    }

    bool BuffDebuffManager::ValidateBuffData(uint32_t playerId, const BuffData& buffData) const
    {
        // Validate basic data
        if (buffData.duration < 0.0f || buffData.duration > 7200.0f) { // Max 2 hours
            return false;
        }

        if (buffData.intensity < 0.1f || buffData.intensity > 10.0f) { // Reasonable intensity range
            return false;
        }

        if (buffData.stackCount < 1.0f || buffData.stackCount > GetMaxStackCount(buffData.buffType)) {
            return false;
        }

        return true;
    }

    bool BuffDebuffManager::ValidateDebuffData(uint32_t playerId, const DebuffData& debuffData) const
    {
        // Validate basic data
        if (debuffData.duration < 0.0f || debuffData.duration > 3600.0f) { // Max 1 hour
            return false;
        }

        if (debuffData.intensity < 0.1f || debuffData.intensity > 10.0f) { // Reasonable intensity range
            return false;
        }

        if (debuffData.stackCount < 1.0f || debuffData.stackCount > GetMaxStackCount(debuffData.debuffType)) {
            return false;
        }

        return true;
    }

    void BuffDebuffManager::BroadcastEffectUpdate(uint32_t playerId, const BuffData& buffData)
    {
        SendBuffUpdateToClients(playerId, buffData);
    }

    void BuffDebuffManager::BroadcastEffectUpdate(uint32_t playerId, const DebuffData& debuffData)
    {
        SendDebuffUpdateToClients(playerId, debuffData);
    }

    // Private implementation methods
    uint32_t BuffDebuffManager::GenerateEffectId()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint32_t> dis(1, UINT32_MAX);
        return dis(gen);
    }

    void BuffDebuffManager::UpdatePlayerEffects(float deltaTime)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [playerId, playerState] : m_playerStates) {
            // Update buff durations
            for (auto& [buffType, effect] : playerState->activeBuffs) {
                if (!effect->isPermanent) {
                    effect->remainingTime -= deltaTime;
                    effect->lastUpdate = currentTime;
                }
            }

            // Update debuff durations
            for (auto& [debuffType, effect] : playerState->activeDebuffs) {
                if (!effect->isPermanent) {
                    effect->remainingTime -= deltaTime;
                    effect->lastUpdate = currentTime;
                }
            }

            // Check for player timeout (60 seconds of inactivity)
            auto timeSinceActivity = std::chrono::duration_cast<std::chrono::seconds>(
                currentTime - playerState->lastActivity).count();

            if (timeSinceActivity >= 60) {
                playerState->isConnected = false;
            }
        }
    }

    void BuffDebuffManager::ProcessEffectExpirations()
    {
        std::vector<std::pair<uint32_t, BuffType>> expiredBuffs;
        std::vector<std::pair<uint32_t, DebuffType>> expiredDebuffs;

        {
            std::shared_lock<std::shared_mutex> lock(m_statesMutex);

            for (auto& [playerId, playerState] : m_playerStates) {
                // Check for expired buffs
                for (auto& [buffType, effect] : playerState->activeBuffs) {
                    if (!effect->isPermanent && effect->remainingTime <= 0.0f) {
                        expiredBuffs.emplace_back(playerId, buffType);
                    }
                }

                // Check for expired debuffs
                for (auto& [debuffType, effect] : playerState->activeDebuffs) {
                    if (!effect->isPermanent && effect->remainingTime <= 0.0f) {
                        expiredDebuffs.emplace_back(playerId, debuffType);
                    }
                }
            }
        }

        // Remove expired effects
        for (auto& [playerId, buffType] : expiredBuffs) {
            RemoveBuff(playerId, buffType);
            m_totalEffectsExpired++;
        }

        for (auto& [playerId, debuffType] : expiredDebuffs) {
            RemoveDebuff(playerId, debuffType);
            m_totalEffectsExpired++;
        }
    }

    EffectCategory BuffDebuffManager::GetBuffCategory(BuffType buffType) const
    {
        if (static_cast<uint16_t>(buffType) <= 4) return EffectCategory::Attribute;
        if (static_cast<uint16_t>(buffType) <= 15) return EffectCategory::Combat;
        if (static_cast<uint16_t>(buffType) <= 23) return EffectCategory::Movement;
        if (static_cast<uint16_t>(buffType) <= 32) return EffectCategory::Stealth;
        if (static_cast<uint16_t>(buffType) <= 43) return EffectCategory::Consumable;
        if (static_cast<uint16_t>(buffType) <= 53) return EffectCategory::Cyberware;
        return EffectCategory::Environmental;
    }

    EffectCategory BuffDebuffManager::GetDebuffCategory(DebuffType debuffType) const
    {
        if (static_cast<uint16_t>(debuffType) <= 7) return EffectCategory::Status;
        if (static_cast<uint16_t>(debuffType) <= 14) return EffectCategory::Environmental;
        if (static_cast<uint16_t>(debuffType) <= 24) return EffectCategory::Combat;
        if (static_cast<uint16_t>(debuffType) <= 33) return EffectCategory::Consumable;
        if (static_cast<uint16_t>(debuffType) <= 43) return EffectCategory::Cyberware;
        return EffectCategory::Psychological;
    }

    float BuffDebuffManager::GetDefaultBuffDuration(BuffType buffType) const
    {
        switch (buffType) {
            case BuffType::StrengthBoost: return 300.0f; // 5 minutes
            case BuffType::DamageBoost: return 180.0f; // 3 minutes
            case BuffType::SpeedBoost: return 120.0f; // 2 minutes
            case BuffType::StimulantBuff: return 240.0f; // 4 minutes
            case BuffType::AlcoholBuff: return 600.0f; // 10 minutes
            default: return 60.0f; // Default 1 minute
        }
    }

    float BuffDebuffManager::GetDefaultDebuffDuration(DebuffType debuffType) const
    {
        switch (debuffType) {
            case DebuffType::Bleeding: return 30.0f; // 30 seconds
            case DebuffType::Poisoned: return 45.0f; // 45 seconds
            case DebuffType::Burning: return 15.0f; // 15 seconds
            case DebuffType::Stunned: return 5.0f; // 5 seconds
            case DebuffType::AlcoholPenalty: return 900.0f; // 15 minutes
            default: return 30.0f; // Default 30 seconds
        }
    }

    void BuffDebuffManager::UpdateBuffToPlayersMapping(uint32_t playerId, BuffType buffType, bool isActive)
    {
        auto& playerList = m_buffToPlayers[buffType];

        if (isActive) {
            if (std::find(playerList.begin(), playerList.end(), playerId) == playerList.end()) {
                playerList.push_back(playerId);
            }
        } else {
            playerList.erase(std::remove(playerList.begin(), playerList.end(), playerId), playerList.end());
        }
    }

    void BuffDebuffManager::UpdateDebuffToPlayersMapping(uint32_t playerId, DebuffType debuffType, bool isActive)
    {
        auto& playerList = m_debuffToPlayers[debuffType];

        if (isActive) {
            if (std::find(playerList.begin(), playerList.end(), playerId) == playerList.end()) {
                playerList.push_back(playerId);
            }
        } else {
            playerList.erase(std::remove(playerList.begin(), playerList.end(), playerId), playerList.end());
        }
    }

    void BuffDebuffManager::RemovePlayerFromAllMappings(uint32_t playerId)
    {
        // Remove from buff mappings
        for (auto& [buffType, playerList] : m_buffToPlayers) {
            playerList.erase(std::remove(playerList.begin(), playerList.end(), playerId), playerList.end());
        }

        // Remove from debuff mappings
        for (auto& [debuffType, playerList] : m_debuffToPlayers) {
            playerList.erase(std::remove(playerList.begin(), playerList.end(), playerId), playerList.end());
        }

        // Remove from category mappings
        for (auto& [category, playerList] : m_categoryToPlayers) {
            playerList.erase(std::remove(playerList.begin(), playerList.end(), playerId), playerList.end());
        }
    }

    // Notification methods
    void BuffDebuffManager::NotifyBuffApplied(uint32_t playerId, const BuffData& buffData)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_buffAppliedCallback) {
            m_buffAppliedCallback(playerId, buffData);
        }
    }

    void BuffDebuffManager::SendBuffUpdateToClients(uint32_t playerId, const BuffData& buffData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void BuffDebuffManager::SendDebuffUpdateToClients(uint32_t playerId, const DebuffData& debuffData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    // Utility functions implementation
    namespace BuffDebuffUtils
    {
        std::string BuffTypeToString(BuffType buffType)
        {
            switch (buffType) {
                case BuffType::StrengthBoost: return "StrengthBoost";
                case BuffType::ReflexesBoost: return "ReflexesBoost";
                case BuffType::TechnicalAbilityBoost: return "TechnicalAbilityBoost";
                case BuffType::IntelligenceBoost: return "IntelligenceBoost";
                case BuffType::CoolBoost: return "CoolBoost";
                case BuffType::DamageBoost: return "DamageBoost";
                case BuffType::ArmorBoost: return "ArmorBoost";
                case BuffType::SpeedBoost: return "SpeedBoost";
                case BuffType::StealthBoost: return "StealthBoost";
                case BuffType::FoodBuff: return "FoodBuff";
                case BuffType::AlcoholBuff: return "AlcoholBuff";
                case BuffType::StimulantBuff: return "StimulantBuff";
                case BuffType::CyberwareBoost: return "CyberwareBoost";
                default: return "Unknown";
            }
        }

        std::string DebuffTypeToString(DebuffType debuffType)
        {
            switch (debuffType) {
                case DebuffType::Bleeding: return "Bleeding";
                case DebuffType::Poisoned: return "Poisoned";
                case DebuffType::Burning: return "Burning";
                case DebuffType::Electrified: return "Electrified";
                case DebuffType::Stunned: return "Stunned";
                case DebuffType::Blinded: return "Blinded";
                case DebuffType::Slowed: return "Slowed";
                case DebuffType::Weakened: return "Weakened";
                case DebuffType::Radiation: return "Radiation";
                case DebuffType::ToxicAir: return "ToxicAir";
                case DebuffType::Suppressed: return "Suppressed";
                case DebuffType::AlcoholPenalty: return "AlcoholPenalty";
                case DebuffType::Fear: return "Fear";
                default: return "Unknown";
            }
        }

        bool IsAttributeBuff(BuffType buffType)
        {
            return static_cast<uint16_t>(buffType) <= 4;
        }

        bool IsCombatBuff(BuffType buffType)
        {
            uint16_t value = static_cast<uint16_t>(buffType);
            return value >= 10 && value <= 15;
        }

        bool IsStatusDebuff(DebuffType debuffType)
        {
            return static_cast<uint16_t>(debuffType) <= 7;
        }

        bool IsEnvironmentalDebuff(DebuffType debuffType)
        {
            uint16_t value = static_cast<uint16_t>(debuffType);
            return value >= 10 && value <= 14;
        }

        float CalculateStackedIntensity(float baseIntensity, float stackCount, bool isBuff)
        {
            if (isBuff) {
                // Buffs get diminishing returns on stacking
                return baseIntensity * (1.0f + (stackCount - 1.0f) * 0.5f);
            } else {
                // Debuffs stack more linearly
                return baseIntensity * stackCount;
            }
        }
    }

    // Missing private method implementations
    void BuffDebuffManager::HandleIncompatibleEffects(uint32_t playerId, BuffType buffType)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);
        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return;
        }

        auto& playerState = it->second;

        // Remove incompatible buffs when applying new ones
        // For example, if applying defensive buff, remove offensive debuffs
        if (BuffDebuffUtils::IsAttributeBuff(buffType)) {
            // Attribute buffs conflict with attribute debuffs
            for (auto debuffIt = playerState->activeDebuffs.begin(); debuffIt != playerState->activeDebuffs.end();) {
                if (BuffDebuffUtils::IsStatusDebuff(debuffIt->first)) {
                    debuffIt = playerState->activeDebuffs.erase(debuffIt);
                } else {
                    ++debuffIt;
                }
            }
        }
    }

    void BuffDebuffManager::HandleIncompatibleEffects(uint32_t playerId, DebuffType debuffType)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);
        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return;
        }

        auto& playerState = it->second;

        // Remove incompatible debuffs when applying new ones
        if (BuffDebuffUtils::IsStatusDebuff(debuffType)) {
            // Status debuffs conflict with attribute buffs
            for (auto buffIt = playerState->activeBuffs.begin(); buffIt != playerState->activeBuffs.end();) {
                if (BuffDebuffUtils::IsAttributeBuff(buffIt->first)) {
                    buffIt = playerState->activeBuffs.erase(buffIt);
                } else {
                    ++buffIt;
                }
            }
        }
    }

    void BuffDebuffManager::NotifyBuffRemoved(uint32_t playerId, BuffType buffType)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_buffRemovedCallback) {
            m_buffRemovedCallback(playerId, buffType);
        }
    }

    void BuffDebuffManager::NotifyDebuffApplied(uint32_t playerId, const DebuffData& debuff)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_debuffAppliedCallback) {
            m_debuffAppliedCallback(playerId, debuff);
        }
    }

    void BuffDebuffManager::NotifyDebuffRemoved(uint32_t playerId, DebuffType debuffType)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_debuffRemovedCallback) {
            m_debuffRemovedCallback(playerId, debuffType);
        }
    }

    void BuffDebuffManager::NotifyEffectStacked(uint32_t playerId, bool isBuff, uint32_t effectId, float newIntensity)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_effectStackedCallback) {
            m_effectStackedCallback(playerId, isBuff, effectId, newIntensity);
        }
    }

    void BuffDebuffManager::NotifyEffectExpired(uint32_t playerId, bool isBuff, uint32_t effectId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_effectExpiredCallback) {
            m_effectExpiredCallback(playerId, isBuff, effectId);
        }
    }

    // Additional missing private method implementations
    EffectPriority BuffDebuffManager::GetEffectPriority(BuffType buffType) const
    {
        switch (buffType) {
            case BuffType::StrengthBoost:
            case BuffType::ReflexesBoost:
            case BuffType::ArmorBoost:
                return EffectPriority::High;
            case BuffType::DamageBoost:
            case BuffType::SpeedBoost:
            case BuffType::CriticalChanceBoost:
                return EffectPriority::Normal;
            default:
                return EffectPriority::Low;
        }
    }

    EffectPriority BuffDebuffManager::GetEffectPriority(DebuffType debuffType) const
    {
        switch (debuffType) {
            case DebuffType::Bleeding:
            case DebuffType::Poisoned:
            case DebuffType::Burning:
                return EffectPriority::High;
            case DebuffType::Stunned:
            case DebuffType::Blinded:
            case DebuffType::Slowed:
                return EffectPriority::Normal;
            default:
                return EffectPriority::Low;
        }
    }

    float BuffDebuffManager::GetDefaultBuffIntensity(BuffType buffType) const
    {
        switch (buffType) {
            case BuffType::StrengthBoost: return 1.2f;
            case BuffType::ReflexesBoost: return 1.2f;
            case BuffType::ArmorBoost: return 1.5f;
            case BuffType::DamageBoost: return 1.3f;
            case BuffType::SpeedBoost: return 1.3f;
            case BuffType::CriticalChanceBoost: return 0.15f;
            case BuffType::CriticalDamageBoost: return 1.5f;
            case BuffType::StaminaBoost: return 1.2f;
            case BuffType::HackingBoost: return 1.25f;
            case BuffType::StealthBoost: return 1.4f;
            case BuffType::TemperatureResistance: return 0.7f;
            case BuffType::RadiationResistance: return 0.6f;
            default: return 1.0f;
        }
    }

    float BuffDebuffManager::GetDefaultDebuffIntensity(DebuffType debuffType) const
    {
        switch (debuffType) {
            case DebuffType::Bleeding: return 0.05f;
            case DebuffType::Poisoned: return 0.03f;
            case DebuffType::Burning: return 0.08f;
            case DebuffType::Electrified: return 0.1f;
            case DebuffType::Stunned: return 1.0f;
            case DebuffType::Blinded: return 0.8f;
            case DebuffType::Slowed: return 0.6f;
            case DebuffType::Weakened: return 0.7f;
            case DebuffType::Radiation: return 0.02f;
            case DebuffType::ToxicAir: return 0.04f;
            case DebuffType::Suppressed: return 0.5f;
            case DebuffType::AlcoholPenalty: return 0.3f;
            case DebuffType::Fear: return 0.9f;
            default: return 0.1f;
        }
    }

    bool BuffDebuffManager::CanEffectStack(BuffType buffType) const
    {
        switch (buffType) {
            case BuffType::StrengthBoost:
            case BuffType::ReflexesBoost:
            case BuffType::ArmorBoost:
            case BuffType::DamageBoost:
            case BuffType::SpeedBoost:
                return true;
            default:
                return false;
        }
    }

    bool BuffDebuffManager::CanEffectStack(DebuffType debuffType) const
    {
        switch (debuffType) {
            case DebuffType::Bleeding:
            case DebuffType::Poisoned:
            case DebuffType::Burning:
            case DebuffType::Radiation:
            case DebuffType::ToxicAir:
                return true;
            default:
                return false;
        }
    }

    float BuffDebuffManager::GetMaxStackCount(BuffType buffType) const
    {
        switch (buffType) {
            case BuffType::StrengthBoost:
            case BuffType::ReflexesBoost:
            case BuffType::DamageBoost:
                return 3.0f;
            case BuffType::ArmorBoost:
            case BuffType::SpeedBoost:
            case BuffType::StaminaBoost:
                return 5.0f;
            default:
                return 1.0f;
        }
    }

    float BuffDebuffManager::GetMaxStackCount(DebuffType debuffType) const
    {
        switch (debuffType) {
            case DebuffType::Bleeding:
            case DebuffType::Poisoned:
            case DebuffType::Burning:
                return 5.0f;
            case DebuffType::Radiation:
            case DebuffType::ToxicAir:
                return 10.0f;
            default:
                return 1.0f;
        }
    }

    void BuffDebuffManager::UpdateCategoryToPlayersMapping(uint32_t playerId, EffectCategory category, bool isAdding)
    {
        std::lock_guard<std::shared_mutex> lock(m_statesMutex);

        auto& playersInCategory = m_categoryToPlayers[category];

        if (isAdding) {
            // Add player to category if not already present
            auto it = std::find(playersInCategory.begin(), playersInCategory.end(), playerId);
            if (it == playersInCategory.end()) {
                playersInCategory.push_back(playerId);
            }
        } else {
            // Remove player from category
            auto it = std::find(playersInCategory.begin(), playersInCategory.end(), playerId);
            if (it != playersInCategory.end()) {
                playersInCategory.erase(it);
            }
        }
    }

    void BuffDebuffManager::CleanupExpiredEffects()
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [playerId, playerState] : m_playerStates) {
            // Clean up expired buffs
            for (auto it = playerState->activeBuffs.begin(); it != playerState->activeBuffs.end();) {
                if (!it->second->isPermanent && it->second->remainingTime <= 0.0f) {
                    it = playerState->activeBuffs.erase(it);
                    playerState->totalEffectsCount--;
                } else {
                    ++it;
                }
            }

            // Clean up expired debuffs
            for (auto it = playerState->activeDebuffs.begin(); it != playerState->activeDebuffs.end();) {
                if (!it->second->isPermanent && it->second->remainingTime <= 0.0f) {
                    it = playerState->activeDebuffs.erase(it);
                    playerState->totalEffectsCount--;
                } else {
                    ++it;
                }
            }
        }
    }

    void BuffDebuffManager::ValidateEffectStates()
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        for (auto& [playerId, playerState] : m_playerStates) {
            // Validate buff states
            for (auto& [buffType, effect] : playerState->activeBuffs) {
                // Ensure intensity is within valid range
                if (effect->intensity < 0.1f || effect->intensity > 10.0f) {
                    effect->intensity = std::clamp(effect->intensity, 0.1f, 10.0f);
                }

                // Ensure stack count is within limits
                float maxStacks = GetMaxStackCount(buffType);
                if (effect->stackCount > maxStacks) {
                    effect->stackCount = maxStacks;
                }

                // Validate duration
                if (effect->duration < 0.0f) {
                    effect->duration = 0.0f;
                    effect->remainingTime = 0.0f;
                }
            }

            // Validate debuff states
            for (auto& [debuffType, effect] : playerState->activeDebuffs) {
                // Ensure intensity is within valid range
                if (effect->intensity < 0.1f || effect->intensity > 10.0f) {
                    effect->intensity = std::clamp(effect->intensity, 0.1f, 10.0f);
                }

                // Ensure stack count is within limits
                float maxStacks = GetMaxStackCount(debuffType);
                if (effect->stackCount > maxStacks) {
                    effect->stackCount = maxStacks;
                }

                // Validate duration
                if (effect->duration < 0.0f) {
                    effect->duration = 0.0f;
                    effect->remainingTime = 0.0f;
                }
            }
        }
    }

}
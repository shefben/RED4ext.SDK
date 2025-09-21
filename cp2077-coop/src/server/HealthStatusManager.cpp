#include "HealthStatusManager.hpp"
#include "RED4ext/Api/Sdk.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace RED4ext
{
    // HealthStatusManager Implementation
    HealthStatusManager& HealthStatusManager::GetInstance()
    {
        static HealthStatusManager instance;
        return instance;
    }

    void HealthStatusManager::Initialize()
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Clear existing data
        m_playerStates.clear();
        m_effectToPlayers.clear();
        m_recentCriticalEvents.clear();

        // Initialize timing
        m_lastUpdate = std::chrono::steady_clock::now();
        m_lastCleanup = m_lastUpdate;
        m_updateInterval = 0.1f; // 10 FPS health updates

        // Initialize statistics
        m_totalHealthUpdates = 0;
        m_totalStatusEffectChanges = 0;
        m_totalCriticalEvents = 0;

        // Initialize effect to players mapping
        for (int i = 0; i < static_cast<int>(StatusEffectType::Damage_Boost) + 1; ++i) {
            StatusEffectType effectType = static_cast<StatusEffectType>(i);
            m_effectToPlayers[effectType] = {};
        }
    }

    void HealthStatusManager::Shutdown()
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        m_playerStates.clear();
        m_effectToPlayers.clear();
        m_recentCriticalEvents.clear();

        // Clear callbacks
        std::lock_guard<std::mutex> callbacksLock(m_callbacksMutex);
        m_healthUpdatedCallback = nullptr;
        m_statusEffectChangedCallback = nullptr;
        m_criticalEventCallback = nullptr;
        m_playerDownedCallback = nullptr;
        m_playerRevivedCallback = nullptr;
        m_conditionsUpdatedCallback = nullptr;
    }

    void HealthStatusManager::Update()
    {
        auto currentTime = std::chrono::steady_clock::now();
        auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - m_lastUpdate).count() / 1000.0f;

        m_lastUpdate = currentTime;

        // Update player health states
        UpdatePlayerHealthStates(deltaTime);

        // Process status effect updates
        ProcessStatusEffectUpdates(deltaTime);

        // Validate player states
        ValidatePlayerStates();

        // Periodic cleanup (every 30 seconds)
        auto timeSinceCleanup = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - m_lastCleanup).count();

        if (timeSinceCleanup >= 30) {
            CleanupExpiredEffects();
            CleanupOldEvents();
            m_lastCleanup = currentTime;
        }
    }

    void HealthStatusManager::AddPlayer(uint32_t playerId, const std::string& playerName)
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        auto playerState = std::make_unique<PlayerHealthState>();
        playerState->playerId = playerId;
        playerState->playerName = playerName;
        playerState->healthData.playerId = playerId;
        playerState->conditions.playerId = playerId;
        playerState->isConnected = true;
        playerState->syncPriority = 1.0f;

        m_playerStates[playerId] = std::move(playerState);
    }

    void HealthStatusManager::RemovePlayer(uint32_t playerId)
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Remove from effect mappings
        RemovePlayerFromAllEffectMappings(playerId);

        // Remove player state
        m_playerStates.erase(playerId);
    }

    void HealthStatusManager::UpdatePlayerHealth(uint32_t playerId, const HealthSyncData& healthData)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return;
        }

        auto& playerState = it->second;

        // Validate health data
        if (!ValidateHealthData(playerId, healthData)) {
            return;
        }

        // Check for significant changes
        HealthSyncData oldData = playerState->healthData;
        bool shouldSync = HealthStatusUtils::ShouldSyncImmediately(oldData, healthData);

        // Update health data
        playerState->healthData = healthData;
        playerState->lastHealthUpdate = std::chrono::steady_clock::now();
        playerState->lastActivity = playerState->lastHealthUpdate;

        m_totalHealthUpdates++;

        lock.unlock();

        // Check for critical events
        if (oldData.currentHealth > 0.0f && healthData.currentHealth <= 0.0f) {
            OnPlayerDowned(playerId);
        } else if (oldData.currentHealth <= 0.0f && healthData.currentHealth > 0.0f) {
            OnPlayerRevived(playerId);
        }

        // Notify listeners
        NotifyHealthUpdated(playerId, healthData);

        // Broadcast if significant change or forced sync
        if (shouldSync) {
            BroadcastHealthUpdate(playerId);
        }
    }

    void HealthStatusManager::ApplyStatusEffect(uint32_t playerId, const StatusEffectData& effect)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return;
        }

        auto& playerState = it->second;

        // Add or update status effect
        playerState->activeEffects[effect.effectType] = effect;
        playerState->lastStatusUpdate = std::chrono::steady_clock::now();
        playerState->lastActivity = playerState->lastStatusUpdate;

        // Update effect mapping
        UpdateEffectToPlayersMapping(playerId, effect.effectType, true);

        // Update health data status flags
        UpdateHealthDataFromEffects(playerState.get());

        m_totalStatusEffectChanges++;

        lock.unlock();

        // Notify listeners
        NotifyStatusEffectChanged(playerId, effect);

        // Broadcast status effect update
        BroadcastStatusEffectUpdate(playerId, effect.effectType);
    }

    void HealthStatusManager::RemoveStatusEffect(uint32_t playerId, StatusEffectType effectType)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return;
        }

        auto& playerState = it->second;

        // Check if effect exists
        auto effectIt = playerState->activeEffects.find(effectType);
        if (effectIt == playerState->activeEffects.end()) {
            return;
        }

        StatusEffectData removedEffect = effectIt->second;
        removedEffect.isActive = false;
        removedEffect.timestamp = std::chrono::steady_clock::now();

        // Remove status effect
        playerState->activeEffects.erase(effectIt);
        playerState->lastStatusUpdate = std::chrono::steady_clock::now();
        playerState->lastActivity = playerState->lastStatusUpdate;

        // Update effect mapping
        UpdateEffectToPlayersMapping(playerId, effectType, false);

        // Update health data status flags
        UpdateHealthDataFromEffects(playerState.get());

        m_totalStatusEffectChanges++;

        lock.unlock();

        // Notify listeners
        NotifyStatusEffectChanged(playerId, removedEffect);

        // Broadcast status effect removal
        BroadcastStatusEffectUpdate(playerId, effectType);
    }

    void HealthStatusManager::UpdatePlayerConditions(uint32_t playerId, const PlayerConditions& conditions)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return;
        }

        auto& playerState = it->second;

        playerState->conditions = conditions;
        playerState->lastConditionUpdate = std::chrono::steady_clock::now();
        playerState->lastActivity = playerState->lastConditionUpdate;

        lock.unlock();

        // Notify listeners
        NotifyConditionsUpdated(playerId, conditions);
    }

    void HealthStatusManager::OnPlayerDowned(uint32_t playerId, uint32_t attackerId, const std::string& weaponType)
    {
        auto* playerState = GetPlayerHealthState(playerId);
        if (!playerState) {
            return;
        }

        // Create critical event
        CriticalHealthEvent event;
        event.playerId = playerId;
        event.eventType = HealthEventType::Downed;
        event.healthBefore = playerState->healthData.currentHealth;
        event.healthAfter = 0.0f;
        event.attackerId = attackerId;
        event.weaponType = weaponType;
        event.canBeRevived = true;
        event.reviveTime = 10.0f; // 10 seconds to revive
        event.timestamp = std::chrono::steady_clock::now();

        // Add to recent events
        {
            std::lock_guard<std::mutex> lock(m_eventsMutex);
            playerState->recentEvents.push_back(event);
            m_recentCriticalEvents.push_back(event);
        }

        // Update health data
        playerState->healthData.isUnconscious = true;
        playerState->healthData.currentHealth = 0.0f;
        playerState->healthData.healthPercentage = 0.0f;

        m_totalCriticalEvents++;

        // Notify listeners
        NotifyCriticalEvent(playerId, event);
        NotifyPlayerDowned(playerId, attackerId);

        // Broadcast critical event
        BroadcastCriticalEvent(playerId, event);
    }

    void HealthStatusManager::OnPlayerRevived(uint32_t playerId, uint32_t reviverId)
    {
        auto* playerState = GetPlayerHealthState(playerId);
        if (!playerState) {
            return;
        }

        // Create critical event
        CriticalHealthEvent event;
        event.playerId = playerId;
        event.eventType = HealthEventType::Revived;
        event.healthBefore = 0.0f;
        event.healthAfter = 25.0f; // Revive with 25% health
        event.attackerId = reviverId;
        event.canBeRevived = false;
        event.timestamp = std::chrono::steady_clock::now();

        // Add to recent events
        {
            std::lock_guard<std::mutex> lock(m_eventsMutex);
            playerState->recentEvents.push_back(event);
            m_recentCriticalEvents.push_back(event);
        }

        // Update health data
        playerState->healthData.isUnconscious = false;
        playerState->healthData.currentHealth = 25.0f;
        playerState->healthData.healthPercentage = 0.25f;

        m_totalCriticalEvents++;

        // Notify listeners
        NotifyCriticalEvent(playerId, event);
        NotifyPlayerRevived(playerId, reviverId);

        // Broadcast critical event
        BroadcastCriticalEvent(playerId, event);
    }

    void HealthStatusManager::OnPlayerDamaged(uint32_t playerId, float damage, uint32_t attackerId)
    {
        auto* playerState = GetPlayerHealthState(playerId);
        if (!playerState) {
            return;
        }

        // Create damage event
        CriticalHealthEvent event;
        event.playerId = playerId;
        event.eventType = HealthEventType::Damage;
        event.healthBefore = playerState->healthData.currentHealth;
        event.healthAfter = playerState->healthData.currentHealth - damage;
        event.damageAmount = damage;
        event.attackerId = attackerId;
        event.timestamp = std::chrono::steady_clock::now();

        // Add to recent events if significant damage
        if (damage >= 10.0f) {
            std::lock_guard<std::mutex> lock(m_eventsMutex);
            playerState->recentEvents.push_back(event);
            m_recentCriticalEvents.push_back(event);
            m_totalCriticalEvents++;

            // Notify listeners
            NotifyCriticalEvent(playerId, event);

            // Broadcast if significant damage
            BroadcastCriticalEvent(playerId, event);
        }
    }

    PlayerHealthState* HealthStatusManager::GetPlayerHealthState(uint32_t playerId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        return (it != m_playerStates.end()) ? it->second.get() : nullptr;
    }

    const PlayerHealthState* HealthStatusManager::GetPlayerHealthState(uint32_t playerId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        return (it != m_playerStates.end()) ? it->second.get() : nullptr;
    }

    std::vector<uint32_t> HealthStatusManager::GetPlayersInCriticalCondition() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        std::vector<uint32_t> criticalPlayers;
        for (const auto& [playerId, playerState] : m_playerStates) {
            if (playerState->healthData.isUnconscious ||
                playerState->healthData.healthPercentage < 0.2f ||
                playerState->healthData.isBleeding ||
                playerState->healthData.isPoisoned) {
                criticalPlayers.push_back(playerId);
            }
        }

        return criticalPlayers;
    }

    std::vector<uint32_t> HealthStatusManager::GetPlayersWithStatusEffect(StatusEffectType effectType) const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_effectToPlayers.find(effectType);
        return (it != m_effectToPlayers.end()) ? it->second : std::vector<uint32_t>();
    }

    std::vector<uint32_t> HealthStatusManager::GetPlayersInCombat() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        std::vector<uint32_t> combatPlayers;
        for (const auto& [playerId, playerState] : m_playerStates) {
            if (playerState->healthData.isInCombat) {
                combatPlayers.push_back(playerId);
            }
        }

        return combatPlayers;
    }

    bool HealthStatusManager::ValidateHealthData(uint32_t playerId, const HealthSyncData& healthData) const
    {
        // Validate health values
        if (!IsValidHealthValue(healthData.currentHealth, healthData.maxHealth)) {
            return false;
        }

        if (!IsValidArmorValue(healthData.currentArmor, healthData.maxArmor)) {
            return false;
        }

        if (!IsValidStaminaValue(healthData.currentStamina, healthData.maxStamina)) {
            return false;
        }

        // Check for reasonable health change rates
        auto* playerState = GetPlayerHealthState(playerId);
        if (playerState) {
            auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                healthData.timestamp - playerState->lastHealthUpdate).count() / 1000.0f;

            if (deltaTime > 0.0f) {
                float changeRate = CalculateHealthChangeRate(playerId, healthData.currentHealth, deltaTime);
                bool isHealing = healthData.currentHealth > playerState->healthData.currentHealth;

                if (!IsHealthChangeRateValid(changeRate, isHealing)) {
                    return false;
                }
            }
        }

        return true;
    }

    void HealthStatusManager::BroadcastHealthUpdate(uint32_t playerId)
    {
        auto* playerState = GetPlayerHealthState(playerId);
        if (!playerState) {
            return;
        }

        SendHealthUpdateToClients(playerId, playerState->healthData);
    }

    void HealthStatusManager::BroadcastStatusEffectUpdate(uint32_t playerId, StatusEffectType effectType)
    {
        auto* playerState = GetPlayerHealthState(playerId);
        if (!playerState) {
            return;
        }

        auto it = playerState->activeEffects.find(effectType);
        if (it != playerState->activeEffects.end()) {
            SendStatusEffectUpdateToClients(playerId, it->second);
        } else {
            // Send removal update
            StatusEffectData removeData;
            removeData.playerId = playerId;
            removeData.effectType = effectType;
            removeData.isActive = false;
            removeData.timestamp = std::chrono::steady_clock::now();
            SendStatusEffectUpdateToClients(playerId, removeData);
        }
    }

    // Private implementation methods
    void HealthStatusManager::UpdatePlayerHealthStates(float deltaTime)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [playerId, playerState] : m_playerStates) {
            // Check for player timeout (30 seconds of inactivity)
            auto timeSinceActivity = std::chrono::duration_cast<std::chrono::seconds>(
                currentTime - playerState->lastActivity).count();

            if (timeSinceActivity >= 30) {
                playerState->isConnected = false;
                // Don't automatically remove - let the session manager handle this
            }

            // Update sync priority based on health status
            playerState->syncPriority = HealthStatusUtils::CalculateSyncPriority(*playerState);
        }
    }

    void HealthStatusManager::ProcessStatusEffectUpdates(float deltaTime)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [playerId, playerState] : m_playerStates) {
            std::vector<StatusEffectType> expiredEffects;

            for (auto& [effectType, effectData] : playerState->activeEffects) {
                // Calculate elapsed time
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    currentTime - effectData.startTime).count();

                // Check if effect has expired
                if (effectData.duration > 0.0f && elapsed >= effectData.duration) {
                    expiredEffects.push_back(effectType);
                }
            }

            // Remove expired effects
            for (StatusEffectType effectType : expiredEffects) {
                lock.unlock();
                RemoveStatusEffect(playerId, effectType);
                lock.lock();
            }
        }
    }

    void HealthStatusManager::CleanupExpiredEffects()
    {
        // This is handled in ProcessStatusEffectUpdates
    }

    void HealthStatusManager::CleanupOldEvents()
    {
        std::lock_guard<std::mutex> lock(m_eventsMutex);

        auto currentTime = std::chrono::steady_clock::now();
        auto cutoffTime = currentTime - std::chrono::minutes(5); // Keep events for 5 minutes

        // Clean up global events
        m_recentCriticalEvents.erase(
            std::remove_if(m_recentCriticalEvents.begin(), m_recentCriticalEvents.end(),
                [cutoffTime](const CriticalHealthEvent& event) {
                    return event.timestamp < cutoffTime;
                }),
            m_recentCriticalEvents.end());

        // Clean up per-player events
        std::shared_lock<std::shared_mutex> statesLock(m_statesMutex);
        for (auto& [playerId, playerState] : m_playerStates) {
            playerState->recentEvents.erase(
                std::remove_if(playerState->recentEvents.begin(), playerState->recentEvents.end(),
                    [cutoffTime](const CriticalHealthEvent& event) {
                        return event.timestamp < cutoffTime;
                    }),
                playerState->recentEvents.end());
        }
    }

    bool HealthStatusManager::IsValidHealthValue(float health, float maxHealth) const
    {
        return health >= 0.0f && health <= maxHealth && maxHealth > 0.0f && maxHealth <= 10000.0f;
    }

    bool HealthStatusManager::IsValidArmorValue(float armor, float maxArmor) const
    {
        return armor >= 0.0f && armor <= maxArmor && maxArmor >= 0.0f && maxArmor <= 10000.0f;
    }

    bool HealthStatusManager::IsValidStaminaValue(float stamina, float maxStamina) const
    {
        return stamina >= 0.0f && stamina <= maxStamina && maxStamina > 0.0f && maxStamina <= 10000.0f;
    }

    void HealthStatusManager::UpdateEffectToPlayersMapping(uint32_t playerId, StatusEffectType effectType, bool isActive)
    {
        auto& playerList = m_effectToPlayers[effectType];

        if (isActive) {
            // Add player if not already present
            if (std::find(playerList.begin(), playerList.end(), playerId) == playerList.end()) {
                playerList.push_back(playerId);
            }
        } else {
            // Remove player
            playerList.erase(std::remove(playerList.begin(), playerList.end(), playerId), playerList.end());
        }
    }

    void HealthStatusManager::RemovePlayerFromAllEffectMappings(uint32_t playerId)
    {
        for (auto& [effectType, playerList] : m_effectToPlayers) {
            playerList.erase(std::remove(playerList.begin(), playerList.end(), playerId), playerList.end());
        }
    }

    void HealthStatusManager::UpdateHealthDataFromEffects(PlayerHealthState* playerState)
    {
        if (!playerState) {
            return;
        }

        // Reset status flags
        playerState->healthData.isBleeding = false;
        playerState->healthData.isPoisoned = false;
        playerState->healthData.isBurning = false;
        playerState->healthData.isElectrified = false;
        playerState->healthData.isStunned = false;
        playerState->healthData.isBlinded = false;

        // Set flags based on active effects
        for (const auto& [effectType, effectData] : playerState->activeEffects) {
            switch (effectType) {
                case StatusEffectType::Bleeding:
                    playerState->healthData.isBleeding = true;
                    break;
                case StatusEffectType::Poisoned:
                    playerState->healthData.isPoisoned = true;
                    break;
                case StatusEffectType::Burning:
                    playerState->healthData.isBurning = true;
                    break;
                case StatusEffectType::Electrified:
                    playerState->healthData.isElectrified = true;
                    break;
                case StatusEffectType::Stunned:
                    playerState->healthData.isStunned = true;
                    break;
                case StatusEffectType::Blinded:
                    playerState->healthData.isBlinded = true;
                    break;
                default:
                    break;
            }
        }
    }

    // Notification methods
    void HealthStatusManager::NotifyHealthUpdated(uint32_t playerId, const HealthSyncData& healthData)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_healthUpdatedCallback) {
            m_healthUpdatedCallback(playerId, healthData);
        }
    }

    float HealthStatusManager::CalculateHealthChangeRate(uint32_t playerId, float newHealth, float deltaTime) const
    {
        auto* playerState = GetPlayerHealthState(playerId);
        if (!playerState || deltaTime <= 0.0f) {
            return 0.0f;
        }

        float oldHealth = playerState->healthData.currentHealth;
        return std::abs(newHealth - oldHealth) / deltaTime;
    }

    bool HealthStatusManager::IsHealthChangeRateValid(float changeRate, bool isHealing) const
    {
        // Maximum health change rates (per second)
        const float maxDamageRate = 200.0f; // 200 HP/sec max damage
        const float maxHealingRate = 100.0f; // 100 HP/sec max healing

        if (isHealing) {
            return changeRate <= maxHealingRate;
        } else {
            return changeRate <= maxDamageRate;
        }
    }

    void HealthStatusManager::SendHealthUpdateToClients(uint32_t playerId, const HealthSyncData& healthData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    // Utility functions implementation
    namespace HealthStatusUtils
    {
        std::string PlayerStanceToString(PlayerStance stance)
        {
            switch (stance) {
                case PlayerStance::Standing: return "Standing";
                case PlayerStance::Crouching: return "Crouching";
                case PlayerStance::Prone: return "Prone";
                case PlayerStance::Combat: return "Combat";
                case PlayerStance::Vehicle: return "Vehicle";
                default: return "Unknown";
            }
        }

        std::string StatusEffectTypeToString(StatusEffectType effectType)
        {
            switch (effectType) {
                case StatusEffectType::None: return "None";
                case StatusEffectType::Bleeding: return "Bleeding";
                case StatusEffectType::Poisoned: return "Poisoned";
                case StatusEffectType::Burning: return "Burning";
                case StatusEffectType::Electrified: return "Electrified";
                case StatusEffectType::Stunned: return "Stunned";
                case StatusEffectType::Blinded: return "Blinded";
                case StatusEffectType::Slowed: return "Slowed";
                case StatusEffectType::Weakened: return "Weakened";
                case StatusEffectType::Berserker: return "Berserker";
                case StatusEffectType::Berserk: return "Berserk";
                case StatusEffectType::Healing: return "Healing";
                case StatusEffectType::Armor_Boost: return "Armor_Boost";
                case StatusEffectType::Speed_Boost: return "Speed_Boost";
                case StatusEffectType::Damage_Boost: return "Damage_Boost";
                default: return "Unknown";
            }
        }

        bool IsDebuffEffect(StatusEffectType effectType)
        {
            switch (effectType) {
                case StatusEffectType::Bleeding:
                case StatusEffectType::Poisoned:
                case StatusEffectType::Burning:
                case StatusEffectType::Electrified:
                case StatusEffectType::Stunned:
                case StatusEffectType::Blinded:
                case StatusEffectType::Slowed:
                case StatusEffectType::Weakened:
                    return true;
                default:
                    return false;
            }
        }

        bool IsBuffEffect(StatusEffectType effectType)
        {
            switch (effectType) {
                case StatusEffectType::Berserker:
                case StatusEffectType::Berserk:
                case StatusEffectType::Healing:
                case StatusEffectType::Armor_Boost:
                case StatusEffectType::Speed_Boost:
                case StatusEffectType::Damage_Boost:
                    return true;
                default:
                    return false;
            }
        }

        float CalculateHealthPercentage(float current, float maximum)
        {
            if (maximum <= 0.0f) {
                return 0.0f;
            }
            return std::clamp(current / maximum, 0.0f, 1.0f);
        }

        bool ShouldSyncImmediately(const HealthSyncData& oldData, const HealthSyncData& newData)
        {
            // Sync immediately if health changed significantly
            float healthDiff = std::abs(oldData.currentHealth - newData.currentHealth);
            if (healthDiff >= 10.0f) {
                return true;
            }

            // Sync if status conditions changed
            if (oldData.isInCombat != newData.isInCombat ||
                oldData.isUnconscious != newData.isUnconscious ||
                oldData.isBleeding != newData.isBleeding ||
                oldData.isPoisoned != newData.isPoisoned ||
                oldData.isBurning != newData.isBurning ||
                oldData.isElectrified != newData.isElectrified) {
                return true;
            }

            return false;
        }

        float CalculateSyncPriority(const PlayerHealthState& state)
        {
            float priority = 1.0f;

            // Higher priority for critical health
            if (state.healthData.healthPercentage < 0.3f) {
                priority += 2.0f;
            }

            // Higher priority for unconscious players
            if (state.healthData.isUnconscious) {
                priority += 3.0f;
            }

            // Higher priority for players in combat
            if (state.healthData.isInCombat) {
                priority += 1.5f;
            }

            // Higher priority for players with debuffs
            if (state.healthData.isBleeding || state.healthData.isPoisoned ||
                state.healthData.isBurning || state.healthData.isElectrified) {
                priority += 1.0f;
            }

            return priority;
        }
    }

    // Missing function implementations
    void HealthStatusManager::ValidatePlayerStates()
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [playerId, playerState] : m_playerStates) {
            // Validate health values
            if (!IsValidHealthValue(playerState->healthData.currentHealth, playerState->healthData.maxHealth)) {
                // Reset to safe values
                playerState->healthData.currentHealth = std::clamp(playerState->healthData.currentHealth, 0.0f, playerState->healthData.maxHealth);
                playerState->healthData.healthPercentage = HealthStatusUtils::CalculateHealthPercentage(playerState->healthData.currentHealth, playerState->healthData.maxHealth);
            }

            // Validate armor values
            if (!IsValidArmorValue(playerState->healthData.currentArmor, playerState->healthData.maxArmor)) {
                playerState->healthData.currentArmor = std::clamp(playerState->healthData.currentArmor, 0.0f, playerState->healthData.maxArmor);
                playerState->healthData.armorPercentage = HealthStatusUtils::CalculateHealthPercentage(playerState->healthData.currentArmor, playerState->healthData.maxArmor);
            }

            // Validate stamina values
            if (!IsValidStaminaValue(playerState->healthData.currentStamina, playerState->healthData.maxStamina)) {
                playerState->healthData.currentStamina = std::clamp(playerState->healthData.currentStamina, 0.0f, playerState->healthData.maxStamina);
                playerState->healthData.staminaPercentage = HealthStatusUtils::CalculateHealthPercentage(playerState->healthData.currentStamina, playerState->healthData.maxStamina);
            }

            // Check for inconsistent unconscious state
            if (playerState->healthData.isUnconscious && playerState->healthData.currentHealth > 0.0f) {
                // Player is marked unconscious but has health - fix this
                if (playerState->healthData.currentHealth > 25.0f) {
                    playerState->healthData.isUnconscious = false;
                }
            }
        }
    }

    void HealthStatusManager::NotifyStatusEffectChanged(uint32_t playerId, const StatusEffectData& effect)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_statusEffectChangedCallback) {
            m_statusEffectChangedCallback(playerId, effect);
        }
    }

    void HealthStatusManager::NotifyCriticalEvent(uint32_t playerId, const CriticalHealthEvent& event)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_criticalEventCallback) {
            m_criticalEventCallback(playerId, event);
        }
    }

    void HealthStatusManager::NotifyPlayerDowned(uint32_t playerId, uint32_t attackerId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_playerDownedCallback) {
            m_playerDownedCallback(playerId, attackerId);
        }
    }

    void HealthStatusManager::NotifyPlayerRevived(uint32_t playerId, uint32_t reviverId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_playerRevivedCallback) {
            m_playerRevivedCallback(playerId, reviverId);
        }
    }

    void HealthStatusManager::NotifyConditionsUpdated(uint32_t playerId, const PlayerConditions& conditions)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_conditionsUpdatedCallback) {
            m_conditionsUpdatedCallback(playerId, conditions);
        }
    }

    void HealthStatusManager::SendStatusEffectUpdateToClients(uint32_t playerId, const StatusEffectData& effect)
    {
        // This would send network messages to all clients about status effect changes
        // Implementation would depend on the networking system
        // For now, this is a placeholder that could be integrated with Net_Broadcast
    }

    void HealthStatusManager::BroadcastCriticalEvent(uint32_t playerId, const CriticalHealthEvent& event)
    {
        // This would broadcast critical events (like downed/revived) to all clients
        // Implementation would depend on the networking system
        // For now, this is a placeholder that could be integrated with Net_Broadcast
    }

    // Additional missing methods from the header
    void HealthStatusManager::UpdatePlayerActivity(uint32_t playerId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it != m_playerStates.end()) {
            it->second->lastActivity = std::chrono::steady_clock::now();
            it->second->isConnected = true;
        }
    }

    void HealthStatusManager::ProcessHealthEvent(uint32_t playerId, const CriticalHealthEvent& event)
    {
        auto* playerState = GetPlayerHealthState(playerId);
        if (!playerState) {
            return;
        }

        // Add to recent events
        {
            std::lock_guard<std::mutex> lock(m_eventsMutex);
            playerState->recentEvents.push_back(event);
            m_recentCriticalEvents.push_back(event);
        }

        m_totalCriticalEvents++;

        // Notify listeners
        NotifyCriticalEvent(playerId, event);

        // Broadcast if it's a significant event
        if (event.eventType == HealthEventType::Downed ||
            event.eventType == HealthEventType::Revived ||
            event.damageAmount >= 25.0f) {
            BroadcastCriticalEvent(playerId, event);
        }
    }

    void HealthStatusManager::SyncCriticalHealth(uint32_t playerId, HealthEventType eventType)
    {
        auto* playerState = GetPlayerHealthState(playerId);
        if (!playerState) {
            return;
        }

        // Force immediate sync for critical events
        BroadcastHealthUpdate(playerId);

        // If player has status effects, sync those too
        for (const auto& [effectType, effectData] : playerState->activeEffects) {
            BroadcastStatusEffectUpdate(playerId, effectType);
        }
    }

    void HealthStatusManager::UpdateStatusEffect(uint32_t playerId, const StatusEffectData& effect)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return;
        }

        auto& playerState = it->second;

        // Update existing status effect
        auto effectIt = playerState->activeEffects.find(effect.effectType);
        if (effectIt != playerState->activeEffects.end()) {
            effectIt->second = effect;
            playerState->lastStatusUpdate = std::chrono::steady_clock::now();
            playerState->lastActivity = playerState->lastStatusUpdate;

            // Update health data status flags
            UpdateHealthDataFromEffects(playerState.get());

            m_totalStatusEffectChanges++;

            lock.unlock();

            // Notify listeners
            NotifyStatusEffectChanged(playerId, effect);

            // Broadcast status effect update
            BroadcastStatusEffectUpdate(playerId, effect.effectType);
        }
    }

    void HealthStatusManager::ClearAllStatusEffects(uint32_t playerId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return;
        }

        auto& playerState = it->second;

        // Collect all effect types to remove
        std::vector<StatusEffectType> effectTypes;
        for (const auto& [effectType, effectData] : playerState->activeEffects) {
            effectTypes.push_back(effectType);
        }

        lock.unlock();

        // Remove all effects
        for (StatusEffectType effectType : effectTypes) {
            RemoveStatusEffect(playerId, effectType);
        }
    }

    void HealthStatusManager::UpdatePlayerPosition(uint32_t playerId, float x, float y, float z)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it != m_playerStates.end()) {
            it->second->healthData.posX = x;
            it->second->healthData.posY = y;
            it->second->healthData.posZ = z;
            it->second->lastActivity = std::chrono::steady_clock::now();
        }
    }

    void HealthStatusManager::UpdatePlayerStance(uint32_t playerId, PlayerStance stance)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it != m_playerStates.end()) {
            it->second->conditions.currentStance = stance;
            it->second->lastConditionUpdate = std::chrono::steady_clock::now();
            it->second->lastActivity = it->second->lastConditionUpdate;
        }
    }

    void HealthStatusManager::OnPlayerHealed(uint32_t playerId, float healing, uint32_t healerId)
    {
        auto* playerState = GetPlayerHealthState(playerId);
        if (!playerState) {
            return;
        }

        // Create healing event
        CriticalHealthEvent event;
        event.playerId = playerId;
        event.eventType = HealthEventType::Healing;
        event.healthBefore = playerState->healthData.currentHealth;
        event.healthAfter = playerState->healthData.currentHealth + healing;
        event.damageAmount = -healing; // Negative damage for healing
        event.attackerId = healerId;
        event.timestamp = std::chrono::steady_clock::now();

        // Add to recent events if significant healing
        if (healing >= 15.0f) {
            std::lock_guard<std::mutex> lock(m_eventsMutex);
            playerState->recentEvents.push_back(event);
            m_recentCriticalEvents.push_back(event);
            m_totalCriticalEvents++;

            // Notify listeners
            NotifyCriticalEvent(playerId, event);

            // Broadcast if significant healing
            BroadcastCriticalEvent(playerId, event);
        }
    }

    bool HealthStatusManager::IsHealthChangeValid(uint32_t playerId, float oldHealth, float newHealth, float deltaTime) const
    {
        if (deltaTime <= 0.0f) {
            return true; // No time passed, any change is valid
        }

        float changeRate = std::abs(newHealth - oldHealth) / deltaTime;
        bool isHealing = newHealth > oldHealth;

        return IsHealthChangeRateValid(changeRate, isHealing);
    }

    void HealthStatusManager::DetectHealthAnomalies(uint32_t playerId)
    {
        auto* playerState = GetPlayerHealthState(playerId);
        if (!playerState) {
            return;
        }

        // Check for suspicious health patterns
        auto currentTime = std::chrono::steady_clock::now();

        // Check if health changed too rapidly
        auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - playerState->lastHealthUpdate).count() / 1000.0f;

        if (deltaTime > 0.0f) {
            // Look for health changes that exceed normal rates
            // This would be expanded to include more sophisticated detection
        }

        // Check for impossible health values
        if (playerState->healthData.currentHealth > playerState->healthData.maxHealth) {
            // Health exceeds maximum - potential cheat
            playerState->healthData.currentHealth = playerState->healthData.maxHealth;
            playerState->healthData.healthPercentage = 1.0f;
        }
    }

    void HealthStatusManager::SetSyncPriority(uint32_t playerId, float priority)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it != m_playerStates.end()) {
            it->second->syncPriority = std::clamp(priority, 0.1f, 10.0f);
        }
    }

    void HealthStatusManager::ForceSyncPlayer(uint32_t playerId)
    {
        BroadcastHealthUpdate(playerId);

        // Also sync all status effects
        auto* playerState = GetPlayerHealthState(playerId);
        if (playerState) {
            for (const auto& [effectType, effectData] : playerState->activeEffects) {
                BroadcastStatusEffectUpdate(playerId, effectType);
            }
        }
    }

    uint32_t HealthStatusManager::GetActivePlayerCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        uint32_t count = 0;
        for (const auto& [playerId, playerState] : m_playerStates) {
            if (playerState->isConnected) {
                count++;
            }
        }
        return count;
    }

    float HealthStatusManager::GetAveragePlayerHealth() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        if (m_playerStates.empty()) {
            return 0.0f;
        }

        float totalHealth = 0.0f;
        uint32_t connectedPlayers = 0;

        for (const auto& [playerId, playerState] : m_playerStates) {
            if (playerState->isConnected) {
                totalHealth += playerState->healthData.healthPercentage;
                connectedPlayers++;
            }
        }

        return connectedPlayers > 0 ? (totalHealth / connectedPlayers) : 0.0f;
    }

    uint32_t HealthStatusManager::GetTotalStatusEffectsActive() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        uint32_t total = 0;
        for (const auto& [playerId, playerState] : m_playerStates) {
            total += static_cast<uint32_t>(playerState->activeEffects.size());
        }
        return total;
    }

    std::unordered_map<StatusEffectType, uint32_t> HealthStatusManager::GetStatusEffectDistribution() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        std::unordered_map<StatusEffectType, uint32_t> distribution;

        for (const auto& [effectType, playerList] : m_effectToPlayers) {
            distribution[effectType] = static_cast<uint32_t>(playerList.size());
        }

        return distribution;
    }

    // Callback setter methods
    void HealthStatusManager::SetHealthUpdatedCallback(HealthUpdatedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_healthUpdatedCallback = callback;
    }

    void HealthStatusManager::SetStatusEffectChangedCallback(StatusEffectChangedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_statusEffectChangedCallback = callback;
    }

    void HealthStatusManager::SetCriticalEventCallback(CriticalEventCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_criticalEventCallback = callback;
    }

    void HealthStatusManager::SetPlayerDownedCallback(PlayerDownedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_playerDownedCallback = callback;
    }

    void HealthStatusManager::SetPlayerRevivedCallback(PlayerRevivedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_playerRevivedCallback = callback;
    }

    void HealthStatusManager::SetConditionsUpdatedCallback(ConditionsUpdatedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_conditionsUpdatedCallback = callback;
    }

    // Additional utility functions that were missing
    namespace HealthStatusUtils
    {
        PlayerStance StringToPlayerStance(const std::string& stanceStr)
        {
            if (stanceStr == "Standing") return PlayerStance::Standing;
            if (stanceStr == "Crouching") return PlayerStance::Crouching;
            if (stanceStr == "Prone") return PlayerStance::Prone;
            if (stanceStr == "Combat") return PlayerStance::Combat;
            if (stanceStr == "Vehicle") return PlayerStance::Vehicle;
            return PlayerStance::Standing; // Default
        }

        StatusEffectType StringToStatusEffectType(const std::string& effectStr)
        {
            if (effectStr == "None") return StatusEffectType::None;
            if (effectStr == "Bleeding") return StatusEffectType::Bleeding;
            if (effectStr == "Poisoned") return StatusEffectType::Poisoned;
            if (effectStr == "Burning") return StatusEffectType::Burning;
            if (effectStr == "Electrified") return StatusEffectType::Electrified;
            if (effectStr == "Stunned") return StatusEffectType::Stunned;
            if (effectStr == "Blinded") return StatusEffectType::Blinded;
            if (effectStr == "Slowed") return StatusEffectType::Slowed;
            if (effectStr == "Weakened") return StatusEffectType::Weakened;
            if (effectStr == "Berserker") return StatusEffectType::Berserker;
            if (effectStr == "Berserk") return StatusEffectType::Berserk;
            if (effectStr == "Healing") return StatusEffectType::Healing;
            if (effectStr == "Armor_Boost") return StatusEffectType::Armor_Boost;
            if (effectStr == "Speed_Boost") return StatusEffectType::Speed_Boost;
            if (effectStr == "Damage_Boost") return StatusEffectType::Damage_Boost;
            return StatusEffectType::None; // Default
        }

        std::string HealthEventTypeToString(HealthEventType eventType)
        {
            switch (eventType) {
                case HealthEventType::Damage: return "Damage";
                case HealthEventType::Healing: return "Healing";
                case HealthEventType::StatusApplied: return "StatusApplied";
                case HealthEventType::StatusRemoved: return "StatusRemoved";
                case HealthEventType::Downed: return "Downed";
                case HealthEventType::Revived: return "Revived";
                case HealthEventType::CriticalCondition: return "CriticalCondition";
                default: return "Unknown";
            }
        }

        HealthEventType StringToHealthEventType(const std::string& eventStr)
        {
            if (eventStr == "Damage") return HealthEventType::Damage;
            if (eventStr == "Healing") return HealthEventType::Healing;
            if (eventStr == "StatusApplied") return HealthEventType::StatusApplied;
            if (eventStr == "StatusRemoved") return HealthEventType::StatusRemoved;
            if (eventStr == "Downed") return HealthEventType::Downed;
            if (eventStr == "Revived") return HealthEventType::Revived;
            if (eventStr == "CriticalCondition") return HealthEventType::CriticalCondition;
            return HealthEventType::Damage; // Default
        }

        bool IsCriticalStatusEffect(StatusEffectType effectType)
        {
            switch (effectType) {
                case StatusEffectType::Bleeding:
                case StatusEffectType::Poisoned:
                case StatusEffectType::Burning:
                case StatusEffectType::Stunned:
                    return true;
                default:
                    return false;
            }
        }

        float CalculateEffectIntensity(const StatusEffectData& effect, float deltaTime)
        {
            if (effect.duration <= 0.0f) {
                return effect.intensity;
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - effect.startTime).count();

            float remainingRatio = 1.0f - (elapsed / effect.duration);
            return effect.intensity * std::max(0.0f, remainingRatio);
        }

        uint32_t HashHealthState(const HealthSyncData& healthData)
        {
            // Simple hash for health state comparison
            uint32_t hash = 0;
            hash ^= static_cast<uint32_t>(healthData.currentHealth * 100.0f);
            hash ^= static_cast<uint32_t>(healthData.currentArmor * 100.0f) << 8;
            hash ^= static_cast<uint32_t>(healthData.currentStamina * 100.0f) << 16;
            hash ^= (healthData.isInCombat ? 1 : 0) << 24;
            hash ^= (healthData.isUnconscious ? 1 : 0) << 25;
            return hash;
        }

        bool IsHealthDataEquivalent(const HealthSyncData& data1, const HealthSyncData& data2, float tolerance)
        {
            return std::abs(data1.currentHealth - data2.currentHealth) <= tolerance &&
                   std::abs(data1.currentArmor - data2.currentArmor) <= tolerance &&
                   std::abs(data1.currentStamina - data2.currentStamina) <= tolerance &&
                   data1.isInCombat == data2.isInCombat &&
                   data1.isUnconscious == data2.isUnconscious &&
                   data1.isBleeding == data2.isBleeding &&
                   data1.isPoisoned == data2.isPoisoned &&
                   data1.isBurning == data2.isBurning &&
                   data1.isElectrified == data2.isElectrified &&
                   data1.isStunned == data2.isStunned &&
                   data1.isBlinded == data2.isBlinded;
        }
    }
}
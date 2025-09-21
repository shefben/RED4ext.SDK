#include "CombatStateManager.hpp"
#include "RED4ext/Api/Sdk.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace RED4ext
{
    // CombatStateManager Implementation
    CombatStateManager& CombatStateManager::GetInstance()
    {
        static CombatStateManager instance;
        return instance;
    }

    void CombatStateManager::Initialize()
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Clear existing data
        m_playerStates.clear();
        m_remoteCombatStates.clear();
        m_combatEngagements.clear();
        m_stateToPlayers.clear();

        // Initialize timing
        m_lastUpdate = std::chrono::steady_clock::now();
        m_lastCleanup = m_lastUpdate;
        m_updateInterval = 0.05f; // 20 FPS combat updates

        // Initialize statistics
        m_totalShotsFired = 0;
        m_totalDamageDealt = 0.0f;
        m_totalCombatEngagements = 0;
        m_totalPlayerKills = 0;

        // Initialize state to players mapping for all combat states
        for (int i = 0; i <= static_cast<int>(CombatState::PostCombat); ++i) {
            CombatState state = static_cast<CombatState>(i);
            m_stateToPlayers[state] = {};
        }
    }

    void CombatStateManager::Shutdown()
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        m_playerStates.clear();
        m_remoteCombatStates.clear();
        m_combatEngagements.clear();
        m_stateToPlayers.clear();

        // Clear callbacks
        std::lock_guard<std::mutex> callbacksLock(m_callbacksMutex);
        m_combatStateChangedCallback = nullptr;
        m_weaponFiredCallback = nullptr;
        m_damageDealtCallback = nullptr;
        m_playerKilledCallback = nullptr;
        m_combatEngagementCallback = nullptr;
    }

    void CombatStateManager::Update()
    {
        auto currentTime = std::chrono::steady_clock::now();
        auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - m_lastUpdate).count() / 1000.0f;

        m_lastUpdate = currentTime;

        // Update player combat states
        UpdatePlayerCombatStates(deltaTime);

        // Process recent events
        ProcessRecentEvents(deltaTime);

        // Update combat engagements
        UpdateCombatEngagements(deltaTime);

        // Validate combat states
        ValidateCombatStates();

        // Periodic cleanup (every 30 seconds)
        auto timeSinceCleanup = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - m_lastCleanup).count();

        if (timeSinceCleanup >= 30) {
            CleanupExpiredData();
            m_lastCleanup = currentTime;
        }
    }

    void CombatStateManager::AddPlayer(uint32_t playerId, const std::string& playerName)
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        auto playerState = std::make_unique<PlayerCombatState>();
        playerState->playerId = playerId;
        playerState->playerName = playerName;
        playerState->localState.playerId = playerId;
        playerState->isConnected = true;
        playerState->syncPriority = 1.0f;
        playerState->currentEngagementId = 0;

        m_playerStates[playerId] = std::move(playerState);

        // Also create remote combat state entry
        auto remoteCombatState = std::make_unique<RemoteCombatState>();
        remoteCombatState->playerId = playerId;
        m_remoteCombatStates[playerId] = std::move(remoteCombatState);
    }

    void CombatStateManager::RemovePlayer(uint32_t playerId)
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Leave any active engagement
        auto playerIt = m_playerStates.find(playerId);
        if (playerIt != m_playerStates.end() && playerIt->second->currentEngagementId != 0) {
            LeaveCombatEngagement(playerId);
        }

        // Remove from all mappings
        RemovePlayerFromAllMappings(playerId);

        // Remove player state and remote state
        m_playerStates.erase(playerId);
        m_remoteCombatStates.erase(playerId);
    }

    bool CombatStateManager::UpdateCombatState(uint32_t playerId, const CombatSyncData& combatData)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return false;
        }

        auto& playerState = it->second;

        // Validate data
        if (!ValidateCombatData(playerId, combatData)) {
            return false;
        }

        // Store previous state for comparison
        CombatState previousState = playerState->localState.combatState;

        // Update local combat state
        playerState->localState.combatState = combatData.combatState;
        playerState->localState.combatStance = combatData.stance;
        playerState->localState.coverState = combatData.coverState;
        playerState->localState.aimingState = combatData.aimingState;
        playerState->localState.movementMode = combatData.movementMode;
        playerState->localState.alertLevel = combatData.alertLevel;
        playerState->localState.currentWeapon = combatData.currentWeapon;
        playerState->localState.weaponDrawn = combatData.weaponDrawn;
        playerState->localState.isReloading = combatData.isReloading;
        playerState->localState.isFiring = combatData.isFiring;
        playerState->localState.currentTarget = combatData.currentTarget;
        playerState->localState.position = combatData.position;
        playerState->localState.aimDirection = combatData.aimDirection;
        playerState->localState.lastUpdate = std::chrono::steady_clock::now();
        playerState->localState.hasStateChanged = true;

        playerState->lastCombatUpdate = playerState->localState.lastUpdate;
        playerState->lastActivity = playerState->lastCombatUpdate;

        // Update state mappings
        UpdateStateToPlayersMapping(playerId, previousState, false);
        UpdateStateToPlayersMapping(playerId, combatData.combatState, true);

        // Check for combat state transitions
        if (previousState != combatData.combatState) {
            lock.unlock();

            // Handle special state transitions
            if (previousState == CombatState::OutOfCombat && combatData.combatState != CombatState::OutOfCombat) {
                OnCombatStarted(playerId);
            } else if (previousState != CombatState::OutOfCombat && combatData.combatState == CombatState::OutOfCombat) {
                OnCombatEnded(playerId);
            }

            // Notify listeners
            NotifyCombatStateChanged(playerId, previousState, combatData.combatState);

            lock.lock();
        }

        lock.unlock();

        // Broadcast update
        BroadcastCombatUpdate(playerId, combatData);

        return true;
    }

    bool CombatStateManager::ProcessWeaponFire(uint32_t playerId, const WeaponFireData& fireData)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return false;
        }

        auto& playerState = it->second;

        // Validate fire data
        if (!ValidateFireData(playerId, fireData)) {
            return false;
        }

        // Check fire rate limiting
        if (IsFireRateLimited(playerId, fireData.weaponId)) {
            return false;
        }

        // Update weapon state
        auto weaponIt = playerState->weapons.find(fireData.weaponId);
        if (weaponIt == playerState->weapons.end()) {
            // Create new weapon state if not exists
            auto weaponState = std::make_unique<WeaponState>();
            weaponState->weaponId = fireData.weaponId;
            weaponState->lastFired = std::chrono::steady_clock::now();
            weaponState->totalShots += fireData.shotsFired;
            weaponState->totalDamageDealt += fireData.damage;
            playerState->weapons[fireData.weaponId] = std::move(weaponState);
        } else {
            weaponIt->second->lastFired = std::chrono::steady_clock::now();
            weaponIt->second->totalShots += fireData.shotsFired;
            weaponIt->second->totalDamageDealt += fireData.damage;
        }

        // Add to recent shots
        playerState->recentShots.push_back(fireData);
        if (playerState->recentShots.size() > 100) { // Keep last 100 shots
            playerState->recentShots.erase(playerState->recentShots.begin());
        }

        // Update player statistics
        playerState->totalShotsFired += fireData.shotsFired;
        playerState->totalDamageDealt += fireData.damage;
        playerState->lastWeaponUpdate = std::chrono::steady_clock::now();
        playerState->lastActivity = playerState->lastWeaponUpdate;

        // Update global statistics
        m_totalShotsFired += fireData.shotsFired;
        m_totalDamageDealt += fireData.damage;

        lock.unlock();

        // Notify listeners
        NotifyWeaponFired(playerId, fireData);

        // Broadcast weapon fire
        BroadcastWeaponFire(playerId, fireData);

        return true;
    }

    bool CombatStateManager::ProcessDamageDealt(uint32_t attackerId, const DamageDealtData& damageData)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(attackerId);
        if (it == m_playerStates.end()) {
            return false;
        }

        auto& playerState = it->second;

        // Validate damage data
        if (!IsDamageValid(damageData)) {
            return false;
        }

        // Add to recent damage
        playerState->recentDamage.push_back(damageData);
        if (playerState->recentDamage.size() > 50) { // Keep last 50 damage instances
            playerState->recentDamage.erase(playerState->recentDamage.begin());
        }

        // Update statistics
        playerState->totalDamageDealt += damageData.damage;
        m_totalDamageDealt += damageData.damage;

        lock.unlock();

        // Notify listeners
        NotifyDamageDealt(attackerId, damageData);

        // Broadcast damage dealt
        BroadcastDamageDealt(attackerId, damageData);

        return true;
    }

    bool CombatStateManager::ProcessPlayerKill(uint32_t killerId, const PlayerKillData& killData)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(killerId);
        if (it == m_playerStates.end()) {
            return false;
        }

        auto& killerState = it->second;

        // Update killer statistics
        killerState->totalKills++;
        m_totalPlayerKills++;

        // Update victim statistics
        auto victimIt = m_playerStates.find(static_cast<uint32_t>(killData.victimId));
        if (victimIt != m_playerStates.end()) {
            victimIt->second->totalDeaths++;
        }

        lock.unlock();

        // Notify listeners
        NotifyPlayerKilled(killerId, killData);

        return true;
    }

    void CombatStateManager::OnCombatStarted(uint32_t playerId)
    {
        // Check if we should start a new combat engagement
        if (ShouldStartCombatEngagement(playerId)) {
            std::vector<uint64_t> nearbyEnemies; // Would be populated by game logic
            StartCombatEngagement(playerId, nearbyEnemies);
        }

        // Broadcast combat event
        CombatEventData eventData;
        eventData.playerId = playerId;
        eventData.eventType = CombatEventType::CombatStarted;
        eventData.timestamp = std::chrono::steady_clock::now();

        auto* playerState = GetPlayerCombatState(playerId);
        if (playerState) {
            eventData.position = playerState->localState.position;
        }

        BroadcastCombatEvent(playerId, eventData);
    }

    void CombatStateManager::OnCombatEnded(uint32_t playerId)
    {
        // Leave any active engagement
        LeaveCombatEngagement(playerId);

        // Broadcast combat event
        CombatEventData eventData;
        eventData.playerId = playerId;
        eventData.eventType = CombatEventType::CombatEnded;
        eventData.timestamp = std::chrono::steady_clock::now();

        auto* playerState = GetPlayerCombatState(playerId);
        if (playerState) {
            eventData.position = playerState->localState.position;
        }

        BroadcastCombatEvent(playerId, eventData);
    }

    uint32_t CombatStateManager::StartCombatEngagement(uint32_t initiatorId, const std::vector<uint64_t>& enemyIds)
    {
        std::unique_lock<std::shared_mutex> lock(m_engagementsMutex);

        uint32_t engagementId = GenerateEngagementId();

        auto engagement = std::make_unique<CombatEngagement>();
        engagement->engagementId = engagementId;
        engagement->participants.push_back(initiatorId);
        engagement->enemyEntities = enemyIds;
        engagement->startTime = std::chrono::steady_clock::now();
        engagement->lastActivity = engagement->startTime;
        engagement->isActive = true;

        // Set center position based on initiator's position
        auto* playerState = GetPlayerCombatState(initiatorId);
        if (playerState) {
            engagement->centerPosition = playerState->localState.position;
            playerState->currentEngagementId = engagementId;
        }

        m_combatEngagements[engagementId] = std::move(engagement);
        m_totalCombatEngagements++;

        lock.unlock();

        // Notify listeners
        NotifyCombatEngagement(engagementId, true);

        return engagementId;
    }

    void CombatStateManager::EndCombatEngagement(uint32_t engagementId)
    {
        std::unique_lock<std::shared_mutex> lock(m_engagementsMutex);

        auto it = m_combatEngagements.find(engagementId);
        if (it == m_combatEngagements.end()) {
            return;
        }

        auto& engagement = it->second;
        engagement->isActive = false;

        // Remove participants from engagement
        for (uint32_t participantId : engagement->participants) {
            auto* playerState = GetPlayerCombatState(participantId);
            if (playerState && playerState->currentEngagementId == engagementId) {
                playerState->currentEngagementId = 0;
            }
        }

        lock.unlock();

        // Notify listeners
        NotifyCombatEngagement(engagementId, false);
    }

    void CombatStateManager::UpdateCombatEngagements(float deltaTime)
    {
        std::unique_lock<std::shared_mutex> lock(m_engagementsMutex);

        auto currentTime = std::chrono::steady_clock::now();
        std::vector<uint32_t> expiredEngagements;

        for (auto& [engagementId, engagement] : m_combatEngagements) {
            if (!engagement->isActive) {
                continue;
            }

            // Check for timeout (5 minutes of inactivity)
            auto timeSinceActivity = std::chrono::duration_cast<std::chrono::minutes>(
                currentTime - engagement->lastActivity).count();

            if (timeSinceActivity >= 5) {
                expiredEngagements.push_back(engagementId);
                continue;
            }

            // Update engagement activity based on participant states
            bool hasActivity = false;
            for (uint32_t participantId : engagement->participants) {
                auto* playerState = GetPlayerCombatState(participantId);
                if (playerState && playerState->localState.combatState == CombatState::ActiveCombat) {
                    hasActivity = true;
                    engagement->lastActivity = currentTime;
                    break;
                }
            }
        }

        // End expired engagements
        for (uint32_t engagementId : expiredEngagements) {
            lock.unlock();
            EndCombatEngagement(engagementId);
            lock.lock();
        }
    }

    bool CombatStateManager::JoinCombatEngagement(uint32_t playerId, uint32_t engagementId)
    {
        std::unique_lock<std::shared_mutex> lock(m_engagementsMutex);

        auto it = m_combatEngagements.find(engagementId);
        if (it == m_combatEngagements.end() || !it->second->isActive) {
            return false;
        }

        auto& engagement = it->second;

        // Check if player is already in this engagement
        auto participantIt = std::find(engagement->participants.begin(),
                                     engagement->participants.end(), playerId);
        if (participantIt != engagement->participants.end()) {
            return false; // Already in engagement
        }

        // Check if player is close enough to the engagement
        auto* playerState = GetPlayerCombatState(playerId);
        if (!playerState) {
            return false;
        }

        float distance = CalculateEngagementDistance(
            playerState->localState.position, engagement->centerPosition);
        if (distance > engagement->engagementRadius) {
            return false; // Too far away
        }

        // Add player to engagement
        engagement->participants.push_back(playerId);
        engagement->lastActivity = std::chrono::steady_clock::now();
        playerState->currentEngagementId = engagementId;

        return true;
    }

    void CombatStateManager::LeaveCombatEngagement(uint32_t playerId)
    {
        auto* playerState = GetPlayerCombatState(playerId);
        if (!playerState || playerState->currentEngagementId == 0) {
            return;
        }

        uint32_t engagementId = playerState->currentEngagementId;

        std::unique_lock<std::shared_mutex> lock(m_engagementsMutex);

        auto it = m_combatEngagements.find(engagementId);
        if (it == m_combatEngagements.end()) {
            return;
        }

        auto& engagement = it->second;

        // Remove player from participants
        auto participantIt = std::find(engagement->participants.begin(),
                                     engagement->participants.end(), playerId);
        if (participantIt != engagement->participants.end()) {
            engagement->participants.erase(participantIt);
        }

        playerState->currentEngagementId = 0;

        // End engagement if no participants left
        if (engagement->participants.empty()) {
            lock.unlock();
            EndCombatEngagement(engagementId);
        }
    }

    PlayerCombatState* CombatStateManager::GetPlayerCombatState(uint32_t playerId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        return (it != m_playerStates.end()) ? it->second.get() : nullptr;
    }

    const PlayerCombatState* CombatStateManager::GetPlayerCombatState(uint32_t playerId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        return (it != m_playerStates.end()) ? it->second.get() : nullptr;
    }

    CombatEngagement* CombatStateManager::GetCombatEngagement(uint32_t engagementId)
    {
        std::shared_lock<std::shared_mutex> lock(m_engagementsMutex);

        auto it = m_combatEngagements.find(engagementId);
        return (it != m_combatEngagements.end()) ? it->second.get() : nullptr;
    }

    std::vector<uint32_t> CombatStateManager::GetPlayersInCombat() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        std::vector<uint32_t> combatPlayers;
        for (const auto& [playerId, playerState] : m_playerStates) {
            if (playerState->localState.combatState == CombatState::InCombat ||
                playerState->localState.combatState == CombatState::ActiveCombat) {
                combatPlayers.push_back(playerId);
            }
        }

        return combatPlayers;
    }

    std::vector<uint32_t> CombatStateManager::GetPlayersInEngagement(uint32_t engagementId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_engagementsMutex);

        auto it = m_combatEngagements.find(engagementId);
        if (it != m_combatEngagements.end()) {
            return it->second->participants;
        }

        return {};
    }

    std::vector<uint32_t> CombatStateManager::GetNearbyPlayers(uint32_t playerId, float radius) const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        std::vector<uint32_t> nearbyPlayers;

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return nearbyPlayers;
        }

        const CombatVector3& playerPos = it->second->localState.position;

        for (const auto& [otherPlayerId, otherPlayerState] : m_playerStates) {
            if (otherPlayerId == playerId) {
                continue;
            }

            float distance = CombatUtils::CalculateDistance(playerPos, otherPlayerState->localState.position);
            if (distance <= radius) {
                nearbyPlayers.push_back(otherPlayerId);
            }
        }

        return nearbyPlayers;
    }

    bool CombatStateManager::ValidateCombatData(uint32_t playerId, const CombatSyncData& data) const
    {
        // Validate position
        if (!CombatUtils::IsValidPosition(data.position)) {
            return false;
        }

        // Validate aim direction
        if (!CombatUtils::IsValidDirection(data.aimDirection)) {
            return false;
        }

        // Validate state transitions (basic checks)
        auto* playerState = GetPlayerCombatState(playerId);
        if (playerState) {
            CombatState currentState = playerState->localState.combatState;

            // Prevent impossible state transitions
            if (currentState == CombatState::OutOfCombat && data.combatState == CombatState::ActiveCombat) {
                // Must go through InCombat first
                return false;
            }
        }

        return true;
    }

    bool CombatStateManager::ValidateFireData(uint32_t playerId, const WeaponFireData& data) const
    {
        // Validate basic data
        if (data.shotsFired == 0 || data.shotsFired > 100) { // Max burst of 100
            return false;
        }

        if (data.damage < 0.0f || data.damage > 10000.0f) { // Max reasonable damage
            return false;
        }

        // Validate position and direction
        if (!CombatUtils::IsValidPosition(data.firePosition) ||
            !CombatUtils::IsValidDirection(data.aimDirection)) {
            return false;
        }

        return true;
    }

    void CombatStateManager::BroadcastCombatUpdate(uint32_t playerId, const CombatSyncData& data)
    {
        SendCombatUpdateToClients(playerId, data);
    }

    void CombatStateManager::BroadcastWeaponFire(uint32_t playerId, const WeaponFireData& fireData)
    {
        SendWeaponFireToClients(playerId, fireData);
    }

    void CombatStateManager::BroadcastDamageDealt(uint32_t attackerId, const DamageDealtData& damageData)
    {
        SendDamageUpdateToClients(attackerId, damageData);
    }

    void CombatStateManager::BroadcastCombatEvent(uint32_t playerId, const CombatEventData& eventData)
    {
        SendCombatEventToClients(playerId, eventData);
    }

    // Private implementation methods
    uint32_t CombatStateManager::GenerateEngagementId()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint32_t> dis(1, UINT32_MAX);
        return dis(gen);
    }

    void CombatStateManager::UpdatePlayerCombatStates(float deltaTime)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [playerId, playerState] : m_playerStates) {
            // Check for player timeout (2 minutes of inactivity)
            auto timeSinceActivity = std::chrono::duration_cast<std::chrono::minutes>(
                currentTime - playerState->lastActivity).count();

            if (timeSinceActivity >= 2) {
                playerState->isConnected = false;
            }

            // Update sync priority based on combat activity
            if (playerState->localState.combatState == CombatState::ActiveCombat) {
                playerState->syncPriority = 3.0f; // High priority for active combat
            } else if (playerState->localState.combatState == CombatState::InCombat) {
                playerState->syncPriority = 2.0f; // Medium priority for combat ready
            } else {
                playerState->syncPriority = 1.0f; // Normal priority
            }

            // Update weapon reload progress
            for (auto& [weaponId, weaponState] : playerState->weapons) {
                if (weaponState->isReloading) {
                    weaponState->reloadProgress += deltaTime / 3.0f; // Assume 3 second reload
                    if (weaponState->reloadProgress >= 1.0f) {
                        weaponState->isReloading = false;
                        weaponState->reloadProgress = 0.0f;
                        weaponState->ammoCount = weaponState->maxAmmo; // Full reload
                    }
                }
            }
        }
    }

    void CombatStateManager::ProcessRecentEvents(float deltaTime)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [playerId, playerState] : m_playerStates) {
            // Clean up old events (older than 1 minute)
            auto& events = playerState->recentEvents;
            events.erase(
                std::remove_if(events.begin(), events.end(),
                    [currentTime](const CombatEventData& event) {
                        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                            currentTime - event.timestamp).count();
                        return elapsed > 60;
                    }),
                events.end());

            // Clean up old damage entries (older than 30 seconds)
            auto& damage = playerState->recentDamage;
            damage.erase(
                std::remove_if(damage.begin(), damage.end(),
                    [currentTime](const DamageDealtData& dmg) {
                        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                            currentTime - dmg.timestamp).count();
                        return elapsed > 30;
                    }),
                damage.end());

            // Clean up old shots (older than 10 seconds)
            auto& shots = playerState->recentShots;
            shots.erase(
                std::remove_if(shots.begin(), shots.end(),
                    [currentTime](const WeaponFireData& shot) {
                        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                            currentTime - shot.timestamp).count();
                        return elapsed > 10;
                    }),
                shots.end());
        }
    }

    void CombatStateManager::CleanupExpiredData()
    {
        ProcessRecentEvents(0.0f); // Force cleanup of old events
    }

    void CombatStateManager::ValidateCombatStates()
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        for (auto& [playerId, playerState] : m_playerStates) {
            // Detect and correct invalid states
            DetectCombatAnomalies(playerId);
        }
    }

    bool CombatStateManager::ShouldStartCombatEngagement(uint32_t playerId) const
    {
        auto* playerState = GetPlayerCombatState(playerId);
        if (!playerState) {
            return false;
        }

        // Start engagement if player entered combat and not already in one
        return playerState->localState.combatState == CombatState::InCombat &&
               playerState->currentEngagementId == 0;
    }

    float CombatStateManager::CalculateEngagementDistance(const CombatVector3& pos1, const CombatVector3& pos2) const
    {
        return CombatUtils::CalculateDistance(pos1, pos2);
    }

    void CombatStateManager::UpdateStateToPlayersMapping(uint32_t playerId, CombatState combatState, bool isActive)
    {
        auto& playerList = m_stateToPlayers[combatState];

        if (isActive) {
            if (std::find(playerList.begin(), playerList.end(), playerId) == playerList.end()) {
                playerList.push_back(playerId);
            }
        } else {
            playerList.erase(std::remove(playerList.begin(), playerList.end(), playerId), playerList.end());
        }
    }

    void CombatStateManager::RemovePlayerFromAllMappings(uint32_t playerId)
    {
        // Remove from state mappings
        for (auto& [state, playerList] : m_stateToPlayers) {
            playerList.erase(std::remove(playerList.begin(), playerList.end(), playerId), playerList.end());
        }
    }

    bool CombatStateManager::IsFireRateLimited(uint32_t playerId, uint64_t weaponId) const
    {
        auto* playerState = GetPlayerCombatState(playerId);
        if (!playerState) {
            return false;
        }

        float currentFireRate = CalculateFireRate(playerId, weaponId);

        // Different weapons have different max fire rates (rounds per second)
        float maxFireRate = 20.0f; // Default max 20 rounds per second

        return currentFireRate > maxFireRate;
    }

    float CombatStateManager::CalculateFireRate(uint32_t playerId, uint64_t weaponId) const
    {
        auto* playerState = GetPlayerCombatState(playerId);
        if (!playerState) {
            return 0.0f;
        }

        // Count shots in the last second for this weapon
        auto currentTime = std::chrono::steady_clock::now();
        uint32_t recentShots = 0;

        for (const auto& shot : playerState->recentShots) {
            if (shot.weaponId == weaponId) {
                auto timeSinceShot = std::chrono::duration_cast<std::chrono::milliseconds>(
                    currentTime - shot.timestamp).count() / 1000.0f;

                if (timeSinceShot < 1.0f) { // Within last second
                    recentShots += shot.shotsFired;
                }
            }
        }

        return static_cast<float>(recentShots);
    }

    bool CombatStateManager::IsDamageValid(const DamageDealtData& damageData) const
    {
        // Check damage bounds
        if (damageData.damage <= 0.0f || damageData.damage > 10000.0f) {
            return false;
        }

        // Validate position
        if (!CombatUtils::IsValidPosition(damageData.position)) {
            return false;
        }

        return true;
    }

    void CombatStateManager::DetectCombatAnomalies(uint32_t playerId)
    {
        auto* playerState = GetPlayerCombatState(playerId);
        if (!playerState) {
            return;
        }

        auto currentTime = std::chrono::steady_clock::now();

        // Check for excessive damage dealing
        float recentDamage = 0.0f;
        for (const auto& dmg : playerState->recentDamage) {
            auto timeSinceDamage = std::chrono::duration_cast<std::chrono::seconds>(
                currentTime - dmg.timestamp).count();

            if (timeSinceDamage < 5) { // Last 5 seconds
                recentDamage += dmg.damage;
            }
        }

        if (recentDamage > 5000.0f) { // More than 5000 damage in 5 seconds is suspicious
            // Log anomaly or take action
        }

        // Check for excessive shot frequency
        uint32_t recentShots = 0;
        for (const auto& shot : playerState->recentShots) {
            auto timeSinceShot = std::chrono::duration_cast<std::chrono::seconds>(
                currentTime - shot.timestamp).count();

            if (timeSinceShot < 5) { // Last 5 seconds
                recentShots += shot.shotsFired;
            }
        }

        if (recentShots > 100) { // More than 100 shots in 5 seconds is suspicious
            // Log anomaly or take action
        }
    }

    // Notification methods
    void CombatStateManager::NotifyCombatStateChanged(uint32_t playerId, CombatState oldState, CombatState newState)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_combatStateChangedCallback) {
            m_combatStateChangedCallback(playerId, oldState, newState);
        }
    }

    void CombatStateManager::NotifyWeaponFired(uint32_t playerId, const WeaponFireData& fireData)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_weaponFiredCallback) {
            m_weaponFiredCallback(playerId, fireData);
        }
    }

    void CombatStateManager::NotifyDamageDealt(uint32_t attackerId, const DamageDealtData& damageData)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_damageDealtCallback) {
            m_damageDealtCallback(attackerId, damageData);
        }
    }

    void CombatStateManager::NotifyPlayerKilled(uint32_t killerId, const PlayerKillData& killData)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_playerKilledCallback) {
            m_playerKilledCallback(killerId, killData);
        }
    }

    void CombatStateManager::NotifyCombatEngagement(uint32_t engagementId, bool started)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_combatEngagementCallback) {
            m_combatEngagementCallback(engagementId, started);
        }
    }

    void CombatStateManager::SendCombatUpdateToClients(uint32_t playerId, const CombatSyncData& data)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void CombatStateManager::SendWeaponFireToClients(uint32_t playerId, const WeaponFireData& fireData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void CombatStateManager::SendDamageUpdateToClients(uint32_t attackerId, const DamageDealtData& damageData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void CombatStateManager::SendCombatEventToClients(uint32_t playerId, const CombatEventData& eventData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    // Additional method implementations needed
    void CombatStateManager::UpdatePlayerActivity(uint32_t playerId)
    {
        auto* playerState = GetPlayerCombatState(playerId);
        if (playerState) {
            playerState->lastActivity = std::chrono::steady_clock::now();
            playerState->isConnected = true;
        }
    }

    void CombatStateManager::SynchronizeCombatState(uint32_t playerId)
    {
        auto* playerState = GetPlayerCombatState(playerId);
        if (!playerState) {
            return;
        }

        // Create sync data from current state
        CombatSyncData syncData;
        syncData.playerId = playerId;
        syncData.combatState = playerState->localState.combatState;
        syncData.stance = playerState->localState.combatStance;
        syncData.coverState = playerState->localState.coverState;
        syncData.aimingState = playerState->localState.aimingState;
        syncData.movementMode = playerState->localState.movementMode;
        syncData.alertLevel = playerState->localState.alertLevel;
        syncData.currentWeapon = playerState->localState.currentWeapon;
        syncData.weaponDrawn = playerState->localState.weaponDrawn;
        syncData.isReloading = playerState->localState.isReloading;
        syncData.isFiring = playerState->localState.isFiring;
        syncData.currentTarget = playerState->localState.currentTarget;
        syncData.position = playerState->localState.position;
        syncData.aimDirection = playerState->localState.aimDirection;
        syncData.timestamp = std::chrono::steady_clock::now();

        BroadcastCombatUpdate(playerId, syncData);
    }

    void CombatStateManager::ForceCombatSync()
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        for (const auto& [playerId, playerState] : m_playerStates) {
            lock.unlock();
            SynchronizeCombatState(playerId);
            lock.lock();
        }
    }

    bool CombatStateManager::UpdateWeaponState(uint32_t playerId, const WeaponSyncData& weaponData)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return false;
        }

        auto& playerState = it->second;

        // Validate weapon data
        if (!ValidateWeaponData(playerId, weaponData)) {
            return false;
        }

        // Update or create weapon state
        auto weaponIt = playerState->weapons.find(weaponData.weaponId);
        if (weaponIt == playerState->weapons.end()) {
            auto weaponState = std::make_unique<WeaponState>();
            weaponState->weaponId = weaponData.weaponId;
            weaponState->weaponType = weaponData.weaponType;
            weaponState->isDrawn = weaponData.isDrawn;
            weaponState->isReloading = weaponData.isReloading;
            weaponState->ammoCount = weaponData.ammoCount;
            weaponState->maxAmmo = weaponData.maxAmmo;
            weaponState->reloadProgress = weaponData.reloadProgress;
            playerState->weapons[weaponData.weaponId] = std::move(weaponState);
        } else {
            weaponIt->second->weaponType = weaponData.weaponType;
            weaponIt->second->isDrawn = weaponData.isDrawn;
            weaponIt->second->isReloading = weaponData.isReloading;
            weaponIt->second->ammoCount = weaponData.ammoCount;
            weaponIt->second->maxAmmo = weaponData.maxAmmo;
            weaponIt->second->reloadProgress = weaponData.reloadProgress;
        }

        playerState->lastWeaponUpdate = std::chrono::steady_clock::now();
        playerState->lastActivity = playerState->lastWeaponUpdate;

        lock.unlock();

        // Broadcast weapon update
        BroadcastWeaponUpdate(playerId, weaponData);

        return true;
    }

    void CombatStateManager::SynchronizeWeaponState(uint32_t playerId)
    {
        auto* playerState = GetPlayerCombatState(playerId);
        if (!playerState) {
            return;
        }

        // Sync all weapons for this player
        for (const auto& [weaponId, weaponState] : playerState->weapons) {
            WeaponSyncData syncData;
            syncData.playerId = playerId;
            syncData.weaponId = weaponId;
            syncData.weaponType = weaponState->weaponType;
            syncData.isDrawn = weaponState->isDrawn;
            syncData.isReloading = weaponState->isReloading;
            syncData.ammoCount = weaponState->ammoCount;
            syncData.maxAmmo = weaponState->maxAmmo;
            syncData.reloadProgress = weaponState->reloadProgress;
            syncData.timestamp = std::chrono::steady_clock::now();

            BroadcastWeaponUpdate(playerId, syncData);
        }
    }

    bool CombatStateManager::UpdateTargeting(uint32_t playerId, const TargetingSyncData& targetingData)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return false;
        }

        auto& playerState = it->second;

        // Update local state targeting info
        playerState->localState.currentTarget = targetingData.targetId;
        playerState->localState.aimDirection = targetingData.aimDirection;
        playerState->localState.aimingState = targetingData.isAiming ? AimingState::HipAiming : AimingState::NotAiming;

        playerState->lastActivity = std::chrono::steady_clock::now();

        return true;
    }

    void CombatStateManager::SynchronizeTargeting(uint32_t playerId)
    {
        auto* playerState = GetPlayerCombatState(playerId);
        if (!playerState) {
            return;
        }

        // Create targeting sync data
        TargetingSyncData syncData;
        syncData.playerId = playerId;
        syncData.targetId = playerState->localState.currentTarget;
        syncData.aimDirection = playerState->localState.aimDirection;
        syncData.isAiming = (playerState->localState.aimingState != AimingState::NotAiming);
        syncData.aimAccuracy = 1.0f; // Would be calculated based on player skill
        syncData.timestamp = std::chrono::steady_clock::now();

        // Would broadcast targeting update
        // BroadcastTargetingUpdate(playerId, syncData);
    }

    void CombatStateManager::ProcessCombatEvent(uint32_t playerId, const CombatEventData& eventData)
    {
        auto* playerState = GetPlayerCombatState(playerId);
        if (!playerState) {
            return;
        }

        // Add to recent events
        playerState->recentEvents.push_back(eventData);
        if (playerState->recentEvents.size() > 20) { // Keep last 20 events
            playerState->recentEvents.erase(playerState->recentEvents.begin());
        }

        playerState->lastActivity = std::chrono::steady_clock::now();

        // Broadcast event
        BroadcastCombatEvent(playerId, eventData);
    }

    bool CombatStateManager::ValidateWeaponData(uint32_t playerId, const WeaponSyncData& data) const
    {
        // Validate ammo counts
        if (data.ammoCount > data.maxAmmo) {
            return false;
        }

        // Validate reload progress
        if (data.reloadProgress < 0.0f || data.reloadProgress > 1.0f) {
            return false;
        }

        return true;
    }

    void CombatStateManager::BroadcastWeaponUpdate(uint32_t playerId, const WeaponSyncData& data)
    {
        SendWeaponUpdateToClients(playerId, data);
    }

    void CombatStateManager::SendWeaponUpdateToClients(uint32_t playerId, const WeaponSyncData& data)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void CombatStateManager::ForceSyncPlayer(uint32_t playerId)
    {
        SynchronizeCombatState(playerId);
        SynchronizeWeaponState(playerId);
        SynchronizeTargeting(playerId);
    }

    void CombatStateManager::SetSyncPriority(uint32_t playerId, float priority)
    {
        auto* playerState = GetPlayerCombatState(playerId);
        if (playerState) {
            playerState->syncPriority = priority;
        }
    }

    uint32_t CombatStateManager::GetActivePlayerCount() const
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

    uint32_t CombatStateManager::GetActiveCombatEngagements() const
    {
        std::shared_lock<std::shared_mutex> lock(m_engagementsMutex);

        uint32_t count = 0;
        for (const auto& [engagementId, engagement] : m_combatEngagements) {
            if (engagement->isActive) {
                count++;
            }
        }

        return count;
    }

    uint32_t CombatStateManager::GetTotalShotsFired() const
    {
        return m_totalShotsFired;
    }

    float CombatStateManager::GetTotalDamageDealt() const
    {
        return m_totalDamageDealt;
    }

    std::unordered_map<uint32_t, float> CombatStateManager::GetPlayerDamageStats() const
    {
        std::unordered_map<uint32_t, float> stats;
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        for (const auto& [playerId, playerState] : m_playerStates) {
            stats[playerId] = playerState->totalDamageDealt;
        }

        return stats;
    }

    std::unordered_map<uint32_t, uint32_t> CombatStateManager::GetPlayerKillStats() const
    {
        std::unordered_map<uint32_t, uint32_t> stats;
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        for (const auto& [playerId, playerState] : m_playerStates) {
            stats[playerId] = playerState->totalKills;
        }

        return stats;
    }

    // Callback setters
    void CombatStateManager::SetCombatStateChangedCallback(CombatStateChangedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_combatStateChangedCallback = callback;
    }

    void CombatStateManager::SetWeaponFiredCallback(WeaponFiredCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_weaponFiredCallback = callback;
    }

    void CombatStateManager::SetDamageDealtCallback(DamageDealtCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_damageDealtCallback = callback;
    }

    void CombatStateManager::SetPlayerKilledCallback(PlayerKilledCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_playerKilledCallback = callback;
    }

    void CombatStateManager::SetCombatEngagementCallback(CombatEngagementCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_combatEngagementCallback = callback;
    }

    // Utility functions implementation
    namespace CombatUtils
    {
        std::string CombatStateToString(CombatState state)
        {
            switch (state) {
                case CombatState::OutOfCombat: return "OutOfCombat";
                case CombatState::CombatReady: return "CombatReady";
                case CombatState::InCombat: return "InCombat";
                case CombatState::ActiveCombat: return "ActiveCombat";
                case CombatState::PostCombat: return "PostCombat";
                default: return "Unknown";
            }
        }

        std::string CombatStanceToString(CombatStance stance)
        {
            switch (stance) {
                case CombatStance::Standing: return "Standing";
                case CombatStance::Crouching: return "Crouching";
                case CombatStance::InCover: return "InCover";
                case CombatStance::Prone: return "Prone";
                case CombatStance::Moving: return "Moving";
                default: return "Unknown";
            }
        }

        bool IsOffensiveCombatState(CombatState state)
        {
            return state == CombatState::ActiveCombat;
        }

        bool IsDefensiveCombatStance(CombatStance stance)
        {
            return stance == CombatStance::InCover || stance == CombatStance::Prone;
        }

        bool IsHighAlertLevel(AlertLevel level)
        {
            return level == AlertLevel::Combat || level == AlertLevel::Panicked;
        }

        float CalculateDistance(const CombatVector3& pos1, const CombatVector3& pos2)
        {
            float dx = pos1.x - pos2.x;
            float dy = pos1.y - pos2.y;
            float dz = pos1.z - pos2.z;
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        }

        CombatVector3 CalculateDirection(const CombatVector3& from, const CombatVector3& to)
        {
            CombatVector3 dir;
            dir.x = to.x - from.x;
            dir.y = to.y - from.y;
            dir.z = to.z - from.z;

            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            if (length > 0.0f) {
                dir.x /= length;
                dir.y /= length;
                dir.z /= length;
            }

            return dir;
        }

        float CalculateDamageMultiplier(DamageType damageType, bool isCritical, bool isHeadshot)
        {
            float multiplier = 1.0f;

            if (isCritical) {
                multiplier *= 1.5f; // 50% bonus for critical hits
            }

            if (isHeadshot) {
                multiplier *= 2.0f; // 100% bonus for headshots
            }

            // Different damage types might have different base multipliers
            switch (damageType) {
                case DamageType::Explosive:
                    multiplier *= 1.2f;
                    break;
                case DamageType::Electrical:
                    multiplier *= 0.9f;
                    break;
                default:
                    break;
            }

            return multiplier;
        }

        bool IsValidPosition(const CombatVector3& position)
        {
            // Check for reasonable position bounds
            const float MAX_COORD = 100000.0f;
            return std::abs(position.x) < MAX_COORD &&
                   std::abs(position.y) < MAX_COORD &&
                   std::abs(position.z) < MAX_COORD;
        }

        bool IsValidDirection(const CombatVector3& direction)
        {
            // Check if direction vector has reasonable magnitude
            float length = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
            return length >= 0.1f && length <= 2.0f; // Allow some tolerance
        }

        bool ShouldSyncCombatState(const CombatSyncData& oldData, const CombatSyncData& newData)
        {
            // Sync if combat state changed
            if (oldData.combatState != newData.combatState) {
                return true;
            }

            // Sync if stance changed
            if (oldData.stance != newData.stance) {
                return true;
            }

            // Sync if weapon state changed
            if (oldData.weaponDrawn != newData.weaponDrawn ||
                oldData.isFiring != newData.isFiring ||
                oldData.isReloading != newData.isReloading) {
                return true;
            }

            // Sync if position changed significantly (more than 1 meter)
            float positionDiff = CalculateDistance(oldData.position, newData.position);
            if (positionDiff > 1.0f) {
                return true;
            }

            return false;
        }

        uint32_t HashCombatState(const PlayerCombatState& state)
        {
            // Simple hash combining multiple state values
            uint32_t hash = 0;
            hash ^= static_cast<uint32_t>(state.localState.combatState) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= static_cast<uint32_t>(state.localState.combatStance) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= static_cast<uint32_t>(state.localState.currentWeapon) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            return hash;
        }

        bool AreCombatStatesEquivalent(const CombatSyncData& data1, const CombatSyncData& data2, float tolerance)
        {
            if (data1.combatState != data2.combatState ||
                data1.stance != data2.stance ||
                data1.weaponDrawn != data2.weaponDrawn) {
                return false;
            }

            float positionDiff = CalculateDistance(data1.position, data2.position);
            return positionDiff <= tolerance;
        }
    }
}
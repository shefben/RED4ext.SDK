#include "CyberwareManager.hpp"
#include "RED4ext/Api/Sdk.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace RED4ext
{
    // CyberwareManager Implementation
    CyberwareManager& CyberwareManager::GetInstance()
    {
        static CyberwareManager instance;
        return instance;
    }

    void CyberwareManager::Initialize()
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Clear existing data
        m_playerStates.clear();
        m_abilityToPlayers.clear();
        m_playerMalfunctions.clear();

        // Initialize timing
        m_lastUpdate = std::chrono::steady_clock::now();
        m_lastCleanup = m_lastUpdate;
        m_updateInterval = 0.3f; // ~3 FPS cyberware updates

        // Initialize statistics
        m_totalCyberwareInstalled = 0;
        m_totalAbilitiesUsed = 0;
        m_totalMalfunctions = 0;
        m_totalSlowMotionActivations = 0;

        // Initialize ability to players mapping for all abilities
        for (int i = 0; i <= static_cast<int>(CyberwareAbility::Thermal_Damage_Protection); ++i) {
            CyberwareAbility ability = static_cast<CyberwareAbility>(i);
            m_abilityToPlayers[ability] = {};
        }
    }

    void CyberwareManager::Shutdown()
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        m_playerStates.clear();
        m_abilityToPlayers.clear();
        m_playerMalfunctions.clear();

        // Clear callbacks
        std::lock_guard<std::mutex> callbacksLock(m_callbacksMutex);
        m_cyberwareInstalledCallback = nullptr;
        m_cyberwareRemovedCallback = nullptr;
        m_abilityActivatedCallback = nullptr;
        m_malfunctionTriggeredCallback = nullptr;
        m_slowMotionActivatedCallback = nullptr;
    }

    void CyberwareManager::Update()
    {
        auto currentTime = std::chrono::steady_clock::now();
        auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - m_lastUpdate).count() / 1000.0f;

        m_lastUpdate = currentTime;

        // Update player cyberware states
        UpdatePlayerCyberware(deltaTime);

        // Update cooldowns
        UpdateCooldowns(deltaTime);

        // Update ability cooldowns
        UpdateAbilityCooldowns(deltaTime);

        // Update slow motion effects
        UpdateSlowMotionEffects(deltaTime);

        // Update malfunctions
        UpdateMalfunctions(deltaTime);

        // Process ability expirations
        ProcessAbilityExpirations();

        // Process slow motion expirations
        ProcessSlowMotionExpirations();

        // Validate cyberware states
        ValidateCyberwareStates();

        // Periodic cleanup (every 2 minutes)
        auto timeSinceCleanup = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - m_lastCleanup).count();

        if (timeSinceCleanup >= 120) {
            CleanupExpiredData();
            m_lastCleanup = currentTime;
        }
    }

    void CyberwareManager::AddPlayer(uint32_t playerId, const std::string& playerName)
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        auto playerState = std::make_unique<PlayerCyberwareState>();
        playerState->playerId = playerId;
        playerState->playerName = playerName;
        playerState->isConnected = true;
        playerState->syncPriority = 1.0f;
        playerState->totalCyberwareCount = 0;

        m_playerStates[playerId] = std::move(playerState);
    }

    void CyberwareManager::RemovePlayer(uint32_t playerId)
    {
        std::unique_lock<std::shared_mutex> lock(m_statesMutex);

        // Remove from all mappings
        RemovePlayerFromAllMappings(playerId);

        // Remove player state
        m_playerStates.erase(playerId);
        m_playerMalfunctions.erase(playerId);
    }

    bool CyberwareManager::InstallCyberware(uint32_t playerId, uint32_t cyberwareId, CyberwareSlot slot, CyberwareAbility primaryAbility)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return false;
        }

        auto& playerState = it->second;

        // Check if cyberware already installed
        if (playerState->installedCyberware.find(cyberwareId) != playerState->installedCyberware.end()) {
            return false; // Already installed
        }

        // Validate compatibility
        if (!IsCyberwareCompatible(slot, primaryAbility)) {
            return false;
        }

        // Create new cyberware entry
        auto cyberware = std::make_unique<ActiveCyberware>();
        cyberware->cyberwareId = cyberwareId;
        cyberware->slot = slot;
        cyberware->state = CyberwareState::Operational;
        cyberware->primaryAbility = primaryAbility;
        cyberware->healthPercentage = 1.0f;
        cyberware->batteryLevel = 1.0f;
        cyberware->isActive = false;
        cyberware->isOnCooldown = false;
        cyberware->cooldownRemaining = 0.0f;
        cyberware->isMalfunctioning = false;
        cyberware->installTime = std::chrono::steady_clock::now();
        cyberware->lastUpdate = cyberware->installTime;

        playerState->installedCyberware[cyberwareId] = std::move(cyberware);
        playerState->totalCyberwareCount++;
        playerState->cyberwareInstalled++;
        playerState->lastCyberwareUpdate = std::chrono::steady_clock::now();
        playerState->lastActivity = playerState->lastCyberwareUpdate;

        m_totalCyberwareInstalled++;

        lock.unlock();

        // Notify listeners
        NotifyCyberwareInstalled(playerId, cyberwareId, slot);

        // Broadcast installation
        CyberwareInstallUpdate installUpdate;
        installUpdate.playerId = playerId;
        installUpdate.cyberwareId = cyberwareId;
        installUpdate.slot = slot;
        installUpdate.primaryAbility = primaryAbility;
        installUpdate.isInstallation = true;
        installUpdate.updateTime = std::chrono::steady_clock::now();

        // Would send to network clients
        // SendCyberwareInstallToClients(installUpdate);

        return true;
    }

    bool CyberwareManager::RemoveCyberware(uint32_t playerId, uint32_t cyberwareId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return false;
        }

        auto& playerState = it->second;

        auto cyberwareIt = playerState->installedCyberware.find(cyberwareId);
        if (cyberwareIt == playerState->installedCyberware.end()) {
            return false; // Not installed
        }

        CyberwareSlot slot = cyberwareIt->second->slot;
        CyberwareAbility ability = cyberwareIt->second->primaryAbility;

        // Remove cyberware
        playerState->installedCyberware.erase(cyberwareIt);
        playerState->totalCyberwareCount--;
        playerState->cyberwareRemoved++;
        playerState->lastCyberwareUpdate = std::chrono::steady_clock::now();

        // Remove from malfunction mapping if present
        UpdateMalfunctionMapping(playerId, cyberwareId, false);

        lock.unlock();

        // Notify listeners
        NotifyCyberwareRemoved(playerId, cyberwareId);

        // Broadcast removal
        CyberwareInstallUpdate installUpdate;
        installUpdate.playerId = playerId;
        installUpdate.cyberwareId = cyberwareId;
        installUpdate.slot = slot;
        installUpdate.primaryAbility = ability;
        installUpdate.isInstallation = false;
        installUpdate.updateTime = std::chrono::steady_clock::now();

        // Would send to network clients
        // SendCyberwareInstallToClients(installUpdate);

        return true;
    }

    bool CyberwareManager::UpdateCyberwareState(uint32_t playerId, const CyberwareSyncData& cyberwareData)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return false;
        }

        auto& playerState = it->second;

        // Validate data
        if (!ValidateCyberwareData(playerId, cyberwareData)) {
            return false;
        }

        auto cyberwareIt = playerState->installedCyberware.find(cyberwareData.cyberwareId);
        if (cyberwareIt == playerState->installedCyberware.end()) {
            return false; // Cyberware not installed
        }

        auto& cyberware = cyberwareIt->second;

        // Update cyberware state
        cyberware->state = cyberwareData.currentState;
        cyberware->healthPercentage = cyberwareData.healthPercentage;
        cyberware->batteryLevel = cyberwareData.batteryLevel;
        cyberware->isActive = cyberwareData.isActive;
        cyberware->isOnCooldown = cyberwareData.isOnCooldown;
        cyberware->cooldownRemaining = cyberwareData.cooldownRemaining;
        cyberware->isMalfunctioning = cyberwareData.isMalfunctioning;
        cyberware->lastUpdate = std::chrono::steady_clock::now();

        playerState->lastCyberwareUpdate = cyberware->lastUpdate;
        playerState->lastActivity = cyberware->lastUpdate;

        lock.unlock();

        // Broadcast update
        BroadcastCyberwareUpdate(playerId, cyberwareData);

        return true;
    }

    bool CyberwareManager::ActivateCyberwareAbility(uint32_t playerId, const CyberwareAbilityData& abilityData)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        if (it == m_playerStates.end()) {
            return false;
        }

        auto& playerState = it->second;

        // Validate ability usage
        if (!ValidateAbilityUsage(playerId, abilityData)) {
            return false;
        }

        // Check rate limiting
        if (IsAbilityUsageRateLimited(playerId, abilityData.abilityType)) {
            return false;
        }

        // Find cyberware with this ability
        ActiveCyberware* targetCyberware = nullptr;
        for (auto& [cyberwareId, cyberware] : playerState->installedCyberware) {
            if (cyberware->primaryAbility == abilityData.abilityType) {
                targetCyberware = cyberware.get();
                break;
            }
        }

        if (!targetCyberware) {
            return false; // No cyberware with this ability
        }

        // Check if cyberware is operational
        if (targetCyberware->state == CyberwareState::Damaged ||
            targetCyberware->state == CyberwareState::Offline ||
            targetCyberware->isMalfunctioning) {
            return false;
        }

        // Check cooldown
        if (targetCyberware->isOnCooldown) {
            return false;
        }

        // Activate ability
        targetCyberware->isActive = true;

        // Add to recent abilities
        playerState->recentAbilities.push_back(abilityData);
        if (playerState->recentAbilities.size() > 10) {
            playerState->recentAbilities.erase(playerState->recentAbilities.begin());
        }

        // Start cooldown
        float cooldownDuration = GetAbilityCooldownDuration(abilityData.abilityType);
        StartCyberwareCooldown(playerId, targetCyberware->cyberwareId, cooldownDuration);

        // Update mapping
        UpdateAbilityToPlayersMapping(playerId, abilityData.abilityType, true);

        playerState->abilitiesUsed++;
        playerState->lastAbilityUpdate = std::chrono::steady_clock::now();
        playerState->lastActivity = playerState->lastAbilityUpdate;

        m_totalAbilitiesUsed++;

        lock.unlock();

        // Handle special abilities
        if (abilityData.abilityType == CyberwareAbility::Sandevistan ||
            abilityData.abilityType == CyberwareAbility::Kerenzikov) {
            // Activate slow motion
            ActivateSlowMotion(playerId, 0.3f, abilityData.duration); // 30% slow motion
        }

        // Notify listeners
        NotifyAbilityActivated(playerId, abilityData);

        // Broadcast ability activation
        BroadcastAbilityActivation(playerId, abilityData);

        return true;
    }

    bool CyberwareManager::ActivateSlowMotion(uint32_t playerId, float factor, float duration)
    {
        auto* playerState = GetPlayerCyberwareState(playerId);
        if (!playerState) {
            return false;
        }

        // Check if already in slow motion
        if (playerState->activeSlowMotion.isActive) {
            return false;
        }

        // Activate slow motion
        playerState->activeSlowMotion.playerId = playerId;
        playerState->activeSlowMotion.factor = factor;
        playerState->activeSlowMotion.duration = duration;
        playerState->activeSlowMotion.remainingTime = duration;
        playerState->activeSlowMotion.isActive = true;
        playerState->activeSlowMotion.startTime = std::chrono::steady_clock::now();
        playerState->activeSlowMotion.timestamp = playerState->activeSlowMotion.startTime;

        m_totalSlowMotionActivations++;

        // Notify listeners
        NotifySlowMotionActivated(playerId, playerState->activeSlowMotion);

        // Broadcast slow motion activation
        BroadcastSlowMotionEffect(playerId, playerState->activeSlowMotion);

        return true;
    }

    void CyberwareManager::StartCyberwareCooldown(uint32_t playerId, uint32_t cyberwareId, float duration)
    {
        auto* cyberware = GetPlayerCyberware(playerId, cyberwareId);
        if (cyberware) {
            cyberware->isOnCooldown = true;
            cyberware->cooldownRemaining = duration;

            // Broadcast cooldown start
            CyberwareCooldownData cooldownData;
            cooldownData.playerId = playerId;
            cooldownData.cyberwareId = cyberwareId;
            cooldownData.isOnCooldown = true;
            cooldownData.cooldownDuration = duration;
            cooldownData.remainingTime = duration;
            cooldownData.timestamp = std::chrono::steady_clock::now();

            // Would send to network clients
            // SendCooldownUpdateToClients(cooldownData);
        }
    }

    void CyberwareManager::TriggerCyberwareMalfunction(uint32_t playerId, uint32_t cyberwareId, MalfunctionType type, MalfunctionSeverity severity)
    {
        auto* cyberware = GetPlayerCyberware(playerId, cyberwareId);
        if (!cyberware) {
            return;
        }

        // Set malfunction state
        cyberware->isMalfunctioning = true;
        cyberware->malfunctionType = type;
        cyberware->malfunctionSeverity = severity;
        cyberware->state = CyberwareState::Malfunctioning;

        // Update malfunction mapping
        UpdateMalfunctionMapping(playerId, cyberwareId, true);

        auto* playerState = GetPlayerCyberwareState(playerId);
        if (playerState) {
            playerState->malfunctionsOccurred++;
        }

        m_totalMalfunctions++;

        // Create malfunction data
        CyberwareMalfunctionData malfunctionData;
        malfunctionData.playerId = playerId;
        malfunctionData.cyberwareId = cyberwareId;
        malfunctionData.malfunctionType = type;
        malfunctionData.severity = severity;
        malfunctionData.isActive = true;
        malfunctionData.startTime = 0.0f; // Would be set by game time
        malfunctionData.timestamp = std::chrono::steady_clock::now();

        // Notify listeners
        NotifyMalfunctionTriggered(playerId, malfunctionData);

        // Broadcast malfunction
        SendMalfunctionUpdateToClients(playerId, malfunctionData);
    }

    PlayerCyberwareState* CyberwareManager::GetPlayerCyberwareState(uint32_t playerId)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        return (it != m_playerStates.end()) ? it->second.get() : nullptr;
    }

    const PlayerCyberwareState* CyberwareManager::GetPlayerCyberwareState(uint32_t playerId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerStates.find(playerId);
        return (it != m_playerStates.end()) ? it->second.get() : nullptr;
    }

    ActiveCyberware* CyberwareManager::GetPlayerCyberware(uint32_t playerId, uint32_t cyberwareId)
    {
        auto* playerState = GetPlayerCyberwareState(playerId);
        if (!playerState) {
            return nullptr;
        }

        auto it = playerState->installedCyberware.find(cyberwareId);
        return (it != playerState->installedCyberware.end()) ? it->second.get() : nullptr;
    }

    std::vector<uint32_t> CyberwareManager::GetPlayersWithCyberware(CyberwareAbility abilityType) const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_abilityToPlayers.find(abilityType);
        return (it != m_abilityToPlayers.end()) ? it->second : std::vector<uint32_t>();
    }

    std::vector<uint32_t> CyberwareManager::GetPlayersWithMalfunctions() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        std::vector<uint32_t> playersWithMalfunctions;
        for (const auto& [playerId, cyberwareIds] : m_playerMalfunctions) {
            if (!cyberwareIds.empty()) {
                playersWithMalfunctions.push_back(playerId);
            }
        }

        return playersWithMalfunctions;
    }

    std::vector<uint32_t> CyberwareManager::GetPlayersInSlowMotion() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        std::vector<uint32_t> playersInSlowMo;
        for (const auto& [playerId, playerState] : m_playerStates) {
            if (playerState->activeSlowMotion.isActive) {
                playersInSlowMo.push_back(playerId);
            }
        }

        return playersInSlowMo;
    }

    bool CyberwareManager::ValidateCyberwareData(uint32_t playerId, const CyberwareSyncData& data) const
    {
        // Validate basic data
        if (!ValidateCyberwareHealth(data.healthPercentage)) {
            return false;
        }

        if (!ValidateBatteryLevel(data.batteryLevel)) {
            return false;
        }

        if (data.cooldownRemaining < 0.0f || data.cooldownRemaining > 300.0f) { // Max 5 minutes
            return false;
        }

        return true;
    }

    bool CyberwareManager::ValidateAbilityUsage(uint32_t playerId, const CyberwareAbilityData& abilityData) const
    {
        // Validate duration
        if (abilityData.duration < 0.0f || abilityData.duration > 60.0f) { // Max 1 minute
            return false;
        }

        // Validate intensity
        if (abilityData.intensity < 0.1f || abilityData.intensity > 5.0f) {
            return false;
        }

        return true;
    }

    void CyberwareManager::BroadcastCyberwareUpdate(uint32_t playerId, const CyberwareSyncData& data)
    {
        SendCyberwareUpdateToClients(playerId, data);
    }

    void CyberwareManager::BroadcastAbilityActivation(uint32_t playerId, const CyberwareAbilityData& abilityData)
    {
        SendAbilityUpdateToClients(playerId, abilityData);
    }

    void CyberwareManager::BroadcastSlowMotionEffect(uint32_t playerId, const SlowMotionData& slowMoData)
    {
        SendSlowMotionUpdateToClients(playerId, slowMoData);
    }

    // Private implementation methods
    void CyberwareManager::UpdatePlayerCyberware(float deltaTime)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [playerId, playerState] : m_playerStates) {
            // Update each cyberware piece
            for (auto& [cyberwareId, cyberware] : playerState->installedCyberware) {
                // Update state based on conditions
                cyberware->state = DetermineOptimalState(*cyberware);
                cyberware->lastUpdate = currentTime;

                // Check for random malfunctions
                if (CyberwareUtils::ShouldTriggerMalfunction(*cyberware, deltaTime)) {
                    TriggerRandomMalfunction(playerId, cyberwareId);
                }
            }

            // Check for player timeout (5 minutes of inactivity)
            auto timeSinceActivity = std::chrono::duration_cast<std::chrono::minutes>(
                currentTime - playerState->lastActivity).count();

            if (timeSinceActivity >= 5) {
                playerState->isConnected = false;
            }
        }
    }

    void CyberwareManager::UpdateCooldowns(float deltaTime)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        for (auto& [playerId, playerState] : m_playerStates) {
            for (auto& [cyberwareId, cyberware] : playerState->installedCyberware) {
                if (cyberware->isOnCooldown) {
                    cyberware->cooldownRemaining -= deltaTime;

                    if (cyberware->cooldownRemaining <= 0.0f) {
                        cyberware->cooldownRemaining = 0.0f;
                        cyberware->isOnCooldown = false;

                        // Broadcast cooldown end
                        CyberwareCooldownData cooldownData;
                        cooldownData.playerId = playerId;
                        cooldownData.cyberwareId = cyberwareId;
                        cooldownData.isOnCooldown = false;
                        cooldownData.remainingTime = 0.0f;
                        cooldownData.timestamp = std::chrono::steady_clock::now();

                        // Would send to network clients
                        // SendCooldownUpdateToClients(cooldownData);
                    }
                }
            }
        }
    }

    void CyberwareManager::UpdateAbilityCooldowns(float deltaTime)
    {
        // Handled in UpdateCooldowns
    }

    void CyberwareManager::UpdateSlowMotionEffects(float deltaTime)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        for (auto& [playerId, playerState] : m_playerStates) {
            if (playerState->activeSlowMotion.isActive) {
                playerState->activeSlowMotion.remainingTime -= deltaTime;

                if (playerState->activeSlowMotion.remainingTime <= 0.0f) {
                    playerState->activeSlowMotion.isActive = false;
                    playerState->activeSlowMotion.remainingTime = 0.0f;
                    playerState->activeSlowMotion.factor = 1.0f;

                    // Broadcast slow motion end
                    SlowMotionUpdate slowMoUpdate;
                    slowMoUpdate.playerId = playerId;
                    slowMoUpdate.slowMoData = playerState->activeSlowMotion;
                    slowMoUpdate.isActivation = false;
                    slowMoUpdate.updateTime = std::chrono::steady_clock::now();

                    // Would send to network clients
                    // SendSlowMotionUpdateToClients(slowMoUpdate);
                }
            }
        }
    }

    void CyberwareManager::UpdateMalfunctions(float deltaTime)
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        std::vector<std::pair<uint32_t, uint32_t>> resolvedMalfunctions; // playerId, cyberwareId

        for (auto& [playerId, playerState] : m_playerStates) {
            for (auto& [cyberwareId, cyberware] : playerState->installedCyberware) {
                if (cyberware->isMalfunctioning) {
                    // Auto-resolve minor malfunctions after time
                    if (cyberware->malfunctionSeverity == MalfunctionSeverity::Minor) {
                        auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::steady_clock::now() - cyberware->lastUpdate).count();

                        if (elapsedTime >= 30) { // 30 seconds for minor malfunctions
                            resolvedMalfunctions.emplace_back(playerId, cyberwareId);
                        }
                    }
                }
            }
        }

        lock.unlock();

        // Resolve malfunctions
        for (auto& [playerId, cyberwareId] : resolvedMalfunctions) {
            ResolveCyberwareMalfunction(playerId, cyberwareId);
        }
    }

    uint32_t CyberwareManager::GenerateCyberwareId()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint32_t> dis(1, UINT32_MAX);
        return dis(gen);
    }

    bool CyberwareManager::IsCyberwareCompatible(CyberwareSlot slot, CyberwareAbility ability) const
    {
        // Check compatibility between slot and ability
        switch (slot) {
            case CyberwareSlot::ArmsCyberware:
            case CyberwareSlot::HandsCyberware:
                return ability == CyberwareAbility::Mantis_Blades ||
                       ability == CyberwareAbility::Monowire ||
                       ability == CyberwareAbility::Projectile_Launch_System ||
                       ability == CyberwareAbility::Gorilla_Arms;

            case CyberwareSlot::LegsCyberware:
                return ability == CyberwareAbility::Reinforced_Tendons ||
                       ability == CyberwareAbility::Lynx_Paws ||
                       ability == CyberwareAbility::Fortified_Ankles;

            case CyberwareSlot::OcularCyberware:
            case CyberwareSlot::EyesCyberware:
                return ability == CyberwareAbility::Kiroshi_Optics ||
                       ability == CyberwareAbility::Ballistic_Coprocessor ||
                       ability == CyberwareAbility::Target_Analysis;

            case CyberwareSlot::NervousSystemCyberware:
                return ability == CyberwareAbility::Kerenzikov ||
                       ability == CyberwareAbility::Sandevistan ||
                       ability == CyberwareAbility::Synaptic_Signal_Optimizer;

            default:
                return true; // Other slots are more flexible
        }
    }

    float CyberwareManager::GetAbilityCooldownDuration(CyberwareAbility abilityType) const
    {
        switch (abilityType) {
            case CyberwareAbility::Mantis_Blades: return 5.0f;
            case CyberwareAbility::Monowire: return 8.0f;
            case CyberwareAbility::Projectile_Launch_System: return 15.0f;
            case CyberwareAbility::Gorilla_Arms: return 3.0f;
            case CyberwareAbility::Sandevistan: return 30.0f;
            case CyberwareAbility::Kerenzikov: return 20.0f;
            case CyberwareAbility::Kiroshi_Optics: return 2.0f;
            default: return 10.0f; // Default cooldown
        }
    }

    void CyberwareManager::SendCyberwareUpdateToClients(uint32_t playerId, const CyberwareSyncData& data)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void CyberwareManager::SendAbilityUpdateToClients(uint32_t playerId, const CyberwareAbilityData& abilityData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void CyberwareManager::SendSlowMotionUpdateToClients(uint32_t playerId, const SlowMotionData& slowMoData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void CyberwareManager::SendMalfunctionUpdateToClients(uint32_t playerId, const CyberwareMalfunctionData& malfunctionData)
    {
        // This would send network messages to all clients
        // Implementation would depend on the networking system
    }

    void CyberwareManager::UpdatePlayerActivity(uint32_t playerId)
    {
        auto* playerState = GetPlayerCyberwareState(playerId);
        if (playerState) {
            playerState->lastActivity = std::chrono::steady_clock::now();
            playerState->isConnected = true;
        }
    }

    void CyberwareManager::SynchronizeCyberware(uint32_t playerId)
    {
        auto* playerState = GetPlayerCyberwareState(playerId);
        if (!playerState) {
            return;
        }

        // Force synchronization of all cyberware for this player
        for (const auto& [cyberwareId, cyberware] : playerState->installedCyberware) {
            CyberwareSyncData syncData;
            syncData.playerId = playerId;
            syncData.cyberwareId = cyberwareId;
            syncData.slotType = cyberware->slot;
            syncData.currentState = cyberware->state;
            syncData.healthPercentage = cyberware->healthPercentage;
            syncData.isActive = cyberware->isActive;
            syncData.isOnCooldown = cyberware->isOnCooldown;
            syncData.cooldownRemaining = cyberware->cooldownRemaining;
            syncData.isMalfunctioning = cyberware->isMalfunctioning;
            syncData.batteryLevel = cyberware->batteryLevel;
            syncData.timestamp = std::chrono::steady_clock::now();

            BroadcastCyberwareUpdate(playerId, syncData);
        }
    }

    bool CyberwareManager::DeactivateCyberwareAbility(uint32_t playerId, CyberwareAbility abilityType)
    {
        auto* playerState = GetPlayerCyberwareState(playerId);
        if (!playerState) {
            return false;
        }

        // Find cyberware with this ability
        for (auto& [cyberwareId, cyberware] : playerState->installedCyberware) {
            if (cyberware->primaryAbility == abilityType && cyberware->isActive) {
                cyberware->isActive = false;

                // Update mapping
                UpdateAbilityToPlayersMapping(playerId, abilityType, false);

                // Broadcast deactivation
                CyberwareAbilityData abilityData;
                abilityData.playerId = playerId;
                abilityData.abilityType = abilityType;
                abilityData.isActivated = false;
                abilityData.timestamp = std::chrono::steady_clock::now();

                BroadcastAbilityActivation(playerId, abilityData);

                return true;
            }
        }

        return false;
    }

    bool CyberwareManager::IsAbilityOnCooldown(uint32_t playerId, CyberwareAbility abilityType) const
    {
        auto* playerState = GetPlayerCyberwareState(playerId);
        if (!playerState) {
            return false;
        }

        // Find cyberware with this ability
        for (const auto& [cyberwareId, cyberware] : playerState->installedCyberware) {
            if (cyberware->primaryAbility == abilityType) {
                return cyberware->isOnCooldown;
            }
        }

        return false;
    }

    bool CyberwareManager::IsPlayerInSlowMotion(uint32_t playerId) const
    {
        auto* playerState = GetPlayerCyberwareState(playerId);
        return playerState && playerState->activeSlowMotion.isActive;
    }

    float CyberwareManager::GetCyberwareCooldownRemaining(uint32_t playerId, uint32_t cyberwareId) const
    {
        auto* playerState = GetPlayerCyberwareState(playerId);
        if (!playerState) {
            return 0.0f;
        }

        auto it = playerState->installedCyberware.find(cyberwareId);
        if (it != playerState->installedCyberware.end()) {
            return it->second->cooldownRemaining;
        }

        return 0.0f;
    }

    void CyberwareManager::ResolveCyberwareMalfunction(uint32_t playerId, uint32_t cyberwareId)
    {
        auto* cyberware = GetPlayerCyberware(playerId, cyberwareId);
        if (!cyberware) {
            return;
        }

        // Clear malfunction state
        cyberware->isMalfunctioning = false;
        cyberware->malfunctionType = MalfunctionType::None;
        cyberware->malfunctionSeverity = MalfunctionSeverity::None;
        cyberware->state = CyberwareState::Operational;

        // Update malfunction mapping
        UpdateMalfunctionMapping(playerId, cyberwareId, false);

        // Create resolution data
        CyberwareMalfunctionData malfunctionData;
        malfunctionData.playerId = playerId;
        malfunctionData.cyberwareId = cyberwareId;
        malfunctionData.malfunctionType = MalfunctionType::None;
        malfunctionData.severity = MalfunctionSeverity::None;
        malfunctionData.isActive = false;
        malfunctionData.timestamp = std::chrono::steady_clock::now();

        // Broadcast resolution
        SendMalfunctionUpdateToClients(playerId, malfunctionData);
    }

    bool CyberwareManager::HasActiveMalfunction(uint32_t playerId, uint32_t cyberwareId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto it = m_playerMalfunctions.find(playerId);
        if (it == m_playerMalfunctions.end()) {
            return false;
        }

        const auto& cyberwareIds = it->second;
        return std::find(cyberwareIds.begin(), cyberwareIds.end(), cyberwareId) != cyberwareIds.end();
    }

    bool CyberwareManager::IsAbilityUsageRateLimited(uint32_t playerId, CyberwareAbility abilityType) const
    {
        return IsAbilityRateLimited(playerId, abilityType);
    }

    void CyberwareManager::DetectCyberwareAnomalies(uint32_t playerId)
    {
        auto* playerState = GetPlayerCyberwareState(playerId);
        if (!playerState) {
            return;
        }

        auto currentTime = std::chrono::steady_clock::now();

        // Check for ability spam
        if (playerState->recentAbilities.size() >= 5) {
            auto timeSinceFirst = std::chrono::duration_cast<std::chrono::seconds>(
                currentTime - playerState->recentAbilities[0].timestamp).count();

            if (timeSinceFirst < 10) { // 5 abilities in 10 seconds is suspicious
                // Log anomaly or take action
            }
        }
    }

    void CyberwareManager::ForceSyncPlayer(uint32_t playerId)
    {
        SynchronizeCyberware(playerId);
    }

    void CyberwareManager::SetSyncPriority(uint32_t playerId, float priority)
    {
        auto* playerState = GetPlayerCyberwareState(playerId);
        if (playerState) {
            playerState->syncPriority = priority;
        }
    }

    uint32_t CyberwareManager::GetActivePlayerCount() const
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

    uint32_t CyberwareManager::GetTotalInstalledCyberware() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        uint32_t count = 0;
        for (const auto& [playerId, playerState] : m_playerStates) {
            count += playerState->totalCyberwareCount;
        }

        return count;
    }

    uint32_t CyberwareManager::GetActiveMalfunctionCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        uint32_t count = 0;
        for (const auto& [playerId, cyberwareIds] : m_playerMalfunctions) {
            count += static_cast<uint32_t>(cyberwareIds.size());
        }

        return count;
    }

    std::unordered_map<CyberwareAbility, uint32_t> CyberwareManager::GetAbilityUsageStats() const
    {
        std::unordered_map<CyberwareAbility, uint32_t> stats;
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        for (const auto& [ability, playerList] : m_abilityToPlayers) {
            stats[ability] = static_cast<uint32_t>(playerList.size());
        }

        return stats;
    }

    std::unordered_map<MalfunctionType, uint32_t> CyberwareManager::GetMalfunctionStats() const
    {
        std::unordered_map<MalfunctionType, uint32_t> stats;
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        for (const auto& [playerId, playerState] : m_playerStates) {
            for (const auto& [cyberwareId, cyberware] : playerState->installedCyberware) {
                if (cyberware->isMalfunctioning) {
                    stats[cyberware->malfunctionType]++;
                }
            }
        }

        return stats;
    }

    void CyberwareManager::ProcessAbilityExpirations()
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [playerId, playerState] : m_playerStates) {
            // Clean up old ability entries
            auto& abilities = playerState->recentAbilities;
            abilities.erase(
                std::remove_if(abilities.begin(), abilities.end(),
                    [currentTime](const CyberwareAbilityData& ability) {
                        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                            currentTime - ability.timestamp).count();
                        return elapsed > 60; // Remove abilities older than 1 minute
                    }),
                abilities.end());
        }
    }

    void CyberwareManager::ProcessSlowMotionExpirations()
    {
        // Handled in UpdateSlowMotionEffects
    }

    void CyberwareManager::CleanupExpiredData()
    {
        ProcessAbilityExpirations();
        ProcessSlowMotionExpirations();
    }

    void CyberwareManager::ValidateCyberwareStates()
    {
        std::shared_lock<std::shared_mutex> lock(m_statesMutex);

        for (auto& [playerId, playerState] : m_playerStates) {
            for (auto& [cyberwareId, cyberware] : playerState->installedCyberware) {
                // Validate and correct invalid states
                if (cyberware->healthPercentage < 0.0f) {
                    cyberware->healthPercentage = 0.0f;
                } else if (cyberware->healthPercentage > 1.0f) {
                    cyberware->healthPercentage = 1.0f;
                }

                if (cyberware->batteryLevel < 0.0f) {
                    cyberware->batteryLevel = 0.0f;
                } else if (cyberware->batteryLevel > 1.0f) {
                    cyberware->batteryLevel = 1.0f;
                }

                if (cyberware->cooldownRemaining < 0.0f) {
                    cyberware->cooldownRemaining = 0.0f;
                    cyberware->isOnCooldown = false;
                }
            }
        }
    }

    float CyberwareManager::GetAbilityBaseDuration(CyberwareAbility abilityType) const
    {
        switch (abilityType) {
            case CyberwareAbility::Sandevistan: return 8.0f;
            case CyberwareAbility::Kerenzikov: return 3.0f;
            case CyberwareAbility::Optical_Camo: return 10.0f;
            case CyberwareAbility::Mantis_Blades: return 1.0f;
            case CyberwareAbility::Monowire: return 1.5f;
            default: return 2.0f;
        }
    }

    CyberwareState CyberwareManager::DetermineOptimalState(const ActiveCyberware& cyberware) const
    {
        if (cyberware.isMalfunctioning) {
            return CyberwareState::Malfunctioning;
        }

        if (cyberware.healthPercentage <= 0.0f) {
            return CyberwareState::Offline;
        }

        if (cyberware.healthPercentage < 0.2f) {
            return CyberwareState::Damaged;
        }

        if (cyberware.healthPercentage < 0.5f) {
            return CyberwareState::Degraded;
        }

        if (cyberware.isActive) {
            return CyberwareState::Active;
        }

        return CyberwareState::Operational;
    }

    void CyberwareManager::UpdateAbilityToPlayersMapping(uint32_t playerId, CyberwareAbility abilityType, bool isActive)
    {
        auto& playerList = m_abilityToPlayers[abilityType];

        if (isActive) {
            if (std::find(playerList.begin(), playerList.end(), playerId) == playerList.end()) {
                playerList.push_back(playerId);
            }
        } else {
            playerList.erase(std::remove(playerList.begin(), playerList.end(), playerId), playerList.end());
        }
    }

    void CyberwareManager::UpdateMalfunctionMapping(uint32_t playerId, uint32_t cyberwareId, bool hasMalfunction)
    {
        auto& cyberwareList = m_playerMalfunctions[playerId];

        if (hasMalfunction) {
            if (std::find(cyberwareList.begin(), cyberwareList.end(), cyberwareId) == cyberwareList.end()) {
                cyberwareList.push_back(cyberwareId);
            }
        } else {
            cyberwareList.erase(std::remove(cyberwareList.begin(), cyberwareList.end(), cyberwareId), cyberwareList.end());
        }
    }

    void CyberwareManager::RemovePlayerFromAllMappings(uint32_t playerId)
    {
        // Remove from ability mappings
        for (auto& [ability, playerList] : m_abilityToPlayers) {
            playerList.erase(std::remove(playerList.begin(), playerList.end(), playerId), playerList.end());
        }

        // Remove from malfunction mappings
        m_playerMalfunctions.erase(playerId);
    }

    bool CyberwareManager::IsAbilityRateLimited(uint32_t playerId, CyberwareAbility abilityType) const
    {
        auto* playerState = GetPlayerCyberwareState(playerId);
        if (!playerState) {
            return false;
        }

        auto currentTime = std::chrono::steady_clock::now();

        // Count recent usages of this ability type
        uint32_t recentUsages = 0;
        for (const auto& ability : playerState->recentAbilities) {
            if (ability.abilityType == abilityType) {
                auto timeSinceUse = std::chrono::duration_cast<std::chrono::seconds>(
                    currentTime - ability.timestamp).count();

                if (timeSinceUse < 10) { // Within last 10 seconds
                    recentUsages++;
                }
            }
        }

        // Rate limit: max 3 uses per 10 seconds for most abilities
        uint32_t maxUsages = 3;
        if (abilityType == CyberwareAbility::Sandevistan || abilityType == CyberwareAbility::Kerenzikov) {
            maxUsages = 1; // More restrictive for time dilation
        }

        return recentUsages >= maxUsages;
    }

    bool CyberwareManager::ValidateCyberwareHealth(float healthPercentage) const
    {
        return healthPercentage >= 0.0f && healthPercentage <= 1.0f;
    }

    bool CyberwareManager::ValidateBatteryLevel(float batteryLevel) const
    {
        return batteryLevel >= 0.0f && batteryLevel <= 1.0f;
    }

    void CyberwareManager::TriggerRandomMalfunction(uint32_t playerId, uint32_t cyberwareId)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<int> typeDis(1, 6); // Skip None
        static std::uniform_int_distribution<int> severityDis(1, 4); // Skip None

        MalfunctionType type = static_cast<MalfunctionType>(typeDis(gen));
        MalfunctionSeverity severity = static_cast<MalfunctionSeverity>(severityDis(gen));

        TriggerCyberwareMalfunction(playerId, cyberwareId, type, severity);
    }

    // Notification methods
    void CyberwareManager::NotifyCyberwareInstalled(uint32_t playerId, uint32_t cyberwareId, CyberwareSlot slot)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_cyberwareInstalledCallback) {
            m_cyberwareInstalledCallback(playerId, cyberwareId, slot);
        }
    }

    void CyberwareManager::NotifyCyberwareRemoved(uint32_t playerId, uint32_t cyberwareId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_cyberwareRemovedCallback) {
            m_cyberwareRemovedCallback(playerId, cyberwareId);
        }
    }

    void CyberwareManager::NotifyAbilityActivated(uint32_t playerId, const CyberwareAbilityData& abilityData)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_abilityActivatedCallback) {
            m_abilityActivatedCallback(playerId, abilityData);
        }
    }

    void CyberwareManager::NotifyMalfunctionTriggered(uint32_t playerId, const CyberwareMalfunctionData& malfunctionData)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_malfunctionTriggeredCallback) {
            m_malfunctionTriggeredCallback(playerId, malfunctionData);
        }
    }

    void CyberwareManager::NotifySlowMotionActivated(uint32_t playerId, const SlowMotionData& slowMoData)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_slowMotionActivatedCallback) {
            m_slowMotionActivatedCallback(playerId, slowMoData);
        }
    }

    // Callback setters
    void CyberwareManager::SetCyberwareInstalledCallback(CyberwareInstalledCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_cyberwareInstalledCallback = callback;
    }

    void CyberwareManager::SetCyberwareRemovedCallback(CyberwareRemovedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_cyberwareRemovedCallback = callback;
    }

    void CyberwareManager::SetAbilityActivatedCallback(AbilityActivatedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_abilityActivatedCallback = callback;
    }

    void CyberwareManager::SetMalfunctionTriggeredCallback(MalfunctionTriggeredCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_malfunctionTriggeredCallback = callback;
    }

    void CyberwareManager::SetSlowMotionActivatedCallback(SlowMotionActivatedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_slowMotionActivatedCallback = callback;
    }

    // Utility functions implementation
    namespace CyberwareUtils
    {
        std::string CyberwareStateToString(CyberwareState state)
        {
            switch (state) {
                case CyberwareState::Operational: return "Operational";
                case CyberwareState::Active: return "Active";
                case CyberwareState::Degraded: return "Degraded";
                case CyberwareState::Damaged: return "Damaged";
                case CyberwareState::Malfunctioning: return "Malfunctioning";
                case CyberwareState::Offline: return "Offline";
                default: return "Unknown";
            }
        }

        std::string CyberwareAbilityToString(CyberwareAbility ability)
        {
            switch (ability) {
                case CyberwareAbility::Mantis_Blades: return "Mantis_Blades";
                case CyberwareAbility::Monowire: return "Monowire";
                case CyberwareAbility::Projectile_Launch_System: return "Projectile_Launch_System";
                case CyberwareAbility::Gorilla_Arms: return "Gorilla_Arms";
                case CyberwareAbility::Reinforced_Tendons: return "Reinforced_Tendons";
                case CyberwareAbility::Lynx_Paws: return "Lynx_Paws";
                case CyberwareAbility::Fortified_Ankles: return "Fortified_Ankles";
                case CyberwareAbility::Kiroshi_Optics: return "Kiroshi_Optics";
                case CyberwareAbility::Ballistic_Coprocessor: return "Ballistic_Coprocessor";
                case CyberwareAbility::Target_Analysis: return "Target_Analysis";
                case CyberwareAbility::Kerenzikov: return "Kerenzikov";
                case CyberwareAbility::Sandevistan: return "Sandevistan";
                case CyberwareAbility::Synaptic_Signal_Optimizer: return "Synaptic_Signal_Optimizer";
                case CyberwareAbility::Biomonitor: return "Biomonitor";
                case CyberwareAbility::Blood_Pump: return "Blood_Pump";
                case CyberwareAbility::Biomodulator: return "Biomodulator";
                case CyberwareAbility::Subdermal_Armor: return "Subdermal_Armor";
                case CyberwareAbility::Optical_Camo: return "Optical_Camo";
                case CyberwareAbility::Thermal_Damage_Protection: return "Thermal_Damage_Protection";
                default: return "Unknown";
            }
        }

        bool IsOffensiveAbility(CyberwareAbility ability)
        {
            switch (ability) {
                case CyberwareAbility::Mantis_Blades:
                case CyberwareAbility::Monowire:
                case CyberwareAbility::Projectile_Launch_System:
                case CyberwareAbility::Gorilla_Arms:
                    return true;
                default:
                    return false;
            }
        }

        bool IsDefensiveAbility(CyberwareAbility ability)
        {
            switch (ability) {
                case CyberwareAbility::Subdermal_Armor:
                case CyberwareAbility::Thermal_Damage_Protection:
                case CyberwareAbility::Biomonitor:
                    return true;
                default:
                    return false;
            }
        }

        bool IsUtilityAbility(CyberwareAbility ability)
        {
            switch (ability) {
                case CyberwareAbility::Kiroshi_Optics:
                case CyberwareAbility::Target_Analysis:
                case CyberwareAbility::Optical_Camo:
                case CyberwareAbility::Reinforced_Tendons:
                case CyberwareAbility::Lynx_Paws:
                    return true;
                default:
                    return false;
            }
        }

        float CalculateAbilityEffectiveness(const ActiveCyberware& cyberware, float baseEffectiveness)
        {
            float effectiveness = baseEffectiveness;

            // Reduce effectiveness based on health
            effectiveness *= cyberware.healthPercentage;

            // Reduce effectiveness based on battery level
            effectiveness *= cyberware.batteryLevel;

            // Reduce effectiveness if malfunctioning
            if (cyberware.isMalfunctioning) {
                effectiveness *= 0.5f; // 50% effectiveness reduction
            }

            return effectiveness;
        }

        bool ShouldTriggerMalfunction(const ActiveCyberware& cyberware, float deltaTime)
        {
            if (cyberware.isMalfunctioning) {
                return false; // Already malfunctioning
            }

            // Base malfunction chance per second
            float baseChance = 0.0001f; // 0.01% per second

            // Increase chance based on health
            if (cyberware.healthPercentage < 0.3f) {
                baseChance *= 5.0f; // 5x more likely with low health
            }

            // Increase chance based on battery level
            if (cyberware.batteryLevel < 0.2f) {
                baseChance *= 3.0f; // 3x more likely with low battery
            }

            // Calculate chance for this frame
            float frameChance = baseChance * deltaTime;

            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

            return dis(gen) < frameChance;
        }
    }
}
#include "CompetitiveGameModeManager.hpp"
#include "RED4ext/Api/Sdk.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace RED4ext
{
    // CompetitiveGameModeManager Implementation
    CompetitiveGameModeManager& CompetitiveGameModeManager::GetInstance()
    {
        static CompetitiveGameModeManager instance;
        return instance;
    }

    void CompetitiveGameModeManager::Initialize()
    {
        std::unique_lock<std::shared_mutex> lock(m_matchesMutex);

        // Clear existing data
        m_matches.clear();
        m_playerToMatch.clear();

        // Initialize statistics
        m_totalMatchesCreated = 0;
        m_totalMatchesCompleted = 0;
        m_lastCleanup = std::chrono::steady_clock::now();
    }

    void CompetitiveGameModeManager::Shutdown()
    {
        // End all active matches
        std::vector<std::string> activeMatches = GetActiveMatches();
        for (const auto& matchId : activeMatches) {
            EndMatch(matchId);
        }

        std::unique_lock<std::shared_mutex> lock(m_matchesMutex);
        m_matches.clear();
        m_playerToMatch.clear();

        // Clear callbacks
        std::lock_guard<std::mutex> callbacksLock(m_callbacksMutex);
        m_matchStartedCallback = nullptr;
        m_matchEndedCallback = nullptr;
        m_playerJoinedCallback = nullptr;
        m_playerLeftCallback = nullptr;
        m_playerKilledCallback = nullptr;
        m_powerupSpawnedCallback = nullptr;
    }

    void CompetitiveGameModeManager::Update()
    {
        auto currentTime = std::chrono::steady_clock::now();

        // Periodic cleanup (every 5 minutes)
        auto timeSinceCleanup = std::chrono::duration_cast<std::chrono::minutes>(
            currentTime - m_lastCleanup).count();

        if (timeSinceCleanup >= 5) {
            CleanupInactiveMatches();
            m_lastCleanup = currentTime;
        }

        // Update all active matches
        std::shared_lock<std::shared_mutex> lock(m_matchesMutex);
        for (auto& [matchId, match] : m_matches) {
            if (match->state == MatchState::InProgress) {
                UpdateMatchLogic(match.get());
            }
        }
    }

    std::string CompetitiveGameModeManager::CreateMatch(uint32_t hostPlayerId, CompetitiveMode gameMode,
                                                       const CompetitiveMatchSettings& settings)
    {
        // Validate settings
        if (!ValidateMatchSettings(gameMode, settings)) {
            return "";
        }

        // Check if host is already in another match
        {
            std::shared_lock<std::shared_mutex> lock(m_matchesMutex);
            auto it = m_playerToMatch.find(hostPlayerId);
            if (it != m_playerToMatch.end()) {
                return ""; // Player already in another match
            }
        }

        std::string matchId = GenerateMatchId();

        auto match = std::make_unique<CompetitiveMatch>();
        match->matchId = matchId;
        match->gameMode = gameMode;
        match->hostPlayerId = hostPlayerId;
        match->settings = settings;
        match->state = MatchState::Waiting;
        match->startTime = std::chrono::steady_clock::now();
        match->lastUpdate = match->startTime;
        match->syncVersion = 1;

        // Add host as first participant
        match->participants.push_back(hostPlayerId);

        auto participant = std::make_unique<CompetitiveParticipant>();
        participant->playerId = hostPlayerId;
        participant->lastActivity = std::chrono::steady_clock::now();
        match->participantData[hostPlayerId] = std::move(participant);

        // Store match
        {
            std::unique_lock<std::shared_mutex> lock(m_matchesMutex);
            m_matches[matchId] = std::move(match);
            m_playerToMatch[hostPlayerId] = matchId;
        }

        m_totalMatchesCreated++;

        // Notify listeners
        NotifyMatchStarted(matchId);

        return matchId;
    }

    MatchJoinResult CompetitiveGameModeManager::JoinMatch(const std::string& matchId, uint32_t playerId)
    {
        std::unique_lock<std::shared_mutex> lock(m_matchesMutex);

        // Check if player is already in another match
        auto playerIt = m_playerToMatch.find(playerId);
        if (playerIt != m_playerToMatch.end()) {
            return MatchJoinResult::AlreadyInMatch;
        }

        auto it = m_matches.find(matchId);
        if (it == m_matches.end()) {
            return MatchJoinResult::MatchNotFound;
        }

        auto& match = it->second;

        // Check match state
        if (match->state != MatchState::Waiting && match->state != MatchState::InProgress) {
            return MatchJoinResult::MatchInProgress;
        }

        // Check capacity
        if (static_cast<int32_t>(match->participants.size()) >= match->settings.maxPlayers) {
            return MatchJoinResult::MatchFull;
        }

        // Check if player can join
        if (!CanPlayerJoinMatch(playerId, matchId)) {
            return MatchJoinResult::NetworkError; // Generic error for various issues
        }

        // Add participant
        match->participants.push_back(playerId);
        match->syncVersion++;

        auto participant = std::make_unique<CompetitiveParticipant>();
        participant->playerId = playerId;
        participant->lastActivity = std::chrono::steady_clock::now();
        match->participantData[playerId] = std::move(participant);

        m_playerToMatch[playerId] = matchId;

        lock.unlock();

        // Notify listeners
        NotifyPlayerJoined(matchId, playerId);

        // Sync match state to new player
        SyncMatchToPlayer(matchId, playerId);

        return MatchJoinResult::Success;
    }

    bool CompetitiveGameModeManager::StartMatch(const std::string& matchId)
    {
        std::unique_lock<std::shared_mutex> lock(m_matchesMutex);

        auto it = m_matches.find(matchId);
        if (it == m_matches.end()) {
            return false;
        }

        auto& match = it->second;

        if (match->state != MatchState::Waiting) {
            return false;
        }

        // Check minimum players
        if (static_cast<int32_t>(match->participants.size()) < GetMinimumPlayers(match->gameMode)) {
            return false;
        }

        match->state = MatchState::InProgress;
        match->startTime = std::chrono::steady_clock::now();
        match->duration = 0.0f;
        match->currentRound = 1;
        match->syncVersion++;

        lock.unlock();

        // Broadcast match state
        BroadcastMatchState(matchId);

        return true;
    }

    bool CompetitiveGameModeManager::EndMatch(const std::string& matchId)
    {
        std::unique_lock<std::shared_mutex> lock(m_matchesMutex);

        auto it = m_matches.find(matchId);
        if (it == m_matches.end()) {
            return false;
        }

        auto& match = it->second;
        bool wasSuccessful = (match->state == MatchState::InProgress);

        // Get participants list before cleanup
        std::vector<uint32_t> participants = match->participants;

        match->state = MatchState::Finished;
        match->lastUpdate = std::chrono::steady_clock::now();

        // Remove players from mapping
        for (uint32_t playerId : participants) {
            m_playerToMatch.erase(playerId);
        }

        lock.unlock();

        m_totalMatchesCompleted++;

        // Notify listeners
        NotifyMatchEnded(matchId, wasSuccessful);

        return true;
    }

    void CompetitiveGameModeManager::OnRaceCheckpointReached(const std::string& matchId, uint32_t playerId, uint32_t checkpointId)
    {
        auto* match = GetMatch(matchId);
        if (!match || match->gameMode != CompetitiveMode::Racing) {
            return;
        }

        auto* participant = FindParticipant(match, playerId);
        if (!participant) {
            return;
        }

        // Add checkpoint to participant's progress
        participant->checkpointsReached.push_back(checkpointId);

        // Check if lap completed (this would need more sophisticated logic)
        bool lapCompleted = false; // Placeholder logic
        if (lapCompleted) {
            float lapTime = 60.0f; // Placeholder - calculate actual lap time
            OnLapCompleted(matchId, playerId, lapTime);
        }

        match->syncVersion++;
        BroadcastMatchState(matchId);
    }

    void CompetitiveGameModeManager::OnLapCompleted(const std::string& matchId, uint32_t playerId, float lapTime)
    {
        auto* match = GetMatch(matchId);
        if (!match || match->gameMode != CompetitiveMode::Racing) {
            return;
        }

        auto* participant = FindParticipant(match, playerId);
        if (!participant) {
            return;
        }

        participant->lapTimes.push_back(lapTime);
        participant->lapsCompleted++;
        participant->totalRaceTime += lapTime;

        if (lapTime < participant->bestLapTime) {
            participant->bestLapTime = lapTime;
        }

        // Check if race finished
        if (participant->lapsCompleted >= match->raceSettings.laps) {
            OnRaceFinished(matchId, playerId, participant->totalRaceTime);
        }

        match->syncVersion++;
        BroadcastMatchState(matchId);
    }

    void CompetitiveGameModeManager::OnRaceFinished(const std::string& matchId, uint32_t playerId, float totalTime)
    {
        auto* match = GetMatch(matchId);
        if (!match) {
            return;
        }

        auto* participant = FindParticipant(match, playerId);
        if (!participant) {
            return;
        }

        // Set position based on finish order
        int32_t finishedCount = 0;
        for (const auto& [pid, pdata] : match->participantData) {
            if (pdata->lapsCompleted >= match->raceSettings.laps && pid != playerId) {
                finishedCount++;
            }
        }
        participant->position = finishedCount + 1;

        // Award points based on position
        int32_t points = std::max(0, 100 - (participant->position - 1) * 10);
        participant->score += points;

        // Check if all players finished
        bool allFinished = true;
        for (const auto& [pid, pdata] : match->participantData) {
            if (pdata->lapsCompleted < match->raceSettings.laps) {
                allFinished = false;
                break;
            }
        }

        if (allFinished) {
            EndMatch(matchId);
        }

        match->syncVersion++;
        BroadcastMatchState(matchId);
    }

    void CompetitiveGameModeManager::OnPlayerKilled(const std::string& matchId, uint32_t killerId, uint32_t victimId, const std::string& weaponType)
    {
        auto* match = GetMatch(matchId);
        if (!match || match->gameMode != CompetitiveMode::Arena) {
            return;
        }

        auto* killer = FindParticipant(match, killerId);
        auto* victim = FindParticipant(match, victimId);

        if (killer) {
            killer->kills++;
            killer->killStreak++;
            killer->score += 100; // 100 points per kill

            if (killer->killStreak > killer->bestKillStreak) {
                killer->bestKillStreak = killer->killStreak;
            }
        }

        if (victim) {
            victim->deaths++;
            victim->killStreak = 0;
            victim->isAlive = false;
            victim->respawnTime = match->settings.respawnTime;
        }

        // Check win conditions
        bool matchWon = CheckArenaWinCondition(match);
        if (matchWon) {
            EndMatch(matchId);
        }

        match->syncVersion++;

        // Notify listeners
        NotifyPlayerKilled(matchId, killerId, victimId);

        BroadcastMatchState(matchId);
    }

    void CompetitiveGameModeManager::OnPlayerAssist(const std::string& matchId, uint32_t assisterId, uint32_t killerId, uint32_t victimId)
    {
        auto* match = GetMatch(matchId);
        if (!match || match->gameMode != CompetitiveMode::Arena) {
            return;
        }

        auto* assister = FindParticipant(match, assisterId);
        if (assister) {
            assister->assists++;
            assister->score += 50; // 50 points per assist
        }

        match->syncVersion++;
        BroadcastMatchState(matchId);
    }

    bool CompetitiveGameModeManager::SpawnPowerup(const std::string& matchId, PowerupType powerupType, float x, float y, float z)
    {
        auto* match = GetMatch(matchId);
        if (!match || match->gameMode != CompetitiveMode::Arena) {
            return false;
        }

        if (!match->arenaSettings.enablePowerups) {
            return false;
        }

        // Notify listeners
        NotifyPowerupSpawned(matchId, powerupType, x, y, z);

        return true;
    }

    void CompetitiveGameModeManager::OnPowerupCollected(const std::string& matchId, uint32_t playerId, PowerupType powerupType)
    {
        auto* match = GetMatch(matchId);
        if (!match || match->gameMode != CompetitiveMode::Arena) {
            return;
        }

        auto* participant = FindParticipant(match, playerId);
        if (!participant) {
            return;
        }

        participant->activePowerups.push_back(powerupType);
        match->syncVersion++;

        BroadcastMatchState(matchId);
    }

    bool CompetitiveGameModeManager::AssignPlayerToTeam(const std::string& matchId, uint32_t playerId, Team teamId)
    {
        auto* match = GetMatch(matchId);
        if (!match) {
            return false;
        }

        auto* participant = FindParticipant(match, playerId);
        if (!participant) {
            return false;
        }

        participant->team = teamId;
        match->syncVersion++;

        BroadcastMatchState(matchId);
        return true;
    }

    CompetitiveMatch* CompetitiveGameModeManager::GetMatch(const std::string& matchId)
    {
        std::shared_lock<std::shared_mutex> lock(m_matchesMutex);

        auto it = m_matches.find(matchId);
        return (it != m_matches.end()) ? it->second.get() : nullptr;
    }

    const CompetitiveMatch* CompetitiveGameModeManager::GetMatch(const std::string& matchId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_matchesMutex);

        auto it = m_matches.find(matchId);
        return (it != m_matches.end()) ? it->second.get() : nullptr;
    }

    std::vector<std::string> CompetitiveGameModeManager::GetActiveMatches() const
    {
        std::shared_lock<std::shared_mutex> lock(m_matchesMutex);

        std::vector<std::string> activeMatches;
        for (const auto& [matchId, match] : m_matches) {
            if (match->state == MatchState::Waiting ||
                match->state == MatchState::Starting ||
                match->state == MatchState::InProgress) {
                activeMatches.push_back(matchId);
            }
        }

        return activeMatches;
    }

    void CompetitiveGameModeManager::BroadcastMatchState(const std::string& matchId)
    {
        auto* match = GetMatch(matchId);
        if (!match) {
            return;
        }

        SendMatchStateToParticipants(match);
    }

    // Private implementation methods
    std::string CompetitiveGameModeManager::GenerateMatchId()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);

        std::stringstream ss;
        ss << "match_";
        for (int i = 0; i < 8; ++i) {
            ss << std::hex << dis(gen);
        }

        return ss.str();
    }

    bool CompetitiveGameModeManager::ValidateMatchSettings(CompetitiveMode gameMode, const CompetitiveMatchSettings& settings) const
    {
        if (settings.maxPlayers <= 0 || settings.maxPlayers > 32) {
            return false;
        }

        if (settings.matchDuration <= 0.0f || settings.matchDuration > 3600.0f) {
            return false;
        }

        return true;
    }

    bool CompetitiveGameModeManager::CanPlayerJoinMatch(uint32_t playerId, const std::string& matchId) const
    {
        // Additional validation logic would go here
        return true;
    }

    int32_t CompetitiveGameModeManager::GetMinimumPlayers(CompetitiveMode gameMode) const
    {
        switch (gameMode) {
            case CompetitiveMode::Racing:
                return 2;
            case CompetitiveMode::Arena:
                return 4;
            case CompetitiveMode::Custom:
                return 1;
            default:
                return 2;
        }
    }

    void CompetitiveGameModeManager::CleanupInactiveMatches()
    {
        std::unique_lock<std::shared_mutex> lock(m_matchesMutex);

        auto currentTime = std::chrono::steady_clock::now();
        std::vector<std::string> matchesToRemove;

        for (const auto& [matchId, match] : m_matches) {
            // Remove matches that have been inactive for more than 1 hour
            auto inactiveDuration = std::chrono::duration_cast<std::chrono::hours>(
                currentTime - match->lastUpdate).count();

            if (inactiveDuration >= 1 && (match->state == MatchState::Finished ||
                                         match->state == MatchState::Cancelled)) {
                matchesToRemove.push_back(matchId);
            }
        }

        for (const auto& matchId : matchesToRemove) {
            m_matches.erase(matchId);
        }
    }

    void CompetitiveGameModeManager::UpdateMatchLogic(CompetitiveMatch* match)
    {
        if (!match) {
            return;
        }

        auto currentTime = std::chrono::steady_clock::now();
        auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - match->lastUpdate).count() / 1000.0f;

        match->duration += deltaTime;
        match->lastUpdate = currentTime;

        // Check time limit
        if (match->duration >= match->settings.matchDuration) {
            EndMatch(match->matchId);
            return;
        }

        // Update game mode specific logic
        switch (match->gameMode) {
            case CompetitiveMode::Racing:
                UpdateRaceLogic(match);
                break;
            case CompetitiveMode::Arena:
                UpdateArenaLogic(match);
                break;
            default:
                break;
        }
    }

    void CompetitiveGameModeManager::UpdateRaceLogic(CompetitiveMatch* match)
    {
        // Update race-specific logic
        // Check for DNFs, penalty times, etc.
    }

    void CompetitiveGameModeManager::UpdateArenaLogic(CompetitiveMatch* match)
    {
        // Update arena-specific logic
        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [playerId, participant] : match->participantData) {
            // Handle respawn timers
            if (!participant->isAlive && participant->respawnTime > 0.0f) {
                participant->respawnTime -= 0.1f; // Assuming 100ms update interval
                if (participant->respawnTime <= 0.0f) {
                    participant->isAlive = true;
                    participant->respawnTime = 0.0f;
                }
            }

            // Update activity
            participant->lastActivity = currentTime;
        }
    }

    bool CompetitiveGameModeManager::CheckArenaWinCondition(const CompetitiveMatch* match) const
    {
        if (!match) {
            return false;
        }

        switch (match->arenaSettings.arenaType) {
            case ArenaType::Deathmatch:
                return CheckDeathMatchWin(match);
            case ArenaType::TeamDeathmatch:
                return CheckTeamDeathMatchWin(match);
            default:
                return false;
        }
    }

    bool CompetitiveGameModeManager::CheckDeathMatchWin(const CompetitiveMatch* match) const
    {
        for (const auto& [playerId, participant] : match->participantData) {
            if (participant->kills >= match->arenaSettings.killLimit) {
                return true;
            }
        }
        return false;
    }

    bool CompetitiveGameModeManager::CheckTeamDeathMatchWin(const CompetitiveMatch* match) const
    {
        std::unordered_map<Team, int32_t> teamKills;

        for (const auto& [playerId, participant] : match->participantData) {
            teamKills[participant->team] += participant->kills;
        }

        for (const auto& [team, kills] : teamKills) {
            if (kills >= match->arenaSettings.killLimit) {
                return true;
            }
        }

        return false;
    }

    CompetitiveParticipant* CompetitiveGameModeManager::FindParticipant(CompetitiveMatch* match, uint32_t playerId)
    {
        if (!match) {
            return nullptr;
        }

        auto it = match->participantData.find(playerId);
        return (it != match->participantData.end()) ? it->second.get() : nullptr;
    }

    const CompetitiveParticipant* CompetitiveGameModeManager::FindParticipant(const CompetitiveMatch* match, uint32_t playerId) const
    {
        if (!match) {
            return nullptr;
        }

        auto it = match->participantData.find(playerId);
        return (it != match->participantData.end()) ? it->second.get() : nullptr;
    }

    void CompetitiveGameModeManager::SendMatchStateToParticipants(const CompetitiveMatch* match)
    {
        // This would send network messages to all participants
        // Implementation would depend on the networking system
    }

    void CompetitiveGameModeManager::NotifyMatchStarted(const std::string& matchId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_matchStartedCallback) {
            m_matchStartedCallback(matchId);
        }
    }

    void CompetitiveGameModeManager::NotifyMatchEnded(const std::string& matchId, bool successful)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_matchEndedCallback) {
            m_matchEndedCallback(matchId, successful);
        }
    }

    void CompetitiveGameModeManager::NotifyPlayerJoined(const std::string& matchId, uint32_t playerId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_playerJoinedCallback) {
            m_playerJoinedCallback(matchId, playerId);
        }
    }

    void CompetitiveGameModeManager::NotifyPlayerKilled(const std::string& matchId, uint32_t killerId, uint32_t victimId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_playerKilledCallback) {
            m_playerKilledCallback(matchId, killerId, victimId);
        }
    }

    void CompetitiveGameModeManager::NotifyPowerupSpawned(const std::string& matchId, PowerupType type, float x, float y, float z)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_powerupSpawnedCallback) {
            m_powerupSpawnedCallback(matchId, type, x, y, z);
        }
    }

    // Utility functions implementation
    namespace CompetitiveUtils
    {
        std::string CompetitiveModeToString(CompetitiveMode mode)
        {
            switch (mode) {
                case CompetitiveMode::None: return "None";
                case CompetitiveMode::Racing: return "Racing";
                case CompetitiveMode::Arena: return "Arena";
                case CompetitiveMode::Custom: return "Custom";
                default: return "Unknown";
            }
        }

        std::string MatchStateToString(MatchState state)
        {
            switch (state) {
                case MatchState::Waiting: return "Waiting";
                case MatchState::Starting: return "Starting";
                case MatchState::InProgress: return "InProgress";
                case MatchState::Paused: return "Paused";
                case MatchState::Finished: return "Finished";
                case MatchState::Cancelled: return "Cancelled";
                default: return "Unknown";
            }
        }

        std::string PowerupTypeToString(PowerupType type)
        {
            switch (type) {
                case PowerupType::HealthBoost: return "HealthBoost";
                case PowerupType::ArmorBoost: return "ArmorBoost";
                case PowerupType::DamageBoost: return "DamageBoost";
                case PowerupType::SpeedBoost: return "SpeedBoost";
                case PowerupType::InfiniteAmmo: return "InfiniteAmmo";
                case PowerupType::Invisibility: return "Invisibility";
                case PowerupType::DoubleScore: return "DoubleScore";
                case PowerupType::QuadDamage: return "QuadDamage";
                default: return "Unknown";
            }
        }

        bool ValidateMatchId(const std::string& matchId)
        {
            return !matchId.empty() && matchId.length() <= 32 &&
                   matchId.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") == std::string::npos;
        }

        float CalculateMatchProgress(const CompetitiveMatch* match)
        {
            if (!match || match->settings.matchDuration <= 0.0f) {
                return 0.0f;
            }

            return std::min(1.0f, match->duration / match->settings.matchDuration);
        }
    }

    // Missing CompetitiveGameModeManager method implementation
    void CompetitiveGameModeManager::SyncMatchToPlayer(const std::string& matchId, uint32_t playerId)
    {
        auto* match = GetMatch(matchId);
        if (!match) {
            return;
        }

        // Send match state to specific player
        // This would normally send a network message with match information
        // For now, we'll just broadcast to all participants as a fallback
        BroadcastMatchState(matchId);
    }
}
#include "FreeRoamFrameworkManager.hpp"
#include "RED4ext/Api/Sdk.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace RED4ext
{
    // FreeRoamFrameworkManager Implementation
    FreeRoamFrameworkManager& FreeRoamFrameworkManager::GetInstance()
    {
        static FreeRoamFrameworkManager instance;
        return instance;
    }

    void FreeRoamFrameworkManager::Initialize()
    {
        std::unique_lock<std::shared_mutex> lock(m_sessionsMutex);

        // Clear existing data
        m_sessions.clear();
        m_playerToSession.clear();
        m_sessionsByType.clear();

        // Initialize session type tracking
        m_sessionsByType[SessionTypeToString(SessionType::FreeRoam)] = {};
        m_sessionsByType[SessionTypeToString(SessionType::Cooperative)] = {};
        m_sessionsByType[SessionTypeToString(SessionType::Competitive)] = {};
        m_sessionsByType[SessionTypeToString(SessionType::Custom)] = {};

        // Initialize statistics
        m_totalSessionsCreated = 0;
        m_totalSessionsCompleted = 0;
        m_lastCleanup = std::chrono::steady_clock::now();
    }

    void FreeRoamFrameworkManager::Shutdown()
    {
        // End all active sessions
        std::vector<std::string> activeSessions = GetActiveSessions();
        for (const auto& sessionId : activeSessions) {
            EndFreeRoamSession(sessionId);
        }

        std::unique_lock<std::shared_mutex> lock(m_sessionsMutex);
        m_sessions.clear();
        m_playerToSession.clear();
        m_sessionsByType.clear();

        // Clear callbacks
        std::lock_guard<std::mutex> callbacksLock(m_callbacksMutex);
        m_sessionStartedCallback = nullptr;
        m_sessionEndedCallback = nullptr;
        m_playerJoinedSessionCallback = nullptr;
        m_playerLeftSessionCallback = nullptr;
        m_gameModeEnabledCallback = nullptr;
        m_gameModeDisabledCallback = nullptr;
        m_worldStateUpdatedCallback = nullptr;
    }

    void FreeRoamFrameworkManager::Update()
    {
        auto currentTime = std::chrono::steady_clock::now();

        // Periodic cleanup (every 5 minutes)
        auto timeSinceCleanup = std::chrono::duration_cast<std::chrono::minutes>(
            currentTime - m_lastCleanup).count();

        if (timeSinceCleanup >= 5) {
            CleanupInactiveSessions();
            m_lastCleanup = currentTime;
        }

        // Update all active sessions
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);
        for (auto& [sessionId, session] : m_sessions) {
            if (session->state == SessionState::Active) {
                UpdateSessionConnections(session.get());
                UpdateSessionGameModes(session.get(), 0.05f); // Assume 20 FPS updates
                ValidateSessionState(session.get());
            }
        }
    }

    std::string FreeRoamFrameworkManager::CreateFreeRoamSession(uint32_t hostPlayerId, const FreeRoamSessionSettings& settings)
    {
        // Validate settings
        if (!ValidateSessionSettings(settings)) {
            return "";
        }

        // Check if player can create session
        if (!CanPlayerCreateSession(hostPlayerId)) {
            return "";
        }

        std::string sessionId = GenerateSessionId();

        auto session = std::make_unique<FreeRoamSession>();
        session->sessionId = sessionId;
        session->sessionType = SessionType::FreeRoam;
        session->state = SessionState::Starting;
        session->hostPlayerId = hostPlayerId;
        session->settings = settings;
        session->startTime = std::chrono::steady_clock::now();
        session->lastUpdate = session->startTime;
        session->syncVersion = 1;

        // Initialize world state
        session->worldState.gameTime = 0.0f;
        session->worldState.activePlayers = 0;
        session->worldState.sessionStartTime = 0.0f;
        session->worldState.lastSyncTime = 0.0f;
        session->worldState.syncVersion = 1;

        // Add host as first participant
        session->participants.push_back(hostPlayerId);

        auto hostConnection = std::make_unique<PlayerConnection>();
        hostConnection->playerId = hostPlayerId;
        hostConnection->playerName = "Host"; // Would be retrieved from player system
        hostConnection->isConnected = true;
        hostConnection->connectionQuality = ConnectionQuality::Excellent;

        session->playerConnections[hostPlayerId] = std::move(hostConnection);

        // Store session
        {
            std::unique_lock<std::shared_mutex> lock(m_sessionsMutex);
            m_sessions[sessionId] = std::move(session);
            m_playerToSession[hostPlayerId] = sessionId;
            m_sessionsByType[FreeRoamUtils::SessionTypeToString(SessionType::FreeRoam)].push_back(sessionId);
        }

        m_totalSessionsCreated++;

        // Notify listeners
        NotifySessionStarted(sessionId);

        return sessionId;
    }

    bool FreeRoamFrameworkManager::JoinFreeRoamSession(const std::string& sessionId, uint32_t playerId, const std::string& password)
    {
        std::unique_lock<std::shared_mutex> lock(m_sessionsMutex);

        // Check if player is already in another session
        auto playerIt = m_playerToSession.find(playerId);
        if (playerIt != m_playerToSession.end()) {
            return false;
        }

        auto it = m_sessions.find(sessionId);
        if (it == m_sessions.end()) {
            return false;
        }

        auto& session = it->second;

        // Validate session can accept new players
        if (!CanJoinSession(sessionId, playerId)) {
            return false;
        }

        // Check password if required
        if (session->settings.isPasswordProtected && session->settings.password != password) {
            return false;
        }

        // Check capacity
        if (static_cast<int32_t>(session->participants.size()) >= session->settings.maxPlayers) {
            return false;
        }

        // Add participant
        session->participants.push_back(playerId);
        session->syncVersion++;

        auto connection = std::make_unique<PlayerConnection>();
        connection->playerId = playerId;
        connection->playerName = "Player"; // Would be retrieved from player system
        connection->isConnected = true;
        connection->connectionQuality = ConnectionQuality::Good;

        session->playerConnections[playerId] = std::move(connection);
        session->worldState.activePlayers = static_cast<int32_t>(session->participants.size());

        m_playerToSession[playerId] = sessionId;

        lock.unlock();

        // Sync world state to new player
        SyncWorldStateToPlayer(sessionId, playerId);

        // Notify listeners
        NotifyPlayerJoinedSession(sessionId, playerId, "Player");

        // Broadcast session update
        BroadcastSessionUpdate(sessionId);

        return true;
    }

    bool FreeRoamFrameworkManager::LeaveFreeRoamSession(const std::string& sessionId, uint32_t playerId)
    {
        std::unique_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto it = m_sessions.find(sessionId);
        if (it == m_sessions.end()) {
            return false;
        }

        auto& session = it->second;

        // Remove from participants list
        auto participantIt = std::find(session->participants.begin(), session->participants.end(), playerId);
        if (participantIt == session->participants.end()) {
            return false;
        }

        session->participants.erase(participantIt);
        session->syncVersion++;

        // Remove connection
        session->playerConnections.erase(playerId);
        session->worldState.activePlayers = static_cast<int32_t>(session->participants.size());

        // If removing host, transfer to another participant
        if (session->hostPlayerId == playerId && !session->participants.empty()) {
            session->hostPlayerId = session->participants[0];
        }

        m_playerToSession.erase(playerId);

        std::string playerName = "Player"; // Would be retrieved from connection

        lock.unlock();

        // If no participants left, end session
        if (session->participants.empty()) {
            EndFreeRoamSession(sessionId);
        } else {
            // Broadcast updated state
            BroadcastSessionUpdate(sessionId);
        }

        // Notify listeners
        NotifyPlayerLeftSession(sessionId, playerId, playerName);

        return true;
    }

    bool FreeRoamFrameworkManager::EndFreeRoamSession(const std::string& sessionId)
    {
        std::unique_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto it = m_sessions.find(sessionId);
        if (it == m_sessions.end()) {
            return false;
        }

        auto& session = it->second;

        // Get participants list before cleanup
        std::vector<uint32_t> participants = session->participants;

        // Update session state
        session->state = SessionState::Ended;
        session->lastUpdate = std::chrono::steady_clock::now();

        // Remove players from mapping
        for (uint32_t playerId : participants) {
            m_playerToSession.erase(playerId);
        }

        // Remove from session type tracking
        auto& typeSessions = m_sessionsByType[FreeRoamUtils::SessionTypeToString(session->sessionType)];
        auto typeIt = std::find(typeSessions.begin(), typeSessions.end(), sessionId);
        if (typeIt != typeSessions.end()) {
            typeSessions.erase(typeIt);
        }

        // Remove session
        m_sessions.erase(sessionId);

        lock.unlock();

        m_totalSessionsCompleted++;

        // Notify listeners
        NotifySessionEnded(sessionId);

        return true;
    }

    bool FreeRoamFrameworkManager::EnableGameMode(const std::string& sessionId, GameMode gameMode, uint32_t requesterId)
    {
        auto* session = GetSession(sessionId);
        if (!session) {
            return false;
        }

        // Check permissions
        if (!CanManageGameModes(sessionId, requesterId)) {
            return false;
        }

        // Check if game mode is already enabled
        for (const auto& instance : session->activeGameModes) {
            if (instance->gameMode == gameMode) {
                return false; // Already enabled
            }
        }

        // Create game mode instance
        std::string instanceId = CreateGameModeInstance(sessionId, gameMode);
        if (instanceId.empty()) {
            return false;
        }

        session->syncVersion++;
        session->lastUpdate = std::chrono::steady_clock::now();

        // Notify listeners
        NotifyGameModeEnabled(sessionId, gameMode);

        // Broadcast update
        BroadcastSessionUpdate(sessionId);

        return true;
    }

    bool FreeRoamFrameworkManager::DisableGameMode(const std::string& sessionId, GameMode gameMode, uint32_t requesterId)
    {
        auto* session = GetSession(sessionId);
        if (!session) {
            return false;
        }

        // Check permissions
        if (!CanManageGameModes(sessionId, requesterId)) {
            return false;
        }

        // Find and remove game mode instance
        auto it = std::find_if(session->activeGameModes.begin(), session->activeGameModes.end(),
            [gameMode](const std::unique_ptr<GameModeInstance>& instance) {
                return instance->gameMode == gameMode;
            });

        if (it != session->activeGameModes.end()) {
            session->activeGameModes.erase(it);
            session->syncVersion++;
            session->lastUpdate = std::chrono::steady_clock::now();

            // Notify listeners
            NotifyGameModeDisabled(sessionId, gameMode);

            // Broadcast update
            BroadcastSessionUpdate(sessionId);

            return true;
        }

        return false;
    }

    void FreeRoamFrameworkManager::UpdateWorldState(const std::string& sessionId, const WorldStateData& worldState)
    {
        auto* session = GetSession(sessionId);
        if (!session) {
            return;
        }

        session->worldState = worldState;
        session->worldState.syncVersion++;
        session->lastUpdate = std::chrono::steady_clock::now();

        // Notify listeners
        NotifyWorldStateUpdated(sessionId);

        // Broadcast to all players
        BroadcastWorldStateUpdate(sessionId);
    }

    void FreeRoamFrameworkManager::SyncWorldStateToPlayer(const std::string& sessionId, uint32_t playerId)
    {
        auto* session = GetSession(sessionId);
        if (!session) {
            return;
        }

        SendSessionStateToPlayer(sessionId, playerId);
    }

    void FreeRoamFrameworkManager::UpdatePlayerPosition(uint32_t playerId, float x, float y, float z)
    {
        auto* connection = GetPlayerConnection(playerId);
        if (connection) {
            connection->lastPosX = connection->posX;
            connection->lastPosY = connection->posY;
            connection->lastPosZ = connection->posZ;
            connection->posX = x;
            connection->posY = y;
            connection->posZ = z;
            connection->lastActivity = std::chrono::steady_clock::now();
        }
    }

    void FreeRoamFrameworkManager::UpdatePlayerPing(uint32_t playerId, int32_t ping, float packetLoss)
    {
        auto* connection = GetPlayerConnection(playerId);
        if (connection) {
            connection->ping = ping;
            connection->packetLoss = packetLoss;
            connection->connectionQuality = FreeRoamUtils::PingToConnectionQuality(ping, packetLoss);
            connection->lastPingTime = std::chrono::steady_clock::now();
        }
    }

    FreeRoamSession* FreeRoamFrameworkManager::GetSession(const std::string& sessionId)
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto it = m_sessions.find(sessionId);
        return (it != m_sessions.end()) ? it->second.get() : nullptr;
    }

    const FreeRoamSession* FreeRoamFrameworkManager::GetSession(const std::string& sessionId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto it = m_sessions.find(sessionId);
        return (it != m_sessions.end()) ? it->second.get() : nullptr;
    }

    PlayerConnection* FreeRoamFrameworkManager::GetPlayerConnection(uint32_t playerId)
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto sessionIt = m_playerToSession.find(playerId);
        if (sessionIt == m_playerToSession.end()) {
            return nullptr;
        }

        auto sessionMapIt = m_sessions.find(sessionIt->second);
        if (sessionMapIt == m_sessions.end()) {
            return nullptr;
        }

        auto& session = sessionMapIt->second;
        auto connectionIt = session->playerConnections.find(playerId);
        return (connectionIt != session->playerConnections.end()) ? connectionIt->second.get() : nullptr;
    }

    std::vector<std::string> FreeRoamFrameworkManager::GetActiveSessions() const
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        std::vector<std::string> activeSessions;
        for (const auto& [sessionId, session] : m_sessions) {
            if (session->state == SessionState::Starting ||
                session->state == SessionState::Active) {
                activeSessions.push_back(sessionId);
            }
        }

        return activeSessions;
    }

    bool FreeRoamFrameworkManager::CanJoinSession(const std::string& sessionId, uint32_t playerId) const
    {
        auto* session = GetSession(sessionId);
        if (!session) {
            return false;
        }

        // Check session state
        if (session->state != SessionState::Starting && session->state != SessionState::Active) {
            return false;
        }

        // Check if drop-in is allowed
        if (!session->settings.allowDropIn && session->state == SessionState::Active) {
            return false;
        }

        // Check capacity
        if (static_cast<int32_t>(session->participants.size()) >= session->settings.maxPlayers) {
            return false;
        }

        return true;
    }

    bool FreeRoamFrameworkManager::CanManageGameModes(const std::string& sessionId, uint32_t playerId) const
    {
        auto* session = GetSession(sessionId);
        if (!session) {
            return false;
        }

        // Host can always manage game modes
        if (session->hostPlayerId == playerId) {
            return true;
        }

        // Check if player has game mode management permission
        return HasSessionPermission(sessionId, playerId, "manage_game_modes");
    }

    // Private implementation methods
    std::string FreeRoamFrameworkManager::GenerateSessionId()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);

        std::stringstream ss;
        ss << "session_";
        for (int i = 0; i < 8; ++i) {
            ss << std::hex << dis(gen);
        }

        return ss.str();
    }

    std::string FreeRoamFrameworkManager::GenerateGameModeInstanceId()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);

        std::stringstream ss;
        ss << "gamemode_";
        for (int i = 0; i < 8; ++i) {
            ss << std::hex << dis(gen);
        }

        return ss.str();
    }

    bool FreeRoamFrameworkManager::ValidateSessionSettings(const FreeRoamSessionSettings& settings) const
    {
        if (settings.maxPlayers <= 0 || settings.maxPlayers > 32) {
            return false;
        }

        if (settings.sessionName.empty() || settings.sessionName.length() > 64) {
            return false;
        }

        if (settings.difficultyScaling < 0.1f || settings.difficultyScaling > 10.0f) {
            return false;
        }

        return true;
    }

    bool FreeRoamFrameworkManager::CanPlayerCreateSession(uint32_t playerId) const
    {
        // Check if player is already in another session
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);
        auto it = m_playerToSession.find(playerId);
        return it == m_playerToSession.end();
    }

    void FreeRoamFrameworkManager::CleanupInactiveSessions()
    {
        std::unique_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto currentTime = std::chrono::steady_clock::now();
        std::vector<std::string> sessionsToRemove;

        for (const auto& [sessionId, session] : m_sessions) {
            // Remove sessions that have been inactive for more than 2 hours
            auto inactiveDuration = std::chrono::duration_cast<std::chrono::hours>(
                currentTime - session->lastUpdate).count();

            if (inactiveDuration >= 2 && session->state == SessionState::Ended) {
                sessionsToRemove.push_back(sessionId);
            }
        }

        for (const auto& sessionId : sessionsToRemove) {
            m_sessions.erase(sessionId);
        }
    }

    void FreeRoamFrameworkManager::UpdateSessionConnections(FreeRoamSession* session)
    {
        if (!session) {
            return;
        }

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& [playerId, connection] : session->playerConnections) {
            // Check for player timeout (5 minutes of inactivity)
            auto timeSinceActivity = std::chrono::duration_cast<std::chrono::minutes>(
                currentTime - connection->lastActivity).count();

            if (timeSinceActivity >= 5) {
                connection->isConnected = false;
                connection->connectionQuality = ConnectionQuality::Disconnected;

                // Handle disconnection
                HandlePlayerDisconnection(session->sessionId, playerId);
            }
        }
    }

    void FreeRoamFrameworkManager::HandlePlayerDisconnection(const std::string& sessionId, uint32_t playerId)
    {
        // Remove player from session
        LeaveFreeRoamSession(sessionId, playerId);

        // Check if host disconnected
        auto* session = GetSession(sessionId);
        if (session && session->hostPlayerId == playerId) {
            HandleHostMigration(sessionId);
        }
    }

    void FreeRoamFrameworkManager::HandleHostMigration(const std::string& sessionId)
    {
        auto* session = GetSession(sessionId);
        if (!session || session->participants.empty()) {
            return;
        }

        // Select new host based on connection quality
        uint32_t newHostId = SelectNewHost(sessionId);
        if (newHostId != 0) {
            session->hostPlayerId = newHostId;
            session->syncVersion++;

            // Broadcast host change to all participants
            BroadcastSessionUpdate(sessionId);
        }
    }

    uint32_t FreeRoamFrameworkManager::SelectNewHost(const std::string& sessionId) const
    {
        auto* session = GetSession(sessionId);
        if (!session) {
            return 0;
        }

        // Find participant with best connection quality
        uint32_t bestCandidate = 0;
        ConnectionQuality bestQuality = ConnectionQuality::Disconnected;

        for (const auto& [playerId, connection] : session->playerConnections) {
            if (connection->isConnected && connection->connectionQuality > bestQuality) {
                bestQuality = connection->connectionQuality;
                bestCandidate = playerId;
            }
        }

        return bestCandidate;
    }

    // Notification methods
    void FreeRoamFrameworkManager::NotifySessionStarted(const std::string& sessionId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_sessionStartedCallback) {
            auto* session = GetSession(sessionId);
            if (session) {
                m_sessionStartedCallback(sessionId, session->settings);
            }
        }
    }

    void FreeRoamFrameworkManager::BroadcastSessionUpdate(const std::string& sessionId)
    {
        auto* session = GetSession(sessionId);
        if (!session) {
            return;
        }

        SendSessionStateToParticipants(session);
    }

    void FreeRoamFrameworkManager::SendSessionStateToParticipants(const FreeRoamSession* session)
    {
        // This would send network messages to all participants
        // Implementation would depend on the networking system
    }

    // Missing notification method implementations
    void FreeRoamFrameworkManager::NotifySessionEnded(const std::string& sessionId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_sessionEndedCallback) {
            m_sessionEndedCallback(sessionId);
        }
    }

    void FreeRoamFrameworkManager::NotifyPlayerJoinedSession(const std::string& sessionId, uint32_t playerId, const std::string& playerName)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_playerJoinedSessionCallback) {
            m_playerJoinedSessionCallback(sessionId, playerId, playerName);
        }
    }

    void FreeRoamFrameworkManager::NotifyPlayerLeftSession(const std::string& sessionId, uint32_t playerId, const std::string& playerName)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_playerLeftSessionCallback) {
            m_playerLeftSessionCallback(sessionId, playerId, playerName);
        }
    }

    void FreeRoamFrameworkManager::NotifyGameModeEnabled(const std::string& sessionId, GameMode gameMode)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_gameModeEnabledCallback) {
            m_gameModeEnabledCallback(sessionId, gameMode);
        }
    }

    void FreeRoamFrameworkManager::NotifyGameModeDisabled(const std::string& sessionId, GameMode gameMode)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_gameModeDisabledCallback) {
            m_gameModeDisabledCallback(sessionId, gameMode);
        }
    }

    void FreeRoamFrameworkManager::NotifyWorldStateUpdated(const std::string& sessionId)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        if (m_worldStateUpdatedCallback) {
            auto* session = GetSession(sessionId);
            if (session) {
                m_worldStateUpdatedCallback(sessionId, session->worldState);
            }
        }
    }

    // Missing public method implementations
    bool FreeRoamFrameworkManager::AddPlayerToSession(const std::string& sessionId, uint32_t playerId, const std::string& playerName)
    {
        return JoinFreeRoamSession(sessionId, playerId, "");
    }

    bool FreeRoamFrameworkManager::RemovePlayerFromSession(const std::string& sessionId, uint32_t playerId)
    {
        return LeaveFreeRoamSession(sessionId, playerId);
    }

    void FreeRoamFrameworkManager::UpdatePlayerActivity(uint32_t playerId)
    {
        auto* connection = GetPlayerConnection(playerId);
        if (connection) {
            connection->lastActivity = std::chrono::steady_clock::now();
        }
    }

    std::string FreeRoamFrameworkManager::CreateGameModeInstance(const std::string& sessionId, GameMode gameMode)
    {
        auto* session = GetSession(sessionId);
        if (!session) {
            return "";
        }

        std::string instanceId = GenerateGameModeInstanceId();

        auto instance = std::make_unique<GameModeInstance>();
        instance->instanceId = instanceId;
        instance->gameMode = gameMode;
        instance->isActive = true;
        instance->startTime = std::chrono::steady_clock::now();
        instance->lastUpdate = instance->startTime;

        session->activeGameModes.push_back(std::move(instance));

        return instanceId;
    }

    bool FreeRoamFrameworkManager::RemoveGameModeInstance(const std::string& sessionId, const std::string& instanceId)
    {
        auto* session = GetSession(sessionId);
        if (!session) {
            return false;
        }

        auto it = std::find_if(session->activeGameModes.begin(), session->activeGameModes.end(),
            [&instanceId](const std::unique_ptr<GameModeInstance>& instance) {
                return instance->instanceId == instanceId;
            });

        if (it != session->activeGameModes.end()) {
            session->activeGameModes.erase(it);
            return true;
        }

        return false;
    }

    void FreeRoamFrameworkManager::SyncWorldStateToAllPlayers(const std::string& sessionId)
    {
        auto* session = GetSession(sessionId);
        if (!session) {
            return;
        }

        for (uint32_t playerId : session->participants) {
            SyncWorldStateToPlayer(sessionId, playerId);
        }
    }

    void FreeRoamFrameworkManager::BroadcastWorldStateUpdate(const std::string& sessionId)
    {
        auto* session = GetSession(sessionId);
        if (!session) {
            return;
        }

        SendWorldStateToParticipants(session);
    }

    std::string FreeRoamFrameworkManager::FindSessionByPlayer(uint32_t playerId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        auto it = m_playerToSession.find(playerId);
        return (it != m_playerToSession.end()) ? it->second : "";
    }

    std::vector<std::string> FreeRoamFrameworkManager::GetPublicSessions() const
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        std::vector<std::string> publicSessions;
        for (const auto& [sessionId, session] : m_sessions) {
            if (!session->settings.isPrivate &&
                (session->state == SessionState::Starting || session->state == SessionState::Active)) {
                publicSessions.push_back(sessionId);
            }
        }

        return publicSessions;
    }

    const PlayerConnection* FreeRoamFrameworkManager::GetPlayerConnection(uint32_t playerId) const
    {
        return const_cast<FreeRoamFrameworkManager*>(this)->GetPlayerConnection(playerId);
    }

    std::vector<uint32_t> FreeRoamFrameworkManager::GetSessionParticipants(const std::string& sessionId) const
    {
        auto* session = GetSession(sessionId);
        return session ? session->participants : std::vector<uint32_t>();
    }

    int32_t FreeRoamFrameworkManager::GetSessionPlayerCount(const std::string& sessionId) const
    {
        auto* session = GetSession(sessionId);
        return session ? static_cast<int32_t>(session->participants.size()) : 0;
    }

    bool FreeRoamFrameworkManager::TransferHost(const std::string& sessionId, uint32_t newHostId)
    {
        auto* session = GetSession(sessionId);
        if (!session) {
            return false;
        }

        // Check if new host is in the session
        auto it = std::find(session->participants.begin(), session->participants.end(), newHostId);
        if (it == session->participants.end()) {
            return false;
        }

        session->hostPlayerId = newHostId;
        session->syncVersion++;
        session->lastUpdate = std::chrono::steady_clock::now();

        BroadcastSessionUpdate(sessionId);
        return true;
    }

    bool FreeRoamFrameworkManager::IsHost(const std::string& sessionId, uint32_t playerId) const
    {
        auto* session = GetSession(sessionId);
        return session && session->hostPlayerId == playerId;
    }

    bool FreeRoamFrameworkManager::HasSessionPermission(const std::string& sessionId, uint32_t playerId, const std::string& permission) const
    {
        // Simplified permission check - host has all permissions
        return IsHost(sessionId, playerId);
    }

    uint32_t FreeRoamFrameworkManager::GetActiveSessionCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        uint32_t count = 0;
        for (const auto& [sessionId, session] : m_sessions) {
            if (session->state == SessionState::Starting || session->state == SessionState::Active) {
                count++;
            }
        }
        return count;
    }

    uint32_t FreeRoamFrameworkManager::GetTotalParticipants() const
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        uint32_t total = 0;
        for (const auto& [sessionId, session] : m_sessions) {
            if (session->state == SessionState::Starting || session->state == SessionState::Active) {
                total += static_cast<uint32_t>(session->participants.size());
            }
        }
        return total;
    }

    std::chrono::milliseconds FreeRoamFrameworkManager::GetAverageSessionDuration() const
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        if (m_sessions.empty()) {
            return std::chrono::milliseconds(0);
        }

        auto totalDuration = std::chrono::milliseconds(0);
        uint32_t activeSessions = 0;

        auto currentTime = std::chrono::steady_clock::now();
        for (const auto& [sessionId, session] : m_sessions) {
            if (session->state == SessionState::Starting || session->state == SessionState::Active) {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - session->startTime);
                totalDuration += duration;
                activeSessions++;
            }
        }

        return activeSessions > 0 ? totalDuration / activeSessions : std::chrono::milliseconds(0);
    }

    std::unordered_map<SessionType, uint32_t> FreeRoamFrameworkManager::GetSessionDistribution() const
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionsMutex);

        std::unordered_map<SessionType, uint32_t> distribution;
        distribution[SessionType::FreeRoam] = 0;
        distribution[SessionType::Cooperative] = 0;
        distribution[SessionType::Competitive] = 0;
        distribution[SessionType::Custom] = 0;

        for (const auto& [sessionId, session] : m_sessions) {
            if (session->state == SessionState::Starting || session->state == SessionState::Active) {
                distribution[session->sessionType]++;
            }
        }

        return distribution;
    }

    // Callback setters
    void FreeRoamFrameworkManager::SetSessionStartedCallback(SessionStartedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_sessionStartedCallback = callback;
    }

    void FreeRoamFrameworkManager::SetSessionEndedCallback(SessionEndedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_sessionEndedCallback = callback;
    }

    void FreeRoamFrameworkManager::SetPlayerJoinedSessionCallback(PlayerJoinedSessionCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_playerJoinedSessionCallback = callback;
    }

    void FreeRoamFrameworkManager::SetPlayerLeftSessionCallback(PlayerLeftSessionCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_playerLeftSessionCallback = callback;
    }

    void FreeRoamFrameworkManager::SetGameModeEnabledCallback(GameModeEnabledCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_gameModeEnabledCallback = callback;
    }

    void FreeRoamFrameworkManager::SetGameModeDisabledCallback(GameModeDisabledCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_gameModeDisabledCallback = callback;
    }

    void FreeRoamFrameworkManager::SetWorldStateUpdatedCallback(WorldStateUpdatedCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_worldStateUpdatedCallback = callback;
    }

    void FreeRoamFrameworkManager::SendSessionStateToPlayer(const std::string& sessionId, uint32_t playerId)
    {
        // This would send network messages to a specific player
        // Implementation would depend on the networking system
    }

    void FreeRoamFrameworkManager::NotifyGameModeUpdate(const std::string& sessionId, GameMode gameMode, bool enabled)
    {
        // This would broadcast game mode updates to clients
        // Implementation would depend on the networking system
    }

    void FreeRoamFrameworkManager::NotifyPlayerUpdate(const std::string& sessionId, uint32_t playerId)
    {
        // This would broadcast player state updates to clients
        // Implementation would depend on the networking system
    }

    void FreeRoamFrameworkManager::UpdateSessionGameModes(FreeRoamSession* session, float deltaTime)
    {
        if (!session) {
            return;
        }

        auto currentTime = std::chrono::steady_clock::now();

        for (auto& instance : session->activeGameModes) {
            instance->lastUpdate = currentTime;
            // Additional game mode update logic would go here
        }
    }

    void FreeRoamFrameworkManager::ValidateSessionState(FreeRoamSession* session)
    {
        if (!session) {
            return;
        }

        // Validate participant consistency
        for (auto it = session->participants.begin(); it != session->participants.end();) {
            uint32_t playerId = *it;
            auto connectionIt = session->playerConnections.find(playerId);
            if (connectionIt == session->playerConnections.end() || !connectionIt->second->isConnected) {
                it = session->participants.erase(it);
            } else {
                ++it;
            }
        }

        // Update world state player count
        session->worldState.activePlayers = static_cast<int32_t>(session->participants.size());
    }

    void FreeRoamFrameworkManager::SendWorldStateToParticipants(const FreeRoamSession* session)
    {
        // This would send world state updates to all session participants
        // Implementation would depend on the networking system
    }

    void FreeRoamFrameworkManager::SendGameModeUpdateToParticipants(const FreeRoamSession* session, GameMode gameMode, bool enabled)
    {
        // This would send game mode updates to all session participants
        // Implementation would depend on the networking system
    }

    // Utility functions implementation
    namespace FreeRoamUtils
    {
        std::string SessionTypeToString(SessionType type)
        {
            switch (type) {
                case SessionType::FreeRoam: return "FreeRoam";
                case SessionType::Cooperative: return "Cooperative";
                case SessionType::Competitive: return "Competitive";
                case SessionType::Custom: return "Custom";
                default: return "Unknown";
            }
        }

        std::string GameModeToString(GameMode mode)
        {
            switch (mode) {
                case GameMode::Racing: return "Racing";
                case GameMode::Combat: return "Combat";
                case GameMode::Exploration: return "Exploration";
                case GameMode::Cooperative: return "Cooperative";
                case GameMode::Competitive: return "Competitive";
                default: return "Unknown";
            }
        }

        ConnectionQuality PingToConnectionQuality(int32_t ping, float packetLoss)
        {
            if (packetLoss > 10.0f) {
                return ConnectionQuality::Poor;
            }

            if (ping < 50) {
                return ConnectionQuality::Excellent;
            } else if (ping < 100) {
                return ConnectionQuality::Good;
            } else if (ping < 200) {
                return ConnectionQuality::Fair;
            } else {
                return ConnectionQuality::Poor;
            }
        }

        std::string SessionStateToString(SessionState state)
        {
            switch (state) {
                case SessionState::Inactive: return "Inactive";
                case SessionState::Starting: return "Starting";
                case SessionState::Active: return "Active";
                case SessionState::Pausing: return "Pausing";
                case SessionState::Paused: return "Paused";
                case SessionState::Ending: return "Ending";
                case SessionState::Ended: return "Ended";
                default: return "Unknown";
            }
        }

        bool ValidateSessionId(const std::string& sessionId)
        {
            return !sessionId.empty() && sessionId.length() <= 32 &&
                   sessionId.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") == std::string::npos;
        }

        float CalculateSessionUptime(const FreeRoamSession& session)
        {
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - session.startTime);
            return static_cast<float>(duration.count());
        }

        // Additional missing utility functions
        SessionType StringToSessionType(const std::string& typeStr)
        {
            if (typeStr == "FreeRoam") return SessionType::FreeRoam;
            if (typeStr == "Cooperative") return SessionType::Cooperative;
            if (typeStr == "Competitive") return SessionType::Competitive;
            if (typeStr == "Custom") return SessionType::Custom;
            return SessionType::FreeRoam; // Default
        }

        GameMode StringToGameMode(const std::string& modeStr)
        {
            if (modeStr == "Racing") return GameMode::Racing;
            if (modeStr == "Combat") return GameMode::Combat;
            if (modeStr == "Exploration") return GameMode::Exploration;
            if (modeStr == "Cooperative") return GameMode::Cooperative;
            if (modeStr == "Competitive") return GameMode::Competitive;
            return GameMode::Racing; // Default
        }

        std::string ConnectionQualityToString(ConnectionQuality quality)
        {
            switch (quality) {
                case ConnectionQuality::Excellent: return "Excellent";
                case ConnectionQuality::Good: return "Good";
                case ConnectionQuality::Fair: return "Fair";
                case ConnectionQuality::Poor: return "Poor";
                case ConnectionQuality::Disconnected: return "Disconnected";
                default: return "Unknown";
            }
        }

        bool ValidatePlayerName(const std::string& playerName)
        {
            return !playerName.empty() && playerName.length() <= 32 &&
                   playerName.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_ -") == std::string::npos;
        }

        bool ValidatePassword(const std::string& password)
        {
            return password.length() <= 64; // Allow empty passwords
        }

        uint32_t CalculateSessionLoad(const FreeRoamSession& session)
        {
            return static_cast<uint32_t>((static_cast<float>(session.participants.size()) / static_cast<float>(session.settings.maxPlayers)) * 100.0f);
        }

        bool ShouldMigrateHost(const FreeRoamSession& session, uint32_t currentHostId)
        {
            auto it = session.playerConnections.find(currentHostId);
            if (it == session.playerConnections.end()) {
                return true; // Host not found, definitely migrate
            }

            const auto& hostConnection = it->second;
            return !hostConnection->isConnected || hostConnection->connectionQuality == ConnectionQuality::Poor;
        }
    }
}
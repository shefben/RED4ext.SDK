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
    // Session and game mode enums
    enum class SessionType : uint8_t
    {
        FreeRoam = 0,
        Cooperative = 1,
        Competitive = 2,
        Custom = 3
    };

    // Utility function to convert SessionType to string
    inline std::string SessionTypeToString(SessionType type) {
        switch(type) {
            case SessionType::FreeRoam: return "FreeRoam";
            case SessionType::Cooperative: return "Cooperative";
            case SessionType::Competitive: return "Competitive";
            case SessionType::Custom: return "Custom";
            default: return "Unknown";
        }
    }

    enum class GameMode : uint8_t
    {
        Racing = 0,
        Combat = 1,
        Exploration = 2,
        Cooperative = 3,
        Competitive = 4
    };

    enum class ConnectionQuality : uint8_t
    {
        Excellent = 0,
        Good = 1,
        Fair = 2,
        Poor = 3,
        Disconnected = 4
    };

    enum class EconomyMode : uint8_t
    {
        Individual = 0,
        Shared = 1,
        Pooled = 2
    };

    enum class ProgressMode : uint8_t
    {
        Individual = 0,
        Shared = 1,
        HostOnly = 2
    };

    enum class SessionState : uint8_t
    {
        Inactive = 0,
        Starting = 1,
        Active = 2,
        Pausing = 3,
        Paused = 4,
        Ending = 5,
        Ended = 6
    };

    // Data structures
    struct FreeRoamSessionSettings
    {
        std::string sessionName;
        std::string description;
        int32_t maxPlayers;
        bool allowDropIn;
        bool allowDropOut;
        bool isPasswordProtected;
        bool isPrivate;
        std::string password;
        bool persistentWorld;
        bool syncAllSingleplayerContent;
        bool allowCustomLocations;
        bool allowWorldEvents;
        bool voiceChatEnabled;
        bool textChatEnabled;
        bool crossPlatformEnabled;
        float difficultyScaling;
        EconomyMode economySharing;
        ProgressMode progressSharing;

        FreeRoamSessionSettings()
            : maxPlayers(8), allowDropIn(true), allowDropOut(true),
              isPasswordProtected(false), isPrivate(false), persistentWorld(true),
              syncAllSingleplayerContent(true), allowCustomLocations(true),
              allowWorldEvents(true), voiceChatEnabled(true), textChatEnabled(true),
              crossPlatformEnabled(true), difficultyScaling(1.0f),
              economySharing(EconomyMode::Individual), progressSharing(ProgressMode::Individual) {}
    };

    struct WorldStateData
    {
        float gameTime;
        std::string weatherState;
        float timeScale;
        int32_t activePlayers;
        float sessionStartTime;
        float lastSyncTime;
        uint32_t syncVersion;

        WorldStateData()
            : gameTime(0.0f), timeScale(1.0f), activePlayers(0),
              sessionStartTime(0.0f), lastSyncTime(0.0f), syncVersion(0) {}
    };

    struct PlayerConnection
    {
        uint32_t playerId;
        std::string playerName;
        std::chrono::steady_clock::time_point connectionTime;
        std::chrono::steady_clock::time_point lastPingTime;
        std::chrono::steady_clock::time_point lastActivity;
        int32_t ping;
        bool isConnected;
        ConnectionQuality connectionQuality;
        float packetLoss;

        // Position tracking
        float posX, posY, posZ;
        float lastPosX, lastPosY, lastPosZ;

        PlayerConnection()
            : playerId(0), connectionTime(std::chrono::steady_clock::now()),
              lastPingTime(std::chrono::steady_clock::now()),
              lastActivity(std::chrono::steady_clock::now()), ping(0),
              isConnected(false), connectionQuality(ConnectionQuality::Good),
              packetLoss(0.0f), posX(0.0f), posY(0.0f), posZ(0.0f),
              lastPosX(0.0f), lastPosY(0.0f), lastPosZ(0.0f) {}
    };

    struct GameModeInstance
    {
        std::string instanceId;
        GameMode gameMode;
        bool isActive;
        std::vector<uint32_t> participants;
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point lastUpdate;
        std::unordered_map<std::string, float> modeParameters;

        GameModeInstance()
            : gameMode(GameMode::Racing), isActive(false),
              startTime(std::chrono::steady_clock::now()),
              lastUpdate(std::chrono::steady_clock::now()) {}
    };

    struct FreeRoamSession
    {
        std::string sessionId;
        std::string roomId;
        SessionType sessionType;
        SessionState state;
        uint32_t hostPlayerId;
        FreeRoamSessionSettings settings;
        WorldStateData worldState;

        std::vector<uint32_t> participants;
        std::unordered_map<uint32_t, std::unique_ptr<PlayerConnection>> playerConnections;
        std::vector<std::unique_ptr<GameModeInstance>> activeGameModes;

        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point lastUpdate;
        uint32_t syncVersion;

        FreeRoamSession()
            : sessionType(SessionType::FreeRoam), state(SessionState::Inactive),
              hostPlayerId(0), startTime(std::chrono::steady_clock::now()),
              lastUpdate(std::chrono::steady_clock::now()), syncVersion(0) {}
    };

    // Main free roam framework manager
    class FreeRoamFrameworkManager
    {
    public:
        static FreeRoamFrameworkManager& GetInstance();

        // Lifecycle
        void Initialize();
        void Shutdown();
        void Update();

        // Session management
        std::string CreateFreeRoamSession(uint32_t hostPlayerId, const FreeRoamSessionSettings& settings);
        bool JoinFreeRoamSession(const std::string& sessionId, uint32_t playerId, const std::string& password = "");
        bool LeaveFreeRoamSession(const std::string& sessionId, uint32_t playerId);
        bool EndFreeRoamSession(const std::string& sessionId);

        // Player management
        bool AddPlayerToSession(const std::string& sessionId, uint32_t playerId, const std::string& playerName);
        bool RemovePlayerFromSession(const std::string& sessionId, uint32_t playerId);
        void UpdatePlayerPosition(uint32_t playerId, float x, float y, float z);
        void UpdatePlayerActivity(uint32_t playerId);
        void UpdatePlayerPing(uint32_t playerId, int32_t ping, float packetLoss);

        // Game mode management
        bool EnableGameMode(const std::string& sessionId, GameMode gameMode, uint32_t requesterId);
        bool DisableGameMode(const std::string& sessionId, GameMode gameMode, uint32_t requesterId);
        std::string CreateGameModeInstance(const std::string& sessionId, GameMode gameMode);
        bool RemoveGameModeInstance(const std::string& sessionId, const std::string& instanceId);

        // World state synchronization
        void UpdateWorldState(const std::string& sessionId, const WorldStateData& worldState);
        void SyncWorldStateToPlayer(const std::string& sessionId, uint32_t playerId);
        void SyncWorldStateToAllPlayers(const std::string& sessionId);
        void BroadcastWorldStateUpdate(const std::string& sessionId);

        // Session queries
        FreeRoamSession* GetSession(const std::string& sessionId);
        const FreeRoamSession* GetSession(const std::string& sessionId) const;
        std::string FindSessionByPlayer(uint32_t playerId) const;
        std::vector<std::string> GetActiveSessions() const;
        std::vector<std::string> GetPublicSessions() const;

        // Player queries
        PlayerConnection* GetPlayerConnection(uint32_t playerId);
        const PlayerConnection* GetPlayerConnection(uint32_t playerId) const;
        std::vector<uint32_t> GetSessionParticipants(const std::string& sessionId) const;
        int32_t GetSessionPlayerCount(const std::string& sessionId) const;

        // Host management
        bool TransferHost(const std::string& sessionId, uint32_t newHostId);
        bool IsHost(const std::string& sessionId, uint32_t playerId) const;
        uint32_t SelectNewHost(const std::string& sessionId) const;

        // Permission system integration
        bool HasSessionPermission(const std::string& sessionId, uint32_t playerId, const std::string& permission) const;
        bool CanJoinSession(const std::string& sessionId, uint32_t playerId) const;
        bool CanManageGameModes(const std::string& sessionId, uint32_t playerId) const;

        // Statistics and monitoring
        uint32_t GetActiveSessionCount() const;
        uint32_t GetTotalParticipants() const;
        std::chrono::milliseconds GetAverageSessionDuration() const;
        std::unordered_map<SessionType, uint32_t> GetSessionDistribution() const;

        // Event callbacks
        using SessionStartedCallback = std::function<void(const std::string& sessionId, const FreeRoamSessionSettings& settings)>;
        using SessionEndedCallback = std::function<void(const std::string& sessionId)>;
        using PlayerJoinedSessionCallback = std::function<void(const std::string& sessionId, uint32_t playerId, const std::string& playerName)>;
        using PlayerLeftSessionCallback = std::function<void(const std::string& sessionId, uint32_t playerId, const std::string& playerName)>;
        using GameModeEnabledCallback = std::function<void(const std::string& sessionId, GameMode gameMode)>;
        using GameModeDisabledCallback = std::function<void(const std::string& sessionId, GameMode gameMode)>;
        using WorldStateUpdatedCallback = std::function<void(const std::string& sessionId, const WorldStateData& worldState)>;

        void SetSessionStartedCallback(SessionStartedCallback callback);
        void SetSessionEndedCallback(SessionEndedCallback callback);
        void SetPlayerJoinedSessionCallback(PlayerJoinedSessionCallback callback);
        void SetPlayerLeftSessionCallback(PlayerLeftSessionCallback callback);
        void SetGameModeEnabledCallback(GameModeEnabledCallback callback);
        void SetGameModeDisabledCallback(GameModeDisabledCallback callback);
        void SetWorldStateUpdatedCallback(WorldStateUpdatedCallback callback);

        // Network synchronization
        void BroadcastSessionUpdate(const std::string& sessionId);
        void SendSessionStateToPlayer(const std::string& sessionId, uint32_t playerId);
        void NotifyGameModeUpdate(const std::string& sessionId, GameMode gameMode, bool enabled);
        void NotifyPlayerUpdate(const std::string& sessionId, uint32_t playerId);

    private:
        FreeRoamFrameworkManager() = default;
        ~FreeRoamFrameworkManager() = default;
        FreeRoamFrameworkManager(const FreeRoamFrameworkManager&) = delete;
        FreeRoamFrameworkManager& operator=(const FreeRoamFrameworkManager&) = delete;

        // Internal data
        std::unordered_map<std::string, std::unique_ptr<FreeRoamSession>> m_sessions;
        std::unordered_map<uint32_t, std::string> m_playerToSession; // Player ID to session ID mapping
        std::unordered_map<std::string, std::vector<std::string>> m_sessionsByType; // Session type to session IDs

        // Thread safety
        mutable std::shared_mutex m_sessionsMutex;
        mutable std::mutex m_callbacksMutex;

        // Statistics
        uint32_t m_totalSessionsCreated;
        uint32_t m_totalSessionsCompleted;
        std::chrono::steady_clock::time_point m_lastCleanup;

        // Event callbacks
        SessionStartedCallback m_sessionStartedCallback;
        SessionEndedCallback m_sessionEndedCallback;
        PlayerJoinedSessionCallback m_playerJoinedSessionCallback;
        PlayerLeftSessionCallback m_playerLeftSessionCallback;
        GameModeEnabledCallback m_gameModeEnabledCallback;
        GameModeDisabledCallback m_gameModeDisabledCallback;
        WorldStateUpdatedCallback m_worldStateUpdatedCallback;

        // Internal methods
        std::string GenerateSessionId();
        std::string GenerateGameModeInstanceId();

        void CleanupInactiveSessions();
        void UpdateSessionConnections(FreeRoamSession* session);
        void UpdateSessionGameModes(FreeRoamSession* session, float deltaTime);
        void ValidateSessionState(FreeRoamSession* session);

        bool ValidateSessionSettings(const FreeRoamSessionSettings& settings) const;
        bool CanPlayerCreateSession(uint32_t playerId) const;

        void HandlePlayerDisconnection(const std::string& sessionId, uint32_t playerId);
        void HandleHostMigration(const std::string& sessionId);

        void NotifySessionStarted(const std::string& sessionId);
        void NotifySessionEnded(const std::string& sessionId);
        void NotifyPlayerJoinedSession(const std::string& sessionId, uint32_t playerId, const std::string& playerName);
        void NotifyPlayerLeftSession(const std::string& sessionId, uint32_t playerId, const std::string& playerName);
        void NotifyGameModeEnabled(const std::string& sessionId, GameMode gameMode);
        void NotifyGameModeDisabled(const std::string& sessionId, GameMode gameMode);
        void NotifyWorldStateUpdated(const std::string& sessionId);

        void SendSessionStateToParticipants(const FreeRoamSession* session);
        void SendWorldStateToParticipants(const FreeRoamSession* session);
        void SendGameModeUpdateToParticipants(const FreeRoamSession* session, GameMode gameMode, bool enabled);
    };

    // Utility functions for free roam framework
    namespace FreeRoamUtils
    {
        std::string SessionTypeToString(SessionType type);
        SessionType StringToSessionType(const std::string& typeStr);

        std::string GameModeToString(GameMode mode);
        GameMode StringToGameMode(const std::string& modeStr);

        std::string ConnectionQualityToString(ConnectionQuality quality);
        ConnectionQuality PingToConnectionQuality(int32_t ping, float packetLoss);

        std::string SessionStateToString(SessionState state);

        bool ValidateSessionId(const std::string& sessionId);
        bool ValidatePlayerName(const std::string& playerName);
        bool ValidatePassword(const std::string& password);

        float CalculateSessionUptime(const FreeRoamSession& session);
        uint32_t CalculateSessionLoad(const FreeRoamSession& session);
        bool ShouldMigrateHost(const FreeRoamSession& session, uint32_t currentHostId);
    }

    // Network message structures for client-server communication
    struct SessionStateUpdate
    {
        std::string sessionId;
        SessionType sessionType;
        SessionState state;
        FreeRoamSessionSettings settings;
        WorldStateData worldState;
        std::vector<uint32_t> participants;
        std::vector<GameMode> activeGameModes;
        uint32_t syncVersion;
    };

    struct PlayerConnectionUpdate
    {
        std::string sessionId;
        uint32_t playerId;
        std::string playerName;
        bool isConnected;
        ConnectionQuality quality;
        int32_t ping;
        float posX, posY, posZ;
    };

    struct GameModeUpdate
    {
        std::string sessionId;
        std::string instanceId;
        GameMode gameMode;
        bool isActive;
        std::vector<uint32_t> participants;
        std::unordered_map<std::string, float> parameters;
    };

    struct WorldStateUpdate
    {
        std::string sessionId;
        WorldStateData worldState;
        std::chrono::steady_clock::time_point updateTime;
    };
}
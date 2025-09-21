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
    // Competitive game mode enums matching REDscript
    enum class CompetitiveMode : uint8_t
    {
        None = 0,
        Racing = 1,
        Arena = 2,
        Custom = 3
    };

    enum class MatchState : uint8_t
    {
        Waiting = 0,
        Starting = 1,
        InProgress = 2,
        Paused = 3,
        Finished = 4,
        Cancelled = 5
    };

    enum class MatchJoinResult : uint8_t
    {
        Success = 0,
        MatchFull = 1,
        AlreadyInMatch = 2,
        MatchNotFound = 3,
        MatchInProgress = 4,
        Banned = 5,
        NetworkError = 6
    };

    enum class ArenaType : uint8_t
    {
        Deathmatch = 0,
        TeamDeathmatch = 1,
        Elimination = 2,
        LastManStanding = 3,
        CaptureTheFlag = 4,
        Domination = 5,
        KingOfTheHill = 6
    };

    enum class RaceType : uint8_t
    {
        Circuit = 0,
        Sprint = 1,
        TimeTrial = 2,
        Elimination = 3,
        Drift = 4,
        Demolition = 5
    };

    enum class PowerupType : uint8_t
    {
        HealthBoost = 0,
        ArmorBoost = 1,
        DamageBoost = 2,
        SpeedBoost = 3,
        InfiniteAmmo = 4,
        Invisibility = 5,
        DoubleScore = 6,
        QuadDamage = 7
    };

    enum class Team : uint8_t
    {
        None = 0,
        Team1 = 1,
        Team2 = 2,
        Team3 = 3,
        Team4 = 4
    };

    // Match data structures
    struct CompetitiveMatchSettings
    {
        int32_t maxPlayers;
        float matchDuration;
        bool allowSpectators;
        bool isRanked;
        bool enableVoiceChat;
        bool autoBalance;
        float respawnTime;
        uint32_t rounds;

        CompetitiveMatchSettings()
            : maxPlayers(8), matchDuration(300.0f), allowSpectators(true),
              isRanked(false), enableVoiceChat(true), autoBalance(true),
              respawnTime(5.0f), rounds(1) {}
    };

    struct ArenaSettings
    {
        ArenaType arenaType;
        bool enablePowerups;
        int32_t killLimit;
        bool friendlyFire;
        bool allowCyberware;
        std::string arenaMap;
        int32_t teamSize;

        ArenaSettings()
            : arenaType(ArenaType::Deathmatch), enablePowerups(true),
              killLimit(20), friendlyFire(false), allowCyberware(true),
              arenaMap("Default"), teamSize(4) {}
    };

    struct RaceSettings
    {
        RaceType raceType;
        int32_t laps;
        float checkpointTolerance;
        bool enableTraffic;
        std::string trackName;

        RaceSettings()
            : raceType(RaceType::Circuit), laps(3), checkpointTolerance(10.0f),
              enableTraffic(false), trackName("Night City Circuit") {}
    };

    struct CompetitiveParticipant
    {
        uint32_t playerId;
        std::string playerName;
        Team team;
        int32_t score;
        int32_t kills;
        int32_t deaths;
        int32_t assists;
        int32_t killStreak;
        int32_t bestKillStreak;

        // Race-specific
        int32_t lapsCompleted;
        std::vector<uint32_t> checkpointsReached;
        std::vector<float> lapTimes;
        float totalRaceTime;
        float bestLapTime;
        int32_t position;

        // Arena-specific
        std::vector<PowerupType> activePowerups;
        float respawnTime;
        bool isAlive;
        std::chrono::steady_clock::time_point lastActivity;

        CompetitiveParticipant()
            : playerId(0), team(Team::None), score(0), kills(0), deaths(0),
              assists(0), killStreak(0), bestKillStreak(0), lapsCompleted(0),
              totalRaceTime(0.0f), bestLapTime(999999.0f), position(1),
              respawnTime(0.0f), isAlive(true),
              lastActivity(std::chrono::steady_clock::now()) {}
    };

    struct CompetitiveMatch
    {
        std::string matchId;
        CompetitiveMode gameMode;
        MatchState state;
        uint32_t hostPlayerId;
        CompetitiveMatchSettings settings;

        // Mode-specific settings
        ArenaSettings arenaSettings;
        RaceSettings raceSettings;

        std::vector<uint32_t> participants;
        std::unordered_map<uint32_t, std::unique_ptr<CompetitiveParticipant>> participantData;
        std::vector<uint32_t> spectators;

        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point lastUpdate;
        float duration;
        uint32_t currentRound;
        bool isRanked;

        // Statistics
        uint32_t syncVersion;
        std::unordered_map<std::string, float> matchStatistics;

        CompetitiveMatch()
            : gameMode(CompetitiveMode::None), state(MatchState::Waiting),
              hostPlayerId(0), startTime(std::chrono::steady_clock::now()),
              lastUpdate(std::chrono::steady_clock::now()), duration(0.0f),
              currentRound(0), isRanked(false), syncVersion(0) {}
    };

    // Main competitive game mode manager
    class CompetitiveGameModeManager
    {
    public:
        static CompetitiveGameModeManager& GetInstance();

        // Lifecycle
        void Initialize();
        void Shutdown();
        void Update();

        // Match management
        std::string CreateMatch(uint32_t hostPlayerId, CompetitiveMode gameMode,
                               const CompetitiveMatchSettings& settings);
        bool StartMatch(const std::string& matchId);
        bool EndMatch(const std::string& matchId);
        bool CancelMatch(const std::string& matchId);
        bool PauseMatch(const std::string& matchId);
        bool ResumeMatch(const std::string& matchId);

        // Participant management
        MatchJoinResult JoinMatch(const std::string& matchId, uint32_t playerId);
        bool LeaveMatch(const std::string& matchId, uint32_t playerId);
        bool KickPlayer(const std::string& matchId, uint32_t adminId, uint32_t targetId);
        std::vector<uint32_t> GetMatchParticipants(const std::string& matchId) const;

        // Race-specific functionality
        void OnRaceCheckpointReached(const std::string& matchId, uint32_t playerId, uint32_t checkpointId);
        void OnLapCompleted(const std::string& matchId, uint32_t playerId, float lapTime);
        void OnRaceFinished(const std::string& matchId, uint32_t playerId, float totalTime);
        void OnVehicleCollision(const std::string& matchId, uint32_t playerId1, uint32_t playerId2, float damage);

        // Arena-specific functionality
        void OnPlayerKilled(const std::string& matchId, uint32_t killerId, uint32_t victimId, const std::string& weaponType);
        void OnPlayerAssist(const std::string& matchId, uint32_t assisterId, uint32_t killerId, uint32_t victimId);
        bool SpawnPowerup(const std::string& matchId, PowerupType powerupType, float x, float y, float z);
        void OnPowerupCollected(const std::string& matchId, uint32_t playerId, PowerupType powerupType);

        // Team management
        bool AssignPlayerToTeam(const std::string& matchId, uint32_t playerId, Team teamId);
        void BalanceTeams(const std::string& matchId);
        std::unordered_map<Team, std::vector<uint32_t>> GetTeams(const std::string& matchId) const;

        // Spectator management
        bool AddSpectator(const std::string& matchId, uint32_t playerId);
        bool RemoveSpectator(const std::string& matchId, uint32_t playerId);
        std::vector<uint32_t> GetSpectators(const std::string& matchId) const;

        // Match queries
        CompetitiveMatch* GetMatch(const std::string& matchId);
        const CompetitiveMatch* GetMatch(const std::string& matchId) const;
        std::vector<std::string> GetActiveMatches() const;
        std::vector<std::string> GetMatchesByMode(CompetitiveMode gameMode) const;
        std::string FindPlayerMatch(uint32_t playerId) const;

        // Statistics and scoring
        void UpdatePlayerScore(const std::string& matchId, uint32_t playerId, int32_t points);
        void UpdatePlayerStatistic(const std::string& matchId, uint32_t playerId, const std::string& statName, float value);
        std::vector<CompetitiveParticipant> GetLeaderboard(const std::string& matchId) const;

        // Match settings
        bool UpdateMatchSettings(const std::string& matchId, const CompetitiveMatchSettings& settings);
        bool UpdateArenaSettings(const std::string& matchId, const ArenaSettings& settings);
        bool UpdateRaceSettings(const std::string& matchId, const RaceSettings& settings);

        // Event callbacks
        using MatchStartedCallback = std::function<void(const std::string& matchId)>;
        using MatchEndedCallback = std::function<void(const std::string& matchId, bool successful)>;
        using PlayerJoinedCallback = std::function<void(const std::string& matchId, uint32_t playerId)>;
        using PlayerLeftCallback = std::function<void(const std::string& matchId, uint32_t playerId)>;
        using PlayerKilledCallback = std::function<void(const std::string& matchId, uint32_t killerId, uint32_t victimId)>;
        using PowerupSpawnedCallback = std::function<void(const std::string& matchId, PowerupType type, float x, float y, float z)>;

        void SetMatchStartedCallback(MatchStartedCallback callback);
        void SetMatchEndedCallback(MatchEndedCallback callback);
        void SetPlayerJoinedCallback(PlayerJoinedCallback callback);
        void SetPlayerLeftCallback(PlayerLeftCallback callback);
        void SetPlayerKilledCallback(PlayerKilledCallback callback);
        void SetPowerupSpawnedCallback(PowerupSpawnedCallback callback);

        // Network synchronization
        void BroadcastMatchState(const std::string& matchId);
        void SyncMatchToPlayer(const std::string& matchId, uint32_t playerId);
        void NotifyMatchEvent(const std::string& matchId, const std::string& eventType, const std::string& eventData);

        // Statistics and monitoring
        uint32_t GetActiveMatchCount() const;
        uint32_t GetTotalParticipants() const;
        std::chrono::milliseconds GetAverageMatchDuration() const;
        std::unordered_map<CompetitiveMode, uint32_t> GetMatchDistribution() const;

    private:
        CompetitiveGameModeManager() = default;
        ~CompetitiveGameModeManager() = default;
        CompetitiveGameModeManager(const CompetitiveGameModeManager&) = delete;
        CompetitiveGameModeManager& operator=(const CompetitiveGameModeManager&) = delete;

        // Internal data
        std::unordered_map<std::string, std::unique_ptr<CompetitiveMatch>> m_matches;
        std::unordered_map<uint32_t, std::string> m_playerToMatch; // Player ID to match ID mapping

        // Thread safety
        mutable std::shared_mutex m_matchesMutex;
        mutable std::mutex m_callbacksMutex;

        // Statistics
        uint32_t m_totalMatchesCreated;
        uint32_t m_totalMatchesCompleted;
        std::chrono::steady_clock::time_point m_lastCleanup;

        // Event callbacks
        MatchStartedCallback m_matchStartedCallback;
        MatchEndedCallback m_matchEndedCallback;
        PlayerJoinedCallback m_playerJoinedCallback;
        PlayerLeftCallback m_playerLeftCallback;
        PlayerKilledCallback m_playerKilledCallback;
        PowerupSpawnedCallback m_powerupSpawnedCallback;

        // Internal methods
        std::string GenerateMatchId();
        bool ValidateMatchSettings(CompetitiveMode gameMode, const CompetitiveMatchSettings& settings) const;
        bool CanPlayerJoinMatch(uint32_t playerId, const std::string& matchId) const;
        int32_t GetMinimumPlayers(CompetitiveMode gameMode) const;

        void CleanupInactiveMatches();
        void UpdateMatchLogic(CompetitiveMatch* match);
        void UpdateRaceLogic(CompetitiveMatch* match);
        void UpdateArenaLogic(CompetitiveMatch* match);

        bool CheckRaceWinCondition(const CompetitiveMatch* match) const;
        bool CheckArenaWinCondition(const CompetitiveMatch* match) const;
        bool CheckDeathMatchWin(const CompetitiveMatch* match) const;
        bool CheckTeamDeathMatchWin(const CompetitiveMatch* match) const;

        CompetitiveParticipant* FindParticipant(CompetitiveMatch* match, uint32_t playerId);
        const CompetitiveParticipant* FindParticipant(const CompetitiveMatch* match, uint32_t playerId) const;

        void NotifyMatchStarted(const std::string& matchId);
        void NotifyMatchEnded(const std::string& matchId, bool successful);
        void NotifyPlayerJoined(const std::string& matchId, uint32_t playerId);
        void NotifyPlayerLeft(const std::string& matchId, uint32_t playerId);
        void NotifyPlayerKilled(const std::string& matchId, uint32_t killerId, uint32_t victimId);
        void NotifyPowerupSpawned(const std::string& matchId, PowerupType type, float x, float y, float z);

        void SendMatchStateToParticipants(const CompetitiveMatch* match);
        void SendLeaderboardUpdate(const CompetitiveMatch* match);
    };

    // Utility functions for competitive game modes
    namespace CompetitiveUtils
    {
        std::string CompetitiveModeToString(CompetitiveMode mode);
        CompetitiveMode StringToCompetitiveMode(const std::string& modeStr);

        std::string MatchStateToString(MatchState state);
        MatchState StringToMatchState(const std::string& stateStr);

        std::string ArenaTypeToString(ArenaType type);
        ArenaType StringToArenaType(const std::string& typeStr);

        std::string RaceTypeToString(RaceType type);
        RaceType StringToRaceType(const std::string& typeStr);

        std::string PowerupTypeToString(PowerupType type);
        PowerupType StringToPowerupType(const std::string& typeStr);

        std::string TeamToString(Team team);
        Team StringToTeam(const std::string& teamStr);

        bool ValidateMatchId(const std::string& matchId);
        float CalculateMatchProgress(const CompetitiveMatch* match);
        uint32_t CalculatePlayerRating(const CompetitiveParticipant* participant, CompetitiveMode gameMode);
    }

    // Network message structures for client-server communication
    struct MatchStateUpdate
    {
        std::string matchId;
        CompetitiveMode gameMode;
        MatchState state;
        float duration;
        uint32_t currentRound;
        uint32_t maxRounds;
        std::vector<CompetitiveParticipant> participants;
        uint32_t syncVersion;
    };

    struct LeaderboardUpdate
    {
        std::string matchId;
        std::vector<CompetitiveParticipant> rankedParticipants;
        std::chrono::steady_clock::time_point updateTime;
    };

    struct MatchEvent
    {
        std::string matchId;
        std::string eventType;
        uint32_t playerId;
        std::string eventData;
        std::chrono::steady_clock::time_point timestamp;
    };
}
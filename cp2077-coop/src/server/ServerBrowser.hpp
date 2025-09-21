#pragma once

#include "RED4ext/Api/Sdk.hpp"
#include "RED4ext/Common.hpp"
#include "RED4ext/NativeTypes.hpp"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>
#include <functional>

namespace RED4ext
{
    // Server information structure
    struct ServerInfo
    {
        std::string serverId;
        std::string serverName;
        std::string hostName;
        std::string description;
        std::string gameMode;
        std::string mapName;
        uint32_t currentPlayers;
        uint32_t maxPlayers;
        uint32_t ping;
        bool isPasswordProtected;
        bool isModded;
        bool allowsCrossPlatform;
        std::string version;
        std::string region;
        std::vector<std::string> tags;
        std::chrono::steady_clock::time_point lastHeartbeat;

        // Gameplay settings
        bool friendlyFire;
        bool persistentWorld;
        float difficulty;
        bool allowsCustomContent;

        // Network info
        std::string ipAddress;
        uint16_t port;

        ServerInfo() = default;
        ServerInfo(const std::string& id, const std::string& name, const std::string& host)
            : serverId(id), serverName(name), hostName(host), currentPlayers(0),
              maxPlayers(32), ping(0), isPasswordProtected(false), isModded(false),
              allowsCrossPlatform(true), friendlyFire(false), persistentWorld(true),
              difficulty(1.0f), allowsCustomContent(false), port(7777),
              lastHeartbeat(std::chrono::steady_clock::now()) {}
    };

    // Server filter criteria
    struct ServerFilters
    {
        std::string nameFilter;
        std::string gameModeFilter;
        std::string regionFilter;
        uint32_t maxPing = 999;
        bool showPasswordProtected = true;
        bool showModded = true;
        bool showFull = false;
        bool showEmpty = true;
        std::vector<std::string> requiredTags;
        std::vector<std::string> excludedTags;
    };

    // Matchmaking preferences
    struct MatchmakingPreferences
    {
        std::string preferredGameMode;
        std::string preferredRegion;
        uint32_t maxPing = 150;
        bool allowPasswordProtected = false;
        bool allowModded = false;
        float skillLevel = 0.5f; // 0.0 to 1.0
        std::vector<std::string> preferredTags;
    };

    // Server search result
    enum class ServerSearchResult : uint8_t
    {
        Success = 0,
        NoServersFound = 1,
        NetworkError = 2,
        Timeout = 3,
        FilterTooRestrictive = 4
    };

    // Matchmaking result
    enum class MatchmakingResult : uint8_t
    {
        Success = 0,
        NoSuitableServers = 1,
        AllServersFull = 2,
        NetworkError = 3,
        Timeout = 4,
        Cancelled = 5
    };

    class ServerBrowser
    {
    public:
        static ServerBrowser& GetInstance();

        // Server discovery
        void RefreshServerList();
        std::vector<ServerInfo> GetServerList() const;
        std::vector<ServerInfo> GetFilteredServerList(const ServerFilters& filters) const;

        // Server queries
        ServerInfo* FindServer(const std::string& serverId);
        bool PingServer(const std::string& serverId);
        bool GetServerDetails(const std::string& serverId, ServerInfo& details);

        // Server operations
        bool ConnectToServer(const std::string& serverId, const std::string& password = "");
        void DisconnectFromServer();

        // Favorites management
        void AddServerToFavorites(const std::string& serverId);
        void RemoveServerFromFavorites(const std::string& serverId);
        std::vector<ServerInfo> GetFavoriteServers() const;
        bool IsServerFavorite(const std::string& serverId) const;

        // Recent servers
        void AddServerToRecent(const std::string& serverId);
        std::vector<ServerInfo> GetRecentServers() const;

        // Server registration (for hosts)
        bool RegisterServer(const ServerInfo& serverInfo);
        void UpdateServerInfo(const ServerInfo& serverInfo);
        void UnregisterServer();
        void SendHeartbeat();

        // Search and filtering
        void SearchServers(const std::string& searchTerm);
        void ApplyFilters(const ServerFilters& filters);
        void ClearFilters();

        // Sorting
        enum class SortCriteria : uint8_t
        {
            Name,
            Players,
            Ping,
            GameMode,
            LastUpdated
        };

        void SortServers(SortCriteria criteria, bool ascending = true);

        // Events and callbacks
        using ServerListUpdatedCallback = std::function<void()>;
        using ServerConnectedCallback = std::function<void(const std::string&)>;
        using ServerDisconnectedCallback = std::function<void(const std::string&)>;

        void SetServerListUpdatedCallback(ServerListUpdatedCallback callback);
        void SetServerConnectedCallback(ServerConnectedCallback callback);
        void SetServerDisconnectedCallback(ServerDisconnectedCallback callback);

        // Status
        bool IsRefreshing() const;
        bool IsConnected() const;
        std::string GetCurrentServerId() const;

        // Configuration
        void SetRefreshInterval(uint32_t seconds);
        void SetMaxServersToDisplay(uint32_t maxServers);
        void EnableAutoRefresh(bool enabled);

        // Statistics
        uint32_t GetTotalServersFound() const;
        uint32_t GetFilteredServersCount() const;
        std::chrono::milliseconds GetLastRefreshDuration() const;

        // Initialization and cleanup
        void Initialize();
        void Shutdown();
        void Update();

    private:
        ServerBrowser() = default;
        ~ServerBrowser() = default;
        ServerBrowser(const ServerBrowser&) = delete;
        ServerBrowser& operator=(const ServerBrowser&) = delete;

        // Internal state
        std::vector<ServerInfo> m_serverList;
        std::vector<ServerInfo> m_filteredServerList;
        std::vector<std::string> m_favoriteServers;
        std::vector<std::string> m_recentServers;
        ServerFilters m_currentFilters;

        std::string m_currentServerId;
        bool m_isConnected = false;
        bool m_isRefreshing = false;
        bool m_autoRefreshEnabled = true;

        // Configuration
        uint32_t m_refreshInterval = 30; // seconds
        uint32_t m_maxServersToDisplay = 1000;
        std::chrono::steady_clock::time_point m_lastRefresh;
        std::chrono::milliseconds m_lastRefreshDuration{0};

        // Callbacks
        ServerListUpdatedCallback m_serverListUpdatedCallback;
        ServerConnectedCallback m_serverConnectedCallback;
        ServerDisconnectedCallback m_serverDisconnectedCallback;

        // Thread safety
        mutable std::shared_mutex m_serverListMutex;
        mutable std::mutex m_favoritesMutex;
        mutable std::mutex m_configMutex;

        // Internal methods
        void FetchServersFromMasterServer();
        void UpdateServerPings();
        void CleanupStaleServers();
        void ApplyCurrentFilters();
        bool MatchesFilters(const ServerInfo& server, const ServerFilters& filters) const;
        void NotifyServerListUpdated();
        void LoadFavorites();
        void SaveFavorites();
        void LoadRecentServers();
        void SaveRecentServers();

        // Network helpers
        bool SendServerQuery(const std::string& address, uint16_t port, ServerInfo& info);
        uint32_t MeasurePing(const std::string& address, uint16_t port);
        bool ValidateServerVersion(const std::string& serverVersion);
    };

    class Matchmaker
    {
    public:
        static Matchmaker& GetInstance();

        // Matchmaking operations
        void StartMatchmaking(const MatchmakingPreferences& preferences);
        void StopMatchmaking();
        bool IsMatchmaking() const;

        // Quick match
        void QuickMatch(const std::string& gameMode = "");

        // Skill-based matchmaking
        void FindSkillBasedMatch(const MatchmakingPreferences& preferences);

        // Party matchmaking
        void FindPartyMatch(const std::vector<uint32_t>& partyMembers,
                           const MatchmakingPreferences& preferences);

        // Custom matchmaking
        void FindCustomMatch(const std::function<bool(const ServerInfo&)>& customFilter);

        // Region-based matching
        void FindRegionalMatch(const std::string& region,
                              const MatchmakingPreferences& preferences);

        // Events and callbacks
        using MatchFoundCallback = std::function<void(const ServerInfo&)>;
        using MatchmakingFailedCallback = std::function<void(MatchmakingResult)>;
        using MatchmakingProgressCallback = std::function<void(float)>; // 0.0 to 1.0

        void SetMatchFoundCallback(MatchFoundCallback callback);
        void SetMatchmakingFailedCallback(MatchmakingFailedCallback callback);
        void SetMatchmakingProgressCallback(MatchmakingProgressCallback callback);

        // Configuration
        void SetMatchmakingTimeout(uint32_t seconds);
        void SetMaxSearchRadius(uint32_t maxPing);
        void EnableSkillMatching(bool enabled);

        // Statistics
        std::chrono::milliseconds GetAverageMatchTime() const;
        uint32_t GetSuccessfulMatchesCount() const;
        float GetCurrentSearchProgress() const;

        // Initialization and cleanup
        void Initialize();
        void Shutdown();
        void Update();

    private:
        Matchmaker() = default;
        ~Matchmaker() = default;
        Matchmaker(const Matchmaker&) = delete;
        Matchmaker& operator=(const Matchmaker&) = delete;

        // Internal state
        bool m_isMatchmaking = false;
        MatchmakingPreferences m_currentPreferences;
        std::chrono::steady_clock::time_point m_matchmakingStartTime;
        std::vector<ServerInfo> m_candidateServers;
        uint32_t m_currentSearchRadius = 50; // ping

        // Configuration
        uint32_t m_matchmakingTimeout = 300; // 5 minutes
        uint32_t m_maxSearchRadius = 300;
        bool m_skillMatchingEnabled = true;

        // Statistics
        uint32_t m_successfulMatches = 0;
        std::chrono::milliseconds m_totalMatchTime{0};

        // Callbacks
        MatchFoundCallback m_matchFoundCallback;
        MatchmakingFailedCallback m_matchmakingFailedCallback;
        MatchmakingProgressCallback m_matchmakingProgressCallback;

        // Thread safety
        mutable std::mutex m_matchmakingMutex;

        // Internal methods
        void PerformMatchmakingStep();
        std::vector<ServerInfo> FindCandidateServers(const MatchmakingPreferences& preferences);
        ServerInfo SelectBestMatch(const std::vector<ServerInfo>& candidates,
                                  const MatchmakingPreferences& preferences);
        float CalculateServerScore(const ServerInfo& server,
                                  const MatchmakingPreferences& preferences);
        bool IsServerSuitable(const ServerInfo& server,
                             const MatchmakingPreferences& preferences);
        void ExpandSearchRadius();
        void UpdateMatchmakingProgress();
        void CompleteMatchmaking(const ServerInfo& selectedServer);
        void FailMatchmaking(MatchmakingResult result);

        // Skill matching helpers
        float CalculateSkillCompatibility(const ServerInfo& server, float playerSkill);
        float EstimateServerSkillLevel(const ServerInfo& server);
    };

    // Utility functions for server management
    namespace ServerUtils
    {
        std::string GenerateServerId();
        bool ValidateServerName(const std::string& name);
        bool ValidateServerDescription(const std::string& description);
        std::string FormatPlayerCount(uint32_t current, uint32_t max);
        std::string FormatPing(uint32_t ping);
        std::string GetRegionFromIP(const std::string& ipAddress);
        bool IsPrivateIP(const std::string& ipAddress);
        std::vector<std::string> ParseServerTags(const std::string& tagString);
        std::string ServerTagsToString(const std::vector<std::string>& tags);
    }
}
#include "ServerBrowser.hpp"
#include "RED4ext/Api/Sdk.hpp"
#include "RED4ext/Common.hpp"
#include <algorithm>
#include <chrono>
#include <random>
#include <regex>

namespace RED4ext
{
    // ServerBrowser Implementation
    ServerBrowser& ServerBrowser::GetInstance()
    {
        static ServerBrowser instance;
        return instance;
    }

    void ServerBrowser::Initialize()
    {
        std::unique_lock<std::shared_mutex> lock(m_serverListMutex);

        m_serverList.clear();
        m_filteredServerList.clear();
        m_favoriteServers.clear();
        m_recentServers.clear();

        // Initialize configuration
        m_refreshInterval = 30; // 30 seconds
        m_maxServersToDisplay = 1000;
        m_autoRefreshEnabled = true;
        m_isRefreshing = false;
        m_isConnected = false;

        // Load persistent data
        LoadFavorites();
        LoadRecentServers();

        // Initial server list refresh
        RefreshServerList();
    }

    void ServerBrowser::Shutdown()
    {
        std::unique_lock<std::shared_mutex> lock(m_serverListMutex);

        // Save persistent data
        SaveFavorites();
        SaveRecentServers();

        // Clear all data
        m_serverList.clear();
        m_filteredServerList.clear();
        m_favoriteServers.clear();
        m_recentServers.clear();

        // Clear callbacks
        m_serverListUpdatedCallback = nullptr;
        m_serverConnectedCallback = nullptr;
        m_serverDisconnectedCallback = nullptr;
    }

    void ServerBrowser::Update()
    {
        auto currentTime = std::chrono::steady_clock::now();

        // Auto refresh check
        if (m_autoRefreshEnabled && !m_isRefreshing) {
            auto timeSinceRefresh = std::chrono::duration_cast<std::chrono::seconds>(
                currentTime - m_lastRefresh).count();

            if (timeSinceRefresh >= m_refreshInterval) {
                RefreshServerList();
            }
        }

        // Update server pings periodically
        UpdateServerPings();

        // Clean up stale servers
        CleanupStaleServers();
    }

    void ServerBrowser::RefreshServerList()
    {
        if (m_isRefreshing) {
            return;
        }

        m_isRefreshing = true;
        m_lastRefresh = std::chrono::steady_clock::now();

        auto startTime = std::chrono::steady_clock::now();

        // Fetch servers from master server in background thread
        std::thread([this, startTime]() {
            FetchServersFromMasterServer();

            auto endTime = std::chrono::steady_clock::now();
            m_lastRefreshDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                endTime - startTime);

            m_isRefreshing = false;

            // Apply current filters
            ApplyCurrentFilters();

            // Notify listeners
            NotifyServerListUpdated();
        }).detach();
    }

    std::vector<ServerInfo> ServerBrowser::GetServerList() const
    {
        std::shared_lock<std::shared_mutex> lock(m_serverListMutex);
        return m_serverList;
    }

    std::vector<ServerInfo> ServerBrowser::GetFilteredServerList(const ServerFilters& filters) const
    {
        std::shared_lock<std::shared_mutex> lock(m_serverListMutex);

        std::vector<ServerInfo> filtered;

        for (const auto& server : m_serverList) {
            if (MatchesFilters(server, filters)) {
                filtered.push_back(server);
            }
        }

        return filtered;
    }

    ServerInfo* ServerBrowser::FindServer(const std::string& serverId)
    {
        std::shared_lock<std::shared_mutex> lock(m_serverListMutex);

        auto it = std::find_if(m_serverList.begin(), m_serverList.end(),
            [&serverId](const ServerInfo& server) {
                return server.serverId == serverId;
            });

        return (it != m_serverList.end()) ? &(*it) : nullptr;
    }

    bool ServerBrowser::PingServer(const std::string& serverId)
    {
        auto server = FindServer(serverId);
        if (!server) {
            return false;
        }

        // Measure ping in background thread
        std::thread([this, serverId, server]() {
            uint32_t ping = MeasurePing(server->ipAddress, server->port);

            // Update ping
            std::unique_lock<std::shared_mutex> lock(m_serverListMutex);
            auto serverPtr = FindServer(serverId);
            if (serverPtr) {
                serverPtr->ping = ping;
                serverPtr->lastHeartbeat = std::chrono::steady_clock::now();
            }
        }).detach();

        return true;
    }

    bool ServerBrowser::GetServerDetails(const std::string& serverId, ServerInfo& details)
    {
        auto server = FindServer(serverId);
        if (!server) {
            return false;
        }

        details = *server;
        return true;
    }

    bool ServerBrowser::ConnectToServer(const std::string& serverId, const std::string& password)
    {
        auto server = FindServer(serverId);
        if (!server) {
            return false;
        }

        // Validate server version compatibility
        if (!ValidateServerVersion(server->version)) {
            return false;
        }

        // Check if server requires password
        if (server->isPasswordProtected && password.empty()) {
            return false;
        }

        // Add to recent servers
        AddServerToRecent(serverId);

        // Update connection state
        m_currentServerId = serverId;
        m_isConnected = true;

        // Notify connection callback
        if (m_serverConnectedCallback) {
            m_serverConnectedCallback(serverId);
        }

        return true;
    }

    void ServerBrowser::DisconnectFromServer()
    {
        if (!m_isConnected) {
            return;
        }

        std::string disconnectedServerId = m_currentServerId;

        m_currentServerId.clear();
        m_isConnected = false;

        // Notify disconnection callback
        if (m_serverDisconnectedCallback) {
            m_serverDisconnectedCallback(disconnectedServerId);
        }
    }

    void ServerBrowser::AddServerToFavorites(const std::string& serverId)
    {
        std::lock_guard<std::mutex> lock(m_favoritesMutex);

        auto it = std::find(m_favoriteServers.begin(), m_favoriteServers.end(), serverId);
        if (it == m_favoriteServers.end()) {
            m_favoriteServers.push_back(serverId);
            SaveFavorites();
        }
    }

    void ServerBrowser::RemoveServerFromFavorites(const std::string& serverId)
    {
        std::lock_guard<std::mutex> lock(m_favoritesMutex);

        auto it = std::find(m_favoriteServers.begin(), m_favoriteServers.end(), serverId);
        if (it != m_favoriteServers.end()) {
            m_favoriteServers.erase(it);
            SaveFavorites();
        }
    }

    std::vector<ServerInfo> ServerBrowser::GetFavoriteServers() const
    {
        std::shared_lock<std::shared_mutex> serverLock(m_serverListMutex);
        std::lock_guard<std::mutex> favLock(m_favoritesMutex);

        std::vector<ServerInfo> favorites;

        for (const auto& serverId : m_favoriteServers) {
            auto it = std::find_if(m_serverList.begin(), m_serverList.end(),
                [&serverId](const ServerInfo& server) {
                    return server.serverId == serverId;
                });

            if (it != m_serverList.end()) {
                favorites.push_back(*it);
            }
        }

        return favorites;
    }

    bool ServerBrowser::IsServerFavorite(const std::string& serverId) const
    {
        std::lock_guard<std::mutex> lock(m_favoritesMutex);

        return std::find(m_favoriteServers.begin(), m_favoriteServers.end(), serverId)
               != m_favoriteServers.end();
    }

    void ServerBrowser::AddServerToRecent(const std::string& serverId)
    {
        std::lock_guard<std::mutex> lock(m_favoritesMutex);

        // Remove if already exists
        auto it = std::find(m_recentServers.begin(), m_recentServers.end(), serverId);
        if (it != m_recentServers.end()) {
            m_recentServers.erase(it);
        }

        // Add to front
        m_recentServers.insert(m_recentServers.begin(), serverId);

        // Limit to 20 recent servers
        if (m_recentServers.size() > 20) {
            m_recentServers.resize(20);
        }

        SaveRecentServers();
    }

    std::vector<ServerInfo> ServerBrowser::GetRecentServers() const
    {
        std::shared_lock<std::shared_mutex> serverLock(m_serverListMutex);
        std::lock_guard<std::mutex> favLock(m_favoritesMutex);

        std::vector<ServerInfo> recent;

        for (const auto& serverId : m_recentServers) {
            auto it = std::find_if(m_serverList.begin(), m_serverList.end(),
                [&serverId](const ServerInfo& server) {
                    return server.serverId == serverId;
                });

            if (it != m_serverList.end()) {
                recent.push_back(*it);
            }
        }

        return recent;
    }

    bool ServerBrowser::RegisterServer(const ServerInfo& serverInfo)
    {
        std::unique_lock<std::shared_mutex> lock(m_serverListMutex);

        // Check for duplicate
        auto it = std::find_if(m_serverList.begin(), m_serverList.end(),
            [&serverInfo](const ServerInfo& server) {
                return server.serverId == serverInfo.serverId;
            });

        if (it != m_serverList.end()) {
            // Update existing server
            *it = serverInfo;
            it->lastHeartbeat = std::chrono::steady_clock::now();
        } else {
            // Add new server
            ServerInfo newServer = serverInfo;
            newServer.lastHeartbeat = std::chrono::steady_clock::now();
            m_serverList.push_back(newServer);
        }

        return true;
    }

    void ServerBrowser::UpdateServerInfo(const ServerInfo& serverInfo)
    {
        RegisterServer(serverInfo); // Same implementation
    }

    void ServerBrowser::UnregisterServer()
    {
        // This would be called by a server when it shuts down
        // Implementation depends on how servers identify themselves
    }

    void ServerBrowser::SendHeartbeat()
    {
        // This would be called periodically by a server to maintain its listing
        // Implementation depends on server architecture
    }

    void ServerBrowser::SearchServers(const std::string& searchTerm)
    {
        ServerFilters filters;
        filters.nameFilter = searchTerm;
        ApplyFilters(filters);
    }

    void ServerBrowser::ApplyFilters(const ServerFilters& filters)
    {
        m_currentFilters = filters;
        ApplyCurrentFilters();
    }

    void ServerBrowser::ClearFilters()
    {
        m_currentFilters = ServerFilters();
        ApplyCurrentFilters();
    }

    void ServerBrowser::SortServers(SortCriteria criteria, bool ascending)
    {
        std::unique_lock<std::shared_mutex> lock(m_serverListMutex);

        switch (criteria) {
            case SortCriteria::Name:
                std::sort(m_filteredServerList.begin(), m_filteredServerList.end(),
                    [ascending](const ServerInfo& a, const ServerInfo& b) {
                        return ascending ? a.serverName < b.serverName : a.serverName > b.serverName;
                    });
                break;

            case SortCriteria::Players:
                std::sort(m_filteredServerList.begin(), m_filteredServerList.end(),
                    [ascending](const ServerInfo& a, const ServerInfo& b) {
                        return ascending ? a.currentPlayers < b.currentPlayers :
                                         a.currentPlayers > b.currentPlayers;
                    });
                break;

            case SortCriteria::Ping:
                std::sort(m_filteredServerList.begin(), m_filteredServerList.end(),
                    [ascending](const ServerInfo& a, const ServerInfo& b) {
                        return ascending ? a.ping < b.ping : a.ping > b.ping;
                    });
                break;

            case SortCriteria::GameMode:
                std::sort(m_filteredServerList.begin(), m_filteredServerList.end(),
                    [ascending](const ServerInfo& a, const ServerInfo& b) {
                        return ascending ? a.gameMode < b.gameMode : a.gameMode > b.gameMode;
                    });
                break;

            case SortCriteria::LastUpdated:
                std::sort(m_filteredServerList.begin(), m_filteredServerList.end(),
                    [ascending](const ServerInfo& a, const ServerInfo& b) {
                        return ascending ? a.lastHeartbeat < b.lastHeartbeat :
                                         a.lastHeartbeat > b.lastHeartbeat;
                    });
                break;
        }
    }

    void ServerBrowser::SetServerListUpdatedCallback(ServerListUpdatedCallback callback)
    {
        m_serverListUpdatedCallback = callback;
    }

    void ServerBrowser::SetServerConnectedCallback(ServerConnectedCallback callback)
    {
        m_serverConnectedCallback = callback;
    }

    void ServerBrowser::SetServerDisconnectedCallback(ServerDisconnectedCallback callback)
    {
        m_serverDisconnectedCallback = callback;
    }

    bool ServerBrowser::IsRefreshing() const
    {
        return m_isRefreshing;
    }

    bool ServerBrowser::IsConnected() const
    {
        return m_isConnected;
    }

    std::string ServerBrowser::GetCurrentServerId() const
    {
        return m_currentServerId;
    }

    void ServerBrowser::SetRefreshInterval(uint32_t seconds)
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        m_refreshInterval = seconds;
    }

    void ServerBrowser::SetMaxServersToDisplay(uint32_t maxServers)
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        m_maxServersToDisplay = maxServers;
    }

    void ServerBrowser::EnableAutoRefresh(bool enabled)
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        m_autoRefreshEnabled = enabled;
    }

    uint32_t ServerBrowser::GetTotalServersFound() const
    {
        std::shared_lock<std::shared_mutex> lock(m_serverListMutex);
        return static_cast<uint32_t>(m_serverList.size());
    }

    uint32_t ServerBrowser::GetFilteredServersCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_serverListMutex);
        return static_cast<uint32_t>(m_filteredServerList.size());
    }

    std::chrono::milliseconds ServerBrowser::GetLastRefreshDuration() const
    {
        return m_lastRefreshDuration;
    }

    // Private implementation methods

    void ServerBrowser::FetchServersFromMasterServer()
    {
        // This would implement the actual network communication with master server
        // For now, we'll simulate some servers

        std::unique_lock<std::shared_mutex> lock(m_serverListMutex);

        // Clear existing servers
        m_serverList.clear();

        // Add some example servers (in real implementation, this would fetch from master server)
        ServerInfo server1("server_001", "Night City Legends", "nightcity.example.com");
        server1.description = "Roleplay focused server with custom missions";
        server1.gameMode = "Free Roam";
        server1.mapName = "Night City";
        server1.currentPlayers = 24;
        server1.maxPlayers = 32;
        server1.ping = 45;
        server1.isPasswordProtected = false;
        server1.isModded = true;
        server1.version = "1.0.0";
        server1.region = "NA-West";
        server1.tags = {"roleplay", "custom-missions", "friendly"};
        server1.ipAddress = "192.168.1.100";

        ServerInfo server2("server_002", "Corpo Wars PvP", "corpowars.example.com");
        server2.description = "Competitive PvP with corporate factions";
        server2.gameMode = "Team Deathmatch";
        server2.mapName = "Corporate Plaza";
        server2.currentPlayers = 16;
        server2.maxPlayers = 24;
        server2.ping = 67;
        server2.isPasswordProtected = true;
        server2.isModded = false;
        server2.version = "1.0.0";
        server2.region = "EU-West";
        server2.tags = {"pvp", "competitive", "hardcore"};
        server2.ipAddress = "192.168.1.101";

        m_serverList.push_back(server1);
        m_serverList.push_back(server2);
    }

    void ServerBrowser::UpdateServerPings()
    {
        // Update pings for all servers periodically
        for (auto& server : m_serverList) {
            // Check if ping needs updating (every 30 seconds)
            auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - server.lastHeartbeat).count();

            if (timeSinceUpdate >= 30) {
                // Update ping in background thread
                std::thread([this, &server]() {
                    server.ping = MeasurePing(server.ipAddress, server.port);
                    server.lastHeartbeat = std::chrono::steady_clock::now();
                }).detach();
            }
        }
    }

    void ServerBrowser::CleanupStaleServers()
    {
        std::unique_lock<std::shared_mutex> lock(m_serverListMutex);

        auto currentTime = std::chrono::steady_clock::now();

        m_serverList.erase(
            std::remove_if(m_serverList.begin(), m_serverList.end(),
                [currentTime](const ServerInfo& server) {
                    auto timeSinceHeartbeat = std::chrono::duration_cast<std::chrono::minutes>(
                        currentTime - server.lastHeartbeat).count();
                    return timeSinceHeartbeat >= 5; // Remove servers not seen in 5 minutes
                }),
            m_serverList.end());
    }

    void ServerBrowser::ApplyCurrentFilters()
    {
        std::unique_lock<std::shared_mutex> lock(m_serverListMutex);

        m_filteredServerList.clear();

        for (const auto& server : m_serverList) {
            if (MatchesFilters(server, m_currentFilters)) {
                m_filteredServerList.push_back(server);
            }
        }

        // Limit results
        if (m_filteredServerList.size() > m_maxServersToDisplay) {
            m_filteredServerList.resize(m_maxServersToDisplay);
        }
    }

    bool ServerBrowser::MatchesFilters(const ServerInfo& server, const ServerFilters& filters) const
    {
        // Name filter
        if (!filters.nameFilter.empty()) {
            std::string serverNameLower = server.serverName;
            std::string filterLower = filters.nameFilter;
            std::transform(serverNameLower.begin(), serverNameLower.end(), serverNameLower.begin(), ::tolower);
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);

            if (serverNameLower.find(filterLower) == std::string::npos) {
                return false;
            }
        }

        // Game mode filter
        if (!filters.gameModeFilter.empty() && server.gameMode != filters.gameModeFilter) {
            return false;
        }

        // Region filter
        if (!filters.regionFilter.empty() && server.region != filters.regionFilter) {
            return false;
        }

        // Ping filter
        if (server.ping > filters.maxPing) {
            return false;
        }

        // Password protected filter
        if (!filters.showPasswordProtected && server.isPasswordProtected) {
            return false;
        }

        // Modded filter
        if (!filters.showModded && server.isModded) {
            return false;
        }

        // Full servers filter
        if (!filters.showFull && server.currentPlayers >= server.maxPlayers) {
            return false;
        }

        // Empty servers filter
        if (!filters.showEmpty && server.currentPlayers == 0) {
            return false;
        }

        // Required tags
        for (const auto& requiredTag : filters.requiredTags) {
            if (std::find(server.tags.begin(), server.tags.end(), requiredTag) == server.tags.end()) {
                return false;
            }
        }

        // Excluded tags
        for (const auto& excludedTag : filters.excludedTags) {
            if (std::find(server.tags.begin(), server.tags.end(), excludedTag) != server.tags.end()) {
                return false;
            }
        }

        return true;
    }

    void ServerBrowser::NotifyServerListUpdated()
    {
        if (m_serverListUpdatedCallback) {
            m_serverListUpdatedCallback();
        }
    }

    void ServerBrowser::LoadFavorites()
    {
        // Load favorite servers from persistent storage
        // Implementation would depend on storage mechanism
    }

    void ServerBrowser::SaveFavorites()
    {
        // Save favorite servers to persistent storage
        // Implementation would depend on storage mechanism
    }

    void ServerBrowser::LoadRecentServers()
    {
        // Load recent servers from persistent storage
        // Implementation would depend on storage mechanism
    }

    void ServerBrowser::SaveRecentServers()
    {
        // Save recent servers to persistent storage
        // Implementation would depend on storage mechanism
    }

    bool ServerBrowser::SendServerQuery(const std::string& address, uint16_t port, ServerInfo& info)
    {
        // Send query packet to server and parse response
        // Implementation would depend on network protocol
        return false; // Placeholder
    }

    uint32_t ServerBrowser::MeasurePing(const std::string& address, uint16_t port)
    {
        // Measure network latency to server
        auto start = std::chrono::high_resolution_clock::now();

        // Simulate ping measurement (in real implementation, would send ping packet)
        std::this_thread::sleep_for(std::chrono::milliseconds(10 + rand() % 100));

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        return static_cast<uint32_t>(duration.count());
    }

    bool ServerBrowser::ValidateServerVersion(const std::string& serverVersion)
    {
        // Validate that server version is compatible with client
        // For now, accept all versions
        return !serverVersion.empty();
    }

    // Matchmaker Implementation
    Matchmaker& Matchmaker::GetInstance()
    {
        static Matchmaker instance;
        return instance;
    }

    void Matchmaker::Initialize()
    {
        m_isMatchmaking = false;
        m_matchmakingTimeout = 300; // 5 minutes
        m_maxSearchRadius = 300; // 300ms ping
        m_skillMatchingEnabled = true;
        m_successfulMatches = 0;
        m_totalMatchTime = std::chrono::milliseconds(0);
        m_currentSearchRadius = 50; // Start with 50ms ping radius
    }

    void Matchmaker::Shutdown()
    {
        StopMatchmaking();

        m_matchFoundCallback = nullptr;
        m_matchmakingFailedCallback = nullptr;
        m_matchmakingProgressCallback = nullptr;
    }

    void Matchmaker::Update()
    {
        if (!m_isMatchmaking) {
            return;
        }

        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - m_matchmakingStartTime).count();

        if (elapsed >= m_matchmakingTimeout) {
            FailMatchmaking(MatchmakingResult::Timeout);
            return;
        }

        // Perform matchmaking step
        PerformMatchmakingStep();
    }

    void Matchmaker::StartMatchmaking(const MatchmakingPreferences& preferences)
    {
        if (m_isMatchmaking) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_matchmakingMutex);

        m_isMatchmaking = true;
        m_currentPreferences = preferences;
        m_matchmakingStartTime = std::chrono::steady_clock::now();
        m_currentSearchRadius = 50; // Reset search radius
        m_candidateServers.clear();

        UpdateMatchmakingProgress();
    }

    void Matchmaker::StopMatchmaking()
    {
        if (!m_isMatchmaking) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_matchmakingMutex);

        m_isMatchmaking = false;
        m_candidateServers.clear();

        if (m_matchmakingFailedCallback) {
            m_matchmakingFailedCallback(MatchmakingResult::Cancelled);
        }
    }

    bool Matchmaker::IsMatchmaking() const
    {
        return m_isMatchmaking;
    }

    void Matchmaker::QuickMatch(const std::string& gameMode)
    {
        MatchmakingPreferences preferences;
        preferences.preferredGameMode = gameMode;
        preferences.maxPing = 150;
        preferences.allowPasswordProtected = false;
        preferences.allowModded = false;
        preferences.skillLevel = 0.5f;

        StartMatchmaking(preferences);
    }

    void Matchmaker::FindSkillBasedMatch(const MatchmakingPreferences& preferences)
    {
        MatchmakingPreferences skillPrefs = preferences;
        skillPrefs.maxPing = std::min(preferences.maxPing, 100u); // Lower ping for skill matching

        StartMatchmaking(skillPrefs);
    }

    void Matchmaker::FindPartyMatch(const std::vector<uint32_t>& partyMembers,
                                   const MatchmakingPreferences& preferences)
    {
        // Party matchmaking would need special handling for group size
        // For now, use standard matchmaking
        StartMatchmaking(preferences);
    }

    void Matchmaker::FindCustomMatch(const std::function<bool(const ServerInfo&)>& customFilter)
    {
        // Custom matchmaking with user-defined filter
        MatchmakingPreferences preferences;
        preferences.maxPing = 200;

        // Store custom filter for use in matchmaking
        StartMatchmaking(preferences);
    }

    void Matchmaker::FindRegionalMatch(const std::string& region,
                                      const MatchmakingPreferences& preferences)
    {
        MatchmakingPreferences regionalPrefs = preferences;
        regionalPrefs.preferredRegion = region;

        StartMatchmaking(regionalPrefs);
    }

    void Matchmaker::SetMatchFoundCallback(MatchFoundCallback callback)
    {
        m_matchFoundCallback = callback;
    }

    void Matchmaker::SetMatchmakingFailedCallback(MatchmakingFailedCallback callback)
    {
        m_matchmakingFailedCallback = callback;
    }

    void Matchmaker::SetMatchmakingProgressCallback(MatchmakingProgressCallback callback)
    {
        m_matchmakingProgressCallback = callback;
    }

    void Matchmaker::SetMatchmakingTimeout(uint32_t seconds)
    {
        m_matchmakingTimeout = seconds;
    }

    void Matchmaker::SetMaxSearchRadius(uint32_t maxPing)
    {
        m_maxSearchRadius = maxPing;
    }

    void Matchmaker::EnableSkillMatching(bool enabled)
    {
        m_skillMatchingEnabled = enabled;
    }

    std::chrono::milliseconds Matchmaker::GetAverageMatchTime() const
    {
        if (m_successfulMatches == 0) {
            return std::chrono::milliseconds(0);
        }

        return m_totalMatchTime / m_successfulMatches;
    }

    uint32_t Matchmaker::GetSuccessfulMatchesCount() const
    {
        return m_successfulMatches;
    }

    float Matchmaker::GetCurrentSearchProgress() const
    {
        if (!m_isMatchmaking) {
            return 0.0f;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - m_matchmakingStartTime).count();

        return std::min(1.0f, static_cast<float>(elapsed) / m_matchmakingTimeout);
    }

    void Matchmaker::PerformMatchmakingStep()
    {
        // Find candidate servers
        m_candidateServers = FindCandidateServers(m_currentPreferences);

        if (!m_candidateServers.empty()) {
            // Select best match
            ServerInfo bestMatch = SelectBestMatch(m_candidateServers, m_currentPreferences);
            CompleteMatchmaking(bestMatch);
        } else {
            // Expand search radius and try again
            ExpandSearchRadius();

            if (m_currentSearchRadius > m_maxSearchRadius) {
                FailMatchmaking(MatchmakingResult::NoSuitableServers);
            }
        }

        UpdateMatchmakingProgress();
    }

    std::vector<ServerInfo> Matchmaker::FindCandidateServers(const MatchmakingPreferences& preferences)
    {
        auto& serverBrowser = ServerBrowser::GetInstance();
        auto allServers = serverBrowser.GetServerList();

        std::vector<ServerInfo> candidates;

        for (const auto& server : allServers) {
            if (IsServerSuitable(server, preferences)) {
                candidates.push_back(server);
            }
        }

        return candidates;
    }

    ServerInfo Matchmaker::SelectBestMatch(const std::vector<ServerInfo>& candidates,
                                          const MatchmakingPreferences& preferences)
    {
        if (candidates.empty()) {
            return ServerInfo();
        }

        // Calculate scores for all candidates
        std::vector<std::pair<ServerInfo, float>> scoredServers;

        for (const auto& server : candidates) {
            float score = CalculateServerScore(server, preferences);
            scoredServers.emplace_back(server, score);
        }

        // Sort by score (higher is better)
        std::sort(scoredServers.begin(), scoredServers.end(),
            [](const auto& a, const auto& b) {
                return a.second > b.second;
            });

        return scoredServers[0].first;
    }

    float Matchmaker::CalculateServerScore(const ServerInfo& server,
                                          const MatchmakingPreferences& preferences)
    {
        float score = 0.0f;

        // Ping score (lower ping = higher score)
        float pingScore = 1.0f - (static_cast<float>(server.ping) / preferences.maxPing);
        score += pingScore * 0.3f;

        // Population score (prefer servers with some players but not full)
        float populationRatio = static_cast<float>(server.currentPlayers) / server.maxPlayers;
        float populationScore = 1.0f - std::abs(populationRatio - 0.7f); // Target 70% full
        score += populationScore * 0.2f;

        // Game mode match
        if (!preferences.preferredGameMode.empty() && server.gameMode == preferences.preferredGameMode) {
            score += 0.2f;
        }

        // Region match
        if (!preferences.preferredRegion.empty() && server.region == preferences.preferredRegion) {
            score += 0.1f;
        }

        // Skill matching
        if (m_skillMatchingEnabled) {
            float skillCompatibility = CalculateSkillCompatibility(server, preferences.skillLevel);
            score += skillCompatibility * 0.2f;
        }

        return score;
    }

    bool Matchmaker::IsServerSuitable(const ServerInfo& server,
                                     const MatchmakingPreferences& preferences)
    {
        // Basic suitability checks
        if (server.ping > m_currentSearchRadius) {
            return false;
        }

        if (server.currentPlayers >= server.maxPlayers) {
            return false; // Server is full
        }

        if (!preferences.allowPasswordProtected && server.isPasswordProtected) {
            return false;
        }

        if (!preferences.allowModded && server.isModded) {
            return false;
        }

        // Game mode preference
        if (!preferences.preferredGameMode.empty() && server.gameMode != preferences.preferredGameMode) {
            return false;
        }

        return true;
    }

    void Matchmaker::ExpandSearchRadius()
    {
        m_currentSearchRadius = std::min(m_currentSearchRadius * 2, m_maxSearchRadius);
    }

    void Matchmaker::UpdateMatchmakingProgress()
    {
        if (m_matchmakingProgressCallback) {
            float progress = GetCurrentSearchProgress();
            m_matchmakingProgressCallback(progress);
        }
    }

    void Matchmaker::CompleteMatchmaking(const ServerInfo& selectedServer)
    {
        std::lock_guard<std::mutex> lock(m_matchmakingMutex);

        auto matchTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_matchmakingStartTime);

        m_successfulMatches++;
        m_totalMatchTime += matchTime;

        m_isMatchmaking = false;

        if (m_matchFoundCallback) {
            m_matchFoundCallback(selectedServer);
        }
    }

    void Matchmaker::FailMatchmaking(MatchmakingResult result)
    {
        std::lock_guard<std::mutex> lock(m_matchmakingMutex);

        m_isMatchmaking = false;

        if (m_matchmakingFailedCallback) {
            m_matchmakingFailedCallback(result);
        }
    }

    float Matchmaker::CalculateSkillCompatibility(const ServerInfo& server, float playerSkill)
    {
        float serverSkill = EstimateServerSkillLevel(server);
        float skillDifference = std::abs(serverSkill - playerSkill);

        // Return compatibility score (1.0 = perfect match, 0.0 = poor match)
        return std::max(0.0f, 1.0f - skillDifference);
    }

    float Matchmaker::EstimateServerSkillLevel(const ServerInfo& server)
    {
        // Estimate server skill level based on various factors
        // This is a simplified implementation

        // Check server tags for skill indicators
        for (const auto& tag : server.tags) {
            if (tag == "beginner" || tag == "casual") {
                return 0.3f;
            } else if (tag == "hardcore" || tag == "competitive") {
                return 0.8f;
            } else if (tag == "pro" || tag == "expert") {
                return 0.9f;
            }
        }

        // Default to medium skill
        return 0.5f;
    }

    // Utility functions
    namespace ServerUtils
    {
        std::string GenerateServerId()
        {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dis(100000, 999999);

            return "server_" + std::to_string(dis(gen));
        }

        bool ValidateServerName(const std::string& name)
        {
            return !name.empty() && name.length() <= 64;
        }

        bool ValidateServerDescription(const std::string& description)
        {
            return description.length() <= 256;
        }

        std::string FormatPlayerCount(uint32_t current, uint32_t max)
        {
            return std::to_string(current) + "/" + std::to_string(max);
        }

        std::string FormatPing(uint32_t ping)
        {
            return std::to_string(ping) + "ms";
        }

        std::string GetRegionFromIP(const std::string& ipAddress)
        {
            // Simplified region detection based on IP
            // In real implementation, would use GeoIP database

            if (ipAddress.substr(0, 3) == "192") {
                return "Local";
            }

            return "Unknown";
        }

        bool IsPrivateIP(const std::string& ipAddress)
        {
            // Check for private IP ranges
            return ipAddress.substr(0, 3) == "192" ||
                   ipAddress.substr(0, 3) == "172" ||
                   ipAddress.substr(0, 2) == "10.";
        }

        std::vector<std::string> ParseServerTags(const std::string& tagString)
        {
            std::vector<std::string> tags;
            std::stringstream ss(tagString);
            std::string tag;

            while (std::getline(ss, tag, ',')) {
                // Trim whitespace
                tag.erase(0, tag.find_first_not_of(" \t"));
                tag.erase(tag.find_last_not_of(" \t") + 1);

                if (!tag.empty()) {
                    tags.push_back(tag);
                }
            }

            return tags;
        }

        std::string ServerTagsToString(const std::vector<std::string>& tags)
        {
            std::string result;

            for (size_t i = 0; i < tags.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += tags[i];
            }

            return result;
        }
    }
}
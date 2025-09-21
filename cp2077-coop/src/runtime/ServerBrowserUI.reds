// Server Browser UI Integration
// REDscript interface for server browser and matchmaking functionality

// Server Browser UI Manager
public class ServerBrowserUIManager extends ScriptableSystem {
    private static let s_instance: ref<ServerBrowserUIManager>;
    private let m_isUIVisible: Bool = false;
    private let m_currentServerList: array<ref<ServerEntry>>;
    private let m_filteredServerList: array<ref<ServerEntry>>;
    private let m_favoriteServers: array<String>;
    private let m_recentServers: array<String>;
    private let m_currentFilters: ServerBrowserFilters;
    private let m_isRefreshing: Bool = false;
    private let m_selectedServer: ref<ServerEntry>;
    private let m_sortCriteria: EServerSortCriteria = EServerSortCriteria.Name;
    private let m_sortAscending: Bool = true;
    private let m_refreshTimer: Float = 0.0;
    private let m_autoRefreshInterval: Float = 30.0; // 30 seconds

    public static func GetInstance() -> ref<ServerBrowserUIManager> {
        if !IsDefined(ServerBrowserUIManager.s_instance) {
            ServerBrowserUIManager.s_instance = new ServerBrowserUIManager();
        }
        return ServerBrowserUIManager.s_instance;
    }

    public func Initialize() -> Void {
        // Initialize server browser
        this.InitializeFilters();
        this.LoadFavoriteServers();
        this.LoadRecentServers();

        // Register for server browser callbacks
        Native_RegisterServerBrowserCallbacks();

        LogChannel(n"ServerBrowser", s"[ServerBrowser] Server Browser UI initialized");
    }

    private func InitializeFilters() -> Void {
        // Set default filters
        this.m_currentFilters.nameFilter = "";
        this.m_currentFilters.gameModeFilter = "";
        this.m_currentFilters.regionFilter = "";
        this.m_currentFilters.maxPing = 150;
        this.m_currentFilters.showPasswordProtected = false;
        this.m_currentFilters.showModded = true;
        this.m_currentFilters.showFull = false;
        this.m_currentFilters.showEmpty = true;
        this.m_currentFilters.minPlayers = 0;
        this.m_currentFilters.maxPlayers = 999;
    }

    public func OnUpdate(deltaTime: Float) -> Void {
        this.m_refreshTimer += deltaTime;

        // Auto refresh server list
        if this.m_refreshTimer >= this.m_autoRefreshInterval {
            this.RefreshServerList();
            this.m_refreshTimer = 0.0;
        }
    }

    // UI Control Methods
    public func ShowServerBrowser() -> Void {
        if this.m_isUIVisible {
            return;
        }

        this.m_isUIVisible = true;

        // Create and show server browser UI
        this.CreateServerBrowserUI();

        // Initial server list refresh
        this.RefreshServerList();

        LogChannel(n"ServerBrowser", s"[ServerBrowser] Server browser opened");
    }

    public func HideServerBrowser() -> Void {
        if !this.m_isUIVisible {
            return;
        }

        this.m_isUIVisible = false;

        // Hide server browser UI
        this.DestroyServerBrowserUI();

        LogChannel(n"ServerBrowser", s"[ServerBrowser] Server browser closed");
    }

    public func RefreshServerList() -> Void {
        if this.m_isRefreshing {
            return;
        }

        this.m_isRefreshing = true;

        // Call native function to refresh server list
        Native_RefreshServerList();

        // Update UI
        this.UpdateRefreshingState(true);

        LogChannel(n"ServerBrowser", s"[ServerBrowser] Refreshing server list...");
    }

    // Server List Management
    public func OnServerListUpdated(servers: array<ServerNetworkData>) -> Void {
        // Convert network data to UI entries
        ArrayClear(this.m_currentServerList);

        for serverData in servers {
            let entry = new ServerEntry();
            entry.InitializeFromNetworkData(serverData);
            ArrayPush(this.m_currentServerList, entry);
        }

        // Apply current filters
        this.ApplyFilters();

        // Sort servers
        this.SortServers(this.m_sortCriteria, this.m_sortAscending);

        // Update UI
        this.UpdateServerListUI();

        this.m_isRefreshing = false;
        this.UpdateRefreshingState(false);

        LogChannel(n"ServerBrowser", s"[ServerBrowser] Server list updated: " + ArraySize(this.m_currentServerList) + " servers");
    }

    // Server Filtering
    public func SetNameFilter(nameFilter: String) -> Void {
        this.m_currentFilters.nameFilter = nameFilter;
        this.ApplyFilters();
    }

    public func SetGameModeFilter(gameModeFilter: String) -> Void {
        this.m_currentFilters.gameModeFilter = gameModeFilter;
        this.ApplyFilters();
    }

    public func SetRegionFilter(regionFilter: String) -> Void {
        this.m_currentFilters.regionFilter = regionFilter;
        this.ApplyFilters();
    }

    public func SetMaxPing(maxPing: Int32) -> Void {
        this.m_currentFilters.maxPing = maxPing;
        this.ApplyFilters();
    }

    public func SetShowPasswordProtected(show: Bool) -> Void {
        this.m_currentFilters.showPasswordProtected = show;
        this.ApplyFilters();
    }

    public func SetShowModded(show: Bool) -> Void {
        this.m_currentFilters.showModded = show;
        this.ApplyFilters();
    }

    public func SetShowFull(show: Bool) -> Void {
        this.m_currentFilters.showFull = show;
        this.ApplyFilters();
    }

    public func SetShowEmpty(show: Bool) -> Void {
        this.m_currentFilters.showEmpty = show;
        this.ApplyFilters();
    }

    public func ClearFilters() -> Void {
        this.InitializeFilters();
        this.ApplyFilters();
    }

    private func ApplyFilters() -> Void {
        ArrayClear(this.m_filteredServerList);

        for server in this.m_currentServerList {
            if this.ServerMatchesFilters(server) {
                ArrayPush(this.m_filteredServerList, server);
            }
        }

        this.UpdateServerListUI();
    }

    private func ServerMatchesFilters(server: ref<ServerEntry>) -> Bool {
        // Name filter
        if !Equals(this.m_currentFilters.nameFilter, "") {
            let serverNameLower = StrLower(server.GetName());
            let filterLower = StrLower(this.m_currentFilters.nameFilter);
            if !StrContains(serverNameLower, filterLower) {
                return false;
            }
        }

        // Game mode filter
        if !Equals(this.m_currentFilters.gameModeFilter, "") &&
           !Equals(server.GetGameMode(), this.m_currentFilters.gameModeFilter) {
            return false;
        }

        // Region filter
        if !Equals(this.m_currentFilters.regionFilter, "") &&
           !Equals(server.GetRegion(), this.m_currentFilters.regionFilter) {
            return false;
        }

        // Ping filter
        if server.GetPing() > this.m_currentFilters.maxPing {
            return false;
        }

        // Password protection filter
        if !this.m_currentFilters.showPasswordProtected && server.IsPasswordProtected() {
            return false;
        }

        // Modded filter
        if !this.m_currentFilters.showModded && server.IsModded() {
            return false;
        }

        // Full server filter
        if !this.m_currentFilters.showFull && server.IsFull() {
            return false;
        }

        // Empty server filter
        if !this.m_currentFilters.showEmpty && server.IsEmpty() {
            return false;
        }

        // Player count filter
        let playerCount = server.GetCurrentPlayers();
        if playerCount < this.m_currentFilters.minPlayers ||
           playerCount > this.m_currentFilters.maxPlayers {
            return false;
        }

        return true;
    }

    // Server Sorting
    public func SortByName(ascending: Bool) -> Void {
        this.m_sortCriteria = EServerSortCriteria.Name;
        this.m_sortAscending = ascending;
        this.SortServers(this.m_sortCriteria, this.m_sortAscending);
    }

    public func SortByPlayers(ascending: Bool) -> Void {
        this.m_sortCriteria = EServerSortCriteria.Players;
        this.m_sortAscending = ascending;
        this.SortServers(this.m_sortCriteria, this.m_sortAscending);
    }

    public func SortByPing(ascending: Bool) -> Void {
        this.m_sortCriteria = EServerSortCriteria.Ping;
        this.m_sortAscending = ascending;
        this.SortServers(this.m_sortCriteria, this.m_sortAscending);
    }

    public func SortByGameMode(ascending: Bool) -> Void {
        this.m_sortCriteria = EServerSortCriteria.GameMode;
        this.m_sortAscending = ascending;
        this.SortServers(this.m_sortCriteria, this.m_sortAscending);
    }

    private func SortServers(criteria: EServerSortCriteria, ascending: Bool) -> Void {
        // Call native sorting function
        Native_SortServers(Cast<Int32>(criteria), ascending);

        // Update UI
        this.UpdateServerListUI();
    }

    // Server Selection and Connection
    public func SelectServer(serverId: String) -> Void {
        for server in this.m_filteredServerList {
            if Equals(server.GetId(), serverId) {
                this.m_selectedServer = server;
                this.UpdateServerDetailsUI(server);
                break;
            }
        }
    }

    public func ConnectToSelectedServer(password: String) -> Bool {
        if !IsDefined(this.m_selectedServer) {
            return false;
        }

        // Validate connection requirements
        if this.m_selectedServer.IsPasswordProtected() && Equals(password, "") {
            this.ShowPasswordRequiredDialog();
            return false;
        }

        if this.m_selectedServer.IsFull() {
            this.ShowServerFullDialog();
            return false;
        }

        // Attempt connection
        let success = Native_ConnectToServer(this.m_selectedServer.GetId(), password);

        if success {
            // Add to recent servers
            this.AddServerToRecent(this.m_selectedServer.GetId());

            // Hide server browser
            this.HideServerBrowser();

            LogChannel(n"ServerBrowser", s"[ServerBrowser] Connecting to server: " + this.m_selectedServer.GetName());
        } else {
            this.ShowConnectionFailedDialog();
        }

        return success;
    }

    public func ConnectToServer(serverId: String, password: String) -> Bool {
        this.SelectServer(serverId);
        return this.ConnectToSelectedServer(password);
    }

    // Favorites Management
    public func AddServerToFavorites(serverId: String) -> Void {
        if this.IsServerFavorite(serverId) {
            return;
        }

        ArrayPush(this.m_favoriteServers, serverId);
        this.SaveFavoriteServers();

        // Call native function
        Native_AddServerToFavorites(serverId);

        // Update UI
        this.UpdateFavoritesUI();

        LogChannel(n"ServerBrowser", s"[ServerBrowser] Added server to favorites: " + serverId);
    }

    public func RemoveServerFromFavorites(serverId: String) -> Void {
        if !this.IsServerFavorite(serverId) {
            return;
        }

        ArrayRemove(this.m_favoriteServers, serverId);
        this.SaveFavoriteServers();

        // Call native function
        Native_RemoveServerFromFavorites(serverId);

        // Update UI
        this.UpdateFavoritesUI();

        LogChannel(n"ServerBrowser", s"[ServerBrowser] Removed server from favorites: " + serverId);
    }

    public func IsServerFavorite(serverId: String) -> Bool {
        for favoriteId in this.m_favoriteServers {
            if Equals(favoriteId, serverId) {
                return true;
            }
        }
        return false;
    }

    public func GetFavoriteServers() -> array<ref<ServerEntry>> {
        let favorites: array<ref<ServerEntry>>;

        for favoriteId in this.m_favoriteServers {
            for server in this.m_currentServerList {
                if Equals(server.GetId(), favoriteId) {
                    ArrayPush(favorites, server);
                    break;
                }
            }
        }

        return favorites;
    }

    // Recent Servers
    private func AddServerToRecent(serverId: String) -> Void {
        // Remove if already exists
        ArrayRemove(this.m_recentServers, serverId);

        // Add to front
        ArrayInsert(this.m_recentServers, 0, serverId);

        // Limit to 10 recent servers
        if ArraySize(this.m_recentServers) > 10 {
            ArrayResize(this.m_recentServers, 10);
        }

        this.SaveRecentServers();

        // Call native function
        Native_AddServerToRecent(serverId);
    }

    public func GetRecentServers() -> array<ref<ServerEntry>> {
        let recent: array<ref<ServerEntry>>;

        for recentId in this.m_recentServers {
            for server in this.m_currentServerList {
                if Equals(server.GetId(), recentId) {
                    ArrayPush(recent, server);
                    break;
                }
            }
        }

        return recent;
    }

    // UI Update Methods
    private func CreateServerBrowserUI() -> Void {
        // Create server browser UI elements
        // This would integrate with the game's UI system
    }

    private func DestroyServerBrowserUI() -> Void {
        // Destroy server browser UI elements
    }

    private func UpdateServerListUI() -> Void {
        // Update the server list display
        // This would populate the UI with filtered and sorted servers
    }

    private func UpdateServerDetailsUI(server: ref<ServerEntry>) -> Void {
        // Update server details panel with selected server info
    }

    private func UpdateRefreshingState(isRefreshing: Bool) -> Void {
        // Update UI to show/hide refreshing indicator
    }

    private func UpdateFavoritesUI() -> Void {
        // Update favorites display
    }

    // Dialog Methods
    private func ShowPasswordRequiredDialog() -> Void {
        // Show password input dialog
    }

    private func ShowServerFullDialog() -> Void {
        // Show server full message
    }

    private func ShowConnectionFailedDialog() -> Void {
        // Show connection failed message
    }

    // Persistence Methods
    private func LoadFavoriteServers() -> Void {
        // Load favorite servers from storage
        // Placeholder - would load from persistent storage
    }

    private func SaveFavoriteServers() -> Void {
        // Save favorite servers to storage
        // Placeholder - would save to persistent storage
    }

    private func LoadRecentServers() -> Void {
        // Load recent servers from storage
        // Placeholder - would load from persistent storage
    }

    private func SaveRecentServers() -> Void {
        // Save recent servers to storage
        // Placeholder - would save to persistent storage
    }

    // Public API
    public func IsUIVisible() -> Bool {
        return this.m_isUIVisible;
    }

    public func GetCurrentServerCount() -> Int32 {
        return ArraySize(this.m_currentServerList);
    }

    public func GetFilteredServerCount() -> Int32 {
        return ArraySize(this.m_filteredServerList);
    }

    public func GetSelectedServer() -> ref<ServerEntry> {
        return this.m_selectedServer;
    }

    public func IsRefreshing() -> Bool {
        return this.m_isRefreshing;
    }
}

// Server Entry Class for UI Display
public class ServerEntry extends ScriptableComponent {
    private let m_id: String;
    private let m_name: String;
    private let m_description: String;
    private let m_hostName: String;
    private let m_gameMode: String;
    private let m_mapName: String;
    private let m_currentPlayers: Int32;
    private let m_maxPlayers: Int32;
    private let m_ping: Int32;
    private let m_isPasswordProtected: Bool;
    private let m_isModded: Bool;
    private let m_allowsCrossPlatform: Bool;
    private let m_version: String;
    private let m_region: String;
    private let m_tags: array<String>;
    private let m_ipAddress: String;
    private let m_port: Int32;

    public func InitializeFromNetworkData(data: ServerNetworkData) -> Void {
        this.m_id = data.serverId;
        this.m_name = data.serverName;
        this.m_description = data.description;
        this.m_hostName = data.hostName;
        this.m_gameMode = data.gameMode;
        this.m_mapName = data.mapName;
        this.m_currentPlayers = data.currentPlayers;
        this.m_maxPlayers = data.maxPlayers;
        this.m_ping = data.ping;
        this.m_isPasswordProtected = data.isPasswordProtected;
        this.m_isModded = data.isModded;
        this.m_allowsCrossPlatform = data.allowsCrossPlatform;
        this.m_version = data.version;
        this.m_region = data.region;
        this.m_tags = data.tags;
        this.m_ipAddress = data.ipAddress;
        this.m_port = data.port;
    }

    // Getters
    public func GetId() -> String { return this.m_id; }
    public func GetName() -> String { return this.m_name; }
    public func GetDescription() -> String { return this.m_description; }
    public func GetHostName() -> String { return this.m_hostName; }
    public func GetGameMode() -> String { return this.m_gameMode; }
    public func GetMapName() -> String { return this.m_mapName; }
    public func GetCurrentPlayers() -> Int32 { return this.m_currentPlayers; }
    public func GetMaxPlayers() -> Int32 { return this.m_maxPlayers; }
    public func GetPing() -> Int32 { return this.m_ping; }
    public func IsPasswordProtected() -> Bool { return this.m_isPasswordProtected; }
    public func IsModded() -> Bool { return this.m_isModded; }
    public func AllowsCrossPlatform() -> Bool { return this.m_allowsCrossPlatform; }
    public func GetVersion() -> String { return this.m_version; }
    public func GetRegion() -> String { return this.m_region; }
    public func GetTags() -> array<String> { return this.m_tags; }
    public func GetIPAddress() -> String { return this.m_ipAddress; }
    public func GetPort() -> Int32 { return this.m_port; }

    // Helper methods
    public func IsFull() -> Bool {
        return this.m_currentPlayers >= this.m_maxPlayers;
    }

    public func IsEmpty() -> Bool {
        return this.m_currentPlayers == 0;
    }

    public func GetPlayerCountString() -> String {
        return ToString(this.m_currentPlayers) + "/" + ToString(this.m_maxPlayers);
    }

    public func GetPingString() -> String {
        if this.m_ping < 50 {
            return ToString(this.m_ping) + "ms (Excellent)";
        } else if this.m_ping < 100 {
            return ToString(this.m_ping) + "ms (Good)";
        } else if this.m_ping < 150 {
            return ToString(this.m_ping) + "ms (Fair)";
        } else {
            return ToString(this.m_ping) + "ms (Poor)";
        }
    }

    public func HasTag(tag: String) -> Bool {
        for serverTag in this.m_tags {
            if Equals(serverTag, tag) {
                return true;
            }
        }
        return false;
    }
}

// Matchmaker UI Manager
public class MatchmakerUIManager extends ScriptableComponent {
    private static let s_instance: ref<MatchmakerUIManager>;
    private let m_isMatchmaking: Bool = false;
    private let m_matchmakingProgress: Float = 0.0;
    private let m_currentPreferences: MatchmakingUIPreferences;
    private let m_estimatedWaitTime: Float = 0.0;

    public static func GetInstance() -> ref<MatchmakerUIManager> {
        if !IsDefined(MatchmakerUIManager.s_instance) {
            MatchmakerUIManager.s_instance = new MatchmakerUIManager();
        }
        return MatchmakerUIManager.s_instance;
    }

    public func Initialize() -> Void {
        // Register for matchmaker callbacks
        Native_RegisterMatchmakerCallbacks();

        LogChannel(n"Matchmaker", s"[Matchmaker] Matchmaker UI initialized");
    }

    // Quick Match
    public func StartQuickMatch(gameMode: String) -> Void {
        if this.m_isMatchmaking {
            return;
        }

        this.m_isMatchmaking = true;
        this.m_matchmakingProgress = 0.0;

        // Set default preferences for quick match
        this.m_currentPreferences.preferredGameMode = gameMode;
        this.m_currentPreferences.maxPing = 150;
        this.m_currentPreferences.allowPasswordProtected = false;
        this.m_currentPreferences.allowModded = false;
        this.m_currentPreferences.skillLevel = 0.5; // Medium skill

        // Start matchmaking
        Native_StartQuickMatch(gameMode);

        // Show matchmaking UI
        this.ShowMatchmakingUI();

        LogChannel(n"Matchmaker", s"[Matchmaker] Quick match started for: " + gameMode);
    }

    // Custom Matchmaking
    public func StartCustomMatchmaking(preferences: MatchmakingUIPreferences) -> Void {
        if this.m_isMatchmaking {
            return;
        }

        this.m_isMatchmaking = true;
        this.m_matchmakingProgress = 0.0;
        this.m_currentPreferences = preferences;

        // Start custom matchmaking
        Native_StartCustomMatchmaking(preferences);

        // Show matchmaking UI
        this.ShowMatchmakingUI();

        LogChannel(n"Matchmaker", s"[Matchmaker] Custom matchmaking started");
    }

    // Stop Matchmaking
    public func StopMatchmaking() -> Void {
        if !this.m_isMatchmaking {
            return;
        }

        this.m_isMatchmaking = false;
        this.m_matchmakingProgress = 0.0;

        // Stop native matchmaking
        Native_StopMatchmaking();

        // Hide matchmaking UI
        this.HideMatchmakingUI();

        LogChannel(n"Matchmaker", s"[Matchmaker] Matchmaking stopped");
    }

    // Matchmaking Events
    public func OnMatchFound(serverData: ServerNetworkData) -> Void {
        this.m_isMatchmaking = false;
        this.m_matchmakingProgress = 1.0;

        // Hide matchmaking UI
        this.HideMatchmakingUI();

        // Show match found dialog
        this.ShowMatchFoundDialog(serverData);

        LogChannel(n"Matchmaker", s"[Matchmaker] Match found: " + serverData.serverName);
    }

    public func OnMatchmakingFailed(reason: EMatchmakingFailReason) -> Void {
        this.m_isMatchmaking = false;
        this.m_matchmakingProgress = 0.0;

        // Hide matchmaking UI
        this.HideMatchmakingUI();

        // Show failure dialog
        this.ShowMatchmakingFailedDialog(reason);

        let reasonText = this.GetFailureReasonText(reason);
        LogChannel(n"Matchmaker", s"[Matchmaker] Matchmaking failed: " + reasonText);
    }

    public func OnMatchmakingProgress(progress: Float) -> Void {
        this.m_matchmakingProgress = progress;

        // Update progress UI
        this.UpdateMatchmakingProgressUI(progress);
    }

    // UI Methods
    private func ShowMatchmakingUI() -> Void {
        // Show matchmaking progress dialog
    }

    private func HideMatchmakingUI() -> Void {
        // Hide matchmaking progress dialog
    }

    private func UpdateMatchmakingProgressUI(progress: Float) -> Void {
        // Update progress bar and estimated time
    }

    private func ShowMatchFoundDialog(serverData: ServerNetworkData) -> Void {
        // Show match found dialog with server details and join option
    }

    private func ShowMatchmakingFailedDialog(reason: EMatchmakingFailReason) -> Void {
        // Show failure dialog with retry option
    }

    private func GetFailureReasonText(reason: EMatchmakingFailReason) -> String {
        switch reason {
            case EMatchmakingFailReason.NoServersFound:
                return "No suitable servers found";
            case EMatchmakingFailReason.AllServersFull:
                return "All servers are full";
            case EMatchmakingFailReason.NetworkError:
                return "Network connection error";
            case EMatchmakingFailReason.Timeout:
                return "Matchmaking timed out";
            case EMatchmakingFailReason.Cancelled:
                return "Matchmaking was cancelled";
            default:
                return "Unknown error";
        }
    }

    // Getters
    public func IsMatchmaking() -> Bool {
        return this.m_isMatchmaking;
    }

    public func GetMatchmakingProgress() -> Float {
        return this.m_matchmakingProgress;
    }

    public func GetCurrentPreferences() -> MatchmakingUIPreferences {
        return this.m_currentPreferences;
    }
}

// Data Structures
public struct ServerBrowserFilters {
    public let nameFilter: String;
    public let gameModeFilter: String;
    public let regionFilter: String;
    public let maxPing: Int32;
    public let showPasswordProtected: Bool;
    public let showModded: Bool;
    public let showFull: Bool;
    public let showEmpty: Bool;
    public let minPlayers: Int32;
    public let maxPlayers: Int32;
}

public struct MatchmakingUIPreferences {
    public let preferredGameMode: String;
    public let preferredRegion: String;
    public let maxPing: Int32;
    public let allowPasswordProtected: Bool;
    public let allowModded: Bool;
    public let skillLevel: Float; // 0.0 to 1.0
    public let preferredTags: array<String>;
}

public struct ServerNetworkData {
    public let serverId: String;
    public let serverName: String;
    public let description: String;
    public let hostName: String;
    public let gameMode: String;
    public let mapName: String;
    public let currentPlayers: Int32;
    public let maxPlayers: Int32;
    public let ping: Int32;
    public let isPasswordProtected: Bool;
    public let isModded: Bool;
    public let allowsCrossPlatform: Bool;
    public let version: String;
    public let region: String;
    public let tags: array<String>;
    public let ipAddress: String;
    public let port: Int32;
}

// Enumerations
public enum EServerSortCriteria : Uint8 {
    Name = 0,
    Players = 1,
    Ping = 2,
    GameMode = 3,
    Region = 4,
    LastUpdated = 5
}

public enum EMatchmakingFailReason : Uint8 {
    NoServersFound = 0,
    AllServersFull = 1,
    NetworkError = 2,
    Timeout = 3,
    Cancelled = 4
}

// Native function declarations for C++ integration
native func Native_RegisterServerBrowserCallbacks() -> Void;
native func Native_RefreshServerList() -> Void;
native func Native_SortServers(criteria: Int32, ascending: Bool) -> Void;
native func Native_ConnectToServer(serverId: String, password: String) -> Bool;
native func Native_AddServerToFavorites(serverId: String) -> Void;
native func Native_RemoveServerFromFavorites(serverId: String) -> Void;
native func Native_AddServerToRecent(serverId: String) -> Void;

native func Native_RegisterMatchmakerCallbacks() -> Void;
native func Native_StartQuickMatch(gameMode: String) -> Void;
native func Native_StartCustomMatchmaking(preferences: MatchmakingUIPreferences) -> Void;
native func Native_StopMatchmaking() -> Void;

// Network event handlers
@addMethod(PlayerPuppet)
public func OnNetworkServerListUpdated(servers: array<ServerNetworkData>) -> Void {
    ServerBrowserUIManager.GetInstance().OnServerListUpdated(servers);
}

@addMethod(PlayerPuppet)
public func OnNetworkMatchFound(serverData: ServerNetworkData) -> Void {
    MatchmakerUIManager.GetInstance().OnMatchFound(serverData);
}

@addMethod(PlayerPuppet)
public func OnNetworkMatchmakingFailed(reason: EMatchmakingFailReason) -> Void {
    MatchmakerUIManager.GetInstance().OnMatchmakingFailed(reason);
}

@addMethod(PlayerPuppet)
public func OnNetworkMatchmakingProgress(progress: Float) -> Void {
    MatchmakerUIManager.GetInstance().OnMatchmakingProgress(progress);
}

// Integration with main menu system
@wrapMethod(MenuScenario_SingleplayerMenu)
protected cb func OnInitialize() -> Bool {
    let result = wrappedMethod();

    // Initialize server browser when main menu loads
    ServerBrowserUIManager.GetInstance().Initialize();
    MatchmakerUIManager.GetInstance().Initialize();

    return result;
}

// Console commands for testing
@addMethod(PlayerPuppet)
public func ShowServerBrowser() -> Void {
    ServerBrowserUIManager.GetInstance().ShowServerBrowser();
}

@addMethod(PlayerPuppet)
public func StartQuickMatch(gameMode: String) -> Void {
    MatchmakerUIManager.GetInstance().StartQuickMatch(gameMode);
}
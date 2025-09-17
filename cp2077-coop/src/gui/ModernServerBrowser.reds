// Modern Server Browser with advanced filtering and search capabilities
// Provides intuitive server discovery and connection management

import Codeware.UI

public struct ServerInfo {
    public var id: Uint32;
    public var name: String;
    public var description: String;
    public var ip: String;
    public var port: Uint32;
    public var currentPlayers: Uint32;
    public var maxPlayers: Uint32;
    public var hasPassword: Bool;
    public var gameMode: String;
    public var mapName: String;
    public var ping: Uint32;
    public var version: String;
    public var region: String;
    public var tags: array<String>;
    public var lastUpdate: Uint64;
    public var isOfficial: Bool;
    public var isFavorite: Bool;
}

public enum ServerSortMode {
    Name = 0,
    Players = 1,
    Ping = 2,
    GameMode = 3,
    Region = 4
}

public class ModernServerBrowser {
    private var container: wref<inkCanvas>;
    private var parentUI: wref<CoopUI>;
    
    // UI Components
    private var searchBar: wref<inkTextInput>;
    private var refreshButton: wref<inkButton>;
    private var filterPanel: wref<inkHorizontalPanel>;
    private var serverList: wref<inkScrollArea>;
    private var serverDetails: wref<inkVerticalPanel>;
    private var actionButtons: wref<inkHorizontalPanel>;
    
    // Filter components
    private var regionFilter: wref<inkButton>; // Dropdown-like
    private var gameModeFilter: wref<inkButton>;
    private var playerCountFilter: wref<inkButton>;
    private var pingFilter: wref<inkButton>;
    private var passwordFilter: wref<inkButton>;
    private var favoritesOnly: wref<inkToggle>;
    
    // Action buttons
    private var joinButton: wref<inkButton>;
    private var favoriteButton: wref<inkButton>;
    private var refreshServerButton: wref<inkButton>;
    private var connectDirectButton: wref<inkButton>;
    
    // Data
    private var allServers: array<ServerInfo>;
    private var filteredServers: array<ServerInfo>;
    private var selectedServer: ref<ServerInfo>;
    private var sortMode: ServerSortMode;
    private var sortDescending: Bool;
    private var isRefreshing: Bool;
    
    // Search and filter state
    private var searchQuery: String;
    private var filterRegion: String;
    private var filterGameMode: String;
    private var filterMaxPing: Uint32;
    private var filterShowPasswordProtected: Bool;
    private var filterShowFavoritesOnly: Bool;
    private var filterMinPlayers: Uint32;
    private var filterMaxPlayers: Uint32;
    
    public func Initialize(parent: wref<inkCanvas>, ui: wref<CoopUI>) -> Void {
        this.parentUI = ui;
        this.InitializeDefaults();
        this.CreateUI(parent);
        this.RefreshServerList();
    }
    
    private func InitializeDefaults() -> Void {
        this.sortMode = ServerSortMode.Ping;
        this.sortDescending = false;
        this.searchQuery = "";
        this.filterRegion = "Any";
        this.filterGameMode = "Any";
        this.filterMaxPing = 200u;
        this.filterShowPasswordProtected = true;
        this.filterShowFavoritesOnly = false;
        this.filterMinPlayers = 0u;
        this.filterMaxPlayers = 32u;
    }
    
    private func CreateUI(parent: wref<inkCanvas>) -> Void {
        this.container = new inkCanvas();
        this.container.SetName(n"ServerBrowserContainer");
        this.container.SetAnchor(inkEAnchor.Fill);
        this.container.SetVisible(false);
        parent.AddChild(this.container);
        
        // Main layout - horizontal split
        let mainPanel = new inkHorizontalPanel();
        mainPanel.SetAnchor(inkEAnchor.Fill);
        mainPanel.SetChildMargin(new inkMargin(10.0, 0.0, 10.0, 0.0));
        this.container.AddChild(mainPanel);
        
        // Left panel - server list and filters
        let leftPanel = new inkVerticalPanel();
        leftPanel.SetSize(new Vector2(800.0, 0.0));
        leftPanel.SetChildMargin(new inkMargin(0.0, 5.0, 0.0, 5.0));
        mainPanel.AddChild(leftPanel);
        
        // Search and refresh bar
        this.CreateSearchBar(leftPanel);
        
        // Filter panel
        this.CreateFilterPanel(leftPanel);
        
        // Server list
        this.CreateServerList(leftPanel);
        
        // Right panel - server details and actions
        let rightPanel = new inkVerticalPanel();
        rightPanel.SetSize(new Vector2(400.0, 0.0));
        rightPanel.SetChildMargin(new inkMargin(0.0, 5.0, 0.0, 5.0));
        mainPanel.AddChild(rightPanel);
        
        // Server details
        this.CreateServerDetails(rightPanel);
        
        // Action buttons
        this.CreateActionButtons(rightPanel);
    }
    
    private func CreateSearchBar(parent: wref<inkVerticalPanel>) -> Void {
        let searchPanel = new inkHorizontalPanel();
        searchPanel.SetSize(new Vector2(0.0, 40.0));
        searchPanel.SetChildMargin(new inkMargin(5.0, 0.0, 5.0, 0.0));
        parent.AddChild(searchPanel);
        
        this.searchBar = new inkTextInput();
        this.searchBar.SetText(this.searchQuery);
        this.searchBar.SetSize(new Vector2(600.0, 35.0));
        this.searchBar.RegisterToCallback(n"OnInput", this, n"OnSearchChanged");
        searchPanel.AddChild(this.searchBar);
        
        this.refreshButton = new inkButton();
        this.refreshButton.SetText("🔄 REFRESH");
        this.refreshButton.SetSize(new Vector2(120.0, 35.0));
        this.refreshButton.RegisterToCallback(n"OnRelease", this, n"OnRefreshClicked");
        searchPanel.AddChild(this.refreshButton);
        
        let directConnectButton = new inkButton();
        directConnectButton.SetText("DIRECT IP");
        directConnectButton.SetSize(new Vector2(100.0, 35.0));
        directConnectButton.RegisterToCallback(n"OnRelease", this, n"OnDirectConnectClicked");
        searchPanel.AddChild(directConnectButton);
    }
    
    private func CreateFilterPanel(parent: wref<inkVerticalPanel>) -> Void {
        this.filterPanel = new inkHorizontalPanel();
        this.filterPanel.SetSize(new Vector2(0.0, 50.0));
        this.filterPanel.SetChildMargin(new inkMargin(5.0, 5.0, 5.0, 5.0));
        parent.AddChild(this.filterPanel);
        
        // Region filter
        this.regionFilter = new inkButton();
        this.regionFilter.SetText("Region: Any");
        this.regionFilter.SetSize(new Vector2(120.0, 40.0));
        this.regionFilter.RegisterToCallback(n"OnRelease", this, n"OnRegionFilterClicked");
        this.filterPanel.AddChild(this.regionFilter);
        
        // Game mode filter
        this.gameModeFilter = new inkButton();
        this.gameModeFilter.SetText("Mode: Any");
        this.gameModeFilter.SetSize(new Vector2(120.0, 40.0));
        this.gameModeFilter.RegisterToCallback(n"OnRelease", this, n"OnGameModeFilterClicked");
        this.filterPanel.AddChild(this.gameModeFilter);
        
        // Ping filter
        this.pingFilter = new inkButton();
        this.pingFilter.SetText("Ping: <200ms");
        this.pingFilter.SetSize(new Vector2(120.0, 40.0));
        this.pingFilter.RegisterToCallback(n"OnRelease", this, n"OnPingFilterClicked");
        this.filterPanel.AddChild(this.pingFilter);
        
        // Password filter
        this.passwordFilter = new inkButton();
        this.passwordFilter.SetText("All Servers");
        this.passwordFilter.SetSize(new Vector2(120.0, 40.0));
        this.passwordFilter.RegisterToCallback(n"OnRelease", this, n"OnPasswordFilterClicked");
        this.filterPanel.AddChild(this.passwordFilter);
        
        // Favorites toggle
        this.favoritesOnly = new inkToggle();
        this.favoritesOnly.SetText("★ Favorites");
        this.favoritesOnly.RegisterToCallback(n"OnValueChanged", this, n"OnFavoritesToggled");
        this.filterPanel.AddChild(this.favoritesOnly);
    }
    
    private func CreateServerList(parent: wref<inkVerticalPanel>) -> Void {
        // Column headers
        let headerPanel = new inkHorizontalPanel();
        headerPanel.SetSize(new Vector2(0.0, 30.0));
        parent.AddChild(headerPanel);
        
        this.CreateSortableHeader(headerPanel, "SERVER NAME", ServerSortMode.Name, 300.0);
        this.CreateSortableHeader(headerPanel, "PLAYERS", ServerSortMode.Players, 80.0);
        this.CreateSortableHeader(headerPanel, "PING", ServerSortMode.Ping, 60.0);
        this.CreateSortableHeader(headerPanel, "MODE", ServerSortMode.GameMode, 120.0);
        this.CreateSortableHeader(headerPanel, "REGION", ServerSortMode.Region, 80.0);
        
        // Scrollable server list
        this.serverList = new inkScrollArea();
        this.serverList.SetSize(new Vector2(0.0, 400.0));
        parent.AddChild(this.serverList);
        
        let listContainer = new inkVerticalPanel();
        listContainer.SetName(n"ServerListContainer");
        listContainer.SetChildMargin(new inkMargin(0.0, 2.0, 0.0, 2.0));
        this.serverList.AddChild(listContainer);
    }
    
    private func CreateSortableHeader(parent: wref<inkHorizontalPanel>, text: String, sortMode: ServerSortMode, width: Float) -> Void {
        let headerButton = new inkButton();
        headerButton.SetText(text);
        headerButton.SetSize(new Vector2(width, 30.0));
        headerButton.SetUserData(n"sortMode", Cast<Int32>(sortMode));
        headerButton.RegisterToCallback(n"OnRelease", this, n"OnColumnHeaderClicked");
        parent.AddChild(headerButton);
        
        // Add sort indicator if this is current sort
        if sortMode == this.sortMode {
            let indicator = this.sortDescending ? " ▼" : " ▲";
            headerButton.SetText(text + indicator);
        }
    }
    
    private func CreateServerDetails(parent: wref<inkVerticalPanel>) -> Void {
        this.serverDetails = new inkVerticalPanel();
        this.serverDetails.SetChildMargin(new inkMargin(0.0, 5.0, 0.0, 5.0));
        parent.AddChild(this.serverDetails);
        
        // Server name
        let nameTitle = new inkText();
        nameTitle.SetText("SERVER DETAILS");
        nameTitle.SetFontSize(24);
        nameTitle.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));
        this.serverDetails.AddChild(nameTitle);
        
        // Details will be populated when server is selected
        this.UpdateServerDetails();
    }
    
    private func CreateActionButtons(parent: wref<inkVerticalPanel>) -> Void {
        this.actionButtons = new inkHorizontalPanel();
        this.actionButtons.SetChildMargin(new inkMargin(5.0, 5.0, 5.0, 5.0));
        parent.AddChild(this.actionButtons);
        
        this.joinButton = new inkButton();
        this.joinButton.SetText("JOIN SERVER");
        this.joinButton.SetSize(new Vector2(150.0, 50.0));
        this.joinButton.SetEnabled(false);
        this.joinButton.RegisterToCallback(n"OnRelease", this, n"OnJoinServerClicked");
        this.actionButtons.AddChild(this.joinButton);
        
        this.favoriteButton = new inkButton();
        this.favoriteButton.SetText("★ FAVORITE");
        this.favoriteButton.SetSize(new Vector2(120.0, 50.0));
        this.favoriteButton.SetEnabled(false);
        this.favoriteButton.RegisterToCallback(n"OnRelease", this, n"OnToggleFavoriteClicked");
        this.actionButtons.AddChild(this.favoriteButton);
        
        this.refreshServerButton = new inkButton();
        this.refreshServerButton.SetText("📊 INFO");
        this.refreshServerButton.SetSize(new Vector2(80.0, 50.0));
        this.refreshServerButton.SetEnabled(false);
        this.refreshServerButton.RegisterToCallback(n"OnRelease", this, n"OnGetServerInfoClicked");
        this.actionButtons.AddChild(this.refreshServerButton);
    }
    
    // Event Handlers
    protected cb func OnSearchChanged(widget: ref<inkWidget>) -> Bool {
        this.searchQuery = (widget as inkTextInput).GetText();
        this.ApplyFiltersAndSort();
        return true;
    }
    
    protected cb func OnRefreshClicked(e: ref<inkPointerEvent>) -> Bool {
        this.RefreshServerList();
        return true;
    }
    
    protected cb func OnDirectConnectClicked(e: ref<inkPointerEvent>) -> Bool {
        DirectConnectDialog.Show();
        return true;
    }
    
    protected cb func OnColumnHeaderClicked(e: ref<inkPointerEvent>) -> Bool {
        let button = e.GetTarget() as inkButton;
        if !IsDefined(button) { return false; }
        
        let newSortMode = IntEnum<ServerSortMode>(button.GetUserData(n"sortMode") as Int32);
        
        if newSortMode == this.sortMode {
            this.sortDescending = !this.sortDescending;
        } else {
            this.sortMode = newSortMode;
            this.sortDescending = false;
        }
        
        this.ApplyFiltersAndSort();
        this.UpdateColumnHeaders();
        return true;
    }
    
    protected cb func OnServerRowClicked(e: ref<inkPointerEvent>) -> Bool {
        let serverRow = e.GetTarget() as inkWidget;
        if !IsDefined(serverRow) { return false; }
        
        let serverId = serverRow.GetUserData(n"serverId") as Uint32;
        this.SelectServer(serverId);
        return true;
    }
    
    protected cb func OnJoinServerClicked(e: ref<inkPointerEvent>) -> Bool {
        if !IsDefined(this.selectedServer) { return false; }
        
        if this.selectedServer.hasPassword {
            PasswordDialog.Show(this.selectedServer);
        } else {
            this.ConnectToServer(this.selectedServer);
        }
        return true;
    }
    
    protected cb func OnToggleFavoriteClicked(e: ref<inkPointerEvent>) -> Bool {
        if !IsDefined(this.selectedServer) { return false; }
        
        this.selectedServer.isFavorite = !this.selectedServer.isFavorite;
        FavoriteManager.SetFavorite(this.selectedServer.id, this.selectedServer.isFavorite);
        this.UpdateServerDetails();
        this.UpdateFavoriteButton();
        return true;
    }
    
    protected cb func OnGetServerInfoClicked(e: ref<inkPointerEvent>) -> Bool {
        if !IsDefined(this.selectedServer) { return false; }
        
        ServerInfoDialog.Show(this.selectedServer);
        return true;
    }
    
    // Filter event handlers
    protected cb func OnRegionFilterClicked(e: ref<inkPointerEvent>) -> Bool {
        RegionFilterDialog.Show(this);
        return true;
    }
    
    protected cb func OnGameModeFilterClicked(e: ref<inkPointerEvent>) -> Bool {
        GameModeFilterDialog.Show(this);
        return true;
    }
    
    protected cb func OnPingFilterClicked(e: ref<inkPointerEvent>) -> Bool {
        PingFilterDialog.Show(this);
        return true;
    }
    
    protected cb func OnPasswordFilterClicked(e: ref<inkPointerEvent>) -> Bool {
        this.CyclePasswordFilter();
        return true;
    }
    
    protected cb func OnFavoritesToggled(widget: ref<inkWidget>) -> Bool {
        this.filterShowFavoritesOnly = (widget as inkToggle).GetToggleValue();
        this.ApplyFiltersAndSort();
        return true;
    }
    
    // Core functionality
    public func RefreshServerList() -> Void {
        if this.isRefreshing {
            return;
        }
        
        this.isRefreshing = true;
        this.refreshButton.SetEnabled(false);
        this.refreshButton.SetText("REFRESHING...");
        
        LogChannel(n"SERVER_BROWSER", "Refreshing server list...");
        ServerBrowserAPI.RefreshServerList();
    }
    
    public func OnServerListReceived(servers: array<ServerInfo>) -> Void {
        this.allServers = servers;
        this.ApplyFiltersAndSort();
        
        this.isRefreshing = false;
        this.refreshButton.SetEnabled(true);
        this.refreshButton.SetText("🔄 REFRESH");
        
        LogChannel(n"SERVER_BROWSER", "Server list updated: " + IntToString(ArraySize(servers)) + " servers");
    }
    
    private func ApplyFiltersAndSort() -> Void {
        ArrayClear(this.filteredServers);
        
        for server in this.allServers {
            if this.PassesFilters(server) {
                ArrayPush(this.filteredServers, server);
            }
        }
        
        this.SortServers();
        this.UpdateServerListUI();
    }
    
    private func PassesFilters(server: ServerInfo) -> Bool {
        // Search query filter
        if this.searchQuery != "" {
            let query = StrLower(this.searchQuery);
            let name = StrLower(server.name);
            let description = StrLower(server.description);
            if StrFindFirst(name, query) == -1 && StrFindFirst(description, query) == -1 {
                return false;
            }
        }
        
        // Region filter
        if this.filterRegion != "Any" && server.region != this.filterRegion {
            return false;
        }
        
        // Game mode filter
        if this.filterGameMode != "Any" && server.gameMode != this.filterGameMode {
            return false;
        }
        
        // Ping filter
        if server.ping > this.filterMaxPing {
            return false;
        }
        
        // Password filter
        if !this.filterShowPasswordProtected && server.hasPassword {
            return false;
        }
        
        // Favorites filter
        if this.filterShowFavoritesOnly && !server.isFavorite {
            return false;
        }
        
        // Player count filter
        if server.currentPlayers < this.filterMinPlayers || server.currentPlayers > this.filterMaxPlayers {
            return false;
        }
        
        return true;
    }
    
    private func SortServers() -> Void {
        // Simple bubble sort implementation
        let count = ArraySize(this.filteredServers);
        for i in Range(count - 1) {
            for j in Range(count - i - 1) {
                if this.ShouldSwap(this.filteredServers[j], this.filteredServers[j + 1]) {
                    let temp = this.filteredServers[j];
                    this.filteredServers[j] = this.filteredServers[j + 1];
                    this.filteredServers[j + 1] = temp;
                }
            }
        }
    }
    
    private func ShouldSwap(server1: ServerInfo, server2: ServerInfo) -> Bool {
        let result = false;
        
        switch this.sortMode {
            case ServerSortMode.Name:
                result = StrCmp(server1.name, server2.name) > 0;
                break;
            case ServerSortMode.Players:
                result = server1.currentPlayers > server2.currentPlayers;
                break;
            case ServerSortMode.Ping:
                result = server1.ping > server2.ping;
                break;
            case ServerSortMode.GameMode:
                result = StrCmp(server1.gameMode, server2.gameMode) > 0;
                break;
            case ServerSortMode.Region:
                result = StrCmp(server1.region, server2.region) > 0;
                break;
        }
        
        return this.sortDescending ? !result : result;
    }
    
    private func UpdateServerListUI() -> Void {
        let listContainer = this.serverList.GetWidget(n"ServerListContainer") as inkVerticalPanel;
        if !IsDefined(listContainer) { return; }
        
        // Clear existing rows
        listContainer.RemoveAllChildren();
        
        // Add new rows
        for server in this.filteredServers {
            let serverRow = this.CreateServerRow(server);
            listContainer.AddChild(serverRow);
        }
        
        LogChannel(n"SERVER_BROWSER", "Updated server list UI with " + IntToString(ArraySize(this.filteredServers)) + " servers");
    }
    
    private func CreateServerRow(server: ServerInfo) -> ref<inkHorizontalPanel> {
        let row = new inkHorizontalPanel();
        row.SetName(StringToName("server_" + IntToString(server.id)));
        row.SetSize(new Vector2(0.0, 35.0));
        row.SetUserData(n"serverId", server.id);
        row.RegisterToCallback(n"OnRelease", this, n"OnServerRowClicked");
        
        // Server name with icon indicators
        let nameCell = new inkHorizontalPanel();
        nameCell.SetSize(new Vector2(300.0, 35.0));
        
        if server.isOfficial {
            let officialIcon = new inkText();
            officialIcon.SetText("🏛️");
            officialIcon.SetFontSize(16);
            nameCell.AddChild(officialIcon);
        }
        
        if server.hasPassword {
            let passwordIcon = new inkText();
            passwordIcon.SetText("🔒");
            passwordIcon.SetFontSize(16);
            nameCell.AddChild(passwordIcon);
        }
        
        if server.isFavorite {
            let favoriteIcon = new inkText();
            favoriteIcon.SetText("★");
            favoriteIcon.SetFontSize(16);
            favoriteIcon.SetTintColor(new HDRColor(1.0, 1.0, 0.0, 1.0));
            nameCell.AddChild(favoriteIcon);
        }
        
        let nameText = new inkText();
        nameText.SetText(server.name);
        nameText.SetFontSize(18);
        nameText.SetVerticalAlignment(textVerticalAlignment.Center);
        nameCell.AddChild(nameText);
        row.AddChild(nameCell);
        
        // Player count
        let playersText = new inkText();
        playersText.SetText(IntToString(server.currentPlayers) + "/" + IntToString(server.maxPlayers));
        playersText.SetSize(new Vector2(80.0, 35.0));
        playersText.SetVerticalAlignment(textVerticalAlignment.Center);
        playersText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        row.AddChild(playersText);
        
        // Ping with color coding
        let pingText = new inkText();
        pingText.SetText(IntToString(server.ping) + "ms");
        pingText.SetSize(new Vector2(60.0, 35.0));
        pingText.SetVerticalAlignment(textVerticalAlignment.Center);
        pingText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        
        if server.ping <= 50u {
            pingText.SetTintColor(new HDRColor(0.4, 1.0, 0.4, 1.0)); // Green
        } else if server.ping <= 100u {
            pingText.SetTintColor(new HDRColor(1.0, 1.0, 0.4, 1.0)); // Yellow
        } else {
            pingText.SetTintColor(new HDRColor(1.0, 0.4, 0.4, 1.0)); // Red
        }
        row.AddChild(pingText);
        
        // Game mode
        let modeText = new inkText();
        modeText.SetText(server.gameMode);
        modeText.SetSize(new Vector2(120.0, 35.0));
        modeText.SetVerticalAlignment(textVerticalAlignment.Center);
        modeText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        row.AddChild(modeText);
        
        // Region
        let regionText = new inkText();
        regionText.SetText(server.region);
        regionText.SetSize(new Vector2(80.0, 35.0));
        regionText.SetVerticalAlignment(textVerticalAlignment.Center);
        regionText.SetHorizontalAlignment(textHorizontalAlignment.Center);
        row.AddChild(regionText);
        
        return row;
    }
    
    private func SelectServer(serverId: Uint32) -> Void {
        for server in this.filteredServers {
            if server.id == serverId {
                this.selectedServer = server;
                this.UpdateServerDetails();
                this.UpdateActionButtons();
                break;
            }
        }
    }
    
    private func UpdateServerDetails() -> Void {
        // Clear existing details
        this.serverDetails.RemoveAllChildren();
        
        if !IsDefined(this.selectedServer) {
            let noSelectionText = new inkText();
            noSelectionText.SetText("Select a server to view details");
            noSelectionText.SetFontSize(18);
            noSelectionText.SetTintColor(new HDRColor(0.7, 0.7, 0.7, 1.0));
            this.serverDetails.AddChild(noSelectionText);
            return;
        }
        
        let server = this.selectedServer;
        
        // Server name
        let nameText = new inkText();
        nameText.SetText(server.name);
        nameText.SetFontSize(24);
        nameText.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));
        this.serverDetails.AddChild(nameText);
        
        // Description
        if server.description != "" {
            let descText = new inkText();
            descText.SetText(server.description);
            descText.SetFontSize(16);
            descText.SetWrappingAtPosition(350.0);
            this.serverDetails.AddChild(descText);
        }
        
        // Server info
        this.AddDetailRow("Address:", server.ip + ":" + IntToString(server.port));
        this.AddDetailRow("Players:", IntToString(server.currentPlayers) + "/" + IntToString(server.maxPlayers));
        this.AddDetailRow("Game Mode:", server.gameMode);
        this.AddDetailRow("Map:", server.mapName);
        this.AddDetailRow("Region:", server.region);
        this.AddDetailRow("Ping:", IntToString(server.ping) + "ms");
        this.AddDetailRow("Version:", server.version);
        this.AddDetailRow("Password:", server.hasPassword ? "Yes" : "No");
        
        // Tags
        if ArraySize(server.tags) > 0 {
            let tagsPanel = new inkHorizontalPanel();
            tagsPanel.SetChildMargin(new inkMargin(5.0, 2.0, 5.0, 2.0));
            
            let tagsLabel = new inkText();
            tagsLabel.SetText("Tags:");
            tagsLabel.SetFontSize(16);
            tagsLabel.SetTintColor(new HDRColor(0.8, 0.8, 0.8, 1.0));
            tagsPanel.AddChild(tagsLabel);
            
            for tag in server.tags {
                let tagButton = new inkButton();
                tagButton.SetText(tag);
                tagButton.SetSize(new Vector2(80.0, 25.0));
                tagButton.SetStyle(n"ButtonSmall");
                tagsPanel.AddChild(tagButton);
            }
            
            this.serverDetails.AddChild(tagsPanel);
        }
    }
    
    private func AddDetailRow(label: String, value: String) -> Void {
        let row = new inkHorizontalPanel();
        row.SetChildMargin(new inkMargin(0.0, 2.0, 0.0, 2.0));
        
        let labelText = new inkText();
        labelText.SetText(label);
        labelText.SetFontSize(16);
        labelText.SetSize(new Vector2(100.0, 20.0));
        labelText.SetTintColor(new HDRColor(0.8, 0.8, 0.8, 1.0));
        row.AddChild(labelText);
        
        let valueText = new inkText();
        valueText.SetText(value);
        valueText.SetFontSize(16);
        valueText.SetTintColor(new HDRColor(1.0, 1.0, 1.0, 1.0));
        row.AddChild(valueText);
        
        this.serverDetails.AddChild(row);
    }
    
    private func UpdateActionButtons() -> Void {
        let hasSelection = IsDefined(this.selectedServer);
        
        this.joinButton.SetEnabled(hasSelection);
        this.favoriteButton.SetEnabled(hasSelection);
        this.refreshServerButton.SetEnabled(hasSelection);
        
        if hasSelection {
            this.UpdateFavoriteButton();
        }
    }
    
    private func UpdateFavoriteButton() -> Void {
        if IsDefined(this.selectedServer) {
            if this.selectedServer.isFavorite {
                this.favoriteButton.SetText("★ UNFAVORITE");
                this.favoriteButton.SetTintColor(new HDRColor(1.0, 1.0, 0.0, 1.0));
            } else {
                this.favoriteButton.SetText("☆ FAVORITE");
                this.favoriteButton.SetTintColor(new HDRColor(1.0, 1.0, 1.0, 1.0));
            }
        }
    }
    
    private func UpdateColumnHeaders() -> Void {
        // Update sort indicators on column headers
        // This would be implemented by recreating the header buttons
        // For brevity, logging the change
        LogChannel(n"SERVER_BROWSER", "Sort mode changed to: " + IntToString(Cast<Int32>(this.sortMode)) + 
                  (this.sortDescending ? " (descending)" : " (ascending)"));
    }
    
    public func ConnectToServer(server: ServerInfo) -> Void {
        LogChannel(n"SERVER_BROWSER", "Connecting to server: " + server.name + " (" + server.ip + ":" + IntToString(server.port) + ")");
        
        ConnectionManager.ConnectToServer(server);
        this.parentUI.SetState(CoopUIState.Connecting);
    }
    
    public func Show() -> Void {
        this.container.SetVisible(true);
        this.RefreshServerList();
    }
    
    public func Hide() -> Void {
        this.container.SetVisible(false);
    }
    
    public func Cleanup() -> Void {
        // Unregister all callbacks and clean up resources
        if IsDefined(this.searchBar) {
            this.searchBar.UnregisterFromCallback(n"OnInput", this, n"OnSearchChanged");
        }
        
        // Clean up all other event handlers
        LogChannel(n"SERVER_BROWSER", "ModernServerBrowser cleaned up");
    }
    
    // Filter management
    private func CyclePasswordFilter() -> Void {
        if this.filterShowPasswordProtected {
            this.filterShowPasswordProtected = false;
            this.passwordFilter.SetText("No Password");
        } else {
            this.filterShowPasswordProtected = true;
            this.passwordFilter.SetText("All Servers");
        }
        
        this.ApplyFiltersAndSort();
    }
    
    public func SetRegionFilter(region: String) -> Void {
        this.filterRegion = region;
        this.regionFilter.SetText("Region: " + region);
        this.ApplyFiltersAndSort();
    }
    
    public func SetGameModeFilter(gameMode: String) -> Void {
        this.filterGameMode = gameMode;
        this.gameModeFilter.SetText("Mode: " + gameMode);
        this.ApplyFiltersAndSort();
    }
    
    public func SetPingFilter(maxPing: Uint32) -> Void {
        this.filterMaxPing = maxPing;
        this.pingFilter.SetText("Ping: <" + IntToString(maxPing) + "ms");
        this.ApplyFiltersAndSort();
    }
}
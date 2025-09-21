// Room Management UI Integration
// REDscript interface for room creation, joining, and management - synchronized with dedicated server

// Room Manager UI - handles all room-related UI operations
public class RoomManagerUI extends ScriptableSystem {
    private static let s_instance: ref<RoomManagerUI>;
    private let m_currentRoom: ref<RoomEntry>;
    private let m_availableRooms: array<ref<RoomEntry>>;
    private let m_filteredRooms: array<ref<RoomEntry>>;
    private let m_roomFilters: RoomBrowserFilters;
    private let m_isInRoom: Bool = false;
    private let m_isRoomOwner: Bool = false;
    private let m_localPlayerReady: Bool = false;
    private let m_currentPermissionLevel: ERoomPermissionLevel = ERoomPermissionLevel.Player;
    private let m_roomUpdateTimer: Float = 0.0;
    private let m_roomUpdateInterval: Float = 1.0; // 1 FPS for room state updates

    public static func GetInstance() -> ref<RoomManagerUI> {
        if !IsDefined(RoomManagerUI.s_instance) {
            RoomManagerUI.s_instance = new RoomManagerUI();
        }
        return RoomManagerUI.s_instance;
    }

    public func Initialize() -> Void {
        // Register for room management callbacks
        Native_RegisterRoomCallbacks();

        // Initialize filters
        this.InitializeFilters();

        LogChannel(n"RoomManager", s"[RoomManager] Room Manager UI initialized");
    }

    private func InitializeFilters() -> Void {
        this.m_roomFilters.nameFilter = "";
        this.m_roomFilters.roomType = ERoomType.All;
        this.m_roomFilters.gameMode = "";
        this.m_roomFilters.showPasswordProtected = true;
        this.m_roomFilters.showPrivate = false;
        this.m_roomFilters.showFull = false;
        this.m_roomFilters.showEmpty = true;
        this.m_roomFilters.maxPlayers = 50;
        this.m_roomFilters.minPlayers = 1;
    }

    public func OnUpdate(deltaTime: Float) -> Void {
        this.m_roomUpdateTimer += deltaTime;

        if this.m_roomUpdateTimer >= this.m_roomUpdateInterval {
            // Update current room state if in room
            if this.m_isInRoom {
                this.RequestRoomUpdate();
            }

            // Refresh available rooms list
            this.RefreshRoomList();

            this.m_roomUpdateTimer = 0.0;
        }
    }

    // Room Creation
    public func CreateRoom(settings: RoomCreationSettings) -> Bool {
        // Validate settings
        if !this.ValidateRoomSettings(settings) {
            this.ShowRoomCreationError("Invalid room settings");
            return false;
        }

        // Check if player is already in a room
        if this.m_isInRoom {
            this.ShowRoomCreationError("You must leave your current room first");
            return false;
        }

        // Call native function to create room
        let roomId = Native_CreateRoom(settings);

        if !Equals(roomId, "") {
            LogChannel(n"RoomManager", s"[RoomManager] Created room: " + roomId);

            // Automatically join the created room
            this.JoinRoomById(roomId, settings.password);
            return true;
        } else {
            this.ShowRoomCreationError("Failed to create room");
            return false;
        }
    }

    public func CreateQuickRoom(roomType: ERoomType, maxPlayers: Int32) -> Bool {
        let settings: RoomCreationSettings;
        settings.name = this.GetPlayerName() + "'s Room";
        settings.description = "Quick match room";
        settings.roomType = roomType;
        settings.maxPlayers = maxPlayers;
        settings.isPasswordProtected = false;
        settings.isPrivate = false;
        settings.allowSpectators = true;
        settings.enableVoiceChat = true;
        settings.enableTextChat = true;
        settings.gameplayDifficulty = 1.0;

        return this.CreateRoom(settings);
    }

    // Room Joining
    public func JoinRoomById(roomId: String, password: String) -> Bool {
        if this.m_isInRoom {
            this.ShowJoinError("You are already in a room");
            return false;
        }

        let playerName = this.GetPlayerName();
        let result = Native_JoinRoom(roomId, playerName, password, "");

        this.HandleJoinResult(result, roomId);
        return Equals(result, ERoomJoinResult.Success);
    }

    public func JoinRoomByCode(roomCode: String, password: String) -> Bool {
        let roomId = Native_FindRoomByCode(roomCode);
        if Equals(roomId, "") {
            this.ShowJoinError("Room not found");
            return false;
        }

        return this.JoinRoomById(roomId, password);
    }

    public func JoinSelectedRoom(password: String) -> Bool {
        if !IsDefined(this.m_selectedRoom) {
            return false;
        }

        return this.JoinRoomById(this.m_selectedRoom.GetId(), password);
    }

    public func QuickJoinRoom(roomType: ERoomType) -> Bool {
        let playerName = this.GetPlayerName();
        let roomId = Native_QuickJoinRoom(roomType, playerName);

        if !Equals(roomId, "") {
            LogChannel(n"RoomManager", s"[RoomManager] Quick joined room: " + roomId);
            return true;
        } else {
            this.ShowJoinError("No suitable rooms found");
            return false;
        }
    }

    // Room Management
    public func LeaveCurrentRoom() -> Bool {
        if !this.m_isInRoom {
            return false;
        }

        let success = Native_LeaveRoom();

        if success {
            this.OnRoomLeft();
            LogChannel(n"RoomManager", s"[RoomManager] Left room");
        }

        return success;
    }

    public func SetPlayerReady(ready: Bool) -> Bool {
        if !this.m_isInRoom {
            return false;
        }

        let success = Native_SetPlayerReady(ready);

        if success {
            this.m_localPlayerReady = ready;
            this.UpdateRoomUI();
        }

        return success;
    }

    public func ToggleSpectatorMode() -> Bool {
        if !this.m_isInRoom || !IsDefined(this.m_currentRoom) {
            return false;
        }

        if !this.m_currentRoom.AllowsSpectators() {
            return false;
        }

        let isCurrentlySpectator = this.IsLocalPlayerSpectator();
        let success = Native_SetPlayerSpectator(!isCurrentlySpectator);

        if success {
            this.UpdateRoomUI();
        }

        return success;
    }

    // Room Admin Functions
    public func KickPlayer(playerId: Uint32, reason: String) -> Bool {
        if !this.HasPermission(ERoomPermissionLevel.Moderator) {
            return false;
        }

        let success = Native_KickPlayer(playerId, reason);

        if success {
            LogChannel(n"RoomManager", s"[RoomManager] Kicked player: " + playerId);
        }

        return success;
    }

    public func BanPlayer(playerId: Uint32, reason: String, durationMinutes: Int32) -> Bool {
        if !this.HasPermission(ERoomPermissionLevel.Admin) {
            return false;
        }

        let success = Native_BanPlayer(playerId, reason, durationMinutes);

        if success {
            LogChannel(n"RoomManager", s"[RoomManager] Banned player: " + playerId);
        }

        return success;
    }

    public func SetPlayerPermission(playerId: Uint32, level: ERoomPermissionLevel) -> Bool {
        if !this.HasPermission(ERoomPermissionLevel.Admin) {
            return false;
        }

        let success = Native_SetPlayerPermission(playerId, level);

        if success {
            this.UpdateRoomUI();
        }

        return success;
    }

    public func UpdateRoomSettings(settings: RoomUpdateSettings) -> Bool {
        if !this.HasPermission(ERoomPermissionLevel.Admin) {
            return false;
        }

        let success = Native_UpdateRoomSettings(settings);

        if success {
            this.UpdateRoomUI();
            LogChannel(n"RoomManager", s"[RoomManager] Updated room settings");
        }

        return success;
    }

    public func StartRoom() -> Bool {
        if !this.HasPermission(ERoomPermissionLevel.Owner) {
            return false;
        }

        let success = Native_StartRoom();

        if success {
            LogChannel(n"RoomManager", s"[RoomManager] Starting room");
        }

        return success;
    }

    // Room Invitations
    public func InvitePlayer(playerId: Uint32) -> Bool {
        if !this.m_isInRoom {
            return false;
        }

        let inviteId = Native_CreateInvite(playerId);

        if !Equals(inviteId, "") {
            // Send invite notification to player
            this.SendInviteNotification(playerId, inviteId);
            LogChannel(n"RoomManager", s"[RoomManager] Sent invite to player: " + playerId);
            return true;
        }

        return false;
    }

    public func AcceptInvite(inviteId: String) -> Bool {
        let success = Native_AcceptInvite(inviteId);

        if success {
            LogChannel(n"RoomManager", s"[RoomManager] Accepted invite: " + inviteId);
        }

        return success;
    }

    public func DeclineInvite(inviteId: String) -> Bool {
        let success = Native_DeclineInvite(inviteId);

        if success {
            LogChannel(n"RoomManager", s"[RoomManager] Declined invite: " + inviteId);
        }

        return success;
    }

    // Room Discovery and Filtering
    public func RefreshRoomList() -> Void {
        Native_RefreshRoomList();
    }

    public func SetRoomFilters(filters: RoomBrowserFilters) -> Void {
        this.m_roomFilters = filters;
        this.ApplyRoomFilters();
    }

    public func ClearRoomFilters() -> Void {
        this.InitializeFilters();
        this.ApplyRoomFilters();
    }

    private func ApplyRoomFilters() -> Void {
        ArrayClear(this.m_filteredRooms);

        for room in this.m_availableRooms {
            if this.RoomMatchesFilters(room) {
                ArrayPush(this.m_filteredRooms, room);
            }
        }

        this.UpdateRoomListUI();
    }

    private func RoomMatchesFilters(room: ref<RoomEntry>) -> Bool {
        // Name filter
        if !Equals(this.m_roomFilters.nameFilter, "") {
            let roomNameLower = StrLower(room.GetName());
            let filterLower = StrLower(this.m_roomFilters.nameFilter);
            if !StrContains(roomNameLower, filterLower) {
                return false;
            }
        }

        // Room type filter
        if !Equals(this.m_roomFilters.roomType, ERoomType.All) &&
           !Equals(room.GetRoomType(), this.m_roomFilters.roomType) {
            return false;
        }

        // Game mode filter
        if !Equals(this.m_roomFilters.gameMode, "") &&
           !Equals(room.GetGameMode(), this.m_roomFilters.gameMode) {
            return false;
        }

        // Password protection filter
        if !this.m_roomFilters.showPasswordProtected && room.IsPasswordProtected() {
            return false;
        }

        // Private room filter
        if !this.m_roomFilters.showPrivate && room.IsPrivate() {
            return false;
        }

        // Full room filter
        if !this.m_roomFilters.showFull && room.IsFull() {
            return false;
        }

        // Empty room filter
        if !this.m_roomFilters.showEmpty && room.IsEmpty() {
            return false;
        }

        // Player count filters
        let playerCount = room.GetCurrentPlayers();
        if playerCount < this.m_roomFilters.minPlayers ||
           playerCount > this.m_roomFilters.maxPlayers {
            return false;
        }

        return true;
    }

    // Chat System
    public func SendChatMessage(message: String, targetPlayerId: Uint32) -> Bool {
        if !this.m_isInRoom {
            return false;
        }

        if Equals(message, "") || StrLen(message) > 500 {
            return false;
        }

        let success = Native_SendChatMessage(message, targetPlayerId);

        if success {
            // Add to local chat history
            this.AddChatMessage(this.GetPlayerName(), message, targetPlayerId);
        }

        return success;
    }

    // Event Handlers - called from native code
    public func OnRoomListUpdated(rooms: array<RoomNetworkData>) -> Void {
        ArrayClear(this.m_availableRooms);

        for roomData in rooms {
            let roomEntry = new RoomEntry();
            roomEntry.InitializeFromNetworkData(roomData);
            ArrayPush(this.m_availableRooms, roomEntry);
        }

        this.ApplyRoomFilters();

        LogChannel(n"RoomManager", s"[RoomManager] Room list updated: " + ArraySize(this.m_availableRooms) + " rooms");
    }

    public func OnRoomJoined(roomData: RoomNetworkData) -> Void {
        this.m_isInRoom = true;
        this.m_isRoomOwner = roomData.ownerId == this.GetLocalPlayerId();
        this.m_localPlayerReady = false;

        // Create current room entry
        this.m_currentRoom = new RoomEntry();
        this.m_currentRoom.InitializeFromNetworkData(roomData);

        // Update permission level
        this.UpdateLocalPlayerPermission(roomData);

        // Show room UI
        this.ShowRoomUI();

        LogChannel(n"RoomManager", s"[RoomManager] Joined room: " + roomData.name);
    }

    public func OnRoomLeft() -> Void {
        this.m_isInRoom = false;
        this.m_isRoomOwner = false;
        this.m_localPlayerReady = false;
        this.m_currentRoom = null;
        this.m_currentPermissionLevel = ERoomPermissionLevel.Player;

        // Hide room UI
        this.HideRoomUI();

        LogChannel(n"RoomManager", s"[RoomManager] Left room");
    }

    public func OnRoomStateChanged(newState: ERoomState) -> Void {
        if IsDefined(this.m_currentRoom) {
            this.m_currentRoom.SetState(newState);
            this.UpdateRoomStateUI(newState);
        }

        LogChannel(n"RoomManager", s"[RoomManager] Room state changed: " + EnumValueToString("ERoomState", Cast<Int64>(EnumInt(newState))));
    }

    public func OnPlayerJoinedRoom(playerId: Uint32, playerName: String) -> Void {
        if IsDefined(this.m_currentRoom) {
            this.m_currentRoom.AddPlayer(playerId, playerName);
            this.UpdateRoomPlayersUI();
        }

        // Show notification
        this.ShowPlayerJoinedNotification(playerName);

        LogChannel(n"RoomManager", s"[RoomManager] Player joined: " + playerName);
    }

    public func OnPlayerLeftRoom(playerId: Uint32, playerName: String, wasKicked: Bool) -> Void {
        if IsDefined(this.m_currentRoom) {
            this.m_currentRoom.RemovePlayer(playerId);
            this.UpdateRoomPlayersUI();
        }

        // Show notification
        if wasKicked {
            this.ShowPlayerKickedNotification(playerName);
        } else {
            this.ShowPlayerLeftNotification(playerName);
        }

        LogChannel(n"RoomManager", s"[RoomManager] Player left: " + playerName);
    }

    public func OnPlayerReadyChanged(playerId: Uint32, isReady: Bool) -> Void {
        if IsDefined(this.m_currentRoom) {
            this.m_currentRoom.SetPlayerReady(playerId, isReady);
            this.UpdateRoomPlayersUI();
        }
    }

    public func OnChatMessageReceived(senderId: Uint32, senderName: String, message: String, targetId: Uint32) -> Void {
        this.AddChatMessage(senderName, message, targetId);
        this.UpdateChatUI();
    }

    public func OnInviteReceived(inviteData: RoomInviteData) -> Void {
        this.ShowInviteDialog(inviteData);

        LogChannel(n"RoomManager", s"[RoomManager] Received room invite from: " + inviteData.inviterName);
    }

    // Helper Methods
    private func ValidateRoomSettings(settings: RoomCreationSettings) -> Bool {
        if Equals(settings.name, "") || StrLen(settings.name) > 50 {
            return false;
        }

        if StrLen(settings.description) > 200 {
            return false;
        }

        if settings.maxPlayers <= 0 || settings.maxPlayers > 50 {
            return false;
        }

        return true;
    }

    private func HandleJoinResult(result: ERoomJoinResult, roomId: String) -> Void {
        switch result {
            case ERoomJoinResult.Success:
                // Success is handled by OnRoomJoined callback
                break;
            case ERoomJoinResult.RoomFull:
                this.ShowJoinError("Room is full");
                break;
            case ERoomJoinResult.PasswordRequired:
                this.ShowPasswordDialog(roomId);
                break;
            case ERoomJoinResult.IncorrectPassword:
                this.ShowJoinError("Incorrect password");
                break;
            case ERoomJoinResult.Banned:
                this.ShowJoinError("You are banned from this room");
                break;
            case ERoomJoinResult.InviteRequired:
                this.ShowJoinError("This room requires an invitation");
                break;
            case ERoomJoinResult.VersionMismatch:
                this.ShowJoinError("Version mismatch - please update your game");
                break;
            case ERoomJoinResult.RoomNotFound:
                this.ShowJoinError("Room not found");
                break;
            case ERoomJoinResult.AlreadyInRoom:
                this.ShowJoinError("You are already in a room");
                break;
            case ERoomJoinResult.NetworkError:
                this.ShowJoinError("Network error - please try again");
                break;
        }
    }

    private func UpdateLocalPlayerPermission(roomData: RoomNetworkData) -> Void {
        let localPlayerId = this.GetLocalPlayerId();

        for playerData in roomData.players {
            if playerData.playerId == localPlayerId {
                this.m_currentPermissionLevel = playerData.permissionLevel;
                break;
            }
        }
    }

    private func HasPermission(requiredLevel: ERoomPermissionLevel) -> Bool {
        return Cast<Int32>(this.m_currentPermissionLevel) >= Cast<Int32>(requiredLevel);
    }

    private func IsLocalPlayerSpectator() -> Bool {
        if !IsDefined(this.m_currentRoom) {
            return false;
        }

        let localPlayerId = this.GetLocalPlayerId();
        return this.m_currentRoom.IsPlayerSpectator(localPlayerId);
    }

    private func GetPlayerName() -> String {
        // Get player name from game system
        return "Player"; // Placeholder
    }

    private func GetLocalPlayerId() -> Uint32 {
        // Get local player ID
        return 1u; // Placeholder
    }

    private func RequestRoomUpdate() -> Void {
        Native_RequestRoomUpdate();
    }

    // UI Methods
    private func ShowRoomUI() -> Void {
        // Show in-room UI
    }

    private func HideRoomUI() -> Void {
        // Hide in-room UI
    }

    private func UpdateRoomUI() -> Void {
        // Update room UI with current state
    }

    private func UpdateRoomStateUI(state: ERoomState) -> Void {
        // Update UI based on room state
    }

    private func UpdateRoomPlayersUI() -> Void {
        // Update player list UI
    }

    private func UpdateRoomListUI() -> Void {
        // Update available rooms list UI
    }

    private func UpdateChatUI() -> Void {
        // Update chat UI
    }

    private func ShowRoomCreationError(message: String) -> Void {
        // Show room creation error dialog
    }

    private func ShowJoinError(message: String) -> Void {
        // Show join error dialog
    }

    private func ShowPasswordDialog(roomId: String) -> Void {
        // Show password input dialog
    }

    private func ShowInviteDialog(inviteData: RoomInviteData) -> Void {
        // Show room invitation dialog
    }

    private func ShowPlayerJoinedNotification(playerName: String) -> Void {
        // Show player joined notification
    }

    private func ShowPlayerLeftNotification(playerName: String) -> Void {
        // Show player left notification
    }

    private func ShowPlayerKickedNotification(playerName: String) -> Void {
        // Show player kicked notification
    }

    private func AddChatMessage(senderName: String, message: String, targetId: Uint32) -> Void {
        // Add message to chat history
    }

    private func SendInviteNotification(playerId: Uint32, inviteId: String) -> Void {
        // Send invite notification through game systems
    }

    // Public Getters
    public func IsInRoom() -> Bool {
        return this.m_isInRoom;
    }

    public func IsRoomOwner() -> Bool {
        return this.m_isRoomOwner;
    }

    public func GetCurrentRoom() -> ref<RoomEntry> {
        return this.m_currentRoom;
    }

    public func GetAvailableRooms() -> array<ref<RoomEntry>> {
        return this.m_filteredRooms;
    }

    public func GetCurrentPermissionLevel() -> ERoomPermissionLevel {
        return this.m_currentPermissionLevel;
    }

    public func IsLocalPlayerReady() -> Bool {
        return this.m_localPlayerReady;
    }
}

// Room Entry for UI Display
public class RoomEntry extends ScriptableComponent {
    private let m_id: String;
    private let m_name: String;
    private let m_description: String;
    private let m_ownerId: Uint32;
    private let m_ownerName: String;
    private let m_roomType: ERoomType;
    private let m_state: ERoomState;
    private let m_currentPlayers: Int32;
    private let m_maxPlayers: Int32;
    private let m_isPasswordProtected: Bool;
    private let m_isPrivate: Bool;
    private let m_allowSpectators: Bool;
    private let m_gameMode: String;
    private let m_mapName: String;
    private let m_players: array<RoomPlayerEntry>;

    public func InitializeFromNetworkData(data: RoomNetworkData) -> Void {
        this.m_id = data.roomId;
        this.m_name = data.name;
        this.m_description = data.description;
        this.m_ownerId = data.ownerId;
        this.m_ownerName = data.ownerName;
        this.m_roomType = data.roomType;
        this.m_state = data.state;
        this.m_currentPlayers = data.currentPlayers;
        this.m_maxPlayers = data.maxPlayers;
        this.m_isPasswordProtected = data.isPasswordProtected;
        this.m_isPrivate = data.isPrivate;
        this.m_allowSpectators = data.allowSpectators;
        this.m_gameMode = data.gameMode;
        this.m_mapName = data.mapName;

        // Convert player data
        ArrayClear(this.m_players);
        for playerData in data.players {
            let playerEntry: RoomPlayerEntry;
            playerEntry.playerId = playerData.playerId;
            playerEntry.playerName = playerData.playerName;
            playerEntry.permissionLevel = playerData.permissionLevel;
            playerEntry.isReady = playerData.isReady;
            playerEntry.isSpectator = playerData.isSpectator;
            ArrayPush(this.m_players, playerEntry);
        }
    }

    public func AddPlayer(playerId: Uint32, playerName: String) -> Void {
        let playerEntry: RoomPlayerEntry;
        playerEntry.playerId = playerId;
        playerEntry.playerName = playerName;
        playerEntry.permissionLevel = ERoomPermissionLevel.Player;
        playerEntry.isReady = false;
        playerEntry.isSpectator = false;
        ArrayPush(this.m_players, playerEntry);

        this.m_currentPlayers = ArraySize(this.m_players);
    }

    public func RemovePlayer(playerId: Uint32) -> Void {
        let index = -1;
        for i in Range(ArraySize(this.m_players)) {
            if this.m_players[i].playerId == playerId {
                index = i;
                break;
            }
        }

        if index >= 0 {
            ArrayRemove(this.m_players, this.m_players[index]);
            this.m_currentPlayers = ArraySize(this.m_players);
        }
    }

    public func SetPlayerReady(playerId: Uint32, isReady: Bool) -> Void {
        for player in this.m_players {
            if player.playerId == playerId {
                player.isReady = isReady;
                break;
            }
        }
    }

    public func IsPlayerSpectator(playerId: Uint32) -> Bool {
        for player in this.m_players {
            if player.playerId == playerId {
                return player.isSpectator;
            }
        }
        return false;
    }

    // Getters
    public func GetId() -> String { return this.m_id; }
    public func GetName() -> String { return this.m_name; }
    public func GetDescription() -> String { return this.m_description; }
    public func GetOwnerId() -> Uint32 { return this.m_ownerId; }
    public func GetOwnerName() -> String { return this.m_ownerName; }
    public func GetRoomType() -> ERoomType { return this.m_roomType; }
    public func GetState() -> ERoomState { return this.m_state; }
    public func GetCurrentPlayers() -> Int32 { return this.m_currentPlayers; }
    public func GetMaxPlayers() -> Int32 { return this.m_maxPlayers; }
    public func IsPasswordProtected() -> Bool { return this.m_isPasswordProtected; }
    public func IsPrivate() -> Bool { return this.m_isPrivate; }
    public func AllowsSpectators() -> Bool { return this.m_allowSpectators; }
    public func GetGameMode() -> String { return this.m_gameMode; }
    public func GetMapName() -> String { return this.m_mapName; }
    public func GetPlayers() -> array<RoomPlayerEntry> { return this.m_players; }

    public func SetState(state: ERoomState) -> Void { this.m_state = state; }

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

    public func GetRoomTypeString() -> String {
        return EnumValueToString("ERoomType", Cast<Int64>(EnumInt(this.m_roomType)));
    }

    public func GetStateString() -> String {
        return EnumValueToString("ERoomState", Cast<Int64>(EnumInt(this.m_state)));
    }

    public func CanJoin() -> Bool {
        return !this.IsFull() &&
               Equals(this.m_state, ERoomState.Waiting) &&
               !this.m_isPrivate;
    }
}

// Data Structures
public struct RoomCreationSettings {
    public let name: String;
    public let description: String;
    public let password: String;
    public let roomType: ERoomType;
    public let maxPlayers: Int32;
    public let isPasswordProtected: Bool;
    public let isPrivate: Bool;
    public let allowSpectators: Bool;
    public let enableVoiceChat: Bool;
    public let enableTextChat: Bool;
    public let gameplayDifficulty: Float;
    public let gameMode: String;
    public let mapName: String;
}

public struct RoomUpdateSettings {
    public let name: String;
    public let description: String;
    public let password: String;
    public let maxPlayers: Int32;
    public let isPasswordProtected: Bool;
    public let allowSpectators: Bool;
    public let enableVoiceChat: Bool;
    public let enableTextChat: Bool;
    public let gameplayDifficulty: Float;
}

public struct RoomBrowserFilters {
    public let nameFilter: String;
    public let roomType: ERoomType;
    public let gameMode: String;
    public let showPasswordProtected: Bool;
    public let showPrivate: Bool;
    public let showFull: Bool;
    public let showEmpty: Bool;
    public let maxPlayers: Int32;
    public let minPlayers: Int32;
}

public struct RoomNetworkData {
    public let roomId: String;
    public let name: String;
    public let description: String;
    public let ownerId: Uint32;
    public let ownerName: String;
    public let roomType: ERoomType;
    public let state: ERoomState;
    public let currentPlayers: Int32;
    public let maxPlayers: Int32;
    public let isPasswordProtected: Bool;
    public let isPrivate: Bool;
    public let allowSpectators: Bool;
    public let gameMode: String;
    public let mapName: String;
    public let players: array<RoomPlayerData>;
}

public struct RoomPlayerData {
    public let playerId: Uint32;
    public let playerName: String;
    public let permissionLevel: ERoomPermissionLevel;
    public let isReady: Bool;
    public let isSpectator: Bool;
}

public struct RoomPlayerEntry {
    public let playerId: Uint32;
    public let playerName: String;
    public let permissionLevel: ERoomPermissionLevel;
    public let isReady: Bool;
    public let isSpectator: Bool;
}

public struct RoomInviteData {
    public let inviteId: String;
    public let roomId: String;
    public let roomName: String;
    public let inviterId: Uint32;
    public let inviterName: String;
    public let expirationTime: Float;
}

// Enumerations
public enum ERoomType : Uint8 {
    All = 255,
    FreeRoam = 0,
    CooperativeMission = 1,
    CompetitiveMatch = 2,
    PrivateLobby = 3,
    CustomGameMode = 4
}

public enum ERoomState : Uint8 {
    Waiting = 0,
    Starting = 1,
    InProgress = 2,
    Paused = 3,
    Completed = 4,
    Cancelled = 5
}

public enum ERoomJoinResult : Uint8 {
    Success = 0,
    RoomFull = 1,
    PasswordRequired = 2,
    IncorrectPassword = 3,
    Banned = 4,
    InviteRequired = 5,
    VersionMismatch = 6,
    RoomNotFound = 7,
    AlreadyInRoom = 8,
    NetworkError = 9
}

public enum ERoomPermissionLevel : Uint8 {
    Banned = 0,
    Viewer = 1,
    Player = 2,
    Moderator = 3,
    Admin = 4,
    Owner = 5
}

// Native function declarations for C++ integration
native func Native_RegisterRoomCallbacks() -> Void;
native func Native_CreateRoom(settings: RoomCreationSettings) -> String;
native func Native_JoinRoom(roomId: String, playerName: String, password: String, inviteId: String) -> ERoomJoinResult;
native func Native_LeaveRoom() -> Bool;
native func Native_QuickJoinRoom(roomType: ERoomType, playerName: String) -> String;
native func Native_FindRoomByCode(roomCode: String) -> String;

native func Native_SetPlayerReady(ready: Bool) -> Bool;
native func Native_SetPlayerSpectator(spectator: Bool) -> Bool;
native func Native_KickPlayer(playerId: Uint32, reason: String) -> Bool;
native func Native_BanPlayer(playerId: Uint32, reason: String, durationMinutes: Int32) -> Bool;
native func Native_SetPlayerPermission(playerId: Uint32, level: ERoomPermissionLevel) -> Bool;
native func Native_UpdateRoomSettings(settings: RoomUpdateSettings) -> Bool;
native func Native_StartRoom() -> Bool;

native func Native_CreateInvite(playerId: Uint32) -> String;
native func Native_AcceptInvite(inviteId: String) -> Bool;
native func Native_DeclineInvite(inviteId: String) -> Bool;

native func Native_RefreshRoomList() -> Void;
native func Native_RequestRoomUpdate() -> Void;
native func Native_SendChatMessage(message: String, targetPlayerId: Uint32) -> Bool;

// Network event handlers
@addMethod(PlayerPuppet)
public func OnNetworkRoomListUpdated(rooms: array<RoomNetworkData>) -> Void {
    RoomManagerUI.GetInstance().OnRoomListUpdated(rooms);
}

@addMethod(PlayerPuppet)
public func OnNetworkRoomJoined(roomData: RoomNetworkData) -> Void {
    RoomManagerUI.GetInstance().OnRoomJoined(roomData);
}

@addMethod(PlayerPuppet)
public func OnNetworkRoomLeft() -> Void {
    RoomManagerUI.GetInstance().OnRoomLeft();
}

@addMethod(PlayerPuppet)
public func OnNetworkRoomStateChanged(newState: ERoomState) -> Void {
    RoomManagerUI.GetInstance().OnRoomStateChanged(newState);
}

@addMethod(PlayerPuppet)
public func OnNetworkPlayerJoinedRoom(playerId: Uint32, playerName: String) -> Void {
    RoomManagerUI.GetInstance().OnPlayerJoinedRoom(playerId, playerName);
}

@addMethod(PlayerPuppet)
public func OnNetworkPlayerLeftRoom(playerId: Uint32, playerName: String, wasKicked: Bool) -> Void {
    RoomManagerUI.GetInstance().OnPlayerLeftRoom(playerId, playerName, wasKicked);
}

@addMethod(PlayerPuppet)
public func OnNetworkPlayerReadyChanged(playerId: Uint32, isReady: Bool) -> Void {
    RoomManagerUI.GetInstance().OnPlayerReadyChanged(playerId, isReady);
}

@addMethod(PlayerPuppet)
public func OnNetworkChatMessageReceived(senderId: Uint32, senderName: String, message: String, targetId: Uint32) -> Void {
    RoomManagerUI.GetInstance().OnChatMessageReceived(senderId, senderName, message, targetId);
}

@addMethod(PlayerPuppet)
public func OnNetworkInviteReceived(inviteData: RoomInviteData) -> Void {
    RoomManagerUI.GetInstance().OnInviteReceived(inviteData);
}

// Integration with main menu system
@wrapMethod(MenuScenario_MultiplayerMenu)
protected cb func OnInitialize() -> Bool {
    let result = wrappedMethod();

    // Initialize room manager when multiplayer menu loads
    RoomManagerUI.GetInstance().Initialize();

    return result;
}

// Console commands for testing
@addMethod(PlayerPuppet)
public func CreateQuickRoom(roomType: ERoomType, maxPlayers: Int32) -> Void {
    RoomManagerUI.GetInstance().CreateQuickRoom(roomType, maxPlayers);
}

@addMethod(PlayerPuppet)
public func QuickJoinRoom(roomType: ERoomType) -> Void {
    RoomManagerUI.GetInstance().QuickJoinRoom(roomType);
}

@addMethod(PlayerPuppet)
public func LeaveRoom() -> Void {
    RoomManagerUI.GetInstance().LeaveCurrentRoom();
}
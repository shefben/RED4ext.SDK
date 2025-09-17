// Connection management system for seamless multiplayer connectivity
// Handles connection flow, reconnection, and connection state management

import Codeware.UI

public enum ConnectionState {
    Disconnected = 0,
    Connecting = 1,
    Connected = 2,
    Reconnecting = 3,
    Failed = 4
}

public enum DisconnectReason {
    UserRequested = 0,
    ServerShutdown = 1,
    NetworkError = 2,
    Timeout = 3,
    Kicked = 4,
    Banned = 5,
    VersionMismatch = 6,
    ServerFull = 7,
    InvalidPassword = 8,
    Unknown = 9
}

public struct ConnectionInfo {
    public var serverName: String;
    public var serverIP: String;
    public var serverPort: Uint32;
    public var playerName: String;
    public var peerId: Uint32;
    public var ping: Uint32;
    public var connectionTime: Uint64;
    public var lastActivity: Uint64;
}

public class ConnectionManager {
    private static var currentState: ConnectionState;
    private static var connectionInfo: ConnectionInfo;
    private static var lastConnectionAttempt: Uint64;
    private static var reconnectAttempts: Uint32;
    private static var maxReconnectAttempts: Uint32 = 5u;
    private static var reconnectDelay: Uint32 = 5000u; // 5 seconds
    private static var connectionTimeout: Uint32 = 30000u; // 30 seconds
    private static var heartbeatInterval: Uint32 = 5000u; // 5 seconds
    private static var lastHeartbeat: Uint64;
    private static var pendingConnection: ref<ServerInfo>;
    private static var passwordDialog: wref<PasswordDialog>;
    
    // Connection callbacks
    private static var onConnectedCallbacks: array<func(ConnectionInfo)>;
    private static var onDisconnectedCallbacks: array<func(DisconnectReason)>;
    private static var onConnectionFailedCallbacks: array<func(String)>;
    private static var onReconnectingCallbacks: array<func(Uint32)>;
    
    public static func Initialize() -> Void {
        ConnectionManager.currentState = ConnectionState.Disconnected;
        ConnectionManager.reconnectAttempts = 0u;
        ConnectionManager.lastConnectionAttempt = 0u;
        ConnectionManager.lastHeartbeat = 0u;
        
        // Initialize connection info
        ConnectionManager.connectionInfo.serverName = "";
        ConnectionManager.connectionInfo.serverIP = "";
        ConnectionManager.connectionInfo.serverPort = 0u;
        ConnectionManager.connectionInfo.playerName = GetLocalPlayerName();
        ConnectionManager.connectionInfo.peerId = 0u;
        ConnectionManager.connectionInfo.ping = 0u;
        ConnectionManager.connectionInfo.connectionTime = 0u;
        ConnectionManager.connectionInfo.lastActivity = 0u;
        
        LogChannel(n"CONNECTION", "ConnectionManager initialized");
    }
    
    public static func ConnectToServer(server: ServerInfo) -> Void {
        if ConnectionManager.currentState == ConnectionState.Connecting || 
           ConnectionManager.currentState == ConnectionState.Connected {
            LogChannel(n"CONNECTION", "Already connected or connecting");
            return;
        }
        
        LogChannel(n"CONNECTION", "Attempting to connect to: " + server.name + " (" + server.ip + ":" + IntToString(server.port) + ")");
        
        ConnectionManager.pendingConnection = server;
        ConnectionManager.SetConnectionState(ConnectionState.Connecting);
        
        // Update connection info
        ConnectionManager.connectionInfo.serverName = server.name;
        ConnectionManager.connectionInfo.serverIP = server.ip;
        ConnectionManager.connectionInfo.serverPort = server.port;
        ConnectionManager.lastConnectionAttempt = GetCurrentTimeMs();
        
        // Check if password is required
        if server.hasPassword && !IsDefined(ConnectionManager.passwordDialog) {
            ConnectionManager.ShowPasswordDialog(server);
            return;
        }
        
        // Start connection process
        ConnectionManager.BeginConnection(server, "");
    }
    
    public static func ConnectWithPassword(server: ServerInfo, password: String) -> Void {
        if ConnectionManager.pendingConnection.id != server.id {
            LogChannel(n"CONNECTION", "Password connection mismatch");
            return;
        }
        
        ConnectionManager.BeginConnection(server, password);
    }
    
    private static func BeginConnection(server: ServerInfo, password: String) -> Void {
        // Validate version compatibility
        if !VersionCheck.ValidateRemoteVersion(StringToUint(server.version)) {
            ConnectionManager.OnConnectionFailed("Version mismatch - please update your client");
            return;
        }
        
        // Check if server is full
        if server.currentPlayers >= server.maxPlayers {
            ConnectionManager.OnConnectionFailed("Server is full");
            return;
        }
        
        // Start native connection
        let success = Net_ConnectToServer(server.ip, server.port, password);
        if !success {
            ConnectionManager.OnConnectionFailed("Failed to initialize connection");
            return;
        }
        
        // Set connection timeout
        GameInstance.GetDelaySystem(GetGame()).DelayCallback(ConnectionManager, n"CheckConnectionTimeout", 
                                                           Cast<Float>(ConnectionManager.connectionTimeout) / 1000.0);
        
        // Show connecting dialog
        ConnectionManager.ShowConnectingDialog();
    }
    
    public static func Disconnect(reason: DisconnectReason) -> Void {
        if ConnectionManager.currentState == ConnectionState.Disconnected {
            return;
        }
        
        LogChannel(n"CONNECTION", "Disconnecting from server (reason: " + IntToString(Cast<Int32>(reason)) + ")");
        
        // Stop any reconnection attempts
        ConnectionManager.reconnectAttempts = 0u;
        
        // Close network connection
        Net_Disconnect();
        
        // Update state
        ConnectionManager.SetConnectionState(ConnectionState.Disconnected);
        
        // Clear connection info
        ConnectionManager.ClearConnectionInfo();
        
        // Hide any connection dialogs
        ConnectionManager.HideConnectionDialogs();
        
        // Notify listeners
        ConnectionManager.NotifyDisconnected(reason);
        
        // Show appropriate message to user
        ConnectionManager.ShowDisconnectionMessage(reason);
    }
    
    public static func OnConnectionEstablished(peerId: Uint32) -> Void {
        LogChannel(n"CONNECTION", "Connection established with peer ID: " + IntToString(peerId));
        
        ConnectionManager.connectionInfo.peerId = peerId;
        ConnectionManager.connectionInfo.connectionTime = GetCurrentTimeMs();
        ConnectionManager.connectionInfo.lastActivity = GetCurrentTimeMs();
        ConnectionManager.reconnectAttempts = 0u;
        
        ConnectionManager.SetConnectionState(ConnectionState.Connected);
        ConnectionManager.HideConnectionDialogs();
        
        // Start heartbeat
        ConnectionManager.StartHeartbeat();
        
        // Notify listeners
        ConnectionManager.NotifyConnected(ConnectionManager.connectionInfo);
        
        // Show success notification
        NotificationManager.ShowSuccess("Connected to " + ConnectionManager.connectionInfo.serverName);
        
        // Close coop UI
        CoopUIManager.Hide();
    }
    
    public static func OnConnectionFailed(reason: String) -> Void {
        LogChannel(n"CONNECTION", "Connection failed: " + reason);
        
        ConnectionManager.SetConnectionState(ConnectionState.Failed);
        ConnectionManager.HideConnectionDialogs();
        
        // Check if we should attempt reconnection
        if ConnectionManager.ShouldAttemptReconnection() {
            ConnectionManager.AttemptReconnection();
        } else {
            // Notify listeners
            ConnectionManager.NotifyConnectionFailed(reason);
            
            // Show error to user
            NotificationManager.ShowError("Connection failed: " + reason);
            
            // Reset state
            ConnectionManager.SetConnectionState(ConnectionState.Disconnected);
            ConnectionManager.ClearConnectionInfo();
        }
    }
    
    public static func OnConnectionLost() -> Void {
        if ConnectionManager.currentState == ConnectionState.Disconnected {
            return;
        }
        
        LogChannel(n"CONNECTION", "Connection lost to server");
        
        // Stop heartbeat
        ConnectionManager.StopHeartbeat();
        
        // Check if we should attempt reconnection
        if ConnectionManager.ShouldAttemptReconnection() {
            ConnectionManager.SetConnectionState(ConnectionState.Reconnecting);
            ConnectionManager.AttemptReconnection();
        } else {
            ConnectionManager.Disconnect(DisconnectReason.NetworkError);
        }
    }
    
    private static func AttemptReconnection() -> Void {
        if ConnectionManager.reconnectAttempts >= ConnectionManager.maxReconnectAttempts {
            LogChannel(n"CONNECTION", "Max reconnection attempts reached");
            ConnectionManager.OnConnectionFailed("Connection lost - unable to reconnect");
            return;
        }
        
        ConnectionManager.reconnectAttempts++;
        LogChannel(n"CONNECTION", "Reconnection attempt " + IntToString(ConnectionManager.reconnectAttempts) + "/" + IntToString(ConnectionManager.maxReconnectAttempts));
        
        // Show reconnecting dialog
        ConnectionManager.ShowReconnectingDialog();
        
        // Notify listeners
        ConnectionManager.NotifyReconnecting(ConnectionManager.reconnectAttempts);
        
        // Wait before reconnecting
        GameInstance.GetDelaySystem(GetGame()).DelayCallback(ConnectionManager, n"ExecuteReconnection", 
                                                           Cast<Float>(ConnectionManager.reconnectDelay) / 1000.0);
    }
    
    protected static cb func ExecuteReconnection() -> Void {
        if ConnectionManager.currentState != ConnectionState.Reconnecting {
            return;
        }
        
        if !IsDefined(ConnectionManager.pendingConnection) {
            ConnectionManager.OnConnectionFailed("Reconnection failed - no server info");
            return;
        }
        
        // Attempt to reconnect
        ConnectionManager.BeginConnection(ConnectionManager.pendingConnection, "");
    }
    
    private static func ShouldAttemptReconnection() -> Bool {
        // Don't reconnect if user requested disconnection
        if ConnectionManager.currentState == ConnectionState.Disconnected {
            return false;
        }
        
        // Don't reconnect if we haven't reached max attempts
        if ConnectionManager.reconnectAttempts >= ConnectionManager.maxReconnectAttempts {
            return false;
        }
        
        // Don't reconnect if we were kicked or banned
        // (This would be determined by the disconnect reason)
        
        return true;
    }
    
    public static func Update() -> Void {
        if ConnectionManager.currentState == ConnectionState.Connected {
            // Update ping and activity
            ConnectionManager.UpdateConnectionStats();
            
            // Check heartbeat
            ConnectionManager.CheckHeartbeat();
        }
        
        // Process any network events
        ConnectionManager.ProcessNetworkEvents();
    }
    
    private static func UpdateConnectionStats() -> Void {
        ConnectionManager.connectionInfo.ping = Net_GetPing();
        ConnectionManager.connectionInfo.lastActivity = GetCurrentTimeMs();
        
        // Update UI if connection manager UI is open
        let ui = CoopUIManager.GetInstance();
        if IsDefined(ui) {
            ui.UpdateConnectionStatus(true, ConnectionManager.connectionInfo.serverName);
        }
    }
    
    private static func CheckHeartbeat() -> Void {
        let currentTime = GetCurrentTimeMs();
        
        if currentTime - ConnectionManager.lastHeartbeat >= ConnectionManager.heartbeatInterval {
            Net_SendHeartbeat();
            ConnectionManager.lastHeartbeat = currentTime;
        }
        
        // Check for connection timeout (no activity)
        if currentTime - ConnectionManager.connectionInfo.lastActivity > ConnectionManager.connectionTimeout {
            LogChannel(n"CONNECTION", "Connection timeout - no activity");
            ConnectionManager.OnConnectionLost();
        }
    }
    
    private static func StartHeartbeat() -> Void {
        ConnectionManager.lastHeartbeat = GetCurrentTimeMs();
    }
    
    private static func StopHeartbeat() -> Void {
        ConnectionManager.lastHeartbeat = 0u;
    }
    
    private static func ProcessNetworkEvents() -> Void {
        // Process incoming network events
        Net_Poll(5u); // 5ms timeout
        
        // Handle any pending network events
        let networkEvent = Net_GetNextEvent();
        if IsDefined(networkEvent) {
            ConnectionManager.HandleNetworkEvent(networkEvent);
        }
    }
    
    private static func HandleNetworkEvent(event: ref<NetworkEvent>) -> Void {
        switch event.type {
            case NetworkEventType.Connected:
                ConnectionManager.OnConnectionEstablished(event.peerId);
                break;
            case NetworkEventType.Disconnected:
                ConnectionManager.OnConnectionLost();
                break;
            case NetworkEventType.ConnectionFailed:
                ConnectionManager.OnConnectionFailed(event.message);
                break;
            case NetworkEventType.Heartbeat:
                ConnectionManager.connectionInfo.lastActivity = GetCurrentTimeMs();
                break;
        }
    }
    
    protected static cb func CheckConnectionTimeout() -> Void {
        if ConnectionManager.currentState == ConnectionState.Connecting {
            LogChannel(n"CONNECTION", "Connection timeout");
            ConnectionManager.OnConnectionFailed("Connection timeout");
        }
    }
    
    // UI Management
    private static func ShowPasswordDialog(server: ServerInfo) -> Void {
        ConnectionManager.passwordDialog = PasswordDialog.Show(server, ConnectionManager);
    }
    
    private static func ShowConnectingDialog() -> Void {
        let connectingDialog = CoopUIManager.GetInstance().GetConnectingDialog();
        if IsDefined(connectingDialog) {
            connectingDialog.ShowConnecting(ConnectionManager.connectionInfo.serverName);
        }
    }
    
    private static func ShowReconnectingDialog() -> Void {
        let connectingDialog = CoopUIManager.GetInstance().GetConnectingDialog();
        if IsDefined(connectingDialog) {
            connectingDialog.ShowReconnecting(ConnectionManager.reconnectAttempts, ConnectionManager.maxReconnectAttempts);
        }
    }
    
    private static func HideConnectionDialogs() -> Void {
        if IsDefined(ConnectionManager.passwordDialog) {
            ConnectionManager.passwordDialog.Hide();
            ConnectionManager.passwordDialog = null;
        }
        
        let connectingDialog = CoopUIManager.GetInstance().GetConnectingDialog();
        if IsDefined(connectingDialog) {
            connectingDialog.Hide();
        }
    }
    
    private static func ShowDisconnectionMessage(reason: DisconnectReason) -> Void {
        let message = ConnectionManager.GetDisconnectReasonText(reason);
        
        switch reason {
            case DisconnectReason.UserRequested:
                // Don't show notification for user-requested disconnections
                break;
            case DisconnectReason.Kicked:
            case DisconnectReason.Banned:
            case DisconnectReason.VersionMismatch:
                NotificationManager.ShowError(message);
                break;
            default:
                NotificationManager.ShowInfo(message);
                break;
        }
    }
    
    private static func GetDisconnectReasonText(reason: DisconnectReason) -> String {
        switch reason {
            case DisconnectReason.UserRequested:
                return "Disconnected";
            case DisconnectReason.ServerShutdown:
                return "Server shutdown";
            case DisconnectReason.NetworkError:
                return "Network error";
            case DisconnectReason.Timeout:
                return "Connection timeout";
            case DisconnectReason.Kicked:
                return "Kicked from server";
            case DisconnectReason.Banned:
                return "Banned from server";
            case DisconnectReason.VersionMismatch:
                return "Version mismatch";
            case DisconnectReason.ServerFull:
                return "Server is full";
            case DisconnectReason.InvalidPassword:
                return "Invalid password";
            default:
                return "Unknown disconnection reason";
        }
    }
    
    // State Management
    private static func SetConnectionState(newState: ConnectionState) -> Void {
        if ConnectionManager.currentState == newState {
            return;
        }
        
        let oldState = ConnectionManager.currentState;
        ConnectionManager.currentState = newState;
        
        LogChannel(n"CONNECTION", "Connection state changed: " + IntToString(Cast<Int32>(oldState)) + " -> " + IntToString(Cast<Int32>(newState)));
        
        // Update UI state if coop UI is open
        let ui = CoopUIManager.GetInstance();
        if IsDefined(ui) {
            switch newState {
                case ConnectionState.Connecting:
                case ConnectionState.Reconnecting:
                    ui.SetState(CoopUIState.Connecting);
                    break;
                case ConnectionState.Connected:
                    ui.SetState(CoopUIState.Connected);
                    break;
                case ConnectionState.Disconnected:
                case ConnectionState.Failed:
                    if ui.GetCurrentState() == CoopUIState.Connecting {
                        ui.SetState(CoopUIState.ServerBrowser);
                    }
                    break;
            }
        }
    }
    
    private static func ClearConnectionInfo() -> Void {
        ConnectionManager.connectionInfo.serverName = "";
        ConnectionManager.connectionInfo.serverIP = "";
        ConnectionManager.connectionInfo.serverPort = 0u;
        ConnectionManager.connectionInfo.peerId = 0u;
        ConnectionManager.connectionInfo.ping = 0u;
        ConnectionManager.connectionInfo.connectionTime = 0u;
        ConnectionManager.connectionInfo.lastActivity = 0u;
        
        ConnectionManager.pendingConnection = null;
    }
    
    // Callback Management
    public static func RegisterOnConnected(callback: func(ConnectionInfo)) -> Void {
        ArrayPush(ConnectionManager.onConnectedCallbacks, callback);
    }
    
    public static func RegisterOnDisconnected(callback: func(DisconnectReason)) -> Void {
        ArrayPush(ConnectionManager.onDisconnectedCallbacks, callback);
    }
    
    public static func RegisterOnConnectionFailed(callback: func(String)) -> Void {
        ArrayPush(ConnectionManager.onConnectionFailedCallbacks, callback);
    }
    
    public static func RegisterOnReconnecting(callback: func(Uint32)) -> Void {
        ArrayPush(ConnectionManager.onReconnectingCallbacks, callback);
    }
    
    private static func NotifyConnected(info: ConnectionInfo) -> Void {
        for callback in ConnectionManager.onConnectedCallbacks {
            callback(info);
        }
    }
    
    private static func NotifyDisconnected(reason: DisconnectReason) -> Void {
        for callback in ConnectionManager.onDisconnectedCallbacks {
            callback(reason);
        }
    }
    
    private static func NotifyConnectionFailed(message: String) -> Void {
        for callback in ConnectionManager.onConnectionFailedCallbacks {
            callback(message);
        }
    }
    
    private static func NotifyReconnecting(attempts: Uint32) -> Void {
        for callback in ConnectionManager.onReconnectingCallbacks {
            callback(attempts);
        }
    }
    
    // Public API
    public static func GetConnectionState() -> ConnectionState {
        return ConnectionManager.currentState;
    }
    
    public static func GetConnectionInfo() -> ConnectionInfo {
        return ConnectionManager.connectionInfo;
    }
    
    public static func IsConnected() -> Bool {
        return ConnectionManager.currentState == ConnectionState.Connected;
    }
    
    public static func IsConnecting() -> Bool {
        return ConnectionManager.currentState == ConnectionState.Connecting || 
               ConnectionManager.currentState == ConnectionState.Reconnecting;
    }
    
    public static func GetServerName() -> String {
        return ConnectionManager.connectionInfo.serverName;
    }
    
    public static func GetPing() -> Uint32 {
        return ConnectionManager.connectionInfo.ping;
    }
    
    public static func GetConnectionTime() -> Uint64 {
        if ConnectionManager.currentState == ConnectionState.Connected {
            return GetCurrentTimeMs() - ConnectionManager.connectionInfo.connectionTime;
        }
        return 0u;
    }
}

// Supporting systems
public class PasswordDialog extends inkGameController {
    private var server: ServerInfo;
    private var connectionManager: ref<ConnectionManager>;
    private var passwordInput: wref<inkTextInput>;
    private var connectButton: wref<inkButton>;
    private var cancelButton: wref<inkButton>;
    
    public static func Show(server: ServerInfo, manager: ref<ConnectionManager>) -> ref<PasswordDialog> {
        let dialog = new PasswordDialog();
        dialog.server = server;
        dialog.connectionManager = manager;
        dialog.CreateUI();
        
        let hudManager = GameInstance.GetHUDManager(GetGame());
        if IsDefined(hudManager) {
            hudManager.AddLayer(dialog);
        }
        
        return dialog;
    }
    
    private func CreateUI() -> Void {
        let container = new inkCanvas();
        container.SetAnchor(inkEAnchor.Fill);
        this.SetRootWidget(container);
        
        // Background overlay
        let overlay = new inkRectangle();
        overlay.SetAnchor(inkEAnchor.Fill);
        overlay.SetTintColor(new HDRColor(0.0, 0.0, 0.0, 0.7));
        container.AddChild(overlay);
        
        // Dialog panel
        let dialog = new inkVerticalPanel();
        dialog.SetAnchor(inkEAnchor.Center);
        dialog.SetSize(new Vector2(400.0, 200.0));
        dialog.SetChildMargin(new inkMargin(10.0, 10.0, 10.0, 10.0));
        container.AddChild(dialog);
        
        // Background
        let dialogBg = new inkRectangle();
        dialogBg.SetAnchor(inkEAnchor.Fill);
        dialogBg.SetTintColor(new HDRColor(0.1, 0.1, 0.1, 0.9));
        dialog.AddChild(dialogBg);
        
        // Title
        let title = new inkText();
        title.SetText("SERVER PASSWORD REQUIRED");
        title.SetFontSize(20);
        title.SetHorizontalAlignment(textHorizontalAlignment.Center);
        title.SetTintColor(new HDRColor(0.368627, 0.964706, 1.0, 1.0));
        dialog.AddChild(title);
        
        // Server name
        let serverName = new inkText();
        serverName.SetText(this.server.name);
        serverName.SetFontSize(16);
        serverName.SetHorizontalAlignment(textHorizontalAlignment.Center);
        dialog.AddChild(serverName);
        
        // Password input
        this.passwordInput = new inkTextInput();
        this.passwordInput.SetText("");
        this.passwordInput.SetSize(new Vector2(300.0, 30.0));
        this.passwordInput.SetIsPassword(true);
        dialog.AddChild(this.passwordInput);
        
        // Buttons
        let buttonPanel = new inkHorizontalPanel();
        buttonPanel.SetChildMargin(new inkMargin(10.0, 0.0, 10.0, 0.0));
        dialog.AddChild(buttonPanel);
        
        this.connectButton = new inkButton();
        this.connectButton.SetText("CONNECT");
        this.connectButton.SetSize(new Vector2(100.0, 40.0));
        this.connectButton.RegisterToCallback(n"OnRelease", this, n"OnConnectClicked");
        buttonPanel.AddChild(this.connectButton);
        
        this.cancelButton = new inkButton();
        this.cancelButton.SetText("CANCEL");
        this.cancelButton.SetSize(new Vector2(100.0, 40.0));
        this.cancelButton.RegisterToCallback(n"OnRelease", this, n"OnCancelClicked");
        buttonPanel.AddChild(this.cancelButton);
        
        // Focus password input
        this.passwordInput.SetInputFocus();
    }
    
    protected cb func OnConnectClicked(e: ref<inkPointerEvent>) -> Bool {
        let password = this.passwordInput.GetText();
        if password != "" {
            ConnectionManager.ConnectWithPassword(this.server, password);
            this.Hide();
        } else {
            NotificationManager.ShowError("Please enter the server password");
        }
        return true;
    }
    
    protected cb func OnCancelClicked(e: ref<inkPointerEvent>) -> Bool {
        ConnectionManager.OnConnectionFailed("Password entry cancelled");
        this.Hide();
        return true;
    }
    
    public func Hide() -> Void {
        let hudManager = GameInstance.GetHUDManager(GetGame());
        if IsDefined(hudManager) {
            hudManager.RemoveLayer(this);
        }
    }
}

// Utility functions
private static func GetCurrentTimeMs() -> Uint64 {
    return Cast<Uint64>(GameClock.GetTime());
}

private static func GetLocalPlayerName() -> String {
    // This would get the player's name from game systems
    return "Player"; // Placeholder
}
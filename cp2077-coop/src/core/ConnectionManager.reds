// ============================================================================
// Connection Manager
// ============================================================================
// Manages network connections and session state for the coop mod

public class ConnectionManager {
    private static var isInitialized: Bool = false;
    private static var isConnected: Bool = false;
    private static var currentServerId: Uint32 = 0u;
    private static var localPeerId: Uint32 = 0u;
    private static var serverAddress: String = "";
    private static var serverPort: Uint32 = 0u;
    
    // Connection state tracking
    private static var connectionAttempts: Uint32 = 0u;
    private static var maxConnectionAttempts: Uint32 = 3u;
    private static var connectionTimeout: Float = 10.0; // seconds
    private static var lastConnectionAttempt: Float = 0.0;
    
    public static func Initialize() -> Bool {
        if isInitialized {
            LogChannel(n"INFO", "ConnectionManager already initialized");
            return true;
        }
        
        LogChannel(n"INFO", "Initializing ConnectionManager...");
        
        // Initialize native networking
        if !CoopNative_Initialize() {
            LogChannel(n"ERROR", "Failed to initialize native coop systems");
            return false;
        }
        
        // Reset connection state
        isConnected = false;
        currentServerId = 0u;
        localPeerId = 0u;
        connectionAttempts = 0u;
        
        isInitialized = true;
        LogChannel(n"INFO", "ConnectionManager initialized successfully");
        return true;
    }
    
    public static func Shutdown() -> Void {
        if !isInitialized {
            return;
        }
        
        LogChannel(n"INFO", "Shutting down ConnectionManager...");
        
        // Disconnect if connected
        if isConnected {
            Disconnect();
        }
        
        // Shutdown native systems
        CoopNative_Shutdown();
        
        isInitialized = false;
        LogChannel(n"INFO", "ConnectionManager shutdown complete");
    }
    
    public static func ConnectToServer(address: String, port: Uint32) -> Bool {
        if !isInitialized {
            LogChannel(n"ERROR", "ConnectionManager not initialized");
            return false;
        }
        
        if isConnected {
            LogChannel(n"WARNING", "Already connected to a server");
            return false;
        }
        
        if connectionAttempts >= maxConnectionAttempts {
            LogChannel(n"ERROR", "Maximum connection attempts exceeded");
            return false;
        }
        
        LogChannel(n"INFO", "Attempting to connect to " + address + ":" + IntToString(port));
        
        serverAddress = address;
        serverPort = port;
        connectionAttempts += 1u;
        lastConnectionAttempt = EngineTime.ToFloat(GameInstance.GetTimeSystem(GetGame()).GetGameTime());
        
        // Attempt native connection
        if Net_ConnectToServer(address, port) {
            LogChannel(n"INFO", "Connection attempt initiated");
            return true;
        } else {
            LogChannel(n"ERROR", "Failed to initiate connection");
            return false;
        }
    }
    
    public static func StartServer(port: Uint32, maxPlayers: Uint32) -> Bool {
        if !isInitialized {
            LogChannel(n"ERROR", "ConnectionManager not initialized");
            return false;
        }
        
        if isConnected {
            LogChannel(n"WARNING", "Already hosting/connected to a server");
            return false;
        }
        
        LogChannel(n"INFO", "Starting server on port " + IntToString(port) + " for " + IntToString(maxPlayers) + " players");
        
        if Net_StartServer(port, maxPlayers) {
            isConnected = true;
            localPeerId = 1u; // Host is always peer ID 1
            LogChannel(n"INFO", "Server started successfully");
            return true;
        } else {
            LogChannel(n"ERROR", "Failed to start server");
            return false;
        }
    }
    
    public static func Disconnect() -> Void {
        if !isConnected {
            return;
        }
        
        LogChannel(n"INFO", "Disconnecting from server...");
        
        // Send disconnect to native layer
        Net_Disconnect();
        
        // Reset connection state
        isConnected = false;
        currentServerId = 0u;
        localPeerId = 0u;
        connectionAttempts = 0u;
        serverAddress = "";
        serverPort = 0u;
        
        LogChannel(n"INFO", "Disconnected from server");
    }
    
    public static func IsConnected() -> Bool {
        return isConnected;
    }
    
    public static func IsInitialized() -> Bool {
        return isInitialized;
    }
    
    public static func GetLocalPeerId() -> Uint32 {
        return localPeerId;
    }
    
    public static func GetServerAddress() -> String {
        return serverAddress;
    }
    
    public static func GetServerPort() -> Uint32 {
        return serverPort;
    }
    
    public static func Update() -> Void {
        if !isInitialized {
            return;
        }
        
        // Poll native networking
        Net_Poll(16u); // Poll with 16ms timeout
        
        // Check for connection timeout
        if !isConnected && connectionAttempts > 0 {
            let currentTime = EngineTime.ToFloat(GameInstance.GetTimeSystem(GetGame()).GetGameTime());
            if currentTime - lastConnectionAttempt > connectionTimeout {
                LogChannel(n"WARNING", "Connection attempt timed out");
                connectionAttempts = maxConnectionAttempts; // Stop trying
            }
        }
        
        // Update connection status from native layer
        let newConnectionStatus = Net_IsConnected();
        if newConnectionStatus != isConnected {
            isConnected = newConnectionStatus;
            if isConnected {
                LogChannel(n"INFO", "Connected to server");
                localPeerId = Net_GetPeerId();
                connectionAttempts = 0u;
            } else {
                LogChannel(n"INFO", "Disconnected from server");
                localPeerId = 0u;
            }
        }
    }
    
    public static func GetConnectionInfo() -> String {
        if !isConnected {
            return "Not connected";
        }
        
        return "Connected to " + serverAddress + ":" + IntToString(serverPort) + " (Peer ID: " + IntToString(localPeerId) + ")";
    }
    
    public static func CanRetryConnection() -> Bool {
        return connectionAttempts < maxConnectionAttempts;
    }
    
    public static func ResetConnectionAttempts() -> Void {
        connectionAttempts = 0u;
    }
}
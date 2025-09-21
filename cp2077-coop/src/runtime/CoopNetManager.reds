// CoopNet Manager - Main entry point for CoopNet functionality in REDscript

// Core CoopNet management system
public class CoopNetManager extends IScriptable {

    // === CORE SYSTEM MANAGEMENT ===

    // Initialize the CoopNet system
    public native func Initialize() -> Bool;

    // Shutdown the CoopNet system
    public native func Shutdown() -> Void;

    // Check if all systems are ready
    public native func IsReady() -> Bool;

    // Get overall system status
    public native func GetSystemStatus() -> String;

    // Perform system diagnostics
    public native func PerformDiagnostics() -> Bool;

    // === CONFIGURATION MANAGEMENT ===

    // Load configuration from file
    public native func LoadConfiguration(configPath: String) -> Bool;

    // Save configuration to file
    public native func SaveConfiguration(configPath: String) -> Bool;

    // Configuration getters
    public native func GetConfigString(key: String, defaultValue: String) -> String;
    public native func GetConfigInt(key: String, defaultValue: Int32) -> Int32;
    public native func GetConfigFloat(key: String, defaultValue: Float) -> Float;
    public native func GetConfigBool(key: String, defaultValue: Bool) -> Bool;

    // Configuration setters
    public native func SetConfigString(key: String, value: String) -> Bool;
    public native func SetConfigInt(key: String, value: Int32) -> Bool;
    public native func SetConfigFloat(key: String, value: Float) -> Bool;
    public native func SetConfigBool(key: String, value: Bool) -> Bool;

    // === ERROR HANDLING ===

    // Report a regular error
    public native func ReportError(component: String, error: String) -> Void;

    // Report a critical error
    public native func ReportCriticalError(component: String, error: String) -> Void;

    // === EVENT SYSTEM ===

    // Send an event to the system
    public native func SendEvent(eventType: String, eventData: String) -> Void;

    // Register for event callbacks (implementation specific)
    public native func RegisterEventCallback(eventType: String, callback: ref<ICoopNetEventCallback>) -> Void;

    // Unregister event callback
    public native func UnregisterEventCallback(eventType: String) -> Void;

    // === GAME INTEGRATION HOOKS ===

    // Called when the game starts
    public native func OnGameStart() -> Void;

    // Called when the game stops
    public native func OnGameStop() -> Void;

    // Called when a player connects
    public native func OnPlayerConnect(playerId: Uint32) -> Void;

    // Called when a player disconnects
    public native func OnPlayerDisconnect(playerId: Uint32) -> Void;

    // === UTILITY METHODS ===

    // Get singleton instance
    public static func GetInstance() -> ref<CoopNetManager> {
        return GameInstance.GetCoopNetManager();
    }

    // Quick initialization with default settings
    public func QuickInit() -> Bool {
        if this.Initialize() {
            this.LoadConfiguration("config/coopnet.json");
            return true;
        }
        return false;
    }

    // Enable all systems with default settings
    public func EnableAllSystems() -> Void {
        this.SetConfigBool("enable_voice_chat", true);
        this.SetConfigBool("enable_performance_monitoring", true);
        this.SetConfigBool("enable_network_optimization", true);
        this.SetConfigBool("enable_content_streaming", true);
        this.SetConfigBool("enable_database_integration", true);
    }

    // Disable all optional systems
    public func DisableOptionalSystems() -> Void {
        this.SetConfigBool("enable_voice_chat", false);
        this.SetConfigBool("enable_performance_monitoring", false);
        this.SetConfigBool("enable_network_optimization", false);
        this.SetConfigBool("enable_content_streaming", false);
        this.SetConfigBool("enable_database_integration", false);
    }

    // Set debug mode
    public func SetDebugMode(enabled: Bool) -> Void {
        this.SetConfigBool("debug_mode", enabled);
    }

    // Check if debug mode is enabled
    public func IsDebugMode() -> Bool {
        return this.GetConfigBool("debug_mode", false);
    }

    // Get system uptime
    public func GetSystemUptime() -> String {
        return this.GetConfigString("system_uptime", "Unknown");
    }

    // === EVENT HELPER METHODS ===

    // Send a simple event with basic data
    public func SendSimpleEvent(eventType: String, message: String) -> Void {
        let eventData: String = "{\"message\":\"" + message + "\",\"timestamp\":" + ToString(EngineTime.ToFloat(GameInstance.GetSimTime())) + "}";
        this.SendEvent(eventType, eventData);
    }

    // Send player event
    public func SendPlayerEvent(eventType: String, playerId: Uint32, data: String) -> Void {
        let eventData: String = "{\"player_id\":" + ToString(playerId) + ",\"data\":\"" + data + "\",\"timestamp\":" + ToString(EngineTime.ToFloat(GameInstance.GetSimTime())) + "}";
        this.SendEvent(eventType, eventData);
    }

    // === LOGGING HELPERS ===

    // Log info message
    public func LogInfo(component: String, message: String) -> Void {
        // Use CoopNet's logging system instead of game logging
        this.SendSimpleEvent("log_info", "[" + component + "] " + message);
    }

    // Log warning message
    public func LogWarning(component: String, message: String) -> Void {
        this.SendSimpleEvent("log_warning", "[" + component + "] " + message);
    }

    // Log error message
    public func LogError(component: String, message: String) -> Void {
        this.ReportError(component, message);
    }

    // Log critical error
    public func LogCritical(component: String, message: String) -> Void {
        this.ReportCriticalError(component, message);
    }
}

// Event callback interface that REDscript classes can implement
public abstract class ICoopNetEventCallback extends IScriptable {

    // Override this method to handle events
    public native func OnEvent(eventType: String, eventData: String) -> Void;

    // Helper method to parse JSON event data
    protected func ParseEventData(eventData: String) -> ref<inkHashMap> {
        // Simple JSON parsing - in a real implementation this would be more robust
        let data = new inkHashMap();
        // Basic parsing logic would go here
        return data;
    }
}

// System status data structure
public class CoopNetSystemStatus extends IScriptable {
    public let systemName: String;
    public let status: String;
    public let isHealthy: Bool;
    public let isEnabled: Bool;
    public let cpuUsage: Float;
    public let memoryUsage: Uint64;
    public let errorCount: Uint32;
}

// Extended GameInstance to provide CoopNet access
@addMethod(GameInstance)
public static func GetCoopNetManager() -> ref<CoopNetManager> {
    // This would be implemented in C++ to return the singleton instance
    return null;
}

// Global convenience functions for easy access

// Initialize CoopNet (call this early in game startup)
public static func InitializeCoopNet() -> Bool {
    let manager = CoopNetManager.GetInstance();
    if IsDefined(manager) {
        return manager.QuickInit();
    }
    return false;
}

// Shutdown CoopNet (call this during game shutdown)
public static func ShutdownCoopNet() -> Void {
    let manager = CoopNetManager.GetInstance();
    if IsDefined(manager) {
        manager.Shutdown();
    }
}

// Check if CoopNet is ready
public static func IsCoopNetReady() -> Bool {
    let manager = CoopNetManager.GetInstance();
    if IsDefined(manager) {
        return manager.IsReady();
    }
    return false;
}

// Quick error reporting
public static func ReportCoopNetError(component: String, error: String) -> Void {
    let manager = CoopNetManager.GetInstance();
    if IsDefined(manager) {
        manager.ReportError(component, error);
    }
}

// Quick event sending
public static func SendCoopNetEvent(eventType: String, message: String) -> Void {
    let manager = CoopNetManager.GetInstance();
    if IsDefined(manager) {
        manager.SendSimpleEvent(eventType, message);
    }
}

// Get configuration value
public static func GetCoopNetConfig(key: String, defaultValue: String) -> String {
    let manager = CoopNetManager.GetInstance();
    if IsDefined(manager) {
        return manager.GetConfigString(key, defaultValue);
    }
    return defaultValue;
}

// Set configuration value
public static func SetCoopNetConfig(key: String, value: String) -> Bool {
    let manager = CoopNetManager.GetInstance();
    if IsDefined(manager) {
        return manager.SetConfigString(key, value);
    }
    return false;
}
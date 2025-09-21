// CoopNet Multiplayer Integration - Main integration point for game systems

// Main multiplayer integration system that ties everything together
public class CoopNetMultiplayerIntegration extends IScriptable {

    private let m_coopNetManager: ref<CoopNetManager>;
    private let m_voiceSystem: ref<CoopNetVoiceSystem>;
    private let m_performanceMonitor: ref<CoopNetPerformanceMonitor>;
    private let m_isInitialized: Bool;
    private let m_isInMultiplayer: Bool;
    private let m_connectedPlayers: array<Uint32>;

    // === INITIALIZATION ===

    public func Initialize() -> Bool {
        if this.m_isInitialized {
            return true;
        }

        // Get system instances
        this.m_coopNetManager = CoopNetManager.GetInstance();
        this.m_voiceSystem = CoopNetVoiceSystem.GetInstance();
        this.m_performanceMonitor = CoopNetPerformanceMonitor.GetInstance();

        if !IsDefined(this.m_coopNetManager) {
            return false;
        }

        // Initialize CoopNet systems
        if !this.m_coopNetManager.QuickInit() {
            return false;
        }

        // Enable all systems for multiplayer
        this.m_coopNetManager.EnableAllSystems();

        // Initialize voice system
        if IsDefined(this.m_voiceSystem) {
            this.m_voiceSystem.InitializeWithDefaults();
            this.m_voiceSystem.CreateDefaultChannels();
        }

        // Initialize performance monitoring
        if IsDefined(this.m_performanceMonitor) {
            this.m_performanceMonitor.InitializeWithDefaults();
            this.m_performanceMonitor.ApplyPerformancePreset("balanced");
        }

        // Set up event handlers
        this.RegisterEventHandlers();

        this.m_isInitialized = true;
        this.LogInfo("CoopNet multiplayer integration initialized successfully");

        return true;
    }

    public func Shutdown() -> Void {
        if !this.m_isInitialized {
            return;
        }

        this.LeaveMultiplayer();

        if IsDefined(this.m_coopNetManager) {
            this.m_coopNetManager.Shutdown();
        }

        this.m_isInitialized = false;
        this.LogInfo("CoopNet multiplayer integration shut down");
    }

    // === MULTIPLAYER SESSION MANAGEMENT ===

    public func StartMultiplayerSession() -> Bool {
        if !this.m_isInitialized {
            this.LogError("Cannot start multiplayer session - not initialized");
            return false;
        }

        if this.m_isInMultiplayer {
            this.LogWarning("Already in multiplayer session");
            return true;
        }

        this.LogInfo("Starting multiplayer session...");

        // Notify systems that multiplayer is starting
        if IsDefined(this.m_coopNetManager) {
            this.m_coopNetManager.OnGameStart();
            this.m_coopNetManager.SendSimpleEvent("multiplayer_session_start", "Session starting");
        }

        // Join default voice channel
        if IsDefined(this.m_voiceSystem) && this.m_voiceSystem.IsEnabled() {
            this.m_voiceSystem.JoinGeneralChannel();
        }

        // Start performance monitoring for multiplayer
        if IsDefined(this.m_performanceMonitor) {
            this.m_performanceMonitor.StartMonitoringSession("multiplayer");
        }

        this.m_isInMultiplayer = true;
        this.m_connectedPlayers.Clear();

        this.LogInfo("Multiplayer session started successfully");
        return true;
    }

    public func EndMultiplayerSession() -> Bool {
        if !this.m_isInMultiplayer {
            return true;
        }

        this.LogInfo("Ending multiplayer session...");

        // Leave all voice channels
        if IsDefined(this.m_voiceSystem) {
            this.m_voiceSystem.LeaveAllChannels();
        }

        // End performance monitoring session
        if IsDefined(this.m_performanceMonitor) {
            let report = this.m_performanceMonitor.EndMonitoringSession();
            this.LogInfo("Performance report: " + report);
        }

        // Notify systems that multiplayer is ending
        if IsDefined(this.m_coopNetManager) {
            this.m_coopNetManager.OnGameStop();
            this.m_coopNetManager.SendSimpleEvent("multiplayer_session_end", "Session ended");
        }

        // Clear connected players
        this.m_connectedPlayers.Clear();
        this.m_isInMultiplayer = false;

        this.LogInfo("Multiplayer session ended");
        return true;
    }

    public func LeaveMultiplayer() -> Void {
        this.EndMultiplayerSession();
    }

    public func IsInMultiplayer() -> Bool {
        return this.m_isInMultiplayer;
    }

    // === PLAYER MANAGEMENT ===

    public func OnPlayerJoined(playerId: Uint32, playerName: String) -> Void {
        if !this.m_isInMultiplayer {
            return;
        }

        this.LogInfo("Player joined: " + playerName + " (ID: " + ToString(playerId) + ")");

        // Add to connected players list
        ArrayPush(this.m_connectedPlayers, playerId);

        // Notify systems
        if IsDefined(this.m_coopNetManager) {
            this.m_coopNetManager.OnPlayerConnect(playerId);
            this.m_coopNetManager.SendPlayerEvent("player_joined", playerId, playerName);
        }

        // Set up voice communication for new player
        if IsDefined(this.m_voiceSystem) && this.m_voiceSystem.IsEnabled() {
            // New players start unmuted
            this.m_voiceSystem.UnmutePlayer(playerId);
        }
    }

    public func OnPlayerLeft(playerId: Uint32, playerName: String) -> Void {
        this.LogInfo("Player left: " + playerName + " (ID: " + ToString(playerId) + ")");

        // Remove from connected players list
        ArrayRemove(this.m_connectedPlayers, playerId);

        // Notify systems
        if IsDefined(this.m_coopNetManager) {
            this.m_coopNetManager.OnPlayerDisconnect(playerId);
            this.m_coopNetManager.SendPlayerEvent("player_left", playerId, playerName);
        }
    }

    public func GetConnectedPlayers() -> array<Uint32> {
        return this.m_connectedPlayers;
    }

    public func GetPlayerCount() -> Int32 {
        return ArraySize(this.m_connectedPlayers);
    }

    // === VOICE COMMUNICATION INTEGRATION ===

    public func TogglePlayerVoiceMute(playerId: Uint32) -> Bool {
        if !IsDefined(this.m_voiceSystem) {
            return false;
        }

        let wasMuted = this.m_voiceSystem.IsPlayerMuted(playerId);
        let result = this.m_voiceSystem.TogglePlayerMute(playerId);

        if result {
            let action = wasMuted ? "unmuted" : "muted";
            this.LogInfo("Player " + ToString(playerId) + " " + action);
        }

        return result;
    }

    public func SetVoiceQuality(quality: String) -> Bool {
        if !IsDefined(this.m_voiceSystem) {
            return false;
        }

        let result = this.m_voiceSystem.SetVoiceQualityByName(quality);
        if result {
            this.LogInfo("Voice quality set to: " + quality);
        }

        return result;
    }

    public func EnableProximityVoiceChat(maxDistance: Float) -> Bool {
        if !IsDefined(this.m_voiceSystem) {
            return false;
        }

        let result = this.m_voiceSystem.EnableProximityChat(maxDistance);
        if result {
            this.LogInfo("Proximity voice chat enabled with max distance: " + ToString(maxDistance));
        }

        return result;
    }

    public func DisableProximityVoiceChat() -> Bool {
        if !IsDefined(this.m_voiceSystem) {
            return false;
        }

        let result = this.m_voiceSystem.DisableProximityChat();
        if result {
            this.LogInfo("Proximity voice chat disabled");
        }

        return result;
    }

    // === PERFORMANCE OPTIMIZATION ===

    public func SetPerformancePreset(preset: String) -> Bool {
        if !IsDefined(this.m_performanceMonitor) {
            return false;
        }

        let result = this.m_performanceMonitor.ApplyPerformancePreset(preset);
        if result {
            this.LogInfo("Performance preset set to: " + preset);
        }

        return result;
    }

    public func GetPerformanceStatus() -> String {
        if !IsDefined(this.m_performanceMonitor) {
            return "Performance monitoring unavailable";
        }

        let fps = this.m_performanceMonitor.GetCurrentFPS();
        let grade = this.m_performanceMonitor.GetPerformanceGrade();
        let preset = this.m_performanceMonitor.GetCurrentPreset();

        return "FPS: " + ToString(fps) + ", Grade: " + grade + ", Preset: " + preset;
    }

    public func OptimizeForMultiplayer() -> Void {
        if IsDefined(this.m_performanceMonitor) {
            // Use balanced preset for multiplayer
            this.m_performanceMonitor.ApplyPerformancePreset("balanced");
            this.m_performanceMonitor.EnableAdaptiveQuality(true);
        }

        if IsDefined(this.m_voiceSystem) {
            // Optimize voice settings for multiplayer
            this.m_voiceSystem.SetVoiceQualityByName("medium");
            this.m_voiceSystem.SetNoiseSuppression(true);
        }

        this.LogInfo("Applied multiplayer optimizations");
    }

    // === CONFIGURATION AND SETTINGS ===

    public func LoadMultiplayerConfig() -> Bool {
        if !IsDefined(this.m_coopNetManager) {
            return false;
        }

        let result = this.m_coopNetManager.LoadConfiguration("config/multiplayer.json");
        if result {
            this.LogInfo("Multiplayer configuration loaded");
        } else {
            this.LogWarning("Failed to load multiplayer configuration, using defaults");
        }

        return result;
    }

    public func SaveMultiplayerConfig() -> Bool {
        if !IsDefined(this.m_coopNetManager) {
            return false;
        }

        let result = this.m_coopNetManager.SaveConfiguration("config/multiplayer.json");
        if result {
            this.LogInfo("Multiplayer configuration saved");
        }

        return result;
    }

    public func ResetToDefaults() -> Void {
        if IsDefined(this.m_coopNetManager) {
            this.m_coopNetManager.EnableAllSystems();
        }

        if IsDefined(this.m_voiceSystem) {
            this.m_voiceSystem.InitializeWithDefaults();
        }

        if IsDefined(this.m_performanceMonitor) {
            this.m_performanceMonitor.ApplyPerformancePreset("balanced");
        }

        this.LogInfo("Reset to default settings");
    }

    // === DIAGNOSTICS AND STATUS ===

    public func GetSystemStatus() -> String {
        let status = "CoopNet Multiplayer Status:\n";
        status += "Initialized: " + ToString(this.m_isInitialized) + "\n";
        status += "In Multiplayer: " + ToString(this.m_isInMultiplayer) + "\n";
        status += "Connected Players: " + ToString(this.GetPlayerCount()) + "\n";

        if IsDefined(this.m_coopNetManager) {
            status += "CoopNet Status: " + this.m_coopNetManager.GetSystemStatus() + "\n";
        }

        if IsDefined(this.m_voiceSystem) {
            status += "Voice Enabled: " + ToString(this.m_voiceSystem.IsEnabled()) + "\n";
        }

        if IsDefined(this.m_performanceMonitor) {
            status += "Performance: " + this.GetPerformanceStatus() + "\n";
        }

        return status;
    }

    public func RunDiagnostics() -> Bool {
        if !IsDefined(this.m_coopNetManager) {
            return false;
        }

        this.LogInfo("Running system diagnostics...");

        let result = this.m_coopNetManager.PerformDiagnostics();

        if IsDefined(this.m_voiceSystem) {
            let voiceTest = this.m_voiceSystem.RunVoiceTest();
            this.LogInfo("Voice system test: " + ToString(voiceTest));
        }

        if IsDefined(this.m_performanceMonitor) {
            let perfCheck = this.m_performanceMonitor.CheckMinimumRequirements();
            this.LogInfo("Performance check: " + ToString(perfCheck));
        }

        this.LogInfo("Diagnostics completed: " + ToString(result));
        return result;
    }

    // === PRIVATE METHODS ===

    private func RegisterEventHandlers() -> Void {
        // Set up event handlers for inter-system communication
        if IsDefined(this.m_coopNetManager) {
            // Register for system events
            this.m_coopNetManager.SendSimpleEvent("integration_ready", "Multiplayer integration is ready");
        }
    }

    private func LogInfo(message: String) -> Void {
        if IsDefined(this.m_coopNetManager) {
            this.m_coopNetManager.LogInfo("MultiplayerIntegration", message);
        }
    }

    private func LogWarning(message: String) -> Void {
        if IsDefined(this.m_coopNetManager) {
            this.m_coopNetManager.LogWarning("MultiplayerIntegration", message);
        }
    }

    private func LogError(message: String) -> Void {
        if IsDefined(this.m_coopNetManager) {
            this.m_coopNetManager.LogError("MultiplayerIntegration", message);
        }
    }

    // === STATIC INSTANCE MANAGEMENT ===

    private static let s_instance: ref<CoopNetMultiplayerIntegration>;

    public static func GetInstance() -> ref<CoopNetMultiplayerIntegration> {
        if !IsDefined(CoopNetMultiplayerIntegration.s_instance) {
            CoopNetMultiplayerIntegration.s_instance = new CoopNetMultiplayerIntegration();
        }
        return CoopNetMultiplayerIntegration.s_instance;
    }
}

// Event callback implementation for multiplayer events
public class MultiplayerEventCallback extends ICoopNetEventCallback {

    public func OnEvent(eventType: String, eventData: String) -> Void {
        let integration = CoopNetMultiplayerIntegration.GetInstance();

        switch eventType {
            case "player_joined":
                // Handle player join event
                break;

            case "player_left":
                // Handle player leave event
                break;

            case "performance_warning":
                // Handle performance warnings
                if IsDefined(integration) {
                    integration.OptimizeForMultiplayer();
                }
                break;

            case "voice_quality_degraded":
                // Handle voice quality issues
                break;

            default:
                // Handle unknown events
                break;
        }
    }
}

// Global convenience functions for multiplayer integration

// Initialize the entire multiplayer system
public static func InitializeMultiplayer() -> Bool {
    let integration = CoopNetMultiplayerIntegration.GetInstance();
    if IsDefined(integration) {
        return integration.Initialize();
    }
    return false;
}

// Start a multiplayer session
public static func StartMultiplayerSession() -> Bool {
    let integration = CoopNetMultiplayerIntegration.GetInstance();
    if IsDefined(integration) {
        return integration.StartMultiplayerSession();
    }
    return false;
}

// End the current multiplayer session
public static func EndMultiplayerSession() -> Bool {
    let integration = CoopNetMultiplayerIntegration.GetInstance();
    if IsDefined(integration) {
        return integration.EndMultiplayerSession();
    }
    return false;
}

// Check if currently in multiplayer
public static func IsInMultiplayer() -> Bool {
    let integration = CoopNetMultiplayerIntegration.GetInstance();
    if IsDefined(integration) {
        return integration.IsInMultiplayer();
    }
    return false;
}

// Get connected player count
public static func GetMultiplayerPlayerCount() -> Int32 {
    let integration = CoopNetMultiplayerIntegration.GetInstance();
    if IsDefined(integration) {
        return integration.GetPlayerCount();
    }
    return 0;
}

// Quick performance optimization for multiplayer
public static func OptimizeForMultiplayer() -> Void {
    let integration = CoopNetMultiplayerIntegration.GetInstance();
    if IsDefined(integration) {
        integration.OptimizeForMultiplayer();
    }
}

// Get overall multiplayer system status
public static func GetMultiplayerStatus() -> String {
    let integration = CoopNetMultiplayerIntegration.GetInstance();
    if IsDefined(integration) {
        return integration.GetSystemStatus();
    }
    return "Multiplayer integration not available";
}
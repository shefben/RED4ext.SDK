// CoopNet Voice Communication System - REDscript interface for voice chat

// Voice communication system for multiplayer
public class CoopNetVoiceSystem extends IScriptable {

    // === VOICE SYSTEM CONTROL ===

    // Check if voice system is enabled
    public native func IsEnabled() -> Bool;

    // Enable/disable voice system
    public native func SetEnabled(enabled: Bool) -> Bool;

    // Check if voice system is initialized
    public native func IsInitialized() -> Bool;

    // === VOICE CHANNELS ===

    // Create a new voice channel
    public native func CreateChannel(channelName: String, maxParticipants: Uint32) -> Bool;

    // Join a voice channel
    public native func JoinChannel(channelName: String) -> Bool;

    // Leave a voice channel
    public native func LeaveChannel(channelName: String) -> Bool;

    // Get list of active channels
    public native func GetActiveChannels() -> array<String>;

    // Set channel volume (0.0 - 1.0)
    public native func SetChannelVolume(channelName: String, volume: Float) -> Bool;

    // === VOICE QUALITY SETTINGS ===

    // Set voice quality (0=Low, 1=Medium, 2=High, 3=Ultra)
    public native func SetVoiceQuality(quality: Int32) -> Bool;

    // Get current voice quality
    public native func GetVoiceQuality() -> Int32;

    // Enable/disable spatial audio
    public native func SetSpatialAudio(enabled: Bool) -> Bool;

    // Check if spatial audio is enabled
    public native func IsSpatialAudioEnabled() -> Bool;

    // Enable/disable noise suppression
    public native func SetNoiseSuppression(enabled: Bool) -> Bool;

    // Check if noise suppression is enabled
    public native func IsNoiseSuppressionEnabled() -> Bool;

    // === PUSH-TO-TALK SETTINGS ===

    // Set push-to-talk key
    public native func SetPushToTalkKey(keyCode: Uint32) -> Bool;

    // Enable/disable push-to-talk
    public native func SetPushToTalkEnabled(enabled: Bool) -> Bool;

    // Check if push-to-talk is enabled
    public native func IsPushToTalkEnabled() -> Bool;

    // Enable/disable voice activation
    public native func SetVoiceActivation(enabled: Bool) -> Bool;

    // Check if voice activation is enabled
    public native func IsVoiceActivationEnabled() -> Bool;

    // === PLAYER VOICE MANAGEMENT ===

    // Mute a specific player
    public native func MutePlayer(playerId: Uint32) -> Bool;

    // Unmute a specific player
    public native func UnmutePlayer(playerId: Uint32) -> Bool;

    // Check if a player is muted
    public native func IsPlayerMuted(playerId: Uint32) -> Bool;

    // Set volume for a specific player (0.0 - 1.0)
    public native func SetPlayerVolume(playerId: Uint32, volume: Float) -> Bool;

    // Get volume for a specific player
    public native func GetPlayerVolume(playerId: Uint32) -> Float;

    // === STATISTICS ===

    // Get voice communication statistics
    public native func GetVoiceStatistics() -> ref<CoopNetVoiceStatistics>;

    // Get connection quality
    public native func GetConnectionQuality() -> Float;

    // === UTILITY METHODS ===

    // Get singleton instance
    public static func GetInstance() -> ref<CoopNetVoiceSystem> {
        return GameInstance.GetCoopNetVoiceSystem();
    }

    // Initialize with default settings
    public func InitializeWithDefaults() -> Bool {
        if this.SetEnabled(true) {
            this.SetVoiceQuality(2); // High quality
            this.SetSpatialAudio(true);
            this.SetNoiseSuppression(true);
            this.SetPushToTalkEnabled(false);
            this.SetVoiceActivation(true);
            return true;
        }
        return false;
    }

    // Create default channels for multiplayer
    public func CreateDefaultChannels() -> Void {
        this.CreateChannel("General", 32);
        this.CreateChannel("Team", 8);
        this.CreateChannel("Party", 4);
    }

    // Join general channel
    public func JoinGeneralChannel() -> Bool {
        return this.JoinChannel("General");
    }

    // Join team channel
    public func JoinTeamChannel() -> Bool {
        return this.JoinChannel("Team");
    }

    // Join party channel
    public func JoinPartyChannel() -> Bool {
        return this.JoinChannel("Party");
    }

    // Leave all channels
    public func LeaveAllChannels() -> Void {
        let channels = this.GetActiveChannels();
        for channel in channels {
            this.LeaveChannel(channel);
        }
    }

    // Set voice quality by name
    public func SetVoiceQualityByName(qualityName: String) -> Bool {
        let quality: Int32;
        switch qualityName {
            case "low":
                quality = 0;
                break;
            case "medium":
                quality = 1;
                break;
            case "high":
                quality = 2;
                break;
            case "ultra":
                quality = 3;
                break;
            default:
                return false;
        }
        return this.SetVoiceQuality(quality);
    }

    // Get voice quality as string
    public func GetVoiceQualityName() -> String {
        let quality = this.GetVoiceQuality();
        switch quality {
            case 0:
                return "low";
            case 1:
                return "medium";
            case 2:
                return "high";
            case 3:
                return "ultra";
            default:
                return "unknown";
        }
    }

    // Toggle mute for player
    public func TogglePlayerMute(playerId: Uint32) -> Bool {
        if this.IsPlayerMuted(playerId) {
            return this.UnmutePlayer(playerId);
        } else {
            return this.MutePlayer(playerId);
        }
    }

    // Mute all players except specified ones
    public func MuteAllExcept(exemptPlayers: array<Uint32>) -> Void {
        // This would iterate through all connected players
        // and mute everyone not in the exempt list
        let stats = this.GetVoiceStatistics();
        // Implementation would depend on getting player list
    }

    // Unmute all players
    public func UnmuteAllPlayers() -> Void {
        // This would iterate through all connected players
        // and unmute everyone
        let stats = this.GetVoiceStatistics();
        // Implementation would depend on getting player list
    }

    // === PROXIMITY VOICE CHAT ===

    // Enable proximity voice chat (spatial audio based on player distance)
    public func EnableProximityChat(maxDistance: Float) -> Bool {
        // Set spatial audio and configure distance settings
        if this.SetSpatialAudio(true) {
            let manager = CoopNetManager.GetInstance();
            if IsDefined(manager) {
                manager.SetConfigFloat("voice_proximity_distance", maxDistance);
                return true;
            }
        }
        return false;
    }

    // Disable proximity voice chat
    public func DisableProximityChat() -> Bool {
        return this.SetSpatialAudio(false);
    }

    // Set proximity distance
    public func SetProximityDistance(distance: Float) -> Bool {
        let manager = CoopNetManager.GetInstance();
        if IsDefined(manager) {
            return manager.SetConfigFloat("voice_proximity_distance", distance);
        }
        return false;
    }

    // === VOICE EFFECTS ===

    // Enable/disable echo cancellation
    public func SetEchoCancellation(enabled: Bool) -> Bool {
        let manager = CoopNetManager.GetInstance();
        if IsDefined(manager) {
            return manager.SetConfigBool("voice_echo_cancellation", enabled);
        }
        return false;
    }

    // Enable/disable automatic gain control
    public func SetAutomaticGainControl(enabled: Bool) -> Bool {
        let manager = CoopNetManager.GetInstance();
        if IsDefined(manager) {
            return manager.SetConfigBool("voice_auto_gain", enabled);
        }
        return false;
    }

    // Set voice activation sensitivity (0.0 - 1.0)
    public func SetVoiceActivationSensitivity(sensitivity: Float) -> Bool {
        let manager = CoopNetManager.GetInstance();
        if IsDefined(manager) {
            return manager.SetConfigFloat("voice_activation_sensitivity", sensitivity);
        }
        return false;
    }

    // === DEBUGGING AND DIAGNOSTICS ===

    // Get detailed voice system status
    public func GetSystemStatus() -> String {
        let status = "Voice System Status:\n";
        status += "Enabled: " + ToString(this.IsEnabled()) + "\n";
        status += "Initialized: " + ToString(this.IsInitialized()) + "\n";
        status += "Quality: " + this.GetVoiceQualityName() + "\n";
        status += "Spatial Audio: " + ToString(this.IsSpatialAudioEnabled()) + "\n";
        status += "Noise Suppression: " + ToString(this.IsNoiseSuppressionEnabled()) + "\n";
        status += "Push-to-Talk: " + ToString(this.IsPushToTalkEnabled()) + "\n";
        status += "Voice Activation: " + ToString(this.IsVoiceActivationEnabled()) + "\n";

        let stats = this.GetVoiceStatistics();
        if IsDefined(stats) {
            status += "Active Channels: " + ToString(stats.activeChannels) + "\n";
            status += "Connected Players: " + ToString(stats.connectedPlayers) + "\n";
            status += "Audio Quality: " + ToString(stats.audioQuality) + "\n";
            status += "Latency: " + ToString(stats.latency) + "ms\n";
            status += "Packet Loss: " + ToString(stats.packetLoss * 100.0) + "%\n";
        }

        return status;
    }

    // Test voice system functionality
    public func RunVoiceTest() -> Bool {
        if !this.IsEnabled() {
            return false;
        }

        // Test basic functionality
        let originalQuality = this.GetVoiceQuality();
        let testPassed = true;

        // Test quality settings
        if !this.SetVoiceQuality(1) || this.GetVoiceQuality() != 1 {
            testPassed = false;
        }

        // Restore original settings
        this.SetVoiceQuality(originalQuality);

        return testPassed;
    }
}

// Voice statistics data structure
public class CoopNetVoiceStatistics extends IScriptable {
    public let activeChannels: Uint32;
    public let connectedPlayers: Uint32;
    public let audioQuality: Float;
    public let latency: Float;
    public let packetLoss: Float;
    public let packetsProcessed: Uint64;
    public let droppedPackets: Uint64;
}

// Extended GameInstance for voice system access
@addMethod(GameInstance)
public static func GetCoopNetVoiceSystem() -> ref<CoopNetVoiceSystem> {
    // This would be implemented in C++ to return the singleton instance
    return null;
}

// Global convenience functions for voice system

// Quick voice system initialization
public static func InitializeVoiceChat() -> Bool {
    let voiceSystem = CoopNetVoiceSystem.GetInstance();
    if IsDefined(voiceSystem) {
        return voiceSystem.InitializeWithDefaults();
    }
    return false;
}

// Check if voice chat is available
public static func IsVoiceChatAvailable() -> Bool {
    let voiceSystem = CoopNetVoiceSystem.GetInstance();
    if IsDefined(voiceSystem) {
        return voiceSystem.IsEnabled() && voiceSystem.IsInitialized();
    }
    return false;
}

// Quick mute/unmute toggle
public static func ToggleVoiceMute(playerId: Uint32) -> Bool {
    let voiceSystem = CoopNetVoiceSystem.GetInstance();
    if IsDefined(voiceSystem) {
        return voiceSystem.TogglePlayerMute(playerId);
    }
    return false;
}

// Join default voice channel
public static func JoinDefaultVoiceChannel() -> Bool {
    let voiceSystem = CoopNetVoiceSystem.GetInstance();
    if IsDefined(voiceSystem) {
        return voiceSystem.JoinGeneralChannel();
    }
    return false;
}

// Set voice quality quickly
public static func SetVoiceQuality(quality: String) -> Bool {
    let voiceSystem = CoopNetVoiceSystem.GetInstance();
    if IsDefined(voiceSystem) {
        return voiceSystem.SetVoiceQualityByName(quality);
    }
    return false;
}
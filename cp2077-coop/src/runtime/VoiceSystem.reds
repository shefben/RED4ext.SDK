// Comprehensive voice communication system for CP2077 multiplayer
// Handles voice chat, spatial audio, channel management, and voice effects

public enum VoiceQuality {
    Low = 0,
    Medium = 1,
    High = 2,
    Ultra = 3
}

public enum VoiceChannel {
    Global = 0,
    Team = 1,
    Proximity = 2,
    Direct = 3,
    Radio = 4,
    Whisper = 5
}

public enum TransmissionMode {
    PushToTalk = 0,
    VoiceActivated = 1,
    OpenMic = 2,
    Disabled = 3
}

public struct VoiceSettings {
    public var masterVolume: Float;
    public var inputVolume: Float;
    public var outputVolume: Float;
    public var voiceQuality: VoiceQuality;
    public var transmissionMode: TransmissionMode;
    public var pushToTalkKey: Uint32;
    public var voiceActivationThreshold: Float;
    public var proximityDistance: Float;
    public var noiseReduction: Bool;
    public var echoCancellation: Bool;
    public var automaticGainControl: Bool;
    public var spatialAudioEnabled: Bool;
}

public struct PlayerVoiceInfo {
    public var playerId: Uint32;
    public var playerName: String;
    public var isTransmitting: Bool;
    public var isMuted: Bool;
    public var isDeafened: Bool;
    public var currentVolume: Float;
    public var activeChannel: VoiceChannel;
    public var signalStrength: Float;
    public var lastActivityTime: Float;
    public var voiceLatency: Float;
}

public struct VoiceChannelInfo {
    public var channelType: VoiceChannel;
    public var channelName: String;
    public var participants: array<Uint32>;
    public var maxDistance: Float;
    public var requiresPermission: Bool;
    public var volumeMultiplier: Float;
    public var isActive: Bool;
}

public struct VoiceEffectConfig {
    public var radioEffect: Bool;
    public var underwaterEffect: Bool;
    public var reverbProfile: String;
    public var customEffects: array<String>;
    public var effectIntensity: Float;
}

public class VoiceSystem {
    // System state
    private static var isInitialized: Bool = false;
    private static var isVoiceEnabled: Bool = true;
    private static var currentSettings: VoiceSettings;
    private static var playerVoiceStates: array<PlayerVoiceInfo>;
    private static var availableChannels: array<VoiceChannelInfo>;
    private static var voiceEffects: VoiceEffectConfig;

    // Input devices and testing
    private static var availableInputDevices: array<String>;
    private static var availableOutputDevices: array<String>;
    private static var currentInputDevice: String;
    private static var currentOutputDevice: String;
    private static var isRunningVoiceTest: Bool = false;

    // Network and performance
    private static var voiceBandwidth: Uint32;
    private static var averageLatency: Float;
    private static var packetLoss: Uint32;

    // === SYSTEM INITIALIZATION ===

    public static func InitializeVoiceSystem() -> Bool {
        if isInitialized {
            LogChannel(n"VOICE", "Voice system already initialized");
            return true;
        }

        LogChannel(n"VOICE", "Initializing voice communication system...");

        // Initialize default settings
        VoiceSystem.LoadDefaultSettings();

        // Initialize native voice manager
        if !VoiceSystem_Initialize() {
            LogChannel(n"ERROR", "Failed to initialize native voice system");
            return false;
        }

        // Discover available audio devices
        VoiceSystem.RefreshAudioDevices();

        // Initialize voice channels
        VoiceSystem.InitializeVoiceChannels();

        // Setup voice effects
        VoiceSystem.InitializeVoiceEffects();

        isInitialized = true;
        LogChannel(n"VOICE", "Voice system initialized successfully");
        return true;
    }

    public static func ShutdownVoiceSystem() -> Void {
        if !isInitialized {
            return;
        }

        LogChannel(n"VOICE", "Shutting down voice system...");

        // Stop any active transmission
        VoiceSystem.StopTransmission();

        // Leave all channels
        for channel in availableChannels {
            VoiceSystem.LeaveChannel(channel.channelType);
        }

        // Shutdown native system
        VoiceSystem_Shutdown();

        isInitialized = false;
        LogChannel(n"VOICE", "Voice system shut down");
    }

    private static func LoadDefaultSettings() -> Void {
        currentSettings.masterVolume = 0.8;
        currentSettings.inputVolume = 1.0;
        currentSettings.outputVolume = 1.0;
        currentSettings.voiceQuality = VoiceQuality.Medium;
        currentSettings.transmissionMode = TransmissionMode.PushToTalk;
        currentSettings.pushToTalkKey = 84u; // 'T' key
        currentSettings.voiceActivationThreshold = 0.1;
        currentSettings.proximityDistance = 50.0;
        currentSettings.noiseReduction = true;
        currentSettings.echoCancellation = true;
        currentSettings.automaticGainControl = true;
        currentSettings.spatialAudioEnabled = true;
    }

    private static func InitializeVoiceChannels() -> Void {
        ArrayClear(availableChannels);

        // Global channel
        VoiceSystem.CreateChannel(VoiceChannel.Global, "Global Chat", 0.0, false, 1.0);

        // Team channel
        VoiceSystem.CreateChannel(VoiceChannel.Team, "Team Chat", 0.0, false, 1.0);

        // Proximity channel
        VoiceSystem.CreateChannel(VoiceChannel.Proximity, "Proximity Voice", currentSettings.proximityDistance, false, 1.0);

        // Radio channel
        VoiceSystem.CreateChannel(VoiceChannel.Radio, "Radio Communication", 0.0, false, 0.8);

        // Whisper channel
        VoiceSystem.CreateChannel(VoiceChannel.Whisper, "Whisper", 10.0, false, 0.6);

        LogChannel(n"VOICE", "Initialized " + ToString(ArraySize(availableChannels)) + " voice channels");
    }

    private static func CreateChannel(channelType: VoiceChannel, name: String, maxDistance: Float, requiresPermission: Bool, volumeMultiplier: Float) -> Void {
        let channelInfo: VoiceChannelInfo;
        channelInfo.channelType = channelType;
        channelInfo.channelName = name;
        channelInfo.maxDistance = maxDistance;
        channelInfo.requiresPermission = requiresPermission;
        channelInfo.volumeMultiplier = volumeMultiplier;
        channelInfo.isActive = true;

        ArrayPush(availableChannels, channelInfo);
    }

    private static func InitializeVoiceEffects() -> Void {
        voiceEffects.radioEffect = false;
        voiceEffects.underwaterEffect = false;
        voiceEffects.reverbProfile = "none";
        voiceEffects.effectIntensity = 0.5;
    }

    // === DEVICE MANAGEMENT ===

    public static func RefreshAudioDevices() -> Void {
        // Get available input devices
        ArrayClear(availableInputDevices);
        let inputDevices = VoiceSystem_GetInputDevices();
        for device in inputDevices {
            ArrayPush(availableInputDevices, device);
        }

        // Get available output devices
        ArrayClear(availableOutputDevices);
        let outputDevices = VoiceSystem_GetOutputDevices();
        for device in outputDevices {
            ArrayPush(availableOutputDevices, device);
        }

        LogChannel(n"VOICE", "Found " + ToString(ArraySize(availableInputDevices)) + " input devices and " +
                   ToString(ArraySize(availableOutputDevices)) + " output devices");
    }

    public static func SetInputDevice(deviceName: String) -> Bool {
        if !isInitialized {
            return false;
        }

        if !VoiceSystem_SetInputDevice(deviceName) {
            LogChannel(n"ERROR", "Failed to set input device: " + deviceName);
            return false;
        }

        currentInputDevice = deviceName;
        LogChannel(n"VOICE", "Set input device to: " + deviceName);
        return true;
    }

    public static func SetOutputDevice(deviceName: String) -> Bool {
        if !isInitialized {
            return false;
        }

        if !VoiceSystem_SetOutputDevice(deviceName) {
            LogChannel(n"ERROR", "Failed to set output device: " + deviceName);
            return false;
        }

        currentOutputDevice = deviceName;
        LogChannel(n"VOICE", "Set output device to: " + deviceName);
        return true;
    }

    public static func GetAvailableInputDevices() -> array<String> {
        return availableInputDevices;
    }

    public static func GetAvailableOutputDevices() -> array<String> {
        return availableOutputDevices;
    }

    // === VOICE SETTINGS ===

    public static func SetMasterVolume(volume: Float) -> Void {
        currentSettings.masterVolume = ClampF(volume, 0.0, 2.0);
        VoiceSystem_SetMasterVolume(currentSettings.masterVolume);
        LogChannel(n"VOICE", "Set master volume to " + FloatToString(currentSettings.masterVolume));
    }

    public static func SetInputVolume(volume: Float) -> Void {
        currentSettings.inputVolume = ClampF(volume, 0.0, 2.0);
        VoiceSystem_SetInputVolume(currentSettings.inputVolume);
        LogChannel(n"VOICE", "Set input volume to " + FloatToString(currentSettings.inputVolume));
    }

    public static func SetOutputVolume(volume: Float) -> Void {
        currentSettings.outputVolume = ClampF(volume, 0.0, 2.0);
        VoiceSystem_SetOutputVolume(currentSettings.outputVolume);
        LogChannel(n"VOICE", "Set output volume to " + FloatToString(currentSettings.outputVolume));
    }

    public static func SetVoiceQuality(quality: VoiceQuality) -> Void {
        currentSettings.voiceQuality = quality;
        VoiceSystem_SetVoiceQuality(EnumInt(quality));
        LogChannel(n"VOICE", "Set voice quality to " + ToString(EnumInt(quality)));
    }

    public static func SetTransmissionMode(mode: TransmissionMode) -> Void {
        currentSettings.transmissionMode = mode;
        VoiceSystem_SetTransmissionMode(EnumInt(mode));

        let modeNames: array<String> = ["Push-to-Talk", "Voice Activated", "Open Mic", "Disabled"];
        if EnumInt(mode) < ArraySize(modeNames) {
            LogChannel(n"VOICE", "Set transmission mode to " + modeNames[EnumInt(mode)]);
        }
    }

    public static func SetPushToTalkKey(keyCode: Uint32) -> Void {
        currentSettings.pushToTalkKey = keyCode;
        VoiceSystem_SetPushToTalkKey(keyCode);
        LogChannel(n"VOICE", "Set push-to-talk key to " + ToString(keyCode));
    }

    public static func SetVoiceActivationThreshold(threshold: Float) -> Void {
        currentSettings.voiceActivationThreshold = ClampF(threshold, 0.0, 1.0);
        VoiceSystem_SetVoiceActivationThreshold(currentSettings.voiceActivationThreshold);
        LogChannel(n"VOICE", "Set voice activation threshold to " + FloatToString(currentSettings.voiceActivationThreshold));
    }

    public static func SetProximityDistance(distance: Float) -> Void {
        currentSettings.proximityDistance = MaxF(distance, 1.0);
        VoiceSystem_SetProximityDistance(currentSettings.proximityDistance);
        LogChannel(n"VOICE", "Set proximity distance to " + FloatToString(currentSettings.proximityDistance));
    }

    // === AUDIO PROCESSING SETTINGS ===

    public static func SetNoiseReduction(enabled: Bool) -> Void {
        currentSettings.noiseReduction = enabled;
        VoiceSystem_SetNoiseReduction(enabled);
        LogChannel(n"VOICE", "Noise reduction " + (enabled ? "enabled" : "disabled"));
    }

    public static func SetEchoCancellation(enabled: Bool) -> Void {
        currentSettings.echoCancellation = enabled;
        VoiceSystem_SetEchoCancellation(enabled);
        LogChannel(n"VOICE", "Echo cancellation " + (enabled ? "enabled" : "disabled"));
    }

    public static func SetAutomaticGainControl(enabled: Bool) -> Void {
        currentSettings.automaticGainControl = enabled;
        VoiceSystem_SetAutomaticGainControl(enabled);
        LogChannel(n"VOICE", "Automatic gain control " + (enabled ? "enabled" : "disabled"));
    }

    public static func SetSpatialAudio(enabled: Bool) -> Void {
        currentSettings.spatialAudioEnabled = enabled;
        VoiceSystem_SetSpatialAudio(enabled);
        LogChannel(n"VOICE", "Spatial audio " + (enabled ? "enabled" : "disabled"));
    }

    // === TRANSMISSION CONTROL ===

    public static func StartTransmission() -> Bool {
        if !isInitialized || !isVoiceEnabled {
            return false;
        }

        if VoiceSystem_StartTransmission() {
            LogChannel(n"VOICE", "Voice transmission started");
            return true;
        }

        LogChannel(n"ERROR", "Failed to start voice transmission");
        return false;
    }

    public static func StopTransmission() -> Void {
        VoiceSystem_StopTransmission();
        LogChannel(n"VOICE", "Voice transmission stopped");
    }

    public static func IsTransmitting() -> Bool {
        return VoiceSystem_IsTransmitting();
    }

    // === CHANNEL MANAGEMENT ===

    public static func JoinChannel(channel: VoiceChannel) -> Bool {
        if !isInitialized {
            return false;
        }

        let channelInfo = VoiceSystem.GetChannelInfo(channel);
        if Equals(channelInfo.channelName, "") {
            LogChannel(n"ERROR", "Channel not found: " + ToString(EnumInt(channel)));
            return false;
        }

        if VoiceSystem_JoinChannel(EnumInt(channel)) {
            LogChannel(n"VOICE", "Joined channel: " + channelInfo.channelName);
            return true;
        }

        LogChannel(n"ERROR", "Failed to join channel: " + channelInfo.channelName);
        return false;
    }

    public static func LeaveChannel(channel: VoiceChannel) -> Bool {
        if !isInitialized {
            return false;
        }

        let channelInfo = VoiceSystem.GetChannelInfo(channel);
        if Equals(channelInfo.channelName, "") {
            return false;
        }

        if VoiceSystem_LeaveChannel(EnumInt(channel)) {
            LogChannel(n"VOICE", "Left channel: " + channelInfo.channelName);
            return true;
        }

        return false;
    }

    public static func SwitchToChannel(channel: VoiceChannel) -> Bool {
        // Leave current channels and join the new one
        for channelInfo in availableChannels {
            VoiceSystem.LeaveChannel(channelInfo.channelType);
        }

        return VoiceSystem.JoinChannel(channel);
    }

    private static func GetChannelInfo(channel: VoiceChannel) -> VoiceChannelInfo {
        for channelInfo in availableChannels {
            if channelInfo.channelType == channel {
                return channelInfo;
            }
        }

        let emptyInfo: VoiceChannelInfo;
        return emptyInfo;
    }

    public static func GetAvailableChannels() -> array<VoiceChannelInfo> {
        return availableChannels;
    }

    // === PLAYER MANAGEMENT ===

    public static func MutePlayer(playerId: Uint32) -> Void {
        VoiceSystem_MutePlayer(playerId);
        VoiceSystem.UpdatePlayerVoiceState(playerId, "muted", true);
        LogChannel(n"VOICE", "Muted player " + ToString(playerId));
    }

    public static func UnmutePlayer(playerId: Uint32) -> Void {
        VoiceSystem_UnmutePlayer(playerId);
        VoiceSystem.UpdatePlayerVoiceState(playerId, "muted", false);
        LogChannel(n"VOICE", "Unmuted player " + ToString(playerId));
    }

    public static func SetPlayerVolume(playerId: Uint32, volume: Float) -> Void {
        let clampedVolume = ClampF(volume, 0.0, 2.0);
        VoiceSystem_SetPlayerVolume(playerId, clampedVolume);
        VoiceSystem.UpdatePlayerVoiceState(playerId, "volume", clampedVolume);
        LogChannel(n"VOICE", "Set volume for player " + ToString(playerId) + " to " + FloatToString(clampedVolume));
    }

    public static func DeafenSelf(deafened: Bool) -> Void {
        VoiceSystem_SetDeafened(deafened);
        LogChannel(n"VOICE", "Self deafen " + (deafened ? "enabled" : "disabled"));
    }

    private static func UpdatePlayerVoiceState(playerId: Uint32, property: String, value: Variant) -> Void {
        let playerIndex = VoiceSystem.FindPlayerIndex(playerId);
        if playerIndex >= 0 {
            let playerInfo = playerVoiceStates[playerIndex];

            if Equals(property, "muted") {
                playerInfo.isMuted = FromVariant<Bool>(value);
            } else if Equals(property, "deafened") {
                playerInfo.isDeafened = FromVariant<Bool>(value);
            } else if Equals(property, "volume") {
                playerInfo.currentVolume = FromVariant<Float>(value);
            } else if Equals(property, "transmitting") {
                playerInfo.isTransmitting = FromVariant<Bool>(value);
            }

            playerVoiceStates[playerIndex] = playerInfo;
        }
    }

    private static func FindPlayerIndex(playerId: Uint32) -> Int32 {
        for i in Range(ArraySize(playerVoiceStates)) {
            if playerVoiceStates[i].playerId == playerId {
                return i;
            }
        }
        return -1;
    }

    public static func GetPlayerVoiceStates() -> array<PlayerVoiceInfo> {
        return playerVoiceStates;
    }

    public static func GetTalkingPlayers() -> array<Uint32> {
        let talkingPlayers: array<Uint32>;
        for playerInfo in playerVoiceStates {
            if playerInfo.isTransmitting {
                ArrayPush(talkingPlayers, playerInfo.playerId);
            }
        }
        return talkingPlayers;
    }

    // === VOICE EFFECTS ===

    public static func SetRadioEffect(enabled: Bool) -> Void {
        voiceEffects.radioEffect = enabled;
        VoiceSystem_SetRadioEffect(enabled);
        LogChannel(n"VOICE", "Radio effect " + (enabled ? "enabled" : "disabled"));
    }

    public static func SetUnderwaterEffect(enabled: Bool) -> Void {
        voiceEffects.underwaterEffect = enabled;
        VoiceSystem_SetUnderwaterEffect(enabled);
        LogChannel(n"VOICE", "Underwater effect " + (enabled ? "enabled" : "disabled"));
    }

    public static func SetReverbProfile(profile: String) -> Void {
        voiceEffects.reverbProfile = profile;
        VoiceSystem_SetReverbProfile(profile);
        LogChannel(n"VOICE", "Set reverb profile to: " + profile);
    }

    public static func SetEffectIntensity(intensity: Float) -> Void {
        voiceEffects.effectIntensity = ClampF(intensity, 0.0, 1.0);
        VoiceSystem_SetEffectIntensity(voiceEffects.effectIntensity);
        LogChannel(n"VOICE", "Set effect intensity to " + FloatToString(voiceEffects.effectIntensity));
    }

    // === SPATIAL AUDIO ===

    public static func UpdatePlayerPosition(playerId: Uint32, position: Vector3) -> Void {
        if currentSettings.spatialAudioEnabled {
            VoiceSystem_UpdatePlayerPosition(playerId, position.X, position.Y, position.Z);
        }
    }

    public static func UpdateListenerPosition(position: Vector3, orientation: EulerAngles) -> Void {
        if currentSettings.spatialAudioEnabled {
            VoiceSystem_UpdateListenerPosition(position.X, position.Y, position.Z,
                                             orientation.Yaw, orientation.Pitch, orientation.Roll);
        }
    }

    // === DIAGNOSTICS AND TESTING ===

    public static func RunVoiceTest() -> Void {
        if isRunningVoiceTest {
            LogChannel(n"WARNING", "Voice test already in progress");
            return;
        }

        isRunningVoiceTest = true;
        LogChannel(n"VOICE", "Starting voice test...");

        VoiceSystem_RunVoiceTest();

        // The test will complete asynchronously
        GameInstance.GetDelaySystem(GetGame()).DelayCallback(null, n"OnVoiceTestCompleted", 5.0);
    }

    public static func OnVoiceTestCompleted() -> Void {
        isRunningVoiceTest = false;
        let testResults = VoiceSystem_GetVoiceTestResults();
        LogChannel(n"VOICE", "Voice test completed: " + testResults);
    }

    public static func GetInputLevel() -> Float {
        return VoiceSystem_GetInputLevel();
    }

    public static func GetOutputLevel() -> Float {
        return VoiceSystem_GetOutputLevel();
    }

    // === NETWORK DIAGNOSTICS ===

    public static func GetVoiceNetworkStats() -> String {
        let bandwidth = VoiceSystem_GetVoiceBandwidth();
        let latency = VoiceSystem_GetVoiceLatency();
        let packetLoss = VoiceSystem_GetPacketLoss();

        voiceBandwidth = bandwidth;
        averageLatency = latency;
        packetLoss = packetLoss;

        return "Bandwidth: " + ToString(bandwidth) + " kbps, Latency: " + FloatToString(latency) +
               "ms, Packet Loss: " + ToString(packetLoss) + "%";
    }

    public static func GetCurrentSettings() -> VoiceSettings {
        return currentSettings;
    }

    public static func SaveVoiceSettings() -> Void {
        // In a full implementation, this would save settings to a config file
        LogChannel(n"VOICE", "Voice settings saved");
    }

    public static func LoadVoiceSettings() -> Void {
        // In a full implementation, this would load settings from a config file
        LogChannel(n"VOICE", "Voice settings loaded");
    }

    // === EVENT HANDLERS ===

    public static func OnPlayerJoined(playerId: Uint32, playerName: String) -> Void {
        let playerInfo: PlayerVoiceInfo;
        playerInfo.playerId = playerId;
        playerInfo.playerName = playerName;
        playerInfo.isTransmitting = false;
        playerInfo.isMuted = false;
        playerInfo.isDeafened = false;
        playerInfo.currentVolume = 1.0;
        playerInfo.activeChannel = VoiceChannel.Global;
        playerInfo.signalStrength = 1.0;
        playerInfo.lastActivityTime = EngineTime.ToFloat(GameInstance.GetSimTime());
        playerInfo.voiceLatency = 0.0;

        ArrayPush(playerVoiceStates, playerInfo);
        LogChannel(n"VOICE", "Player joined voice system: " + playerName);
    }

    public static func OnPlayerLeft(playerId: Uint32) -> Void {
        let playerIndex = VoiceSystem.FindPlayerIndex(playerId);
        if playerIndex >= 0 {
            let playerInfo = playerVoiceStates[playerIndex];
            ArrayRemove(playerVoiceStates, playerInfo);
            LogChannel(n"VOICE", "Player left voice system: " + playerInfo.playerName);
        }
    }

    public static func OnPlayerStartedTalking(playerId: Uint32) -> Void {
        VoiceSystem.UpdatePlayerVoiceState(playerId, "transmitting", true);
        LogChannel(n"DEBUG", "Player " + ToString(playerId) + " started talking");
    }

    public static func OnPlayerStoppedTalking(playerId: Uint32) -> Void {
        VoiceSystem.UpdatePlayerVoiceState(playerId, "transmitting", false);
        LogChannel(n"DEBUG", "Player " + ToString(playerId) + " stopped talking");
    }
}

// === NATIVE FUNCTION DECLARATIONS ===

// System management
private static native func VoiceSystem_Initialize() -> Bool;
private static native func VoiceSystem_Shutdown() -> Void;

// Device management
private static native func VoiceSystem_GetInputDevices() -> array<String>;
private static native func VoiceSystem_GetOutputDevices() -> array<String>;
private static native func VoiceSystem_SetInputDevice(deviceName: String) -> Bool;
private static native func VoiceSystem_SetOutputDevice(deviceName: String) -> Bool;

// Settings
private static native func VoiceSystem_SetMasterVolume(volume: Float) -> Void;
private static native func VoiceSystem_SetInputVolume(volume: Float) -> Void;
private static native func VoiceSystem_SetOutputVolume(volume: Float) -> Void;
private static native func VoiceSystem_SetVoiceQuality(quality: Int32) -> Void;
private static native func VoiceSystem_SetTransmissionMode(mode: Int32) -> Void;
private static native func VoiceSystem_SetPushToTalkKey(keyCode: Uint32) -> Void;
private static native func VoiceSystem_SetVoiceActivationThreshold(threshold: Float) -> Void;
private static native func VoiceSystem_SetProximityDistance(distance: Float) -> Void;

// Audio processing
private static native func VoiceSystem_SetNoiseReduction(enabled: Bool) -> Void;
private static native func VoiceSystem_SetEchoCancellation(enabled: Bool) -> Void;
private static native func VoiceSystem_SetAutomaticGainControl(enabled: Bool) -> Void;
private static native func VoiceSystem_SetSpatialAudio(enabled: Bool) -> Void;

// Transmission
private static native func VoiceSystem_StartTransmission() -> Bool;
private static native func VoiceSystem_StopTransmission() -> Void;
private static native func VoiceSystem_IsTransmitting() -> Bool;

// Channel management
private static native func VoiceSystem_JoinChannel(channel: Int32) -> Bool;
private static native func VoiceSystem_LeaveChannel(channel: Int32) -> Bool;

// Player management
private static native func VoiceSystem_MutePlayer(playerId: Uint32) -> Void;
private static native func VoiceSystem_UnmutePlayer(playerId: Uint32) -> Void;
private static native func VoiceSystem_SetPlayerVolume(playerId: Uint32, volume: Float) -> Void;
private static native func VoiceSystem_SetDeafened(deafened: Bool) -> Void;

// Voice effects
private static native func VoiceSystem_SetRadioEffect(enabled: Bool) -> Void;
private static native func VoiceSystem_SetUnderwaterEffect(enabled: Bool) -> Void;
private static native func VoiceSystem_SetReverbProfile(profile: String) -> Void;
private static native func VoiceSystem_SetEffectIntensity(intensity: Float) -> Void;

// Spatial audio
private static native func VoiceSystem_UpdatePlayerPosition(playerId: Uint32, x: Float, y: Float, z: Float) -> Void;
private static native func VoiceSystem_UpdateListenerPosition(x: Float, y: Float, z: Float, yaw: Float, pitch: Float, roll: Float) -> Void;

// Diagnostics
private static native func VoiceSystem_RunVoiceTest() -> Void;
private static native func VoiceSystem_GetVoiceTestResults() -> String;
private static native func VoiceSystem_GetInputLevel() -> Float;
private static native func VoiceSystem_GetOutputLevel() -> Float;

// Network statistics
private static native func VoiceSystem_GetVoiceBandwidth() -> Uint32;
private static native func VoiceSystem_GetVoiceLatency() -> Float;
private static native func VoiceSystem_GetPacketLoss() -> Uint32;
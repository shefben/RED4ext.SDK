// ============================================================================
// Game Mode Manager
// ============================================================================
// Manages different cooperative game modes and their settings

public class GameModeManager {
    private static var currentMode: GameMode = GameMode.Cooperative;
    private static var friendlyFire: Bool = false;
    private static var sharedLoot: Bool = true;
    private static var difficulty: Float = 1.0;
    private static var maxPlayers: Uint32 = 4u;
    
    public enum GameMode {
        Cooperative = 0,
        Competitive = 1,
        PvP = 2,
        Campaign = 3
    }
    
    public static func Initialize() -> Void {
        LogChannel(n"GAMEMODE", "GameModeManager initialized");
        
        // Load default settings
        friendlyFire = false;
        sharedLoot = true;
        difficulty = 1.0;
        maxPlayers = 4u;
        currentMode = GameMode.Cooperative;
    }
    
    public static func SetGameMode(mode: GameMode) -> Void {
        LogChannel(n"GAMEMODE", "Setting game mode to: " + IntToString(Cast<Int32>(mode)));
        currentMode = mode;
        
        // Apply mode-specific settings
        switch mode {
            case GameMode.Cooperative:
                SetFriendlyFire(false);
                SetSharedLoot(true);
                break;
            case GameMode.Competitive:
                SetFriendlyFire(false);
                SetSharedLoot(false);
                break;
            case GameMode.PvP:
                SetFriendlyFire(true);
                SetSharedLoot(false);
                break;
            case GameMode.Campaign:
                SetFriendlyFire(false);
                SetSharedLoot(true);
                break;
        }
    }
    
    public static func GetGameMode() -> GameMode {
        return currentMode;
    }
    
    public static func SetFriendlyFire(enabled: Bool) -> Void {
        friendlyFire = enabled;
        LogChannel(n"GAMEMODE", "Friendly fire: " + BoolToString(enabled));
        
        // Notify native layer about friendly fire setting
        Net_BroadcastGameModeSetting("friendly_fire", BoolToString(enabled));
    }
    
    public static func GetFriendlyFire() -> Bool {
        return friendlyFire;
    }
    
    public static func SetSharedLoot(enabled: Bool) -> Void {
        sharedLoot = enabled;
        LogChannel(n"GAMEMODE", "Shared loot: " + BoolToString(enabled));
        
        // Notify native layer about loot sharing setting
        Net_BroadcastGameModeSetting("shared_loot", BoolToString(enabled));
    }
    
    public static func GetSharedLoot() -> Bool {
        return sharedLoot;
    }
    
    public static func SetDifficulty(diff: Float) -> Void {
        difficulty = diff;
        LogChannel(n"GAMEMODE", "Difficulty set to: " + FloatToString(diff));
        
        // Notify native layer about difficulty setting
        Net_BroadcastGameModeSetting("difficulty", FloatToString(diff));
    }
    
    public static func GetDifficulty() -> Float {
        return difficulty;
    }
    
    public static func SetMaxPlayers(max: Uint32) -> Void {
        maxPlayers = max;
        LogChannel(n"GAMEMODE", "Max players set to: " + IntToString(max));
    }
    
    public static func GetMaxPlayers() -> Uint32 {
        return maxPlayers;
    }
    
    public static func GetGameModeString() -> String {
        switch currentMode {
            case GameMode.Cooperative:
                return "Cooperative";
            case GameMode.Competitive:
                return "Competitive";
            case GameMode.PvP:
                return "Player vs Player";
            case GameMode.Campaign:
                return "Campaign Co-op";
            default:
                return "Unknown";
        }
    }
    
    public static func CanDamagePlayer(attackerId: Uint32, victimId: Uint32) -> Bool {
        // Always allow self-damage (for environmental hazards, etc.)
        if attackerId == victimId {
            return true;
        }
        
        // Check game mode and friendly fire settings
        switch currentMode {
            case GameMode.PvP:
                return true; // Always allow player damage in PvP
            case GameMode.Cooperative:
            case GameMode.Campaign:
            case GameMode.Competitive:
                return friendlyFire; // Based on friendly fire setting
            default:
                return false;
        }
    }
    
    public static func ShouldShareLoot(itemId: Uint64, pickupPlayerId: Uint32) -> Bool {
        // In shared loot modes, items go to a shared pool
        return sharedLoot && (currentMode == GameMode.Cooperative || currentMode == GameMode.Campaign);
    }
    
    public static func GetScaledDifficulty() -> Float {
        // Scale difficulty based on number of connected players
        let playerCount = Cast<Float>(SessionState_GetActivePlayerCount());
        if playerCount <= 1.0 {
            return difficulty;
        }
        
        // Increase difficulty slightly with more players
        return difficulty * (1.0 + ((playerCount - 1.0) * 0.15));
    }
    
    public static func OnPlayerJoined(peerId: Uint32) -> Void {
        LogChannel(n"GAMEMODE", "Player joined: " + IntToString(peerId));
        
        // Send current game mode settings to new player
        SendGameModeSettingsTo(peerId);
    }
    
    public static func OnPlayerLeft(peerId: Uint32) -> Void {
        LogChannel(n"GAMEMODE", "Player left: " + IntToString(peerId));
        
        // Handle any mode-specific cleanup
    }
    
    private static func SendGameModeSettingsTo(peerId: Uint32) -> Void {
        // Send current settings to specific player
        // This would be implemented with actual networking calls
        LogChannel(n"GAMEMODE", "Sending game mode settings to peer: " + IntToString(peerId));
    }
}

// Native function declaration for game mode networking
private static native func Net_BroadcastGameModeSetting(setting: String, value: String) -> Void;
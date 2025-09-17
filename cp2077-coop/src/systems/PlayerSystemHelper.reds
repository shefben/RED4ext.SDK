// PlayerSystemHelper.reds - Helper functions for player data and identification
// Provides utilities for getting local player information and managing player data

import Base.*
import String.*
import gameGameInstance
import gamePlayerSystem

public class PlayerSystemHelper {
    
    public static func GetLocalPlayerID() -> String {
        // Get the local player's unique identifier
        let gameInstance = GetGameInstance();
        let playerSystem = gameInstance.GetPlayerSystem();
        let player = playerSystem.GetLocalPlayerMainGameObject() as PlayerPuppet;
        
        if IsDefined(player) {
            // Generate unique player ID based on game data
            let playerName = GetPlayerDisplayName(player);
            let sessionId = GetPlayerSessionId(player);
            
            // Combine name and session for unique ID
            let playerId = playerName + "_" + sessionId;
            LogChannel(n"PLAYER_HELPER", s"Generated player ID: " + playerId);
            return playerId;
        } else {
            LogChannel(n"PLAYER_HELPER", s"Failed to get local player object");
            return "UNKNOWN_PLAYER_" + ToString(GetCurrentTimeMs());
        }
    }
    
    public static func GetLocalPlayerData() -> String {
        // Get comprehensive player data for session management
        let gameInstance = GetGameInstance();
        let playerSystem = gameInstance.GetPlayerSystem();
        let player = playerSystem.GetLocalPlayerMainGameObject() as PlayerPuppet;
        
        if IsDefined(player) {
            let playerData = BuildPlayerDataString(player);
            LogChannel(n"PLAYER_HELPER", s"Generated player data (" + ToString(StrLen(playerData)) + " chars)");
            return playerData;
        } else {
            LogChannel(n"PLAYER_HELPER", s"Failed to get local player data");
            return "{\"error\":\"no_player_data\"}";
        }
    }
    
    public static func GetPlayerDisplayName(player: ref<PlayerPuppet>) -> String {
        // Get the display name for the player
        if IsDefined(player) {
            // Try to get character name from save data
            let characterName = GetCharacterNameFromSave(player);
            if !Equals(characterName, "") {
                return characterName;
            }
            
            // Fallback to default player name
            return "V_" + ToString(GetCurrentTimeMs() % 10000);
        }
        
        return "UNKNOWN_PLAYER";
    }
    
    public static func GetPlayerSessionId(player: ref<PlayerPuppet>) -> String {
        // Generate session-specific ID for the player
        if IsDefined(player) {
            // Use game instance ID and timestamp for unique session ID
            let gameInstanceId = GetGameInstanceId();
            let timestamp = GetCurrentTimeMs();
            return gameInstanceId + "_" + ToString(timestamp % 100000);
        }
        
        return "NO_SESSION";
    }
    
    private static func BuildPlayerDataString(player: ref<PlayerPuppet>) -> String {
        // Build JSON-like string with player data
        let playerName = GetPlayerDisplayName(player);
        let sessionId = GetPlayerSessionId(player);
        let level = GetPlayerLevel(player);
        let streetCred = GetPlayerStreetCred(player);
        
        // Simple JSON construction (in real implementation, use proper JSON library)
        let playerData = "{";
        playerData += "\"name\":\"" + playerName + "\",";
        playerData += "\"sessionId\":\"" + sessionId + "\",";
        playerData += "\"level\":" + ToString(level) + ",";
        playerData += "\"streetCred\":" + ToString(streetCred) + ",";
        playerData += "\"timestamp\":" + ToString(GetCurrentTimeMs());
        playerData += "}";
        
        return playerData;
    }
    
    private static func GetCharacterNameFromSave(player: ref<PlayerPuppet>) -> String {
        // Attempt to get character name from save game data
        // This would need game-specific implementation
        return "V"; // Default name for now
    }
    
    private static func GetGameInstanceId() -> String {
        // Get unique identifier for this game instance
        return "game_" + ToString(GetCurrentTimeMs() % 1000000);
    }
    
    private static func GetPlayerLevel(player: ref<PlayerPuppet>) -> Int32 {
        // Get player level from character data
        if IsDefined(player) {
            let playerDevData = player.GetDevSystem();
            if IsDefined(playerDevData) {
                // Get level from development system
                return 1; // Placeholder - would need actual level system integration
            }
        }
        return 1;
    }
    
    private static func GetPlayerStreetCred(player: ref<PlayerPuppet>) -> Int32 {
        // Get player street cred from character data
        if IsDefined(player) {
            // Get street cred from reputation system
            return 0; // Placeholder - would need actual street cred system integration
        }
        return 0;
    }
}
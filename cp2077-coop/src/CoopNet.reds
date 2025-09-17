// ============================================================================
// CoopNet - Wrapper class for networking native functions
// ============================================================================
// This class provides a convenient interface to all networking functions
// used throughout the cp2077-coop mod.

public class CoopNet {
    // ============================================================================
    // CORE NETWORKING
    // ============================================================================
    
    public static func IsConnected() -> Bool {
        return Net_IsConnected();
    }
    
    public static func IsAuthoritative() -> Bool {
        return Net_IsAuthoritative();
    }
    
    public static func GetPeerId() -> Uint32 {
        return Net_GetPeerId();
    }
    
    public static func GetPeerCount() -> Uint32 {
        return Net_GetPeerCount();
    }
    
    // ============================================================================
    // HASH FUNCTIONS
    // ============================================================================
    
    public static func Fnv1a32(input: String) -> Uint32 {
        return Fnv1a32(input);
    }
    
    public static func Fnv1a64(input: String) -> Uint64 {
        return Fnv1a64(input);
    }
    
    // ============================================================================
    // BROADCASTING FUNCTIONS
    // ============================================================================
    
    public static func Net_BroadcastCineStart(sceneId: Uint32, startTime: Uint32, phaseId: Uint32, solo: Bool) -> Void {
        Net_BroadcastCineStart(sceneId, startTime, phaseId, solo);
    }
    
    public static func BroadcastRuleChange(friendlyFire: Bool) -> Void {
        Net_BroadcastRuleChange(friendlyFire);
    }
    
    public static func AddStats(peerId: Uint32, frags: Uint16, deaths: Uint16, score: Uint32, timeMs: Uint32, ping: Uint16) -> Void {
        Net_AddStats(peerId, frags, deaths, score, timeMs, ping);
    }
    
    public static func BroadcastMatchOver(winnerId: Uint32) -> Void {
        Net_BroadcastMatchOver(winnerId);
    }
    
    public static func Net_BroadcastGigSpawn(questId: Uint32, seed: Uint32) -> Void {
        Net_BroadcastGigSpawn(questId, seed);
    }
    
    public static func Net_BroadcastLootRoll(containerId: Uint32, seed: Uint32, items: array<Uint64>) -> Void {
        Net_BroadcastLootRoll(containerId, seed, items);
    }
    
    // ============================================================================
    // NPC MANAGEMENT
    // ============================================================================
    
    public static func NpcController_ServerTick(deltaTime: Float) -> Void {
        Net_NpcController_ServerTick(deltaTime);
    }
    
    public static func SpawnPhaseNpc() -> Void {
        Net_SpawnPhaseNpc();
    }
    
    // ============================================================================
    // GAME CLOCK WRAPPER
    // ============================================================================
    
    public class GameClock {
        public static func GetTime() -> Uint32 {
            return GameClock_GetTime();
        }
        
        public static func GetTickMs() -> Float {
            return GameClock_GetTickMs();
        }
        
        public static func GetCurrentTick() -> Uint64 {
            return GameClock_GetCurrentTick();
        }
    }
}

// ============================================================================
// GLOBAL WRAPPER FUNCTIONS (for backwards compatibility)
// ============================================================================

public static func Net_IsAuthoritative() -> Bool {
    return CoopNet.IsAuthoritative();
}

public static func Net_GetPeerId() -> Uint32 {
    return CoopNet.GetPeerId();
}

public static func Net_BroadcastHeat(level: Uint8) -> Void {
    Net_BroadcastHeat(level);
}

public static func Net_SendElevatorCall(elevatorId: Uint32, floor: Uint8) -> Void {
    Net_SendElevatorCall(elevatorId, floor);
}

public static func Net_SendTeleportAck(elevatorId: Uint32) -> Void {
    Net_SendTeleportAck(elevatorId);
}
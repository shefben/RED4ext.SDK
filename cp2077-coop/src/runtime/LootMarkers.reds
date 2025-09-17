// ============================================================================
// LootMarkers - UI notification system for loot events
// ============================================================================

public class LootMarkers {
    private static var messages: array<String>;
    private static var timestamps: array<Float>;
    private static let maxMessages: Int32 = 10;
    private static let messageLifespan: Float = 5.0;
    
    public static func Push(message: String) -> Void {
        // Add new message
        messages.PushBack(message);
        timestamps.PushBack(GameInstance.GetEngineTime(GetGame()));
        
        // Remove old messages to maintain size limit
        while ArraySize(messages) > maxMessages {
            messages.RemoveFirst();
            timestamps.RemoveFirst();
        };
        
        LogChannel(n"loot", message);
    }
    
    public static func Clear() -> Void {
        messages.Clear();
        timestamps.Clear();
    }
    
    public static func GetActiveMessages() -> array<String> {
        CleanupExpiredMessages();
        return messages;
    }
    
    private static func CleanupExpiredMessages() -> Void {
        let currentTime = GameInstance.GetEngineTime(GetGame());
        var toRemove: array<Int32>;
        
        var i: Int32 = 0;
        while i < ArraySize(timestamps) {
            if currentTime - timestamps[i] > messageLifespan {
                toRemove.PushBack(i);
            };
            i += 1;
        };
        
        // Remove expired messages in reverse order to maintain indices
        i = ArraySize(toRemove) - 1;
        while i >= 0 {
            let idx = toRemove[i];
            if idx < ArraySize(messages) {
                messages.RemoveByIndex(idx);
            };
            if idx < ArraySize(timestamps) {
                timestamps.RemoveByIndex(idx);
            };
            i -= 1;
        };
    }
    
    public static func Tick() -> Void {
        CleanupExpiredMessages();
    }
}

// Global function for backwards compatibility
public static func LootMarkers_Push(message: String) -> Void {
    LootMarkers.Push(message);
}
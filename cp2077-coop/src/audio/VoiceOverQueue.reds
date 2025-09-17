public struct VOEvent {
    public let lineId: Uint32;
    public let startTs: Uint32;
}

public class VoiceOverQueue {
    public static let events: array<VOEvent>;
    private static let MAX_EVENTS: Int32 = 100; // Prevent unbounded growth
    private static let CLEANUP_THRESHOLD: Uint32 = 30000u; // 30 seconds

    public static func Play(lineId: Uint32) -> Void {
        if Net_IsAuthoritative() {
            CoopNet.Net_BroadcastVOPlay(lineId);
            OnPlay(lineId);
        };
    }

    public static func OnPlay(lineId: Uint32) -> Void {
        let ts = GameClock.GetTime();
        let evt: VOEvent;
        evt.lineId = lineId;
        evt.startTs = ts;
        events.PushBack(evt);
        
        // Cleanup old events to prevent memory leaks
        CleanupOldEvents(ts);
        
        let audio = GameInstance.GetAudioSystem(GetGame());
        if IsDefined(audio) { audio.TriggerEventById(lineId); };
    }
    
    private static func CleanupOldEvents(currentTs: Uint32) -> Void {
        // Remove events older than CLEANUP_THRESHOLD
        let i = 0;
        while i < ArraySize(events) {
            if currentTs - events[i].startTs > CLEANUP_THRESHOLD {
                ArrayRemove(events, events[i]);
            } else {
                i += 1;
            }
        }
        
        // If still too many events, remove oldest ones
        while ArraySize(events) > MAX_EVENTS {
            ArrayRemove(events, events[0]);
        }
    }
    
    public static func ClearAll() -> Void {
        ArrayClear(events);
    }
}

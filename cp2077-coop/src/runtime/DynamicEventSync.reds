public enum DynamicEventType {
    PoliceDispatch = 1,
    GangWar = 2
}

public class DynamicEventSync {
    public static func OnEvent(eventType: Uint8, seed: Uint32) -> Void {
        let players: Uint32 = SessionState.GetActivePlayerCount();
        LogChannel(n"event", "Dynamic event " + IntToString(Cast<Int32>(eventType)) + " seed=" + IntToString(Cast<Int32>(seed)) + " players=" + IntToString(Cast<Int32>(players)));
    }

    public static func SendEvent(eventType: Uint8) -> Void {
        if !CoopSettings.dynamicEvents { return; };
        if Net_IsAuthoritative() {
            let players: Uint32 = SessionState.GetActivePlayerCount();
            let s: Uint32 = CoopNet.Fnv1a32(IntToString(Cast<Int32>(eventType)) + IntToString(Cast<Int32>(CoopNet.GameClock.GetCurrentTick())) + IntToString(Cast<Int32>(players)));
            CoopNet.Net_BroadcastDynamicEvent(eventType, s);
        };
    }
}

public static func DynamicEventSync_OnEvent(pkt: ref<DynamicEventPacket>) -> Void {
    DynamicEventSync.OnEvent(pkt.eventType, pkt.seed);
}

public class CrowdChatterSync {
    public static var currentConv: Uint32 = 0u;
    public static func OnStart(a: Uint32, b: Uint32, line: Uint32, seed: Uint32) -> Void {
        currentConv = CoopNet.Fnv1a32(IntToString(Cast<Int32>(a)) + IntToString(Cast<Int32>(b)) + IntToString(Cast<Int32>(CoopNet.GameClock.GetCurrentTick())));
        RED4ext.ExecuteFunction("LipSyncRand", "Seed", null, seed);
        RED4ext.ExecuteFunction("GestureRand", "Seed", null, seed);
    }
    public static func OnEnd(id: Uint32) -> Void {
        LogChannel(n"CrowdChatter", s"[CrowdChatterSync] Ending conversation - ID: \(id)");

        if currentConv == id {
            // Clean up current conversation
            currentConv = 0u;

            // Reset random generators to default state
            RED4ext.ExecuteFunction("LipSyncRand", "Reset", null);
            RED4ext.ExecuteFunction("GestureRand", "Reset", null);

            // Broadcast end to all players
            Net_BroadcastCrowdChatterEnd(id);

            LogChannel(n"CrowdChatter", s"[CrowdChatterSync] Conversation \(id) ended and synchronized");
        } else {
            LogChannel(n"CrowdChatter", s"[CrowdChatterSync] Warning: Trying to end conversation \(id) but current is \(currentConv)");
        }
    }
}

@wrapMethod(AmbientChatterSystem)
public final func StartConversation(npcA: wref<GameObject>, npcB: wref<GameObject>, lineId: CName) -> Void {
    if GameModeManager.current != GameMode.Coop {
        wrappedMethod(npcA, npcB, lineId);
        return;
    };
    let aId: Uint32 = npcA.GetEntityID();
    let bId: Uint32 = npcB.GetEntityID();
    let line: Uint32 = CoopNet.Fnv1a32(NameToString(lineId));
    let seed: Uint32 = RandRange(0u, 0xffffffffu);
    if Net_IsAuthoritative() {
        CoopNet.Net_BroadcastCrowdChatterStart(aId, bId, line, seed);
    };
    CrowdChatterSync.OnStart(aId, bId, line, seed);
    wrappedMethod(npcA, npcB, lineId);
}

@wrapMethod(AmbientChatterSystem)
public final func EndConversation() -> Void {
    if GameModeManager.current != GameMode.Coop {
        wrappedMethod();
        return;
    };
    if Net_IsAuthoritative() {
        CoopNet.Net_BroadcastCrowdChatterEnd(CrowdChatterSync.currentConv);
    };
    CrowdChatterSync.OnEnd(CrowdChatterSync.currentConv);
    wrappedMethod();
}

public static func CrowdChatterSync_OnStart(a: Uint32, b: Uint32, line: Uint32, seed: Uint32) -> Void {
    CrowdChatterSync.OnStart(a, b, line, seed);
}

public static func CrowdChatterSync_OnEnd(id: Uint32) -> Void {
    CrowdChatterSync.OnEnd(id);
}

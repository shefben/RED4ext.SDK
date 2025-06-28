public class BossPhaseSync {
    public static func OnSwitch(id: Uint32, phase: Uint8) -> Void {
        LogChannel(n"boss", "npc=" + IntToString(Cast<Int32>(id)) + " phase=" + IntToString(Cast<Int32>(phase)));
        CoopNotice.Show("Boss phase " + IntToString(Cast<Int32>(phase)));
    }

    public static func SendSwitch(id: Uint32, phase: Uint8) -> Void {
        if Net_IsAuthoritative() {
            CoopNet.Net_BroadcastBossPhase(id, phase);
        };
    }
}

public static func BossPhaseSync_OnSwitch(id: Uint32, phase: Uint8) -> Void {
    BossPhaseSync.OnSwitch(id, phase);
}

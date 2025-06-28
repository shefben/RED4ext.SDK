public class BossPhaseSync {
    public static func GetBossPhaseMesh(id: Uint32, phase: Uint8) -> Uint32 {
        switch phase {
            case 0u:
                return CoopNet.Fnv1a32("boss_phase0.ent");
            case 1u:
                return CoopNet.Fnv1a32("boss_phase1.ent");
        };
        return CoopNet.Fnv1a32("boss_default.ent");
    }

    public static func OnSwitch(id: Uint32, phase: Uint8) -> Void {
        let npc = GameInstance.GetPlayerSystem(GetGame()).FindObject(id) as GameObject;
        if IsDefined(npc) {
            if HasMethod(npc, n"SetBodyMesh") {
                let meshId = BossPhaseSync.GetBossPhaseMesh(id, phase);
                npc.SetBodyMesh(meshId);
            };
            if HasMethod(npc, n"SwitchAIPhase") {
                npc.SwitchAIPhase(phase);
            };
        };
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

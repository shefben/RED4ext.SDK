public class SkillSync {
    public static func RequestXP(id: Uint16, delta: Int16) -> Void {
        CoopNet.Net_SendSkillXP(id, delta);
    }

    public static func OnXP(peerId: Uint32, id: Uint16, delta: Int16) -> Void {
        let sys = GameInstance.GetScriptableSystemsContainer(GetGame()).Get(n"StatsSystem") as StatsSystem;
        if IsDefined(sys) {
            RED4ext.ExecuteFunction(sys, n"AddSkillXP", Cast<gamedataProficiencyType>(id), Cast<Float>(delta), 0, null);
        };
    }
}

public static func SkillSync_OnXP(peer: Uint32, id: Uint16, delta: Int16) -> Void {
    SkillSync.OnXP(peer, id, delta);
}

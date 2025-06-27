public class GigSpawnVolume extends ScriptedTriggerBase {
    public let questId: Uint32;
    private let triggered: Bool;

    protected cb func OnEnter(instigator: ref<GameObject>) -> Bool {
        if triggered { return false; };
        if !Net_IsAuthoritative() { return false; };
        triggered = true;
        let seed: Uint32 = CoopNet.Fnv1a32(IntToString(Cast<Int32>(questId)) + IntToString(Cast<Int32>(GameInstance.GetSimTime(GetGame()))));
        CoopNet.Net_BroadcastGigSpawn(questId, seed);
        return true;
    }
}

public static func GigSpawner_OnSpawn(q: Uint32, s: Uint32) -> Void {
    LogChannel(n"gig", "Spawn quest=" + IntToString(Cast<Int32>(q)));
}

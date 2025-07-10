public class CrimeTriggerVolume extends ScriptedTriggerBase {
    private let triggered: Bool;

    protected cb func OnEnter(instigator: ref<GameObject>) -> Bool {
        if triggered { return false; };
        if !Net_IsAuthoritative() { return false; };
        if !CoopSettings.dynamicEvents { return false; };
        triggered = true;
        let name: String = NameToString(GetEntityName());
        let eventId: Uint32 = CoopNet.Fnv1a32(name);
        let seed: Uint32 = CoopNet.Fnv1a32(name + IntToString(Cast<Int32>(GameInstance.GetSimTime(GetGame()))));
        var pkt: CoopNet.CrimeEventSpawnPacket;
        pkt.eventId = eventId;
        pkt.seed = seed;
        pkt.count = 3u;
        let base: Uint32 = CoopNet.Fnv1a32(IntToString(Cast<Int32>(GameInstance.GetSimTime(GetGame()))));
        for i in range(0, 3) {
            pkt.npcIds[i] = base + Cast<Uint32>(i);
        };
        CoopNet.Net_BroadcastCrimeEvent(pkt);
        return true;
    }
}

public class CrimeSpawner {
    public static func OnEvent(pkt: ref<CrimeEventSpawnPacket>) -> Void {
        LogChannel(n"ncpd", "Crime event id=" + IntToString(Cast<Int32>(pkt.eventId)));
        for i in range(0, Cast<Int32>(pkt.count)) {
            LogChannel(n"ncpd", "Spawn npc " + IntToString(Cast<Int32>(pkt.npcIds[i])));
        };
    }
}

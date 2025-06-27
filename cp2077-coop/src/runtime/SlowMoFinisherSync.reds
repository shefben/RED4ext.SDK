public class SlowMoFinisherSync {
    public static let stack: Uint32 = 0u;

    public static func OnStart(pkt: ref<SlowMoFinisherPacket>) -> Void {
        if stack == 0u {
            GameInstance.GetTimeSystem(GetGame()).SetLocalTimeDilation(0.3);
        };
        stack += 1u;
        GameInstance.GetDelaySystem(GetGame()).DelayCallback(SlowMoFinisherSync, n"OnEnd", Cast<Float>(pkt.durationMs) / 1000.0);
        let victim = GameInstance.GetPlayerSystem(GetGame()).FindObject(pkt.targetId) as GameObject;
        if IsDefined(victim) && HasMethod(victim, n"SetRagdollMode") {
            victim.SetRagdollMode(true);
        };
    }

    public static func OnEnd() -> Void {
        if stack > 0u { stack -= 1u; };
        if stack == 0u {
            GameInstance.GetTimeSystem(GetGame()).SetLocalTimeDilation(1.0);
        };
    }
}

public static func SlowMoFinisherSync_OnStart(pkt: ref<SlowMoFinisherPacket>) -> Void {
    SlowMoFinisherSync.OnStart(pkt);
}

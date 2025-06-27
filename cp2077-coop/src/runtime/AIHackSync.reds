public class AIHackSync {
    public static func OnHack(target: Uint32, effectId: Uint8) -> Void {
        LogChannel(n"aih", "target=" + IntToString(Cast<Int32>(target)));
        StatusEffectSync.OnApply(target, effectId, 3000u, 1u);
    }

    public static func SendHack(target: Uint32, effectId: Uint8) -> Void {
        if Net_IsAuthoritative() {
            CoopNet.Net_BroadcastAIHack(target, effectId);
        };
    }
}

public static func AIHackSync_OnHack(id: Uint32, eff: Uint8) -> Void {
    AIHackSync.OnHack(id, eff);
}

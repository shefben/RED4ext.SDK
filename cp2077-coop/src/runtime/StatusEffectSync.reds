// Handles replication of status effects (burn, shock, poison, EMP).
public class StatusEffectSync {
    public static func OnApply(id: Uint32, eff: Uint8, dur: Uint16, amp: Uint8) -> Void {
        LogChannel(n"status", "Apply effect=" + IntToString(Cast<Int32>(eff)));
        let obj = GameInstance.FindEntityByID(Cast<EntityID>(id)) as GameObject;
        if !IsDefined(obj) { return; };
        let effSys = GameInstance.GetScriptableSystemsContainer(GetGame()).Get(n"EffectSystem") as EffectSystem;
        if !IsDefined(effSys) { return; };
        let vfx: Uint32 = 0u;
        switch eff {
            case 1u:
                vfx = CoopNet.Fnv1a32("burning_body.ent");
            case 2u:
                vfx = CoopNet.Fnv1a32("shock_body.ent");
            case 3u:
                vfx = CoopNet.Fnv1a32("poison_body.ent");
            case 4u:
                vfx = CoopNet.Fnv1a32("emp_sparks.ent");
            default:
        };
        if vfx != 0u {
            effSys.SpawnEffect(vfx, obj.GetWorldPosition());
        };
    }
    public static func OnTick(id: Uint32, delta: Int16) -> Void {
        LogChannel(n"status", "Tick target=" + IntToString(Cast<Int32>(id)));
    }
}

public static func StatusEffectSync_OnApply(id: Uint32, eff: Uint8, dur: Uint16, amp: Uint8) -> Void {
    StatusEffectSync.OnApply(id, eff, dur, amp);
}

public static func StatusEffectSync_OnTick(id: Uint32, delta: Int16) -> Void {
    StatusEffectSync.OnTick(id, delta);
}

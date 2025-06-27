public class QuestGadgetSync {
    public static func OnFire(pkt: ref<QuestGadgetFirePacket>) -> Void {
        if pkt.gadgetType == Cast<Uint8>(CoopNet.QuestGadgetType.RailGun) {
            let effSys = GameInstance.GetScriptableSystemsContainer(GetGame()).Get(n"EffectSystem") as EffectSystem;
            if IsDefined(effSys) {
                effSys.SpawnEffect(CoopNet.Fnv1a32("rail_recoil.ent"), GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerControlledGameObject().GetWorldPosition());
            };
        } else {
            let target = GameInstance.GetPlayerSystem(GetGame()).FindObject(pkt.targetId) as GameObject;
            if IsDefined(target) && HasMethod(target, n"SetRagdollMode") {
                target.SetRagdollMode(true);
            };
        };
    }
}

public static func QuestGadgetSync_OnFire(pkt: ref<QuestGadgetFirePacket>) -> Void {
    QuestGadgetSync.OnFire(pkt);
}

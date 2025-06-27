public class GrenadeSync {
    public static func OnPrime(pkt: ref<GrenadePrimePacket>) -> Void {
        LogChannel(n"grenade", "prime=" + IntToString(Cast<Int32>(pkt.entityId)));
    }
    public static func OnSnap(pkt: ref<GrenadeSnapPacket>) -> Void {
        let obj = GameInstance.GetPlayerSystem(GetGame()).FindObject(pkt.entityId) as GameObject;
        if IsDefined(obj) {
            obj.SetWorldPosition(pkt.pos);
            if HasMethod(obj, n"SetLinearVelocity") { obj.SetLinearVelocity(pkt.vel); };
        };
    }
}

public static func GrenadeSync_OnPrime(pkt: ref<GrenadePrimePacket>) -> Void {
    GrenadeSync.OnPrime(pkt);
}

public static func GrenadeSync_OnSnap(pkt: ref<GrenadeSnapPacket>) -> Void {
    GrenadeSync.OnSnap(pkt);
}

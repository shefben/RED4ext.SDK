public class CarrySync {
    public static func OnBegin(pkt: ref<CarryBeginPacket>) -> Void {
        LogChannel(n"carry", "begin=" + IntToString(Cast<Int32>(pkt.entityId)));
        let obj = GameInstance.GetPlayerSystem(GetGame()).FindObject(pkt.entityId) as GameObject;
        if IsDefined(obj) {
            if HasMethod(obj, n"SetLinearVelocity") { obj.SetLinearVelocity(Vector3.Empty()); };
        };
    }
    public static func OnSnap(pkt: ref<CarrySnapPacket>) -> Void {
        let obj = GameInstance.GetPlayerSystem(GetGame()).FindObject(pkt.entityId) as GameObject;
        if IsDefined(obj) {
            obj.SetWorldPosition(pkt.pos);
            if HasMethod(obj, n"SetLinearVelocity") { obj.SetLinearVelocity(pkt.vel); };
        };
    }
    public static func OnEnd(pkt: ref<CarryEndPacket>) -> Void {
        let obj = GameInstance.GetPlayerSystem(GetGame()).FindObject(pkt.entityId) as GameObject;
        if IsDefined(obj) {
            obj.SetWorldPosition(pkt.pos);
            if HasMethod(obj, n"SetLinearVelocity") { obj.SetLinearVelocity(pkt.vel); };
        };
    }
}

public static func CarrySync_OnBegin(pkt: ref<CarryBeginPacket>) -> Void {
    CarrySync.OnBegin(pkt);
}

public static func CarrySync_OnSnap(pkt: ref<CarrySnapPacket>) -> Void {
    CarrySync.OnSnap(pkt);
}

public static func CarrySync_OnEnd(pkt: ref<CarryEndPacket>) -> Void {
    CarrySync.OnEnd(pkt);
}

public class VehicleSummonSync {
    public static func OnSummon(pkt: ref<VehicleSummonPacket>) -> Void {
        VehicleProxy_Spawn(pkt^.vehId, &pkt^.pos);
    }
}

public static func VehicleSummonSync_OnSummon(id: Uint32, owner: Uint32, pos: ref<TransformSnap>) -> Void {
    let p: VehicleSummonPacket;
    p.vehId = id;
    p.ownerId = owner;
    p.pos = pos^;
    VehicleSummonSync.OnSummon(&p);
}

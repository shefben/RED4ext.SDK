public class ElevatorSync {
    private static var pendingSector: Uint64;
    private static var pendingPos: Vector3;
    private static var pendingElevator: Uint32;

    public static func OnElevatorCall(id: Uint32, floor: Uint8) -> Void {
        Net_SendElevatorCall(id, floor);
    }

    public static func OnElevatorArrive(id: Uint32, sector: Uint64, pos: Vector3) -> Void {
        let player = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject() as AvatarProxy;
        if IsDefined(player) { player.currentSector = sector; };
        pendingSector = sector;
        pendingPos = pos;
        pendingElevator = id;
        LogChannel(n"DEBUG", "Elevator arrive " + IntToString(id));
    }

    public static func OnStreamingDone(hash: Uint64) -> Void {
        if pendingSector == hash {
            let player = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject() as AvatarProxy;
            if IsDefined(player) {
                player.pos = pendingPos;
            };
            Net_SendTeleportAck(pendingElevator);
            pendingSector = 0uL;
        }
    }
}

public static func ElevatorSync_OnArrive(id: Uint32, sector: Uint64, pos: Vector3) -> Void {
    ElevatorSync.OnElevatorArrive(id, sector, pos);
}

@hook(workElevator.UseFloorButton)
protected func workElevator_UseFloorButton(original: func(ref<workElevator>, Int32), self: ref<workElevator>, floor: Int32) -> Void {
    original(self, floor);
    let name = self.GetClassName();
    let hash = ElevatorSync.Hash32(NameToString(name));
    ElevatorSync.OnElevatorCall(hash, Cast<Uint8>(floor));
}

private static func Hash32(s: String) -> Uint32 {
    var h: Uint32 = 2166136261u;
    var i: Int32 = 0;
    while i < StrLen(s) {
        h = h ^ Cast<Uint8>(s[i]);
        h = h * 16777619u;
        i += 1;
    };
    return h;
}

public class ElevatorSync {
    private static var pendingSector: Uint64;
    private static var pendingPos: Vector3;
    private static var pendingElevator: Uint32;

    public static func OnElevatorCall(id: Uint32, floor: Uint8) -> Void {
        Net_SendElevatorCall(id, floor);
    }

    public static func OnElevatorArrive(id: Uint32, sector: Uint64, pos: Vector3) -> Void {
        let playerObj = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject();
        if IsDefined(playerObj) {
            let player = playerObj as AvatarProxy;
            if IsDefined(player) { 
                player.currentSector = sector; 
            };
        };
        pendingSector = sector;
        pendingPos = pos;
        pendingElevator = id;
        LogChannel(n"DEBUG", "Elevator arrive " + IntToString(id));
    }

    public static func OnStreamingDone(hash: Uint64) -> Void {
        if pendingSector == hash {
            let playerObj = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject();
            if IsDefined(playerObj) {
                let player = playerObj as AvatarProxy;
                if IsDefined(player) {
                    player.pos = pendingPos;
                };
            };
            Net_SendTeleportAck(pendingElevator);
            pendingSector = 0uL;
        }
    }
}

public static func ElevatorSync_OnArrive(id: Uint32, sector: Uint64, pos: Vector3) -> Void {
    ElevatorSync.OnElevatorArrive(id, sector, pos);
}

// TODO: Verify workElevator.UseFloorButton is a valid hook target
/*
@hook(workElevator.UseFloorButton)
protected func workElevator_UseFloorButton(original: func(ref<workElevator>, Int32), self: ref<workElevator>, floor: Int32) -> Void {
    original(self, floor);
    let name = self.GetClassName();
    let hash = Fnv1a32(NameToString(name));
    ElevatorSync.OnElevatorCall(hash, Cast<Uint8>(floor));
}
*/

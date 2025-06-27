public class TransitSystem {
    private static var exitSector: Uint64;
    private static var pendingSector: Uint64;
    private static var pendingPos: Vector3;

    public static func OnBoard(pkt: ref<MetroBoardPacket>) -> Void {
        let streamSys = GameInstance.GetStreamingSystem(GetGame());
        streamSys.LoadScene(n"metro_car_a");
        exitSector = 0uL;
        LogChannel(n"metro", "board line=" + IntToString(Cast<Int32>(pkt.lineId)));
    }

    public static func OnArrive(pkt: ref<MetroArrivePacket>) -> Void {
        exitSector = CoopNet.Fnv1a64("station" + IntToString(Cast<Int32>(pkt.stationId)));
        LogChannel(n"metro", "arrive station=" + IntToString(Cast<Int32>(pkt.stationId)));
    }

    public static func ExitToWorld(pos: Vector3) -> Void {
        if exitSector == 0uL {
            return;
        };
        pendingSector = exitSector;
        pendingPos = pos;
        let player = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject() as AvatarProxy;
        if IsDefined(player) {
            player.currentSector = exitSector;
        };
        GameInstance.GetStreamingSystem(GetGame()).UnloadScene(n"metro_car_a");
        exitSector = 0uL;
    }

    public static func OnStreamingDone(hash: Uint64) -> Void {
        if pendingSector == hash {
            let player = GameInstance.GetPlayerSystem(GetGame()).GetLocalPlayerMainGameObject() as AvatarProxy;
            if IsDefined(player) {
                player.pos = pendingPos;
            };
            pendingSector = 0uL;
        };
    }
}

public static func TransitSystem_OnBoard(pkt: ref<MetroBoardPacket>) -> Void {
    TransitSystem.OnBoard(pkt);
}

public static func TransitSystem_OnArrive(pkt: ref<MetroArrivePacket>) -> Void {
    TransitSystem.OnArrive(pkt);
}

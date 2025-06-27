public class TileGameSync {
    public static func OnStart(phase: Uint32, seed: Uint32) -> Void {
        LogChannel(n"DEBUG", "TileGameStart phase=" + IntToString(Cast<Int32>(phase)));
        if phase != QuestSync.localPhase && phase != SpectatorCam.spectatePhase { return; };
        let hud = new BreachHud();
        hud.Start(Net_GetPeerId(), seed, 5u, 5u);
    }

    public static func OnSelect(peer: Uint32, row: Uint8, col: Uint8) -> Void {
        LogChannel(n"DEBUG", "TileSelect peer=" + IntToString(Cast<Int32>(peer)));
    }

    public static func OnProgress(p: Uint8) -> Void {
        LogChannel(n"DEBUG", "ShardProgress=" + IntToString(Cast<Int32>(p)));
    }

    public static func SendSelect(row: Uint8, col: Uint8) -> Void {
        if Net_IsAuthoritative() {
            CoopNet.Net_BroadcastTileSelect(Net_GetPeerId(), QuestSync.localPhase, row, col);
        } else {
            CoopNet.Net_SendTileSelect(row, col);
        };
    }
}

public static func TileGameSync_OnStart(ph: Uint32, seed: Uint32) -> Void {
    TileGameSync.OnStart(ph, seed);
}

public static func TileGameSync_OnSelect(peer: Uint32, row: Uint8, col: Uint8) -> Void {
    TileGameSync.OnSelect(peer, row, col);
}

public static func TileGameSync_OnProgress(p: Uint8) -> Void {
    TileGameSync.OnProgress(p);
}

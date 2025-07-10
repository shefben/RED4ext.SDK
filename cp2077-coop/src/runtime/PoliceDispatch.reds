public class PoliceDispatch {
    public static let waveTimerMs: Uint32 = 0u;
    public static let nextWaveIdx: Uint8 = 0u;
    private static let prevHeat: Uint8 = 0u;

    public static func OnHeat(level: Uint8) -> Void {
        if level > prevHeat {
            waveTimerMs = 0u;
            nextWaveIdx = 0u;
        };
        prevHeat = level;
    }

    public static func Tick(dtMs: Uint32) -> Void {
        if !CoopSettings.dynamicEvents { return; };
        waveTimerMs += dtMs;
        let interval: Uint32 = HeatSync.heatLevel >= 3u ? 15000u : 30000u;
        if waveTimerMs >= interval {
            waveTimerMs = 0u;
            var seeds: array<Uint32>;
            let count: Uint32 = Cast<Uint32>(Net_GetPeerCount());
            var i: Int32 = 0;
            while i < 4 {
                seeds[i] = CoopNet.Fnv1a32(IntToString(Cast<Int32>(count)) + IntToString(Cast<Int32>(nextWaveIdx)) + IntToString(i));
                i += 1;
            };
            Net_BroadcastNpcSpawnCruiser(nextWaveIdx, seeds);
            nextWaveIdx += 1u;
        };
    }

    public static func OnCruiserSpawn(idx: Uint8, seeds0: Uint32, seeds1: Uint32, seeds2: Uint32, seeds3: Uint32) -> Void {
        LogChannel(n"ncpd", "Spawn cruiser wave " + IntToString(Cast<Int32>(idx)));
    }
}

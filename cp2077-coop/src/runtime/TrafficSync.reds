// Feeds ghost traffic seeds to the local simulator.
public class TrafficSync {
    public static func OnSeed(hash: Uint64, seed: Uint64) -> Void {
        LogChannel(n"traffic", "Seed sector=" + IntToString(Cast<Int32>(hash)));
    }
    public static func OnDespawn(id: Uint32) -> Void {
        LogChannel(n"traffic", "Despawn veh=" + IntToString(Cast<Int32>(id)));
    }
}

public static func TrafficSync_OnSeed(hash: Uint64, seed: Uint64) -> Void {
    TrafficSync.OnSeed(hash, seed);
}

public static func TrafficSync_OnDespawn(id: Uint32) -> Void {
    TrafficSync.OnDespawn(id);
}

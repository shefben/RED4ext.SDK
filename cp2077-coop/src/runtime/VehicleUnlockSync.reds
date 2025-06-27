public class VehicleUnlockSync {
    public static func OnUnlock(peer: Uint32, tpl: Uint32) -> Void {
        LogChannel(n"dealer", "Vehicle unlocked " + IntToString(tpl) + " for " + IntToString(peer));
    }
}

public static func VehicleUnlockSync_OnUnlock(peer: Uint32, tpl: Uint32) -> Void {
    VehicleUnlockSync.OnUnlock(peer, tpl);
}

public class DoorBreachSync {
    public static func OnStart(id: Uint32, phase: Uint32, seed: Uint32) -> Void {
    }
    public static func OnTick(id: Uint32, pct: Uint8) -> Void {
    }
    public static func OnSuccess(id: Uint32) -> Void {
    }
    public static func OnAbort(id: Uint32) -> Void {
    }
}

public static func DoorBreachSync_OnStart(id: Uint32, phase: Uint32, seed: Uint32) -> Void {
    DoorBreachSync.OnStart(id, phase, seed);
}

public static func DoorBreachSync_OnTick(id: Uint32, pct: Uint8) -> Void {
    DoorBreachSync.OnTick(id, pct);
}

public static func DoorBreachSync_OnSuccess(id: Uint32) -> Void {
    DoorBreachSync.OnSuccess(id);
}

public static func DoorBreachSync_OnAbort(id: Uint32) -> Void {
    DoorBreachSync.OnAbort(id);
}

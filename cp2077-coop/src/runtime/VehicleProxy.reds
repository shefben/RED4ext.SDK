public class VehicleProxy extends gameObject {
    public var vehicleId: Uint32;
    public var damage: Uint16;
    public var state: TransformSnap;
    private var lastVel: Vector3;
    private var lastAccel: Vector3;

    public func Spawn(id: Uint32, transform: ref<TransformSnap>) -> Void {
        vehicleId = id;
        // Apply transform in future when physics hooks exist
        LogChannel(n"DEBUG", "Vehicle spawned: " + IntToString(vehicleId));
    }

    public func UpdateAuthoritative(snap: ref<TransformSnap>) -> Void {
        state = snap^;
        lastVel = snap^.vel;
        // Acceleration will be derived from physics later
    }

    // SeatIdx range 0-3
    public func EnterSeat(peerId: Uint32, idx: Uint8) -> Void {
        LogChannel(n"DEBUG", IntToString(peerId) + " entered seat " + IntToString(idx));
    }

    public func ApplyDamage(d: Uint16) -> Void {
        damage += d;
        LogChannel(n"DEBUG", "Vehicle " + IntToString(vehicleId) + " damage=" + IntToString(damage));
    }

    // dtMs should equal CoopNet.kFixedDeltaMs for deterministic physics
    public func Tick(dtMs: Float) -> Void {
        if Net_IsAuthoritative() {
            CoopNet.ServerSimulate(state, dtMs);
        } else {
            ClientPredict(dtMs);
        };
        // NetCore.BroadcastVehicleSnap(vehicleId, state);
        LogChannel(n"DEBUG", "Vehicle tick " + IntToString(vehicleId));
    }

    private func ClientPredict(dtMs: Float) -> Void {
        state.pos += lastVel * (dtMs / 1000.0);
        state.vel += lastAccel * (dtMs / 1000.0);
    }
}

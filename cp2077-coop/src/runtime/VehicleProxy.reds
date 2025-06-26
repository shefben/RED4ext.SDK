public class VehicleProxy extends gameObject {
    public static let proxies: array<ref<VehicleProxy>>;
    public var vehicleId: Uint32;
    public var damage: Uint16;
    public var state: TransformSnap;
    public var destroyed: Bool;
    private var despawnDelay: Float;
    private var lastVel: Vector3;
    private var lastAccel: Vector3;
    private var occupantPeer: Uint32;

    private static func FindProxy(id: Uint32) -> ref<VehicleProxy> {
        for p in proxies { if p.vehicleId == id { return p; }; };
        return null;
    }

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
        occupantPeer = peerId;
        LogChannel(n"DEBUG", IntToString(peerId) + " entered seat " + IntToString(idx));
    }

    public func DetachPart(partId: Uint8) -> Void {
        switch partId {
            case 0u:
                LogChannel(n"DEBUG", "Detach door_L");
                if HasComponent(n"door_L") {
                    SetMeshVisibility(n"door_L", false);
                };
            case 1u:
                LogChannel(n"DEBUG", "Detach door_R");
                if HasComponent(n"door_R") {
                    SetMeshVisibility(n"door_R", false);
                };
            case 2u:
                LogChannel(n"DEBUG", "Detach hood");
                if HasComponent(n"hood") {
                    SetMeshVisibility(n"hood", false);
                };
            case 3u:
                LogChannel(n"DEBUG", "Detach trunk");
                if HasComponent(n"trunk") {
                    SetMeshVisibility(n"trunk", false);
                };
            default:
        };
    }

    public func Explode(vfxId: Uint32, seed: Uint32) -> Void {
        if destroyed { return; };
        destroyed = true;
        despawnDelay = 10.0;
        let effSys = GameInstance.GetScriptableSystemsContainer(GetGame()).Get(n"EffectSystem") as EffectSystem;
        if IsDefined(effSys) {
            effSys.SpawnEffect(vfxId, state.pos);
        };
        // spawn debris deterministic using seed
        let debris = 5u + Cast<Uint32>(seed % 6u);
        let chunkVfx: Uint32 = CoopNet.Fnv1a32("veh_debris_chunk.ent");
        for i in 0u .. debris {
            let ang: Float = (Cast<Float>((seed >> ((i * 3u) & 15u)) & 255u) / 255.0) * 6.28;
            let dir: Vector3 = Vector3(Cos(ang), Sin(ang), 0.5);
            if IsDefined(effSys) {
                effSys.SpawnEffect(chunkVfx, state.pos + dir * 0.5);
            };
        };
    }

    public func ApplyDamage(d: Uint16, side: Bool) -> Void {
        damage += d;
        LogChannel(n"DEBUG", "Vehicle " + IntToString(vehicleId) + " damage=" + IntToString(damage));
        if side && d > 300u {
            CoopNet.Net_BroadcastPartDetach(vehicleId, 0u); // door_L only for now
        };
        if Net_IsAuthoritative() && !destroyed && damage >= 1000u {
            CoopNet.Net_BroadcastVehicleExplode(vehicleId, CoopNet.Fnv1a32("veh_explosion_big.ent"), damage);
            destroyed = true;
            despawnDelay = 10.0;
        };
    }

    // dtMs should equal CoopNet.kFixedDeltaMs for deterministic physics
    public func Tick(dtMs: Float) -> Void {
        if Net_IsAuthoritative() {
            CoopNet.ServerSimulate(state, dtMs);
            let newVel: Vector3 = state.vel;
            let delta: Vector3 = newVel - lastVel;
            var along: Float = 0.0;
            if VectorLength(lastVel) > 0.01 {
                along = VectorDot(delta, VectorNormalize(lastVel));
            };
            let decel: Float = -along / (dtMs / 1000.0);
            if decel > 12.0 && occupantPeer != 0u {
                CoopNet.Net_BroadcastEject(occupantPeer);
                occupantPeer = 0u;
            };
            lastAccel = newVel - lastVel;
            lastVel = newVel;
        } else {
            ClientPredict(dtMs);
        };
        if destroyed {
            despawnDelay -= dtMs / 1000.0;
            if despawnDelay <= 0.0 {
                LogChannel(n"DEBUG", "Vehicle despawn " + IntToString(vehicleId));
                var idx: Int32 = 0;
                while idx < VehicleProxy.proxies.Size() && VehicleProxy.proxies[idx] != this {
                    idx += 1;
                };
                if idx < VehicleProxy.proxies.Size() {
                    VehicleProxy.proxies.Erase(idx);
                };
            };
        };
        LogChannel(n"DEBUG", "Vehicle tick " + IntToString(vehicleId));
    }

    private func ClientPredict(dtMs: Float) -> Void {
        state.pos += lastVel * (dtMs / 1000.0);
        state.vel += lastAccel * (dtMs / 1000.0);
    }
}

public static func VehicleProxy_Spawn(id: Uint32, transform: ref<TransformSnap>) -> Void {
    let v = new VehicleProxy();
    v.Spawn(id, transform);
    VehicleProxy.proxies.PushBack(v);
}

public static func VehicleProxy_Explode(id: Uint32, vfxId: Uint32, seed: Uint32) -> Void {
    let v = VehicleProxy.FindProxy(id);
    if IsDefined(v) { v.Explode(vfxId, seed); };
}

public static func VehicleProxy_Detach(id: Uint32, part: Uint8) -> Void {
    let v = VehicleProxy.FindProxy(id);
    if IsDefined(v) { v.DetachPart(part); };
}
